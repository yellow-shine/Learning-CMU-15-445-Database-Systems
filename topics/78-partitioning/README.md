# 78-partitioning：哈希与范围分区

## 问题与前置知识

分布式存储先回答“这条记录归谁”。前置知识是整数余数、二分查找、向量；可继续阅读 81 数据交换。这里实现纯路由，不启动网络节点。

## 原理与不变量

`hash_route` 用明确的无符号 64 位整数转换后取模，避免负数余数和 INT64_MIN 取绝对值溢出。它是确定性的简易哈希，不是密码哈希；分区数必须大于零。每个键恰好进入一个桶。
`RangeRouter` 要求分割点严格递增，使用 upper_bound：分割点属于右侧分区。切点 [0,10] 表示 (-∞,0)、[0,10)、[10,+∞)。空切点是一个分区。分区数变更会改变路由，现有数据必须迁移，不能直接改变配置。
`statistics` 保留空桶，计算各桶行数、最大桶与平均桶之比；空输入的比值定义为 0。该比值只量化行数，不能代表 CPU 或字节负载。

## 手算轨迹

输入 [-1,0,9,10,10]，切点 [0,10]，路由依次为 [0,1,1,2,2]，计数 [1,2,2]，平均 5/3，最大/平均 = 1.2。
输入 [0,4,8,12] 在四路取模下全部进桶 0，计数 [4,0,0,0]，比值 4。即使桶数多，规律键仍可能严重倾斜。demo 输出 `range: 1 2 2 skew=1.2` 与 `hash: 4 0 0 0 skew=4`。

## 源码导读与成本

`src/partitioning.hpp` 包含两种路由及统计，`src/demo.cpp` 重现上述轨迹，`tests/tests.cpp` 检查空集合、非法切点、零桶、极端整数、边界归属及每条记录只统计一次。
哈希每键 O(1)；范围构造 O(p)，每键 O(log p)；统计 O(n+p) 时间、O(p) 空间。没有磁盘或网络 I/O。

## 局限与反例

不实现一致性哈希、动态拆分、复制、热点键拆散或分布式目录。同一键总在同一桶，热点无法仅靠增加桶数消除。实际系统还要考虑跨节点事务、迁移版本与故障恢复。

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
