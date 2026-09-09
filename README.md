# 07：关系模型——先约束形状，再保存数据

本分支是独立 C++17 教程，不依赖 BusTub。总目录请用 `git show main:README.md` 阅读；相关主题为 08 约束、09 关系代数、10 SQL 规划。

## 问题与前置知识

一个 `vector<vector<string>>` 可以保存表格，却无法区分整数 1 和字符串 "1"，也不能阻止少字段的记录进入表。数据库的关系层首先需要声明合法的“形状”，再接受元组。前置知识是 C++ 的 vector、variant、set 与异常；不需要磁盘页或索引知识。

## 模型与不变量

- `Attribute` 是名字、类型、可空标记。名字必须非空且唯一。
- `Schema` 是有序属性数组。这里字段顺序属于 schema 身份，不做隐式排列匹配。
- `Value` 是 NULL 标记、64 位有符号整数、文本三选一；没有字符串到整数的隐式转换。
- `Tuple` 是有序 Value 数组，字段数必须恰好相等；非 NULL 值类型必须一致，NULL 必须获得许可。
- `Relation` 是 schema 和 `set<Tuple>`。相同元组只保存一次；不同 id 并不是本分支的要求，**整行相等**才去重。

插入先完整调用 `Schema::validate`，再修改集合，因此验证失败不会留下半行。只提供单行插入，没有批量事务或删除。调用者只能获得 const 行集合，不能绕过校验修改内部记录。

NULL 在本分支只是可比较的缺失值标记，两个相同位置的 NULL 参与整行去重；不声称实现 SQL 的三值逻辑。set 的迭代顺序用于稳定演示，不是关系模型保证的查询顺序。零属性 schema 合法：空关系有零行，插入空元组后有一行，这是两个不同对象。

## 具体执行轨迹

Schema 为 `(id:Integer NOT NULL, name:Text NULLABLE)`：

| 步骤 | 输入 | 验证与结果 |
|---|---|---|
| 1 | `(1,"Ada")` | 两列类型正确，插入，行数 1 |
| 2 | `(1,"Ada")` | 验证成功，但集合已存在，返回 false |
| 3 | `(2,NULL)` | name 可空，插入，行数 2 |
| 4 | `("wrong","Bob")` | 第一列类型不符，抛异常，仍是 2 行 |

`demo` 的实际输出：

```text
1 Ada
2 NULL
rejected: value type mismatch
rows=2
```

## 源码导读

从 `src/relation.h` 的三个别名和 Attribute 开始，读 Schema 构造函数的名字去重、validate 的三重检查，最后读 Relation::insert。这里没有隐藏的存储引擎。`src/demo.cpp` 按上表驱动模型；`tests/check.h` 使用抛异常的 check，不依赖 Release 会删除的 assert。`tests/tests.cpp` 独立覆盖重复、空关系、零列元组、未知属性、重复/空名字、缺列、错类型、非空失败，以及失败后的行数。

## 成本与边界

设 n 行、k 列，忽略文本长度时：schema 建立 O(k log k)，查找名字和验证各 O(k)，插入 O(k log(n+1))，空间 O(nk)。文本比较还需计入字符串长度。本模型全在内存，无磁盘 I/O。标准 set 在此实现集合容器，不是假装实现数据库索引。

只支持 Integer/Text/NULL，没有浮点、排序规则、外键、更新、持久化、并发控制。属性名字按字节精确匹配。`display` 是展示而非可逆编码，文本 "NULL" 与 NULL 的显示相同；内部类型仍不同。

## 构建、测试与预期

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

两种模式都应通过一个 CTest，demo 输出如上。测试错误会返回非零。无第三方依赖，关闭 C++ 扩展并启用编译警告。

本次 macOS 的 AppleClang 17 安装缺少默认 libc++ 头搜索路径；若遇到 `cstdint file not found`，在首次配置前执行以下环境修复（不修改项目或安装依赖）：

```sh
export CXXFLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

本次 Debug、Release 均在此环境下真实编译测试通过；未配置同样搜索路径的编辑器诊断不能替代编译器结果。
