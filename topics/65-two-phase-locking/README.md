# 65-two-phase-locking：两阶段锁与升级

## 问题与前置知识

本分支是事务层的独立 C++17 教学实现，不依赖 BusTub 或其他分支。
先了解事务读写、集合、映射和基本串行执行；相关概念可在 `main` 的知识点目录查看。
这里使用**确定性单线程调度模拟**，调用顺序就是交错顺序。没有真实锁等待或线程并发承诺。
模拟的价值是把失败条件固定下来，不依赖 sleep 或操作系统调度运气。

## 原理、不变量与手算轨迹

S 锁之间兼容；S/X、X/S、X/X 在不同事务之间都不兼容。
本事务已有 X 可以满足 S 请求，已有 S 再请求 S 不新增锁。S→X 升级必须没有其他持有者。

2PL 分为增长期（可申请／升级）和收缩期（只能释放）；第一次成功 unlock 进入收缩期。
重复请求已经持有的足够权限是幂等操作，不算新增锁。失败申请不改变阶段或已持有锁。
Strict 2PL 额外保留 X 锁直到 finish；S 可以提前释放但随即进入收缩期。
这不是把所有 S 也保留到结束的 rigorous 2PL。

轨迹：T1 S(row)，T2 S(row)，T1 升级 X 返回 0 且保留 S；T2 finish 释放；T1 再升级返回 1。
demo 对应 `upgrade with reader: 0` 与 `upgrade after release: 1`。
finish 统一模拟提交或中止之后的锁释放，禁止重用同一事务 ID。
测试包含矩阵四种组合、升级、收缩期禁止申请、终态拒绝、Strict X 提前释放失败。
调用者必须在读写数据前主动申请正确模式；此分支专门演示锁协议而非存储引擎。

## 源码导读

- `src/tutorial.hpp`：核心状态与算法；先读数据成员，再按 demo 顺序跟踪状态转换。
- `src/demo.cpp`：小型可重现调度，输出最终状态／判定，不是算法的替代品。
- `tests/test.cpp`：独立正例、冲突反例与边界测试；`check` 失败抛异常，main 返回非零，Release 同样有效。
- `CMakeLists.txt`：无第三方库的 demo 与 CTest 入口，禁用编译器语言扩展。

把读操作、写操作及提交操作分开，是为了观察“读到什么”和“最终能否提交”并非同一个问题。
学习时可交换测试中的两次调用，手算结果，再运行测试验证；不要把一次成功调度当成所有调度都安全的证明。

## 成本

一次申请扫描该键 h 个持有者，O(h + log k + log t)；结束事务扫描全部键 O(k log t)。锁记录空间 O(持锁总数)。

## 构建、测试与预期结果

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

两种模式均应显示 CTest 1/1 通过；直接运行测试输出 `all checks passed`。
demo 的数值含义见上面的具体轨迹，布尔值使用 0/1。
若 macOS 的 AppleClang 报标准库头文件不存在，可仅在本机配置时附加：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

Release 配置可附加相同参数；不需要改系统文件或把 SDK 路径写进项目。

## 局限与不能推出的结论

非阻塞确定性锁调度模拟；失败申请返回 false，调用者显式重试，无等待队列、公平性或死锁检测。

所有状态仅存内存，没有 WAL、磁盘同步、崩溃恢复或跨进程保证。
标准容器仅存储事务元数据；被讲解的冲突规则与状态转换在源码中直接实现。
此示例不提供 SQL、网络服务或生产系统的资源管理；调用者必须使用规定的生命周期。
