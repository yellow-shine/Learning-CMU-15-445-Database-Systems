# 71-isolation-anomalies：五类隔离异常的确定性调度

## 问题与前置知识

本分支是事务层的独立 C++17 教学实现，不依赖 BusTub 或其他分支。
先了解事务读写、集合、映射和基本串行执行；相关概念可在 `main` 的知识点目录查看。
这里使用**确定性单线程调度模拟**，调用顺序就是交错顺序。没有真实锁等待或线程并发承诺。
模拟的价值是把失败条件固定下来，不依赖 sleep 或操作系统调度运气。

## 原理、不变量与手算轨迹

Isolation 是可执行的调度模拟器：每个数据库实例选一种隔离策略，事务显式读／写／提交／中止。
RU 可读其他事务私有写；RC 每次读当前已提交数据；RR 对已存在的读行持有 S 锁到结束，写持有 X 锁，但没有范围／间隙锁；SI 复制 begin 的已提交快照并检查并发写冲突。
RR 的锁不兼容抛 WouldBlock，表示调度此步不能执行，事务仍活动，调用者中止或在释放后重试；这里没有真正阻塞线程。
SI 的快照复制是此分支的简易支撑，不冒充 Undo 链实现。五个函数块是真实数据操作而非预设结论。

| 异常 | 调度 | demo 数值 |
| --- | --- | --- |
| 脏读 | RU: W1(x=9), R2(x), A1, R2(x) | 9→0 |
| 不可重复读 | RC: R1(x), W2(x=1), C2, R1(x) | 0→1 |
| 幻读 | RR: Scan1, Insert2(b), C2, Scan1 | 行数 1→2 |
| 丢失更新 | RC: 两人读 0，先后各提交旧值+1 | 期望2，实际1 |
| 写偏斜 | SI: 两人各读另一人在岗，各写自己离岗 | 在岗2→0 |

RR 与 SI 不是同义词：本例 RR 保护已有行，仍允许新键进入查询结果；SI 固定快照看不到后来插入，但不同写键的 write skew 仍能提交。
RR 在写偏斜调度中两个升级都被对方 S 锁挡住，不能把“会等待／需死锁处理”误写成“一定都立即成功”。
RC 的读改写丢失更新不同于数据库内原子的 `SET x=x+1`，后者不能用这个客户端旧值调度来推断。
数据库产品的 RR 可能实现为 SI 或带 next-key 锁；本教程明确选择记录锁 RR，不代表所有厂商行为。
测试既检查五类反例，也验证更强策略的阻止／快照稳定行为。

## 源码导读

- `src/tutorial.hpp`：核心状态与算法；先读数据成员，再按 demo 顺序跟踪状态转换。
- `src/demo.cpp`：小型可重现调度，输出最终状态／判定，不是算法的替代品。
- `tests/test.cpp`：独立正例、冲突反例与边界测试；`check` 失败抛异常，main 返回非零，Release 同样有效。
- `CMakeLists.txt`：无第三方库的 demo 与 CTest 入口，禁用编译器语言扩展。

把读操作、写操作及提交操作分开，是为了观察“读到什么”和“最终能否提交”并非同一个问题。
学习时可交换测试中的两次调用，手算结果，再运行测试验证；不要把一次成功调度当成所有调度都安全的证明。

## 成本

SI begin 复制 O(n)，其他 begin O(log t)；读写依赖映射及持有者 O(log n+h)。扫描 O(n log n)，提交为异常安全复制 O(n)。空间含每个 SI 快照 O(n)。

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

仅整数键值插入／覆盖，不支持删除、SQL NULL、范围锁或混合隔离级别事务。RR 为记录锁模型，阻塞以异常返回；无真实线程、死锁自动处理或生产 SQL 隔离认证。

所有状态仅存内存，没有 WAL、磁盘同步、崩溃恢复或跨进程保证。
标准容器仅存储事务元数据；被讲解的冲突规则与状态转换在源码中直接实现。
此示例不提供 SQL、网络服务或生产系统的资源管理；调用者必须使用规定的生命周期。
