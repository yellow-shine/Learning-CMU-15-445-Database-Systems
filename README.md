# 50-pipelines：阻塞算子与阶段边界

## 问题与前置知识

执行流水线不是计划树的同义词：排序必须看到所有输入，才知道第一行是谁。
本例在执行层实现 `Scan → Filter → Sort(value ASC) → Projection(key)`，
前置为迭代器、排序和物化概念（47、48）。Sort 是真正的 pipeline breaker，不是打印一个阶段名。

## 执行阶段与不变量

```text
P1: Scan --逐行--> Filter --逐行--> Sort.sink
                                      | 全部输入结束后 stable_sort
P2:                              Sort.source --> Projection
```

Init 只清空状态。第一次 Next 驱动 P1，把通过过滤的行放进 sorted，实际调用 stable_sort 后才建立 P2。
后续 Next 只读 sorted。P1 内一行从 scan 流到 sink 后才取下一行；P2 内从 source 直接投影输出。
任何 sort.source 必须晚于所有 scan 和 sort.ready。事件由真实循环和状态转换产生；若移动排序或提前输出，
测试的前缀轨迹会失败。这里是单线程手工装配的两个流水阶段，不是通用调度器。

## 具体轨迹与预期输出

输入 `(1,30),(2,5),(3,10),(4,10)`，过滤 value >= 10。
- P1.begin 后 scan:1 / filter.pass:1 / sort.sink:1。
- scan:2 / filter.drop:2，不进入排序缓冲。
- key 3、4 都依次 pass、sink。此时 reads=4、buffered_rows=3。
- P1.end → sort.ready → P2.begin；之前没有任何结果。
- sort.source:3 / project:3，然后 4，再 1；最终 P2.end。

Demo 打印上述完整事件序列，最后一行 `result: 3 4 1`。
value 相等时 stable_sort 保留原顺序，因此 3 在 4 前；重复行不去重，无 NULL。
空输入仍有 P1.begin/end、sort.ready、P2.begin/end，但没有 source 事件。
测试逐项检查第一条输出前的完整事件前缀，并检查耗尽幂等、重启、全拒绝与稳定并列值。

## 成本与观察口径

n 行输入、k 行通过过滤：扫描 O(n)，stable_sort 通常 O(k log k)，标准库在内存不足的回退路径可为
O(k log² k) 比较；缓冲 O(k)，另外输入快照 O(n)、教学轨迹 O(n+k)。首次输出前必须扫描 n 行。
不限制调用者只读多少结果：即使只取 1 行，也不能省略 P1。事件日志会干扰性能，不适合用来做吞吐基准。

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

单线程固定 Scan/Filter/内存稳定排序/Projection；不是并行调度器或外部排序，没有溢写与内存预算。trace 是实际事件日志，不代表异步任务执行。
