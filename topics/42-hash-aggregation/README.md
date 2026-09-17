# 42 哈希聚合：每个键维护一个充分统计状态

## 问题与前置知识

GROUP BY 不一定需要排序全部 N 行。执行器可把键映射到一个小状态，逐行更新，最后每个组输出一行。
本教程聚焦哈希分组、状态更新和结果生成，使用 C++17 unordered_map 作为辅助哈希表，不重新实现哈希表存储结构。
前置知识是哈希/相等关系、optional、整数边界。相关主题为 41 排序聚合和 40 外部排序。
本分支内置了来自主题 41 的 Row/State/global_aggregate 源码快照，没有运行时或跨 checkout 依赖；总目录见 `git show main:README.md`。

## 表、状态与不变量

`unordered_map<optional<int>, State>` 为每个分组键保存状态。NULL 键由 C++17 的 optional 哈希及相等操作处理，所有 NULL 属于同一组，但不是整数 0。
每行先查找键，不存在则初始化空 State，然后更新；哈希碰撞仍通过完整键相等判定，不会把不同键合成一组。
不变量：处理前 t 行后，表恰有这 t 行中出现过的键，每个状态概括其所有已见行。
最后遍历表生成 `vector<Group>`。输出顺序**不保证**；SQL 的 GROUP BY 本来也不是 ORDER BY。
demo 单独对 G 个结果键排序，只为产生可复现的展示，不属于聚合算法。

## COUNT、NULL、空输入与 AVG

输入是多重集，不消除重复。键和值均可为 NULL，值是 int64。

|聚合|State 中的含义|
|---|---|
|COUNT(*)|rows，包括值为 NULL 的行|
|COUNT(value)|count，只计非 NULL 值|
|SUM|整数 sum，count=0 时输出 NULL 而非零|
|MIN/MAX|可空极值，忽略 NULL|
|AVG|sum / count，count=0 时 NULL，否则 long double|

空输入的 `hash_aggregate` 返回零组；无 GROUP BY 的 `global_aggregate` 返回一个空状态，COUNT 为 0，其余为 NULL。
全 NULL 组存在，COUNT(*) 非零，COUNT(value)=0；实际值 0 不应被当作 NULL。

`State::merge` 支持合并分区状态：COUNT/SUM 相加，MIN/MAX 取极值，AVG 最后计算。
例如 A=[10]，B=[20,30,40,NULL]，合并后 sum=100，count=4，AVG=25；局部均值的简单平均 `(10+30)/2=20` 错误。
这展示了并行聚合所需的统计量，但本程序本身不启动线程。

## 具体插入轨迹

输入 `(2,10),(1,-4),(2,NULL),(1,6),(NULL,3),(2,20),(3,NULL)`：

|已处理行|被更新的键|rows / count / sum|
|---|---|---|
|(2,10)|新建 2|1 / 1 / 10|
|(1,-4)|新建 1|1 / 1 / -4|
|(2,NULL)|命中 2|2 / 1 / 10|
|(1,6)|命中 1|2 / 2 / 2|
|(NULL,3)|新建 NULL|1 / 1 / 3|
|(2,20)|命中 2|3 / 2 / 30|
|(3,NULL)|新建 3|1 / 0 / 内部零，输出 NULL|

表中有 4 个状态，而不是 7 份排序行副本。demo 输出（显示阶段按键排序）：

```text
key count(*) count(v) sum avg min max
NULL 1 1 3 3.000000 3 3
1 2 2 2 1.000000 -4 6
2 3 2 30 15.000000 10 20
3 1 0 NULL NULL NULL NULL
empty global: 0 NULL
```

## 源码导读与成本

- `src/aggregation.h`：State 的 add/merge 和访问器是复用支撑；hash_aggregate 的查表更新及结果生成是本主题核心。
- `src/demo.cpp`：相同数据覆盖负值、重复键、NULL 键、NULL 值与全 NULL 组。
- `tests/aggregation_test.cpp`：排序分组交叉验证，另有独立 O(NG) 逐键重扫 oracle，避免两条路径共享状态错误而互相“证明”。

哈希良好时，更新期望 O(N)，生成 O(G)，额外内存 O(G)，不拷贝全部输入（API 接收调用者已经拥有的 vector）。
碰撞极端时每次查找 O(G)，总成本可退化 O(NG)。unordered_map 可能扩容并重新散列，迭代顺序不稳定。
输出生成时表和 vector 同时存在，峰值仍 O(G)，但不是只有一份状态。
G 接近 N 时内存优势消失；本教程不提供分区 spill，大数据应升级为 Grace/分区哈希聚合。
显示排序另花 O(G log G)，不要计作哈希构建成本或声称哈希输出有序。

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

若 macOS 标准库头文件搜索失败，向对应配置命令附加当前 SDK 路径，不修改源代码：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

预期 CTest 1/1 通过，demo 如上。检查不依赖 assert，Release 同样有效。
测试包括空输入、全 NULL、NULL/零键区别、重复值、负数、2000 条固定种子输入及反序、513 个不同键触发表增长、排序/朴素参考、不同大小分区合并、正负 SUM 溢出与 COUNT 溢出。

## 数值保证与局限

COUNT 为 uint64、SUM 为 int64。所有溢出检查在修改前完成，拒绝更新不改变 State；add 复用 merge 的检查。
只有每次中间和均可表示的输入才保证成功；即使最终和可表示，中间溢出也会抛异常，顺序和分区可能影响接受性。
AVG 只在最终求值时转换为 long double，舍入精度由平台决定，不是任意精度 DECIMAL。
聚合函数异常时不返回部分结果，但不提供事务系统。无 SQL 解析、复合键、字符串 collation、DISTINCT、HAVING、窗口、磁盘分区、网络或并行实现。
这是聚合算子机制，不是生产哈希表或完整 SQL 引擎。
