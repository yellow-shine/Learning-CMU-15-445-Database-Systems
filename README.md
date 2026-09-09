# 63-transaction-acid：ACID 与原子转账

## 问题与前置知识

本分支是事务层的独立 C++17 教学实现，不依赖 BusTub 或其他分支。
先了解事务读写、集合、映射和基本串行执行；相关概念可在 `main` 的知识点目录查看。
这里使用**确定性单线程调度模拟**，调用顺序就是交错顺序。没有真实锁等待或线程并发承诺。
模拟的价值是把失败条件固定下来，不依赖 sleep 或操作系统调度运气。

## 原理、不变量与手算轨迹

转账必须同时扣款和入账，不能只完成一半。Transaction 在开始时复制余额，所有操作先作用于私有 staged 副本。
commit 用不抛异常的 swap 发布整个副本；abort 和析构丢弃副本。任一转账参数失败会中止整个事务，而非仅跳过本次语句。
状态只能 Active → Committed 或 Active → Aborted，终态再次操作报错。Bank 同时只允许一个活动事务，这是串行化约束，不是并发锁管理器。

| 步骤 | 已发布余额 | 私有余额 |
| --- | --- | --- |
| begin | 100,50 | 100,50 |
| transfer 30 | 100,50 | 70,80 |
| commit | 70,80 | 不再使用 |
| begin; transfer 10 | 70,80 | 60,90 |
| abort | 70,80 | 丢弃 |

因此 demo 输出 `committed: 70 80` 与 `aborted: 70 80`。
A 是发布原子性；C 是余额非负与总额守恒（避免用可能溢出的总和来验证）；I 是本例强制串行；D **未实现**。
越界账户、负金额、余额不足、目标溢出都在修改前检查；同账户转账是经过余额验证的无操作。
测试覆盖已完成状态、异常回滚、析构回滚、空银行和整数边界。

## 源码导读

- `src/tutorial.hpp`：核心状态与算法；先读数据成员，再按 demo 顺序跟踪状态转换。
- `src/demo.cpp`：小型可重现调度，输出最终状态／判定，不是算法的替代品。
- `tests/test.cpp`：独立正例、冲突反例与边界测试；`check` 失败抛异常，main 返回非零，Release 同样有效。
- `CMakeLists.txt`：无第三方库的 demo 与 CTest 入口，禁用编译器语言扩展。

把读操作、写操作及提交操作分开，是为了观察“读到什么”和“最终能否提交”并非同一个问题。
学习时可交换测试中的两次调用，手算结果，再运行测试验证；不要把一次成功调度当成所有调度都安全的证明。

## 成本

开始事务 O(n) 时间和空间；单次转账 O(1)，提交 swap O(1)，析构释放副本 O(n)。无磁盘 I/O。

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

仅允许一个活动事务；Bank 生命周期必须长于 Transaction。Transaction 保存 Bank 引用，因此 Bank 禁止复制／移动构造与赋值，保持对象身份稳定，防止复制忙状态或赋值绕过单活动事务约束。测试以四项类型特征静态断言覆盖这些禁用操作，并保留第二个活动事务被拒绝的运行时检查。原子内存发布不等于崩溃持久性。

所有状态仅存内存，没有 WAL、磁盘同步、崩溃恢复或跨进程保证。
标准容器仅存储事务元数据；被讲解的冲突规则与状态转换在源码中直接实现。
此示例不提供 SQL、网络服务或生产系统的资源管理；调用者必须使用规定的生命周期。
