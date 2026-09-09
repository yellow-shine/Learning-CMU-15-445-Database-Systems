# 77 · ARIES 教学子集：恢复中再次崩溃也能继续

76 可以从零重建整个数据库，但会反复做所有工作。ARIES 用 PageLSN 避免重复页修改，用补偿日志 CLR 记录已经做过的 Undo，并通过 undoNextLSN 跳过已经补偿的动作。本教程实现真正运行的 Analysis / Redo / Undo，真实 WAL 与页文件以及恢复中再次退出后的新进程恢复。

前置知识：73 的 WAL、74 的 Steal/No-Force、76 的 before/after image。支撑文件 I/O 和正常事务操作来自本模块 73/76 的源码快照，不需其他 checkout。总目录在 `main:README.md`。

## 数据结构与不变量

四个 unsigned 64 位整数页，各带 PageLSN。固定 72 字节日志有 kind、LSN、tx、page、before、after、prevLSN、undoNextLSN 和校验。Update 的 before 是旧值；**CLR 的 before 字段复用为被撤销的 Update LSN**，after 是应恢复的旧值。CLR 只可 Redo，不可再次 Undo。Commit 直接视为已完成 winner；End 是 loser 全部撤销后的终止记录。

* LSN 连续增长；prevLSN 必须是同事务的最后一条记录，不允许终止后重用事务号。
* 活跃事务独占写页直到提交，拒绝脏写。没有锁等待或并发执行器。
* 每次页刷盘前同步 WAL，Commit 返回前同步提交记录。
* Undo 先追加并同步 CLR，之后才改变页并刷盘。页的 PageLSN 更新为 CLR 的 LSN，而非被撤销记录的 LSN。
* CLR 的 undoNextLSN 是被撤销 Update 的 prevLSN；加载器验证目标、链、页与 before image 的一致性。

## 三阶段不是三个 print

### Analysis

扫描日志，Update/CLR 更新事务表 `(tx,lastLSN)` 并在 DPT 中记录首次出现的 recLSN；Commit/End 删除事务表项。最终留下 loser。因为没有 fuzzy checkpoint 和刷页日志，DPT 是从创世开始的保守超集，不声称精确反映实际脏页。

### Redo：重复历史

从最小 recLSN 开始，处理所有 Update 和 CLR，包括 loser 与已经 End 的事务。如果 `PageLSN < record.LSN` 才写 after image；否则计入 skipped。**不能只重做 winner**：若先前 CLR 已同步但尚未改页，必须重放 CLR 才能把页修正到 Undo 进度对应的状态。

### Undo：按链和全局逆序

有序 frontier 保存每个 loser 的待处理 LSN，每次选最大的。Update 产生 CLR，CLR 直接跳到 undoNextLSN。没有下一条时写 End。CLR.prevLSN 指向该事务当前最后的日志，而 undoNextLSN 指向尚待撤销的旧历史，二者用途不同。

`recover(hook)` 暴露固定故障点 analysis/redo/clr/undo/end/pages。hook 用于测试 `_exit`，不是算法替身。每次 Undo 都刷完整页快照是有意的同步密集简化，便于验证恢复进度；工业系统会批处理页 I/O。

## 工作轨迹与实际输出

T1 在 LSN1 把 P0 改为 40，LSN2 提交。T2 在 LSN3 把 P1 从 0 改为 20，在 LSN4 改为 99。把全部页刷盘后“重启”：

```
analysis losers=1 dirty=2
CLR lsn=5 target=4 undoNextLSN=3
CLR lsn=6 target=3 undoNextLSN=0
pages=40,0 redo=0 undo=2
repeat redo=0 undo=0 skipped=5
```

第一次 Redo 为零，因为磁盘 PageLSN 已是 1、4。撤销 LSN4 生成 CLR5 后，如果在改页前退出：

1. 磁盘仍 P1=99/PageLSN4，但日志已有 CLR5。
2. 新进程 Analysis 得到 T2.lastLSN=5。
3. Redo 应用 CLR5，把 P1 置 20/PageLSN5。
4. Undo 遇 CLR5 跳到 LSN3，不重复撤销 LSN4。
5. 新增 CLR6，把 P1 置 0；End7 结束 T2。

独立测试检查这一精确状态：恢复后只新增一个 CLR，redone=1、undone=1；再次恢复不新增日志，所有五条页修改被 PageLSN 跳过。另一个测试交错两个 loser，验证补偿目标全局次序为 4、3、2、1。

## 文件格式和失败处理

小端编码，无 native struct padding；FNV-1a 检测意外损坏而非恶意篡改。完整记录校验失败、非法类型/页号、非连续 LSN、错误 prev 链、错误 CLR 目标/undoNext、过早 End、页的 LSN/image 与 WAL 不一致均抛异常。尾部不足 72 字节视为部分写，截断至完整前缀并同步；完整坏末条不可当短尾丢弃。每个短尾长度 1..71 都被测试。

Fd RAII 关闭描述符，read/write 处理短 I/O 和 EINTR，系统调用失败不静默吞掉。页文件整体写临时文件、fsync、rename、同步目录，日志新建也同步目录。异常后必须重新打开恢复，不允许继续使用部分完成的实例。

## 源码、成本和运行

`src/wal.hpp`：Log 加载器 → Store 正常事务操作 → `recover` 的 TT/DPT、Redo、frontier/CLR。`src/demo.cpp` 输出实际表大小与 CLR 字段；`tests/tests.cpp` 用 fork 子进程 `_exit` 跳过析构，再由另一个进程重新打开文件。`src/temp.hpp` 保证唯一临时目录，不接触用户数据。

Analysis/Redo 扫描 O(L)，有序事务表开销使上界 O(L log T)，Undo O(U log T)。日志内存 O(L)，表 O(T+P)，固定 P=4；每次撤销新增 72 字节 CLR，每个 loser 一个 End。DPT 从创世扫描，PageLSN 减少页修改但不减少日志读取。没有检查点或日志回收，不能据此宣传长历史启动性能。

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

macOS 默认找不到 libc++ 时，两个配置命令条件性附加：

```sh
-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

预期 CTest 一项通过，超时 20 秒，运行期检查不受 NDEBUG 影响。测试覆盖提交与偷写四组合、六个恢复中断点、新进程重启、精确 CLR 跳转、PageLSN 幂等、两个 loser 的全局逆序、恢复后新事务、空库、错误输入与损坏日志。

## 与完整 ARIES 的距离

这是物理整值更新的单线程 POSIX 子集，不是完整工业 ARIES：无 fuzzy checkpoint、部分回滚、savepoint、嵌套顶层操作、逻辑 Undo、并发恢复、日志裁剪、分布式事务或真实页撕裂修复。Commit 合并了 winner 的完成语义，不单独写 winner End。必须保留完整日志，打开已有库先 recover，不能恢复活跃实例。页整体原子替换避开了真实扇区撕裂问题。进程退出保留操作系统页缓存；调用 fsync 不代表所有硬件都诚实，测试不证明断电安全。
