# 10：从 SQL 文本到可执行物理计划

独立 C++17 教程，无第三方依赖。总目录见 `git show main:README.md`。前置知识：07 的类型/schema、09 的选择投影乘积、C++ variant 与 unique_ptr。`src/relation.h` 附带 08 中裁剪过的 07 类型验证快照；本分支不使用集合 Relation，而使用保留重复行的 BagTable。

## 要解决的问题

SQL 字符串不能直接用作数组下标。数据库前端必须先识别词法单元，再判断语法结构，再根据真实 catalog 解析名字和类型，最后选择具体执行机制。本教程真正走完这条路径，不按某条 SQL 字符串返回预制答案。

```text
SQL → lex → Token[] → Parser → Query
                              ↓ bind + Catalog
                         LogicalPlan
                              ↓ lower
                         PhysicalPlan → execute → Tuple[]
```

## 精确支持的语法

```text
query   := SELECT ( '*' | column (',' column)* )
           FROM name (',' name)*
           [ WHERE column '=' operand (AND column '=' operand)* ] [';'] EOF
column  := name ['.' name]
operand := column | integer | string | NULL
name    := [A-Za-z_][A-Za-z_0-9]*  （排除本语法关键字）
integer := '-'? [0-9]+           （必须在 int64 范围）
string  := 单引号包围的文本，内部单引号用两个单引号表示
```

关键字、未引用名字不区分 ASCII 大小写，统一转小写；字符串内容保持原样、按字节比较。支持空格、TAB、CR、LF。整数与紧随的字母必须分开。不支持注释、加号、浮点、引用标识符、别名、DISTINCT、ORDER BY、JOIN 关键字、OR、括号、算术、不等比较、IS NULL、子查询、多语句或 DML；遇到这些输入明确报错，不悄悄忽略尾部。FROM 逗号表列表表示乘积，WHERE 列等值可表示内连接。重复 FROM 表由于没有别名而拒绝。

## 四个阶段的机制与不变量

### 词法与语法

`lex` 扫描每个字符，保存 token 类型、内容和起始位置。字符串 token 不会被误认为关键字或分号；`'O''Reilly'` 变成文本 `O'Reilly`。未结束字符串、非法字符和整数溢出被拒绝。Parser 是按上述文法写的递归下降解析器：column 处理可选限定名，operand 区分列引用与字面量，parse 处理列表、可选 WHERE 与最后 EOF。最多允许一个末尾分号。

### 名称绑定

Catalog 建表先规范名字、校验所有元组，再发布表；字段数、类型、NULL 许可或规范化后的重复列名失败不会留下部分表。已发布数据只读。绑定按 FROM 顺序串联列：例如 people 有 2 列，teams 有 2 列，则 teams.label 的全局位置是 3。

未限定列必须只匹配一列；people.id 和 teams.id 都存在时，单独 id 是歧义，不是默认选左边。即使表为空，也检查所有引用和比较类型。列与字面量、列与列都必须同类型；NULL 字面量可与任意列比较。SELECT * 按 FROM/原 schema 顺序展开，重复选择同一列合法，结果元数据是 Attribute 数组而非禁止同名列的 Schema。

### 逻辑到物理

LogicalPlan 表达 `Project(columns, Select(predicates, Product(sources)))`，列名已变为整数下标。每个来源持有只读表的值快照，不留下悬空 catalog 引用。`lower` 根据逻辑结构建立真正的算子树：来源变为 SeqScan；多表乘积变为左深 NestedLoopProduct；非空谓词列表变为 Filter；最终投影变为 Gather。没有代价优化或谓词下推，不能把这种确定性降低称为优化器。

PhysicalPlan 的左右孩子由 unique_ptr 拥有。execute 递归调用孩子，扫描复制行，双循环组合左右行，过滤检查已绑定的列，Gather 按指定位置复制字段。计划可重复执行；catalog 离开作用域后仍可执行。AST 和计划结构是教学用内部表示，只执行 Parser/bind/lower 生成的结构，不接受外部反序列化计划。

