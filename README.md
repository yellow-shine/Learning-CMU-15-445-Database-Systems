# 57 聚合下推：传状态而不是平均值

## 问题和前置知识
在连接之前先按键聚合事实表，可以减少连接输入。必须同时保持重复行的权重。
前置知识是 GROUP BY、内连接、多重集和 AVG。这里计划不是标志位模拟：
Aggregate→Join→Facts/Dimension 改写成 Final→StateJoin→Partial→Facts。
两种计划分别解释执行，原始路径逐行连接后聚合，优化路径连接聚合状态。

## 状态和安全条件
每个键的状态为 (SUM,COUNT)，COUNT 只计非 NULL 值。
Partial 生成每键一个状态，StateJoin 按维表匹配次数复制状态，Final 相加。
AVG 最后才执行 SUM/COUNT，COUNT=0 时返回 NULL。全 NULL 组仍然存在。
只允许事实值上的聚合，且分组键就是事实连接键；维表仅提供键，连接必须是内等值连接。
这些条件由表示类型和 `push_aggregate` 检查，外连接原样保留。
没有去重维表！重复键使 sum 和 count 同时倍增，平均值可能不变但其他聚合会变。

## 具体轨迹
事实 (1,10),(1,20),(2,90)，维表键 1,1,2。
原计划生成 5 行，分组后键 1 的 SUM=60、COUNT=4、AVG=15，键 2 为 90、1、90。
Partial 先生成 (1,30,2),(2,90,1)，StateJoin 产生三个状态，Final 恢复同样答案。
demo 逐键输出 `key=1 sum=60 count=4 avg=15` 和 `key=2 sum=90 count=1 avg=90`。
AVG 的反例：分区 {0,10} 与 {100} 的平均值平均是 52.5，而正确值为 110/3。
外连接反例：事实键 3 没有匹配，原左连接保留它；误用内 StateJoin 会丢掉它。

## 源码导读与验证
`src/aggregate.hpp` 包含专用计划、行执行器、状态执行器及规则；这是本主题全部机制。
`State::merge` 是 partial/final 的代数核心，`states` 必须返回多重集而不是每键覆盖。
`tests/tests.cpp` 比较改写前后 SUM 和 COUNT，而不是只比 AVG，覆盖 0–4 个重复键、
全 NULL、空表、无匹配、外连接边界、平均值反例与非法去重反例。

## 成本和局限
原始连接 O(FD)，Partial O(F log G)，状态连接 O(GD)，Final O(matches log G)。
全部物化在内存，空间为事实/连接结果或状态结果大小；没有磁盘计数或并行调度。
值采用 double，浮点加法不满足严格结合律；整数大小的演示数据可精确表示，
任意浮点数据应使用误差容限，不能声称位级等价。COUNT 假设不溢出 size_t。
不支持 DISTINCT、连接后过滤、维表属性分组、MIN/MAX 和一般 SQL 外连接聚合。

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

若 macOS 的 AppleClang 找不到标准库头文件，仅在本机配置时追加
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`，
不需要修改源码或安装依赖。测试使用显式异常检查，Release 不会删除断言。
本快照为独立 C++17 教学程序，无运行时分支依赖。总目录见 `git show main:README.md`。
