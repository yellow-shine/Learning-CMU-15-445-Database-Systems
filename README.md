# 80-distributed-transactions：两阶段提交与持久化决策

## 问题与前置知识

跨分片转账要求两个参与者最终遵守同一个提交或中止决定。前置知识：事务状态机、日志先行与 fsync；相关分支 79 的复制确认不能代替事务原子决策。本例是 POSIX 本地文件节点和显式消息投递模型，不是真实网络集群。

## 数据与状态机

一个独占目录只代表一个事务，固定两个参与者：账户初值 [10,20]，意图 [-3,+3]。意图与成员集合是程序固定常量，不支持复用目录提交下一笔事务。三份真实日志分别是 coordinator.log、participant0.log、participant1.log。

参与者空日志表示 Initial；prepare(yes) 先持久化 P 再返回 yes，prepare(no) 持久化 A。P 表示已承诺等待决定，不能自行超时中止。终态 C/A 不允许改变。协调者空日志是 Active，decide 仅在两个参与者均 P 时持久化 C，否则持久化 A，然后才允许 deliver 投递决定。没有决定时 deliver 返回 false，Prepared 保持 in-doubt。重复 prepare、decide、deliver 不追加重复日志。

重启后 recover_abort 仅在协调者尚无决定时写 A；这相当于协调者恢复后的 presumed-abort 策略，不是参与者自己猜测。已有 C 必须重放 C。参与者记录 C 后才将意图反映到 value 中，重放不会重复加减。协议保证最终原子决定，但显式消息之间可能一个账户已提交另一个尚 Prepared；这不是分布式一致性快照服务。

## 持久化与错误边界

`Journal` 每次追加一个状态字节，检查 read/write 短操作与 EINTR，随后 fsync 文件；新文件创建后同步文件和目录。仅允许协调者空/C/A，参与者空/P/PC/PA/A。未知字节、非法转移、过长日志拒绝打开，不静默修复。写入失败后对象停止服务，必须重开检查结果，因为失败不证明记录不存在。

每个日志只有一个写者，目录必须预先存在且由调用者独占。不含多事务 ID、校验和、跨日志身份验证或硬件坏块修复；合法字节被篡改成另一合法状态无法检测。fsync 不等于所有设备都诚实执行持久化，目录父目录创建的断电保证由调用者负责。进程中断测试不能证明断电安全。

## 手算轨迹与输出

prepare(0,true)、prepare(1,true) 后两日志均 P；协调者故障则两节点阻塞。重启协调者 decide，先落盘 C，再送给 0：日志为 C / PC / P；此时 0 值为 7，1 仍为 20。再次重启 deliver(1)，得到 C / PC / PC，最终 [7,23]，总额仍 30。
demo 输出 `prepared: blocked=1`、`decision durable, first=7 second=20`、`replayed: 7 23`。

## 源码与测试导读

`src/two_phase.hpp` 的 Journal 负责小型严格日志，Protocol 负责状态机；`src/temp_directory.hpp` 只给 demo/tests 创建独立临时目录并清理自身数据。`tests/tests.cpp` 使用 fork 后 _exit，在 prepare 后、决定后、部分通知后中断，再由父进程重新打开文件，检查 in-doubt、提交重放、中止重放与重复恢复；还覆盖 no vote、非法转移、索引、损坏和打开失败。

固定两节点每步 O(1)，最多五个日志字节，每个有效状态变化一次文件同步；初始化还有目录同步。推广到 p 个节点是 O(p) 消息与空间，延迟由最慢 prepare 和同步写决定。

## 为什么会阻塞

参与者 P 只知道自己投 yes，不知道协调者是否已经写 C。协调者不可达时它不能安全选择 A；否则另一个节点可能已经提交。增加超时不能解决这个知识缺口。本例不提供共识、自动选主、网络分区可用性、并发隔离或生产级 WAL。

## 构建与验证

需要 CMake、C++17 编译器；无第三方依赖。独立检出本分支即可运行。

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

若 macOS 的 AppleClang 报标准头文件缺失，在两次配置命令附加：
```sh
-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```
这只是本机 SDK 搜索路径排错，不是源码依赖。测试使用显式异常检查，Release 不会删除检查；CTest 有 20 秒上限。
返回总目录：`git show main:README.md`。各主题独立，相关分支仅供进一步阅读，不是运行依赖。
