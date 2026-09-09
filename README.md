# 75 · 保存有活跃事务的检查点

检查点解决恢复必须从日志第一条开始重建的成本问题，属于持久性层。前置知识：73 的 WAL、LSN、脏页、事务的提交与未提交状态。`main` 的 README 是总目录。支撑文件 I/O 与 WAL 是本模块 73 的独立源码快照；本分支新增检查点格式、事务表 TT、脏页表 DPT、恢复起点计算与进程中断实验。

## 采用哪一种检查点

这里采用**暂停业务期间创建的已提交镜像检查点**，不是 ARIES fuzzy checkpoint。调用 `checkpoint()` 时单线程没有并发更新，但允许事务活跃，不强迫其提交。

检查点文件包含 magic、截止 LSN、恢复起点、TT/DPT 长度、四页已提交镜像、TT 的 `(tx,lastLSN)` 与 DPT 的 `(page,recLSN)`，末尾校验值。镜像由完整保留日志的已提交投影计算，因此不包含活跃事务的修改；不是简单复制可能已被偷写的磁盘页。

* TT 在 begin/update/commit 时维护；只读事务 lastLSN=0。
* DPT 在第一次更新时插入 recLSN，后续更新不覆盖；完整页快照落盘后清空。
* 安全起点为 `min(cutoff+1, 所有 recLSN, 活跃事务链中的所有 LSN)`。
* 活跃事务即使所有脏页已落盘，也必须保留其更新历史：它可能在检查点之后提交，不能只从 cutoff 开始重做。

`checkpoint()` 先同步 WAL，再写临时检查点、同步文件、rename、同步目录。若发布前退出，旧检查点仍有效；没有旧文件则从零恢复。完整检查点损坏明确报错，不猜测字段、不悄悄采用可疑恢复起点。

## 恢复为什么正确

从检查点的已提交镜像初始化页。候选事务是 TT 中活跃事务及 cutoff 后出现的事务；从 cutoff 后日志确定哪些候选最终提交。从安全起点扫描，只应用这些 winner 的 Update。检查点前已经提交的事务不再重放，否则会把镜像倒退到旧值。没有提交的修改从未进入已提交镜像，因此恢复自然排除它们。

DPT 为扫描起点提供保守界限，TT 同时补上已经刷盘的活跃事务。这里 DPT 不用于 ARIES 的逐页 redo 跳过；读取完整日志验证校验与 LSN 的工作仍然存在，不能声称启动 I/O 已经降到后缀长度。

## 逐步轨迹

| LSN | 动作 | 检查点有关状态 |
| --- | --- | --- |
| 1 | T1: P0=10 | DPT P0→1 |
| 2 | T1 提交，刷页 | TT/DPT 空 |
| 3 | T2: P1=20 | TT T2→3；DPT P1→3 |
| — | checkpoint | cutoff=3；start=3；镜像 `[10,0]` |
| 4 | T2: P1=30 | 未提交 |
| 5 | T2 提交 | 恢复必须包含 LSN 3、4 |

实际 demo 输出：

```
checkpoint cutoff=3 TT=1 DPT=1 start=3
recovery start=3 scanned=3 pages=10,30
```

若去掉最后提交，即使 P1=30 被刷盘，恢复仍为 `[10,0]`。若只发生 LSN 3 就提交，也必须从 3 而非 4 开始，测试覆盖跨检查点活跃事务。

## 源码导读与复杂度

`src/wal.hpp` 先看 Store 的 dirty/active 维护，再看 `Checkpoint`、`safeStart`、`checkpoint`、`loadCheckpoint`、`recover`。加载器验证长度、类型、校验、LSN 边界、重复键、TT/DPT 指向的日志记录，并重新计算安全起点。`tests/tests.cpp` 用 fork + `_exit` 直接跳过析构，再让另一个新进程恢复，测试发布中断及重复恢复。

检查点构建 O(L + T log T)，保存 O(P+T+D)；查找事务使用标准有序集合。启动仍读全日志 O(L) 空间/I/O；实际投影重放扫描 O(L-start+1)，测试记录 `replayScanned`。没有日志裁剪，因为活跃事务链和生成新镜像仍需要历史。P=4，D≤4。构建检查点会阻塞业务，这不是高吞吐在线实现。

## 构建、预期与边界测试

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

macOS 默认找不到 libc++ 时，两个配置命令附加：

```sh
-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

CTest 预期一项通过。覆盖 WAL 提交/未提交与偷写交叉、TT/DPT 实际保存、有活跃事务的检查点、活跃但 DPT 空的安全起点、检查点发布前退出、损坏文件拒绝、尾部截断、重复恢复。检查是运行期异常，不依赖 assert；超时 20 秒。

## 限制与持久性声明

仅 POSIX，单线程单进程打开、四个 unsigned 整数页，严格页写所有权防脏写，事务号不重用。打开已有库必须先 recover，不能恢复正在使用的活跃实例。未实现 CLR、原地 Undo、日志回收、多版本、并发检查点。WAL 为固定 72 字节小端记录：完整校验坏记录失败，短尾自动截断并同步；页与检查点采用完整文件替换，异常后必须重启，不能继续写。FNV 校验不防恶意伪造。`fsync`/目录同步不等于所有设备断电保证；fork 子进程退出保留内核缓存，本实验不是断电测试。
