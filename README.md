# 23：LRU —— 淘汰最久未访问的可替换帧

## 问题与前置

缓冲池满时应保留近期热页。LRU（Least Recently Used）用最近一次访问时间衡量冷热，属于内存管理的替换层；它不负责读盘或 pin 计数。先了解数组、可选值和缓冲池。关联 22 Buffer Pool、24 Clock、25 LRU-K，但本分支完全独立。

## 数据与算法

固定容量数组以 Frame ID 为索引，每项保存可空的最后访问逻辑时间和 evictable 位。access 分配严格递增的时间；set_evictable 由缓冲池在 pin/unpin 时调用。首次访问默认不可淘汰。重复访问不会改变保护状态。

选择时扫描所有已访问且可淘汰的项，取时间最小者，再清空该项。没有候选返回空 optional，而不是把 0 当哨兵。淘汰后的 Frame 再次访问是新项，不继承旧保护状态。逻辑时钟溢出明确抛异常；不会回绕改变先后关系。

不变量是：每个 Frame 只保存一个最新时间；pin 保护优先于冷热；只有 evict 真正移除记录。这里实现真实 LRU 选择而非调用标准排序。为减少链表与迭代器状态，采用 O(F) 扫描变体。

## 具体访问轨迹

容量三，依次 access `0,1,2,0`，每次登记为可淘汰：

|步骤|0 的时间|1 的时间|2 的时间|下个 victim|
|---|---|---|---|---|
|0|1|—|—|0|
|1|1|2|—|0|
|2|1|2|3|0|
|0|4|2|3|1|

连续淘汰输出 `victims: 1 2 0`。若淘汰 1 后保护 2，则先淘汰 0，接着返回空；解除保护后才能淘汰 2。可见“最近使用”不能凌驾于 pin 生命周期。

## 源码与复杂度

`src/lru.h` 的 access 更新时间，set_evictable 检查已登记状态，evict 扫描并重置。`src/demo.cpp` 对应表格。`tests/tests.cpp` 使用独立 recency 顺序向量比对重复访问轨迹，并验证空集、单项、未知 ID、保护、复用以及指定淘汰顺序。

访问与保护设置 O(1)，淘汰 O(F)，固定空间 O(F)。工程实现可用哈希表配双向链表减少选择成本，本例不声称 O(1) 淘汰。

## 限制和反例

单线程，仅接受 `[0, capacity)` Frame ID，不维护 page ID。容量零合法为空集，越界和未登记的保护设置抛异常。顺序扫描大量只访问一次的页可能污染 LRU，使热点被淘汰；LRU-K 通过多次访问历史缓解这一点。这里不实现文件 I/O，也不假装是完整缓冲池；调用者必须准确维护可淘汰状态。

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
