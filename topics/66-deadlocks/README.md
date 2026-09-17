# 66-deadlocks：等待图与死锁牺牲者

## 问题与前置知识

本分支是事务层的独立 C++17 教学实现，不依赖 BusTub 或其他分支。
先了解事务读写、集合、映射和基本串行执行；相关概念可在 `main` 的知识点目录查看。
这里使用**确定性单线程调度模拟**，调用顺序就是交错顺序。没有真实锁等待或线程并发承诺。
模拟的价值是把失败条件固定下来，不依赖 sleep 或操作系统调度运气。

## 原理、不变量与手算轨迹

每个资源只有一个 X 锁持有者，每个事务最多一个尚未完成的请求。
request 遇到其他持有者时记下等待资源，返回 false；等待不是线程阻塞。
waits_for 每次从当前 owner 与 pending 请求重建边，因此持有者结束后不会留下错误的陈旧依赖。
图每个顶点出度至多 1，沿边走，若再次遇到当前路径上的顶点，就截取真实环；走到已完成路径不构成新环。

T1 持有 a，T2 持有 b；T1 请求 b 得 1→2，T2 请求 a 得 2→1。
demo 输出 `cycle size: 2`，选择环内最大 ID 为牺牲者，输出 `victim: 2`。
中止 T2 会释放其所有资源并删除其等待请求；T1 显式重试 b 成功，输出 `retry T1: 1`。
ID 大小仅用于确定性牺牲者策略，不声称衡量工作量。环外等待者即使 ID 更大也不会被选择。
多环调用 resolve_one 多次。pending 事务不能提交，也不能绕过等待继续请求别的资源。
测试涵盖三节点环、无环链、环外等待者、两个独立环、已中止访问和释放后依赖消失。

## 源码导读

- `src/tutorial.hpp`：核心状态与算法；先读数据成员，再按 demo 顺序跟踪状态转换。
- `src/demo.cpp`：小型可重现调度，输出最终状态／判定，不是算法的替代品。
- `tests/test.cpp`：独立正例、冲突反例与边界测试；`check` 失败抛异常，main 返回非零，Release 同样有效。
- `CMakeLists.txt`：无第三方库的 demo 与 CTest 入口，禁用编译器语言扩展。

把读操作、写操作及提交操作分开，是为了观察“读到什么”和“最终能否提交”并非同一个问题。
学习时可交换测试中的两次调用，手算结果，再运行测试验证；不要把一次成功调度当成所有调度都安全的证明。

## 成本

重建等待图 O(w log k)，找环 O(t log t)，释放扫描 O(k)。空间 O(t+k+w)，无 I/O。

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

只模拟独占锁和一次一个 pending 请求，不提供 S 锁、自动唤醒、公平调度或饥饿防止；提交／中止仅管理锁元数据。

所有状态仅存内存，没有 WAL、磁盘同步、崩溃恢复或跨进程保证。
标准容器仅存储事务元数据；被讲解的冲突规则与状态转换在源码中直接实现。
此示例不提供 SQL、网络服务或生产系统的资源管理；调用者必须使用规定的生命周期。