### Bag、NULL 与顺序

每一步使用 vector，**不去重**。同一行出现两次会被扫描两次，投影也保留两次；两边各两条匹配行会产生四条连接结果。WHERE 等值遇到任意 NULL 得到 UNKNOWN，WHERE 只保留 TRUE；全部谓词通过才保留一行，因此这里的 AND 过滤符合三值逻辑，但没有通用三值表达式求值器。

扫描和嵌套循环顺序在本实现中可重复，仅为了演示稳定；没有 ORDER BY 的 SQL 不承诺结果排序。本实现不把集合代数中 NULL 字面标记相等的规则套到 SQL 上。

## 可复现的具体轨迹

people 为 `(1,DB),(2,DB),(3,NULL)`。输入：

```sql
SELECT team FROM people WHERE team = 'DB';
```

Parser 得到一个投影列、一张来源表、一个等值谓词；Binder 将 team 两处均绑定到位置 1，确认右侧为 Text；lower 建立 Gather → Filter → SeqScan。扫描三行，过滤第三行 UNKNOWN，投影两次复制 DB，最终仍为两行，不是一行。随后 demo 查询不存在的 missing 展示绑定失败。

实际输出：

```text
logical: Project[1](Select[1](Product[people]))
physical: Gather(Filter(SeqScan))
DB
DB
rows=2
rejected: unknown column: missing
```

多表例子 `SELECT people.id, teams.label FROM people, teams WHERE people.id=teams.id` 会得到 `Gather(Filter(NestedLoopProduct(SeqScan,SeqScan)))`。两张表的 id=2 各出现两次时输出四行，测试检查精确结果。

## 源码导读与测试

`src/sql.h` 按词法、Parser、Catalog、bind、lower/explain、execute 排列。先看 Query 中字符串列引用与 BoundEquality 中数值下标的区别，再跟随 lower 的树构造，最后看 execute。`src/demo.cpp` 输出由真实计划生成。`tests/tests.cpp` 独立于 demo，使用不会在 Release 消失的 check/rejects：覆盖大小写、重复输入与投影、AND、转义和空文本、int64 两端/溢出、NULL、限定列/歧义、错类型、空表、尾部垃圾、多语句与不支持语法、物理算子结构、重复执行和快照生命周期。错误返回非零。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/demo
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j2
ctest --test-dir build-release --output-on-failure
./build-release/demo
```

两种模式应各通过一个 CTest，demo 如上。若 macOS 的默认 AppleClang 找不到标准库头，给两次 cmake 配置命令都附加 `-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`。本机使用该参数验证，不修改系统、不安装依赖、不硬编码 SDK 路径；编辑器缺少同样路径导致的 std/cstdint 诊断应与实际编译区分。

## 成本、失败与局限

SQL 长度 L，词法/语法 O(L) 时间和空间；列数 k、引用数 q，线性名字绑定 O(qk)，另有表名查找和源表快照复制。单表 n 行、p 个谓词、投影宽度 w：过滤 O(np)，投影 O(rw)，不计文本比较成本。多表中间行数为各来源行数的乘积 P，全量物化最坏 O(Pk) 时间/空间，再加过滤 O(Pp) 和结果 O(rw)。逻辑与物理各复制来源数据以换取独立生命周期；无磁盘 I/O。大表应改成流式算子和哈希连接，本教程刻意暴露乘积爆炸成本。

没有 SQL 写入、索引、持久化、事务、并发、计划缓存、别名、自连接或完整 SQL 类型系统。递归执行的深度随 FROM 表数增加；不对恶意超大查询提供资源配额，适用小型本地教学输入而非网络服务。所有查询只读，词法、绑定、执行失败不会修改 catalog；内存分配失败正常传播异常。
