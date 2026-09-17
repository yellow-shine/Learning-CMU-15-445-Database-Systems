# 67-timestamp-ordering：时间戳顺序控制

## 问题与前置知识

本分支是事务层的独立 C++17 教学实现，不依赖 BusTub 或其他分支。
先了解事务读写、集合、映射和基本串行执行；相关概念可在 `main` 的知识点目录查看。
这里使用**确定性单线程调度模拟**，调用顺序就是交错顺序。没有真实锁等待或线程并发承诺。
模拟的价值是把失败条件固定下来，不依赖 sleep 或操作系统调度运气。

## 原理、不变量与手算轨迹

begin 分配严格递增的事务时间戳 TS。每项保存已提交 value、最大读时间戳 RTS、最近已提交写时间戳 WTS。
读：若 TS < WTS，旧事务读到了“未来”版本，只能中止；否则推进 RTS 并返回本事务私有写或已提交值。
写：若 TS < RTS 或 TS < WTS，中止；否则写入私有缓冲。这里不是 Thomas write rule，不会静默忽略旧写。
提交：对所有缓冲写重新检查同一规则，全部通过才发布，避免缓冲期间新读／新写令原检查失效。

这是**延迟发布的保守 TO 变体**，不是会暴露未提交值的基本即时写 TO。
RTS 即使读者中止也不回退，可能多中止，但不会漏掉冲突。私有写中止不会污染已提交数据。
单线程 commit 中先检查所有键，再只做整数赋值，避免逻辑冲突导致半提交。

手算：x=10，T1 的 TS=1，T2 的 TS=2。T2 读使 RTS(x)=2；T1 写因 1<2 被拒绝。
demo 输出 `young reads: 10`、`old write accepted: 0`、`young commits: 1`。
另一个反例是 T1 已缓冲写 x，随后 T2 读 x；T1 必须在提交重检时中止。
测试同时覆盖 WTS 读冲突、WTS 写冲突、多键提交原子性、读己之写和未提交隔离。

## 源码导读

- `src/tutorial.hpp`：核心状态与算法；先读数据成员，再按 demo 顺序跟踪状态转换。
- `src/demo.cpp`：小型可重现调度，输出最终状态／判定，不是算法的替代品。
- `tests/test.cpp`：独立正例、冲突反例与边界测试；`check` 失败抛异常，main 返回非零，Release 同样有效。
- `CMakeLists.txt`：无第三方库的 demo 与 CTest 入口，禁用编译器语言扩展。

把读操作、写操作及提交操作分开，是为了观察“读到什么”和“最终能否提交”并非同一个问题。
学习时可交换测试中的两次调用，手算结果，再运行测试验证；不要把一次成功调度当成所有调度都安全的证明。

## 成本

读写 O(log n + log t + log w)；提交检查／发布 O(w log n)。状态 O(n+t+未提交写总数)，不回收事务 ID 元数据。

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

延迟发布 TO 比基本算法保守，RTS 不随中止回退；固定键集合，未知键抛异常但不自动中止事务。无等待或重试调度器。

所有状态仅存内存，没有 WAL、磁盘同步、崩溃恢复或跨进程保证。
标准容器仅存储事务元数据；被讲解的冲突规则与状态转换在源码中直接实现。
此示例不提供 SQL、网络服务或生产系统的资源管理；调用者必须使用规定的生命周期。
