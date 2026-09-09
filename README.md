# 70-snapshot-isolation：快照隔离与首次提交者胜出

## 问题与前置知识

本分支是事务层的独立 C++17 教学实现，不依赖 BusTub 或其他分支。
先了解事务读写、集合、映射和基本串行执行；相关概念可在 `main` 的知识点目录查看。
这里使用**确定性单线程调度模拟**，调用顺序就是交错顺序。没有真实锁等待或线程并发承诺。
模拟的价值是把失败条件固定下来，不依赖 sleep 或操作系统调度运气。

## 原理、不变量与手算轨迹

本分支随附 69 的 Row／Undo 与私有写缓冲源码快照，独立编译；新增 scan 并聚焦 Snapshot Isolation 的事务规则。
SI 要求所有普通读都使用 begin 时刻的已提交快照，读己之写覆盖快照；范围扫描同样重建旧状态并叠加本事务写集。
提交时检查每个写目标的最新提交时间戳：若大于自己的 snapshot，存在重叠事务的写写冲突，整个事务中止。
检查包括后来插入的新键和被删除的 tombstone，不能仅检查仍然存在的值。

轨迹：A、B 在 x=0 时开始，私有写分别为 1 与 2；A 先提交，将最新时间戳从 0 推进至 1。
B 仍读到自己的 2，但提交检查 1>snapshot(0)，所以中止。demo 输出 A commits=1、B still reads own=2、B commits=0、published x=1。
冲突比较的是提交先后，不是事务编号；读到私有结果不能证明事务能够提交。

关键反例：alice=bob=1，各表示在岗。A 读 bob 后把 alice 写 0；B 读 alice 后把 bob 写 0。
写集不相交，SI 允许两者成功，最终无人值班。这是 write skew，不是丢失更新。
测试明确保留此反例，避免错误宣称“多版本阻止全部异常”。
其他测试覆盖快照点读稳定、插入／删除后扫描稳定、读己之写、同键插入冲突、删除更新冲突及失败不发布其他键。

## 源码导读

- `src/tutorial.hpp`：核心状态与算法；先读数据成员，再按 demo 顺序跟踪状态转换。
- `src/demo.cpp`：小型可重现调度，输出最终状态／判定，不是算法的替代品。
- `tests/test.cpp`：独立正例、冲突反例与边界测试；`check` 失败抛异常，main 返回非零，Release 同样有效。
- `CMakeLists.txt`：无第三方库的 demo 与 CTest 入口，禁用编译器语言扩展。

把读操作、写操作及提交操作分开，是为了观察“读到什么”和“最终能否提交”并非同一个问题。
学习时可交换测试中的两次调用，手算结果，再运行测试验证；不要把一次成功调度当成所有调度都安全的证明。

## 成本

点读 O(log n+v)，全表扫描 O(n log n+H+w log n) 上界；提交复制全表历史 O(n+H) 加写集处理。历史空间 O(H)，事务缓冲 O(w)。

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

SI 不保证可串行化，允许 write skew；全表复制提交，无历史回收、索引、SQL NULL、范围锁或真实线程并发。

所有状态仅存内存，没有 WAL、磁盘同步、崩溃恢复或跨进程保证。
标准容器仅存储事务元数据；被讲解的冲突规则与状态转换在源码中直接实现。
此示例不提供 SQL、网络服务或生产系统的资源管理；调用者必须使用规定的生命周期。
