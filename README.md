# 51-access-executors：顺序扫描与索引访问路径

## 问题与前置知识

同一个谓词可以走不同访问路径。本主题属于执行层，而不是索引结构课：
用明确标记的静态有序数组索引，展示查找候选 RID 后回表，与逐行检查全表的差异。
前置为二分查找、稳定行标识和选择谓词。没有用标准容器冒充 B+Tree。

## 数据结构、算法与不变量

AccessTable 拥有不可修改的 Row 数组，RID 是从 0 开始的数组下标；构造时建立 `(key,RID)` 有序数组。
相同 key 按 RID 排序，每行恰有一个索引条目。不可修改接口避免索引与表失配。
SeqScan 对每行检查 `low <= key <= high AND value >= minimum`。
IndexScan 手写二分定位第一个 key >= low，再遍历到 key > high，按 RID 回表判断 value 残余谓词。
残余条件不能仅靠 key 索引回答；因此未通过残余条件的候选也计一次 heap_reads。
low > high 被定义为空区间，不访问数据。结果物化为 RID 数组，每次调用状态独立，无游标耗尽协议。

## 可复算轨迹

RID 0..5 的行是 `(8,80),(2,20),(5,1),(2,25),(9,90),(4,40)`。
索引为 `(2,1),(2,3),(4,5),(5,2),(8,0),(9,4)`。
查询 key 在 [2,4]、value >= 22：
- SeqScan 读取 6 行，RID 3、5 匹配。
- 二分 3 次比较定位索引位置 0；回表 RID 1、3、5，RID 1 被残余条件拒绝。

Demo 输出 `SeqScan RIDs: 3 5 heap reads=6` 和
`IndexScan RIDs: 3 5 heap reads=3 index entries=3 seek comparisons=3`。
这些是逻辑访问计数，不是缓存缺失、磁盘页数或执行时间；index_entries 只计区间内条目，不含二分及终止检查。

## 正确性与复杂度

重复 key 不覆盖行，重复行保留。SeqScan 按 RID 顺序、IndexScan 按 key/RID 顺序，
一般只保证结果多重集相等，不能无 ORDER BY 宣称相同顺序。测试先规范化 RID 后比较，
对 101 行、所有 [-8,8] 边界组合和三个残余阈值做差分，另测空表、不相交、反向区间和整数极值。

索引一次构建 O(n log n)、空间 O(n)。SeqScan O(n)；IndexScan O(log n+c)，c 为区间候选数，
结果空间 O(k)。索引不是总更快：全区间仍回表 n 次并增加索引访问，随机访问在磁盘上还可能更昂贵。

## 构建与验证

本分支完全独立，C++17 标准库足够，无需 BusTub 或其它分支。使用 CMake 3.16+：

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

如果 macOS 的 AppleClang 报找不到标准库头文件，给上述两个配置命令附加
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`；
这是本机 SDK 搜索路径排错，不是算法的依赖，不应硬编码进 CMake。

## 源码导读与测试入口

先读 `src/engine.h` 的数据结构，再沿调用方向读算子，最后读 `src/demo.cpp` 的装配。
`tests/topic_tests.cpp` 是独立正确性程序，CHECK 失败抛异常并返回非零，Release 不会移除检查。
CTest 运行它，demo 只用于可见轨迹，不代替测试。实现为单头文件，便于对照循环和状态变化；
没有 SQL 解析器，也没有依赖隐藏的运行时或文件数据。总目录请通过 `git show main:README.md` 查看。

## 范围与局限

静态有序数组辅助索引，不是 B+Tree；不可变内存表、物化 RID 输出、无页 I/O/NULL/优化器。只保证多重集等价，不保证两个访问路径的输出顺序相同。
