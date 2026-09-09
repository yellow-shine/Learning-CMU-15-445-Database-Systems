# 73 · Write-Ahead Logging：先让日志可靠，再让页离开内存

这是数据库持久性层的独立 C++17 教程。前置知识是文件描述符、事务提交与缓冲页；相关主题是 74 的缓冲策略、75 的检查点及 77 的 ARIES。总目录请在 `main` 的 README 查看。本分支不需要检出其他分支。

## 问题和不变量

只写数据页，崩溃时无法判断其中的值属于已提交还是未提交事务。我们有四个整数页，更新记录保存 before/after、事务号、前一事务记录 LSN。LSN 从 1 连续增长，提交也是日志记录。

* `flushPages()` 先 `fsync(wal)`，检查所有 PageLSN ≤ durableLSN，再写页快照。
* `commit()` 写 Commit 并同步，返回后才允许调用者认为提交完成。采用 no-force，不要求页同步。
* 页写入临时文件、同步、rename、同步父目录。日志新建也同步父目录。
* 同一页由一个活跃事务独占直到提交，拒绝脏写；事务号永不重用。

`write`/`read` 循环处理短 I/O 和 EINTR，系统调用失败抛异常。异常后应关闭实例并重新恢复，而不是继续写。格式为小端、每条 8 个 64 位字段及一个 FNV-1a 校验值；校验不是安全认证。完整坏记录立即失败，不静默跳过；不足 72 字节的末尾视为未完成写，截断并同步后才允许追加。

## 具体轨迹

| LSN | 动作 | 页状态 |
| --- | --- | --- |
| 1 | T1 更新 P0: 0→40 | 内存 P0=40 |
| 2 | T1 Commit + fsync | 提交持久，页可尚未写 |
| 3 | T2 更新 P1: 0→99；flushPages | 未提交 P1 也被偷写 |
| 重启 | 只重放已提交事务到零初始镜像 | P0=40，P1=0 |

`demo` 输出：

```
durableLSN=3 stolen page1=99
recovered page0=40 page1=0
```

本主题恢复支撑故意从创世零镜像重建已提交投影，不是 ARIES 原地 Undo。这样既消除偷写的未提交值，也补齐未落盘的提交值；重复恢复结果相同。每次打开已有库后必须先 `recover()` 再接受事务；不能对活跃实例调用恢复。

## 源码导读与成本

`src/wal.hpp`：Fd/编码函数 → Log 验证与追加 → Store 的 update/commit/flushPages → recover。`src/temp.hpp` 仅提供独立临时目录；demo 用正常作用域展示机制，真正的进程中断在 `tests/tests.cpp`：fork 子进程直接 `_exit` 跳过析构，另一新进程恢复，再第三次恢复检查幂等。

每次更新追加 72 字节，时间 O(1)；提交一次日志同步；刷页一次日志同步、一次 72 字节快照同步及目录同步。全量恢复时间及日志内存 O(L)，事务集合 O(T)，固定四页空间 O(1)。这里没有日志回收，也没有磁盘容量无限的保证。

## 构建与测试

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

若 macOS SDK 的 libc++ 头文件没有被默认找到，在两个配置命令附加：

```sh
-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

测试覆盖提交/未提交 × 偷写/未偷写四种进程退出、重复恢复、空库、越界、写冲突、重复事务号、部分尾记录及完整校验损坏；测试使用运行期检查，Release 不会消除，CTest 超时 20 秒。

## 边界

仅 POSIX/macOS/Linux，单进程单线程、四个 unsigned 64 位整数页，无删除、锁等待、回滚 API、并发打开或日志压缩。页快照整体替换，不能据此推导真实多页原子写；日志必须保留从零开始的完整历史。不处理设备谎报同步、位翻转后的自动修复或磁盘满后的继续运行。`fsync` 不等于所有硬件都可靠；进程 `_exit` 保留操作系统页缓存，测试证明进程崩溃恢复，不证明断电安全。尚需独立代码审查，不能把本机测试通过当作生产认证。
