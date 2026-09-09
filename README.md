# 08：关系约束——失败的语句必须像没发生过

独立 C++17 教程；总目录见 `git show main:README.md`。前置知识：07 的 Schema/Tuple 校验、vector、异常与函数对象。本分支附带 07 的类型/schema 源码快照（移除不需要的 Relation），无跨分支依赖。重点是完整性约束，不是磁盘存储或并发事务。

## 为什么只有类型还不够

`(id=1, department=999, age=-3)` 可以完全符合字段类型，却指向不存在的部门、包含负年龄，甚至占用别人的主键。约束将“业务允许的状态”收紧到类型检查之上：主键唯一、引用存在、值非空、谓词成立。正确的失败路径不仅要报错，还要保留原数据库状态。

## 数据结构、范围与不变量

`Database` 保存 Table 数组。Table 有名字、Schema、单列主键下标、外键列表、检查函数列表、元组数组。所有表只能通过 Database 写入；`table()` 返回 const 引用，成功写操作会替换底层数组，旧引用不能继续使用。

- 建表检查表名唯一、主键下标有效且 NOT NULL。所有表必须有一个单列主键。
- 外键只能指向**已经建立的表的主键**，引用列类型相同；因此不支持自引用、环、复合键或任意 UNIQUE 目标。
- 行插入先检查字段数、类型、非空。主键重复即错误，包括整行相同的再次插入。
- 外键 NULL 使用 MATCH SIMPLE 的豁免：不用找父行；非 NULL 必须找到相等主键。
- CHECK 是调用者提供的纯 `bool(const Tuple&)` 函数，不是 SQL 表达式。返回 false 或抛异常都使操作失败。NULL 如何参与 CHECK 由函数决定，本 demo 的 age 列为非空整数。
- 支持单行 insert、按主键 erase；delete 使用 RESTRICT，没有级联、更新、多行 SQL 或延迟检查。

语句原子性来自“复制→修改候选→验证全部约束→swap”。任何验证异常发生在 swap 前，原数据库不动。检查函数必须无副作用、确定性、不能重入修改数据库；外部副作用不在回滚范围内。这是教学级内存原子性，不是持久化事务。

## 一条删除为何失败：具体轨迹

建立 departments(id)，people(id, dept NULLABLE, age)，添加 dept 外键和 `age >= 0` 检查。

1. 插入部门 `(10)`，候选数据库没有引用冲突，提交。
2. 插入人员 `(1,10,20)`：主键新、部门 10 存在、年龄非负，提交。
3. 删除部门 10：只从候选中移除。遍历 people 时发现 10 已不存在，抛 `missing foreign key`，丢弃候选。
4. 先删除人员 1，然后删除部门 10，均通过。

实际 `demo` 输出：

```text
delete rejected: missing foreign key
parents=1
after child-first delete: parents=0
```

反例：先插入子行后插父行会立即失败，不能靠以后补齐引用。`erase` 删除不存在的合法类型主键返回 false；类型错误或 NULL 主键仍报错。

## 源码阅读顺序

1. `src/relation.h`：Value、Attribute、Schema 与完整元组验证。
2. `src/constraints.h`：create 验证定义；insert/erase 构造候选；validate 遍历主键集合、检查谓词与外键父表。
3. `src/demo.cpp`：父子删除的成功/失败轨迹。
4. `tests/tests.cpp`：主键重复、外键缺失、非空、负年龄、形状/类型失败后整张表仍相同；父删除回滚、NULL 外键、空删除、非法建表及会抛异常的检查函数。`check.h` 不使用 assert，Release 也执行验证。

## 成本与局限

设总行数 N，每行 k 列；每次写入复制空间 O(Nk)，这是刻意的简单上限，大表应改用 undo 日志。验证主键在每表 n 行上是 O(n log n) 次值比较；外键在每个子行上扫描父表，单个 c×p 关系 O(cp)，另有按表名的线性查找与检查函数自身成本。没有 I/O；不提供索引、并发、持久化、DDL 删除或跨语句事务。别把“异常后原对象不变”误称为 ACID 数据库。复制还会复制函数对象，因此复杂捕获不是本教程推荐的用法。

## 构建与验证

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

应看到一个 CTest 通过，demo 如上。无第三方依赖，C++ 扩展关闭，编译警告开启。测试失败返回非零。

本次 macOS AppleClang 17 默认头路径缺失，若出现 `cstdint file not found`，首次配置两种模式之前执行：

```sh
export CXXFLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

本次 Debug/Release 编译、CTest、demo 均使用该环境实测通过；不将本机 SDK 路径硬编码到 CMake。编辑器未采用相同搜索路径时会产生标准库缺失的假诊断，以实际编译检查为准。
