# 69-mvcc-versioning：Undo 链与历史元组重建

## 问题与前置知识

本分支是事务层的独立 C++17 教学实现，不依赖 BusTub 或其他分支。
先了解事务读写、集合、映射和基本串行执行；相关概念可在 `main` 的知识点目录查看。
这里使用**确定性单线程调度模拟**，调用顺序就是交错顺序。没有真实锁等待或线程并发承诺。
模拟的价值是把失败条件固定下来，不依赖 sleep 或操作系统调度运气。

## 原理、不变量与手算轨迹

元组在这里是一列可空整数；optional 的空表示整行不存在／删除，不表示 SQL NULL。
Row 保存最新已提交值和提交时间戳，undo 数组按提交顺序存完整 before-image：上一个值与上一个时间戳。
读取从最新值开始，若其时间戳大于快照，就逆向应用一条 Undo，直到重建出 timestamp≤snapshot 的状态。
这是真正的历史重建，不是直接返回当前值。完整 before-image 是最简单的 Undo 形式，不做列级 delta 压缩。

事务 begin 捕获当前提交时钟，写入私有缓冲；仅自身读得到未提交写。commit 通过写冲突检查后给所有写分配同一个新提交时间戳。
中止丢弃缓冲，不在链中留下未提交版本。新插入键从不存在的 timestamp=0 状态起链；删除也追加 Undo 并发布 tombstone，因此旧快照仍能看到被删除行。

手算：初始 `(ts=0,100)`；提交写 80 后头为 `(1,80)`，undo 为 `[(0,100)]`。
快照 0 逆向应用 undo 得 100，快照 1 直接返回 80。
demo 依次输出 `before commit: 100`、`old snapshot: 100`、`new snapshot: 80`。
三代测试进一步核对 10→20→30→删除→40，并验证历史快照看不到后来插入的键。

MVCC 是版本存储机制，并不自动等于可串行化。本例附带 first-committer-wins 支撑提交安全；下一主题聚焦其 SI 语义。

## 源码导读

- `src/tutorial.hpp`：核心状态与算法；先读数据成员，再按 demo 顺序跟踪状态转换。
- `src/demo.cpp`：小型可重现调度，输出最终状态／判定，不是算法的替代品。
- `tests/test.cpp`：独立正例、冲突反例与边界测试；`check` 失败抛异常，main 返回非零，Release 同样有效。
- `CMakeLists.txt`：无第三方库的 demo 与 CTest 入口，禁用编译器语言扩展。

把读操作、写操作及提交操作分开，是为了观察“读到什么”和“最终能否提交”并非同一个问题。
学习时可交换测试中的两次调用，手算结果，再运行测试验证；不要把一次成功调度当成所有调度都安全的证明。

## 成本

查键 O(log n)，重建 O(v) 遍历该行需要撤销的版本。写缓冲 O(log w)；为异常安全，提交复制全表及全部历史 O(n+H)，再追加 w 条 undo。空间 O(n+H+事务缓冲)。

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

不回收历史版本和终态事务；完整一列 before-image，不是压缩 delta。提交复制全表，适合小型教学；实现 SI 的写冲突检查但不阻止 write skew。

所有状态仅存内存，没有 WAL、磁盘同步、崩溃恢复或跨进程保证。
标准容器仅存储事务元数据；被讲解的冲突规则与状态转换在源码中直接实现。
此示例不提供 SQL、网络服务或生产系统的资源管理；调用者必须使用规定的生命周期。
