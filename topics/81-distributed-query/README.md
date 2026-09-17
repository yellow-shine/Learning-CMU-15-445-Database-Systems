# 81-distributed-query：广播、重分区连接与局部聚合

## 问题与前置知识

分片之间如何把可连接的行送到同一个节点？前置知识是等值连接、哈希分桶和 SUM/COUNT；相关分支 78 讲路由，82 讲真正线程并行。本例节点是本地向量，消息是复制到目标向量的行，没有网络或线程。

## 多重集语义与交换机制

Row 是整数 key/value，连接产生 (key,left_value,right_value)，采用 SQL 风格多重集，不做去重，不支持 NULL。相同 key 左边 m 行、右边 n 行必须产生 m×n 行。输出无排序保证。

broadcast_join 先以输入位置轮转分配左表（模拟原有任意分片），将完整右表复制到每个节点，在节点上构建右表的 key→行列表并探测左表。左行仅属于一个节点，因此右表广播不会额外重复全局结果。
repartition_join 将两边都按相同的 uint64(key)%p 路由到目标节点。相同键必定相遇，不同节点的输出直接拼接。使用显式 Exchange 计数器统计发送的行副本：广播为 p×|R|，重分区为 |L|+|R|，计数包括到本地的逻辑投递，不是假测网络字节。

partial_aggregate 对已有分片先按 key 求 (sum,count)，再将这些状态按 key 发送至归并节点，合并状态后计算 AVG。不能对各分片 AVG 直接平均，因为分片行数不同。空输入不生成分组。value 限定 int，sum 是 int64，计数是 uint64；合并显式检查溢出，失败抛异常。

## 手算轨迹与预期输出

L=[(1,10),(1,11),(2,20)]，R=[(1,100),(1,101),(3,300)]，两节点：广播右边 6 行，重分区两边共 6 行。key=1 的 2×2=4 个结果都保留；key=2/3 无匹配。
局部聚合输入分片 [[(1,10)],[(1,11),(2,20)]]：发出 3 个局部状态，key=1 合并 sum=21,count=2,avg=10.5；key=2 为 20/1。
demo 输出 `broadcast rows=4 sent=6`、`repartition rows=4 sent=6`、`key=1 sum=21 count=2 avg=10.5 partials=3`。

## 源码与检查

`src/distributed_query.hpp` 的 exchange、local_join、两种 join、partial_aggregate 是真实数据路径。标准 unordered_map 仅作为连接哈希表，不代替数据交换机制。`tests/tests.cpp` 用集中式双循环连接和逐行聚合作 oracle；对空表、全重复键、负键、多种节点数及固定种子随机输入做排序后差分比较，并测试零节点及聚合溢出。

## 成本与局限

设节点数 p，连接结果 z。广播内存 O(|L|+p|R|+z)，期望时间同阶；重分区内存与期望时间 O(|L|+|R|+z+p)。哈希极端碰撞可能退化。局部聚合哈希阶段期望 O(n+g) 时间，最终有序 map 插入额外 O(k log k)（k 是全局不同键数），最多 O(n+p) 状态空间；交换 g 为各分片不同键数量之和，只有重复键多时才显著省流量。
单机内存物化所有输入、消息和输出，不提供流控、spill、失败重试、去重投递、代价优化、真实网络计时或 SQL 解析。热点键仍集中到一个节点，增加并行度不能拆散一个热点；广播只适合足够小的构建端。

## 构建与验证

需要 CMake、C++17 编译器；无第三方依赖。独立检出本分支即可运行。

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

若 macOS 的 AppleClang 报标准头文件缺失，在两次配置命令附加：

```sh
-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

这只是本机 SDK 搜索路径排错，不是源码依赖。测试使用显式异常检查，Release 不会删除检查；CTest 有 20 秒上限。
返回总目录：`git show main:README.md`。各主题独立，相关分支仅供进一步阅读，不是运行依赖。
