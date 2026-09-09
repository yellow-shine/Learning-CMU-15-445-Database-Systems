# 74 · 缓冲策略如何决定 Undo / Redo

数据库缓冲池决定何时把脏页交给磁盘；事务管理器决定哪些修改已提交。两者不是同一个时钟。本教程用**确定性内存模型**让四种策略产生可观察的磁盘状态，并故意关闭 Undo 或 Redo 构造反例。不是实际文件恢复系统；真实 WAL 与进程崩溃见 73，ARIES 见 77。前置知识是脏页、事务提交和 before/after image。返回目录：`git show main:README.md`。

## 两个独立开关

* Steal：允许未提交脏页被换出，磁盘可能含 loser 的值，因此需要 Undo。
* No-Steal：活跃事务拥有的页不能被换出，`evict` 返回 false。可能因为全部帧被活跃事务占用而无法继续分配。
* Force：提交时所有本事务拥有的页都写回。已提交值不会只留在内存，因此不需要 Redo。
* No-Force：提交不强制写页。崩溃可能丢失内存中已提交更新，因此需要 Redo。

| 策略 | Undo 必需 | Redo 必需 |
| --- | --- | --- |
| No-Steal + No-Force | 否 | 是 |
| No-Steal + Force | 否 | 否 |
| Steal + No-Force | 是 | 是 |
| Steal + Force | 是 | 否 |

“必需”指存在需要它的合法执行，不代表每次崩溃都要改页。No-Steal + Force 也不代表完整数据库不需要日志：这里假设提交决策及多页 force 原子完成，不模拟提交中途断电。

## 工作轨迹：一个 winner、一个 loser

初始两个磁盘页为 `[0,0]`。T1 把 P0 改为 10 并提交；T2 把 P1 改为 99，尝试换出但不提交。`crash()` 丢弃内存，重新读模型的磁盘数组。demo 的真实输出：

```
steal=0 force=0 crash=[0,0] undo=0 redo=1 recovered=[10,0]
steal=0 force=1 crash=[10,0] undo=0 redo=0 recovered=[10,0]
steal=1 force=0 crash=[0,99] undo=1 redo=1 recovered=[10,0]
steal=1 force=1 crash=[10,99] undo=1 redo=0 recovered=[10,0]
```

Redo 正序应用 winner 的 after image；Undo 逆序应用 loser 的 before image。测试特意把同一页改两次：逆序不能换成正序，否则最终值是中间版本。页的严格事务所有权阻止脏写，提交才释放；否则一个 loser 的 before image 可能覆盖其他事务的提交结果。

## 源码路线与成本

`src/policies.hpp` 的 `BufferModel` 保存 memory/disk、页 owner、事务状态及日志。先读 `evict` 的 No-Steal 拒绝条件，再读 `commit` 的 Force 写回，最后读 `recover` 的两个独立分支。`src/demo.cpp` 枚举策略，`tests/tests.cpp` 不依赖 demo，也不使用会被 Release 消除的 assert。

更新 O(1)，换出 O(1)，提交扫描 P 个页 O(P)，恢复 O(L)，日志空间 O(L)。本例 P=2、最多两个事务，`writes` 统计模型页写次数，不是测量真实 I/O 延迟。Steal 放松容量约束但引入 Undo；No-Force 合并多次写但引入 Redo。

## 运行与验证

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

macOS 若缺少 libc++ 头文件，两个配置命令条件性附加：
```sh
-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

测试逐一验证四种策略的崩溃页内容、缺失 Undo/Redo 的必要性反例、两次更新、重复恢复、空输入、已提交换出、无效事务号与写冲突。预期 CTest 一项通过，测试打印 `four policies: ... passed`。

## 明确边界

没有文件、fsync、线程、进程终止、真正容量驱逐或日志损坏解析；`crash` 只是确定性状态转换，不能证明断电安全。提交是模型中的原子步骤；只允许 1、2 两个不重用的事务号和 0、1 两个页号，恢复后用于检查结果而不是继续接收业务。无锁等待、回滚 API 或部分页提交。这样缩小的是物理系统范围，不是四种策略与 Undo/Redo 的因果关系。完整生产系统还需要 WAL 顺序和恢复中再次崩溃的处理。
