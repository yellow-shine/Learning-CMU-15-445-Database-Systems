# 76 · Redo / Undo：重做已提交，撤销未提交

在 Steal + No-Force 下，磁盘可能既多了 loser 的值，又少了 winner 的值。本教程实现真实 WAL 文件、正序 Redo、逆序 before-image Undo、恢复完成标记与重复恢复。前置知识是 73 的 WAL、74 的缓冲策略。总目录见 `main:README.md`。文件 I/O 和四页存储是本模块 73 的源码快照；本主题独立构建，不依赖其他 checkout。

## 算法与不变量

日志记录为 Update、Commit、Abort 三类，带递增 LSN、事务号、页号、before/after、prevLSN。Abort 在这里表示**恢复已完成撤销**，不是业务主动中止请求。事务对页持严格写所有权，不允许脏写；因此未提交事务的 before image 不会覆盖其他事务在它之后提交的同页更新。

恢复分四步：

1. 扫描提交和已完成 Abort，分类 winner、旧 loser、新 loser。
2. 从零初始镜像正序重放所有非旧 loser 的 Update：重现历史，包括新 loser。
3. 逆序撤销新 loser，写 before image。重复赋值而非反向算术，避免多次恢复累计改变结果。
4. 同步 WAL、原子替换页快照，然后为每个新 loser 追加并同步 Abort。

先保存撤销结果再标记完成。为什么保留 Abort？恢复后允许新事务修改原来 loser 的页；若下一次恢复再次撤销旧 loser，会覆盖新事务的提交。正序重放必须跳过有 Abort 的历史事务。

这里故意不信任上次恢复部分完成的磁盘镜像，而是从零完整重建；恢复期间再次退出可重新执行全部步骤，不需要 CLR。这是 77 之前的简单算法，不是 ARIES 的原地增量恢复。完整日志不能删除。

## 具体轨迹

| LSN | 日志或动作 | 结果 |
| --- | --- | --- |
| 1 | T1 P0: 0→40 | 内存更新 |
| 2 | T1 Commit，同步 | winner |
| 3 | T2 P1: 0→99，刷页 | loser 值已到磁盘 |
| 恢复 Redo | 重放 1、3 | `[40,99]` |
| 恢复 Undo | 撤销 3 | `[40,0]` |
| 4 | 页同步后 T2 Abort | 后续跳过旧 T2 |

实际 demo 输出：

```
durableLSN=3 stolen page1=99 redo=0 undo=0
recovered page0=40 page1=0 redo=2 undo=1
```

若 T2 连续把 P1 从 0→20→30，必须先撤销 30→20，再撤销 20→0。正序 Undo 会错误留下 20。测试在 Redo 完成、第一次 Undo 和页落盘后分别 `_exit`，再由新进程完成恢复，并提交 T3=77，验证旧 Undo 不会覆盖新提交。

## 文件校验与失败策略

`Log` 每条固定 72 字节、小端编码、FNV-1a 校验；长度为完整记录的前缀加短尾时，只截掉不足一条的尾部并 fsync，然后才允许追加。完整记录即使位于末尾，只要校验错误也立即失败。测试枚举 1..71 字节的所有短尾长度，并验证校验正确但页号非法的记录也被拒绝。LSN 必须连续，prevLSN 必须向后且同事务，类型和页号必须合法。

`writeAll`/`readAll` 处理短 I/O 和 EINTR，异常向上传播；失败后关闭并重启，禁止继续使用部分写入的实例。页是四个 `(value,PageLSN)` 的整体快照，先同步临时文件、rename、同步目录。刷页前同步日志，提交记录也同步。PageLSN 在此不用于跳过 Redo，Undo 后置零，因为恢复镜像会从创世重算。

## 源码和成本

`src/wal.hpp`：编码与 Fd → Log → Store 的正常事务操作 → `recover` 四步；`redone/undone` 是实际赋值计数，hook 是测试崩溃注入点，不是打印阶段替代算法。`tests/tests.cpp` 是独立运行期检查，Release 不会删除。`src/temp.hpp` 用唯一临时目录，析构清理；子进程 `_exit` 不触发数据库析构，父进程管理目录。

日志解析 O(L)，集合分类与查找 O(L log T)，空间 O(L+T)，四页 O(1)。每次更新追加 72 字节，提交一次同步。恢复每个新 loser 追加一条 Abort；再次完整恢复不追加重复 Abort。未进行日志压缩、分段读取或检查点优化，恢复成本随历史增长。

## 构建与预期

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

macOS 缺少 libc++ 头文件时，两个配置命令条件性附加：

```sh
-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

CTest 预期一项通过。覆盖提交/未提交 × 偷写/未偷写、空库、越界、冲突、重复恢复、恢复中再次退出、恢复后新提交、所有短尾长度、完整坏记录。CTest 超时 20 秒。

## 明确限制

POSIX 单进程单线程，四个 unsigned 整数页，事务号不可重用，无业务 abort、并发打开、脏写、锁等待、多版本或 CLR。打开已有库必须先 recover，不能对活跃实例恢复。FNV 不提供对恶意篡改的认证，校验失败停止服务而非自动修复。恢复要求零初始镜像与完整历史；不要把它当作真实数据库就地恢复算法。`fsync` 及目录同步的设备保证依赖系统，进程退出仍保留内核缓存，测试不能证明断电安全。
