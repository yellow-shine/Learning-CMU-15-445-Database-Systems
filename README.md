# 09：集合语义的关系代数

本分支独立实现选择、投影、并、交、差、笛卡尔积、等值连接。总目录见 `git show main:README.md`。前置知识为 07 的关系/schema 与 C++ set、variant、函数对象；`src/relation.h` 是该分支的支撑快照，检出本分支即可构建，不需要另一个工作区。

## 问题与原理

如何把“找出 DB 组成员的组名”分解成可组合的运算？选择先保留满足谓词的整行，投影再只取指定列。关系代数提供这种组合语言，尚不是 SQL 解析器，也不是查询优化器。

Relation 是有类型的元组集合。所有输出都经过 Schema 验证和 set 插入，因此不产生重复元组，输入保持不变。字段名字非空且唯一；元组字段数、类型、NULL 许可必须匹配。NULL 在这里是可比较的字面标记，NULL 等于 NULL；这**不是 SQL 三值逻辑**。SQL 默认保留重复行，本分支每一步都去重。

| 操作 | 实现规则 | 输出 schema |
| --- | --- | --- |
| select | 对每行执行纯 bool 谓词，为真则保留 | 与输入相同 |
| project | 按给定名字顺序复制列，再去重 | 按投影顺序排列 |
| combine / Union | 左集合全部，加右集合全部 | 两边必须完全相同 |
| combine / Intersection | 左行在右集合中存在才保留 | 同上 |
| combine / Difference | 左行在右集合中不存在才保留 | 同上 |
| product | 双循环枚举所有左右配对 | 左列加 L.，右列加 R. |
| join | 双循环，只输出指定键相等的配对 | 与 product 相同，保留两侧连接键 |

兼容 schema 是名字、位置、类型、可空标记均相等，不隐式改名或强制类型转换。连接键类型必须相同；两边可空许可可以不同。使用固定的左右前缀解决同名列；嵌套组合会出现 `L.L.id` 这样的名字。连接不是自然连接，不会自动删除右键。需要单个键时显式投影。

## 具体轨迹与输出

输入 people 为 `(1,DB),(2,DB),(3,AI)`，db 是选择 team=DB 的结果：

1. select 逐行检查，保留 `(1,DB),(2,DB)`，共 2 行。
2. project(db, {team}) 先产生 `(DB),(DB)`，set 合成 1 行。
3. people ∪ db 为 3 行，people ∩ db 为 2 行，people − db 仅 `(3,AI)`。
4. people × db 枚举 3×2=6 个四列元组。
5. team 等值连接只留下 DB 的 2×2=4 个配对，例如 `(1,DB,2,DB)`。

`demo` 的实际输出应为：

```text
selected=2 projected=1
union=3 intersection=2 difference=1
product=6 join=4
```

## 不变量与容易混淆的边界

空集合投影仍为空；非空集合投影零列得到只含空元组的单位关系。空元组关系与任意关系做乘积保持行值，但列名仍加前缀。对空输入也先检查列名与 schema，不能因为没有行就接受非法查询。重复投影列名、未知列名、空谓词、集合 schema 不兼容、连接类型不兼容均抛 invalid_argument。谓词异常只丢弃尚未返回的输出，不修改输入；要求调用者谓词无外部副作用。

## 源码导读与验证

先读 `src/relation.h` 的 validate/insert，再读 `src/algebra.h`：project 的列绑定在循环外；combine 共享兼容性检查；paired_schema 统一列命名；join 直接执行双循环而非先存整个乘积。`src/demo.cpp` 对应上面的例子。`tests/tests.cpp` 用非 assert 检查选择的精确行、投影去重、零列、三种集合运算、所有 schema 不兼容类别、重复输入、空集、连接键保留与类型错误，并验证 join 等于 product 后选择。

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

两种模式各一个 CTest，失败返回非零。若 macOS 默认 AppleClang 找不到标准库头文件，给两次 cmake 配置命令附加 `-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`；本机验证使用此参数，不将 SDK 路径写死到项目。未设置同样路径的编辑器诊断不是实际构建结果。

## 成本与局限

设左右行数 n、m，元组宽度 k，输出行数 r，不计字符串长度。选择 O(n·谓词成本 + rk log(r+1))；投影绑定 p 个名字 O(pk)，扫描与输出 O(np log(r+1))；集合运算 O((n+m)k log(n+m+1))；乘积 O(nmk log(nm+1))；连接双循环比较 O(nm)，输出另需 O(rk log(r+1))。结果空间 O(rk)，没有磁盘 I/O；长字符串比较需要额外计费。

教学实现全量物化，无索引、流式执行、NULL 三值逻辑、自然/外连接、排序规则、SQL 或优化改写。标准 set 是集合容器，不声称实现数据库索引。大表连接应换哈希算法，但这里保留最直接可核查的双循环。
