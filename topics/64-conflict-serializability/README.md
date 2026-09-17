# 64-conflict-serializability：冲突可串行化与优先图

## 问题与前置知识

本分支是事务层的独立 C++17 教学实现，不依赖 BusTub 或其他分支。
先了解事务读写、集合、映射和基本串行执行；相关概念可在 `main` 的知识点目录查看。
这里使用**确定性单线程调度模拟**，调用顺序就是交错顺序。没有真实锁等待或线程并发承诺。
模拟的价值是把失败条件固定下来，不依赖 sleep 或操作系统调度运气。

## 原理、不变量与手算轨迹

两个不同事务访问同一键且至少一个为写，就存在冲突；按操作的先后方向添加边，而不是按事务编号大小连边。
同一事务内操作不建立自环，读读和异键不冲突。图保留没有边的事务，集合去掉重复边。
Kahn 算法反复移除零入度顶点；移除数量不足意味着剩余子图有环，否则移除顺序就是冲突等价串行顺序。

轨迹：`R1(x), W2(x), R2(y), W1(y)`。
前一对得到 `1 -> 2`，后一对得到 `2 -> 1`；无零入度顶点，demo 最后输出 `serializable: 0`。
对照 `W1(x), R2(x), W2(y), R3(y)` 得到 1→2→3，返回 `[1,2,3]`。
这里判断的是冲突可串行化，不是视图可串行化，也不检查可恢复性。
测试另外穷举 4096 个四操作小调度，以二顶点图的双向边判据核对结果。

## 源码导读

- `src/tutorial.hpp`：核心状态与算法；先读数据成员，再按 demo 顺序跟踪状态转换。
- `src/demo.cpp`：小型可重现调度，输出最终状态／判定，不是算法的替代品。
- `tests/test.cpp`：独立正例、冲突反例与边界测试；`check` 失败抛异常，main 返回非零，Release 同样有效。
- `CMakeLists.txt`：无第三方库的 demo 与 CTest 入口，禁用编译器语言扩展。

把读操作、写操作及提交操作分开，是为了观察“读到什么”和“最终能否提交”并非同一个问题。
学习时可交换测试中的两次调用，手算结果，再运行测试验证；不要把一次成功调度当成所有调度都安全的证明。

## 成本

m 个操作成对比较 O(m²)，图最多 O(t²) 空间；有序容器使拓扑处理 O((t+e) log t)。无 I/O。

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

只接受已给定的读写调度，不包含提交、中止或实际数据执行；不判断视图可串行化。

所有状态仅存内存，没有 WAL、磁盘同步、崩溃恢复或跨进程保证。
标准容器仅存储事务元数据；被讲解的冲突规则与状态转换在源码中直接实现。
此示例不提供 SQL、网络服务或生产系统的资源管理；调用者必须使用规定的生命周期。
