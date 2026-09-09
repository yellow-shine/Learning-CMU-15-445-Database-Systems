# 47-volcano-execution：逐元组拉取执行

## 问题与前置知识

数据库执行层怎样在不保存整个中间结果的情况下执行 `SELECT key WHERE value >= 10`？
前置是 C++ 引用、虚函数和顺序容器。这里通过 Scan → Filter → Projection 展示 Volcano；
48 与 49 分别改用完整物化和批量。三个主题采用同一输入与查询，不能用耗时单次测量推断谁更快。

## 原理与不变量

父算子的 Next 向子算子拉取一行。Scan 保存游标；Filter 每次可能跳过多行；Projection 真正把双列 Row
转换为单列 key。Init 递归重置整棵树，包括计数器；构造后也处于初始状态，但推荐显式 Init。
引用的子算子必须比父算子活得久。返回 false 不修改输出，耗尽状态不会再次读取子节点。
没有预取，所以取第一条结果只访问必要的输入前缀。重复行保留，顺序保持输入顺序，无 NULL。

## 具体轨迹

| 拉取 | Scan 消费 | 返回 key |
|---|---|---|
| 1 | (1,5)、(2,20) | 2 |
| 2 | (3,10) | 3 |
| 3 | (2,20) | 2 |
| 4 | EOF | false |

运行 demo 输出 `2 3 2`，下一行 `scan calls=5 reads=4 filter calls=4 projection calls=4`。
Scan 的 5 次调用包含 EOF，reads 只统计真实行。测试还验证首条结果的读取数、重复 EOF 不访问子节点、
Init 重放、空输入以及全部被过滤。这些反例区分“没有结果”和“尚未初始化”。

## 成本

读取 n 行，过滤输出 k 行：Scan 调用 n+1 次，Filter 和 Projection 各 k+1 次。
时间 O(n)，除输入表的 O(n) 存储之外，流水处理额外空间 O(1)。
Scan 拥有表的快照是教学输入容器，不是扫描阶段物化中间结果；没有页 I/O。

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

单线程内存整数表，仅 >= 过滤与 key 投影；无 NULL、SQL 解析、磁盘或并发。虚调用次数用于机制对比，不等于 CPU 时间。
