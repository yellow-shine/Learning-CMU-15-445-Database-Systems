# 68-optimistic-concurrency-control：乐观并发控制

## 问题与前置知识

本分支是事务层的独立 C++17 教学实现，不依赖 BusTub 或其他分支。
先了解事务读写、集合、映射和基本串行执行；相关概念可在 `main` 的知识点目录查看。
这里使用**确定性单线程调度模拟**，调用顺序就是交错顺序。没有真实锁等待或线程并发承诺。
模拟的价值是把失败条件固定下来，不依赖 sleep 或操作系统调度运气。

## 原理、不变量与手算轨迹

OCC 把事务分成 Read（含私有写）、Validate、Write 三阶段，验证失败进入 Aborted，成功发布后进入 Committed。
每项保存单调版本号；首次读记住值与版本，重复读返回缓存；写也记录目标版本，故盲写冲突不会漏检。
提交验证整个读集当前版本是否仍等于观察版本，全部通过后才更新写集并递增版本号。
Validate 与 Write 在一次不可交错的模拟调用中完成；若把它们拆成可交错的线程方法，会出现验证后状态变化的竞态。

轨迹：counter=0/version=0，A 与 B 各读到 0 并私有写 1。
A 验证版本 0，发布 1/version=1；B 验证看到版本 1≠0，中止。
demo 输出 `commit A: 1`、`commit B: 0`、`counter: 1`。
正确重试必须 begin 新事务并重新读取，不可直接拿旧结果再次提交。

缓存读保证同一键重复读一致，但不同键可能来自不同时间；验证成功才证明这个固定键读集可按提交顺序串行化。
测试包括读写冲突、盲写冲突、互不相交写成功、write skew 被读集验证拦截、回滚和多键失败不发布。

## 源码导读

- `src/tutorial.hpp`：核心状态与算法；先读数据成员，再按 demo 顺序跟踪状态转换。
- `src/demo.cpp`：小型可重现调度，输出最终状态／判定，不是算法的替代品。
- `tests/test.cpp`：独立正例、冲突反例与边界测试；`check` 失败抛异常，main 返回非零，Release 同样有效。
- `CMakeLists.txt`：无第三方库的 demo 与 CTest 入口，禁用编译器语言扩展。

把读操作、写操作及提交操作分开，是为了观察“读到什么”和“最终能否提交”并非同一个问题。
学习时可交换测试中的两次调用，手算结果，再运行测试验证；不要把一次成功调度当成所有调度都安全的证明。

## 成本

首次／重复读写为 O(log n + log t + log r)，提交 O((r+w) log n)，每事务缓存 O(r+w)。无版本链或磁盘 I/O。

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

固定键集合的保守逐键版本验证，不支持范围扫描／插入；读阶段不保证跨键快照或中止事务的 opacity，只有通过验证的事务可提交。

所有状态仅存内存，没有 WAL、磁盘同步、崩溃恢复或跨进程保证。
标准容器仅存储事务元数据；被讲解的冲突规则与状态转换在源码中直接实现。
此示例不提供 SQL、网络服务或生产系统的资源管理；调用者必须使用规定的生命周期。
