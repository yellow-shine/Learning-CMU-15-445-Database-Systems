# 25：LRU-K —— 用第 K 次历史区分一次性扫描与热点

## 问题与前置知识

单次扫描能把普通 LRU 中的热点挤走。LRU-K 记录多次访问，优先淘汰没有足够历史的页，使“只访问一次”不等于热页。它属于缓冲池替换层；先了解 LRU、队列与逻辑时间。关联 22、23，但本分支没有运行时依赖。

## 数据结构和明确的策略

固定 Frame 数组，每项有最多 K 个严格递增的访问时间，队首最老，另有 evictable 位。全局时钟每次 access 加一。超出 K 就弹出最老记录；首次登记默认不可淘汰，access 不改变已有保护位。

当前时刻 t，历史足够 K 的 backward K-distance 为 `t - history.front()`，距离最大的优先，因此无需每次计算 t，只比较队首时间。

历史不足 K 的距离定义为正无穷，**优先于所有历史完整项**。多个无穷距离候选按最早第一次访问淘汰，不按最后访问；若时间相等则较小 Frame ID 优先（本 API 每次递增时钟，实际不会生成相等时间）。历史完整项同样取队首最早者，平局沿用较小 ID。不可淘汰项从候选中排除。淘汰删除历史，Frame 重用不继承旧页温度。

## 工作轨迹

K=2，访问 `0,1,0,2,1` 并允许淘汰：

|Frame|保留历史|t=5 时距离|淘汰次序|
|---|---|---|---|
|0|[1,3]|4|第二|
|1|[2,5]|3|第三|
|2|[4]|无穷|第一|

demo 输出 `K=2 victims: 2 0 1`。冷页 tie-break 反例：K=3，轨迹 `0,1,0,2`，0 虽最后访问比 1 新，仍因首次访问最早而先淘汰。K=1 时退化为 LRU，轨迹 `0,1,2,0` 得 `1,2,0`。

## 源码与不变量

`src/lru_k.h`：access 维护有界历史，before 实现冷热分层比较，evict 扫描最优候选并清空。`src/demo.cpp` 是上述轨迹。`tests/tests.cpp` 覆盖冷热优先级、无穷距离 tie-break、滚动截断、淘汰后重新冷启动、保护项、空集、单项、K=1 和 K=0 拒绝。

每项历史只含本次驻留期间最近 K 次访问，按时间升序；已登记但 pinned 的项仍可记录访问。时钟达到 uint64 上限抛异常而非回绕。未知项设置保护、越界 Frame ID 同样报错。

## 成本与局限

一次访问均摊 O(1)，选择 O(F)，空间 O(FK)，不是 O(1) 选择的工程实现。使用标准 deque 存历史，但淘汰算法由源码明确实现，不调用排序替代算法。

单线程，不维护真实 pin count、不读磁盘；容量零合法，K 必须正数。没有跨淘汰 ghost history，没有关联引用过滤，也不根据工作负载自动调 K；较大的 K 增加空间并延迟新热点晋升。它只是明确语义的教学变体，不能保证所有访问分布都比 LRU 好。

## 构建与验证

仅需 C++17、CMake 和标准线程库，无第三方依赖。回到总目录：`git show main:README.md`。

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

若 macOS 的 AppleClang 报标准头文件找不到，在两条配置命令中各附加：
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`。
这是本机 SDK 搜索路径排错，不是源码依赖。CTest 失败抛异常返回非零，Release 不依赖 assert；测试超时 20 秒。
