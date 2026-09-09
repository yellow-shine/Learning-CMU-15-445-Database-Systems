# 27：Page Guard —— 把 pin 和读写锁变成作用域资源

## 问题与前置知识

手动 fetch/unpin 容易在异常、早退、移动时漏释放，也容易释放两次。Guard 用 RAII 表达“持有此页且可以读／写”的生命周期，处于缓冲池 API 层。先了解析构、移动语义、shared_mutex 与 22 缓冲池。本分支附带 22 的 Disk、TempFile、BufferPool 支撑快照，并为 BufferPool 增加元数据锁与每 Frame 页锁；可独立构建。

## 获取、移动、释放

构造先 fetch 增加 pin，再获取页锁。读 Guard 持共享锁，只返回 const Page；写 Guard 持独占锁，mutable_data 在暴露可写引用时保守登记 dirty。若获取锁抛异常，构造的 catch 撤销 pin。

移动转移 pool 指针、Frame 指针、锁所有权、dirty 标记，源变空，不再 unpin；移动赋值先释放目标原来持有的页。自移动不改变资源。默认构造和显式 drop 后也是空状态，访问空 Guard 抛 logic_error，重复 drop 无副作用。

析构／drop **先解页锁，再 unpin 并合并 dirty**。反过来会让仍锁住的页变成候选，并可能形成元数据锁／页锁逆序死锁。只要 pin 尚在，Frame 就不会被复用；页表与 pin/dirty 都在元数据锁下更新。Guard 与 BufferPool 不可复制。

## 一条真实轨迹

容量 1，磁盘两页零值：

|作用域|资源状态|结果|
|---|---|---|
|WritePageGuard(0)|pin=1，独占锁|mutable_data 写 G|
|写作用域退出|解锁，pin=0，dirty=true|可淘汰|
|ReadPageGuard(1)|淘汰 0，写回 G|页 1 被 pin|
|页 1 作用域退出|pin=0|可重载 0|
|ReadPageGuard(0)|读盘获得 G|共享读|
|最后退出|页 0 pin=0|不会重复 unpin|

demo 输出 `guard reload: G` 和 `pins after scope: 0`。测试另有写后抛用户异常路径，修改仍登记 dirty；移动赋值覆盖一个已持有的 Guard 时，旧页 pin 减一，新页 pin 不增加。

## 源码导读与测试

`src/page_guard.h` 是重点：模板布尔参数选择 unique_lock 或 shared_lock；mutable_data 的 static_assert 禁止读 Guard 写入。`src/buffer_pool.h` 的 fetch 只保护元数据，不在返回前持页锁；flush 在元数据锁内取得共享页锁，防止与正在写的 Guard 竞争字节。`src/disk.h` 为真实固定页 I/O，`temp_file.h` 只服务 demo 和独立测试。

`tests/tests.cpp` 验证移动构造／赋值／自移动、空态、重复 drop、异常展开、dirty 保留、全 pinned 失败后不泄漏、读锁并存和独占互斥。真实 async 线程用 future/promise 协调；try_lock 检查持锁时排他性，阻塞 writer 在 reader 释放后完成。只读 Guard 释放后移走磁盘文件再 flush 不触发写，证明没有误标 dirty。测试不靠 sleep 或 Release 会消失的 assert。

## 成本与约束

命中与 unpin 期望 O(1)，缺页选择 O(F)，空间 O(F×256)；Guard 固定大小，不分配页副本。页锁等待时间取决于竞争，shared_mutex 不保证公平。元数据锁覆盖 I/O，吞吐受限；本分支教学重点是所有权而非高并发 BufferPool。

**调用约束**：Pool 必须比 Guard 活得久，Disk 比 Pool 活得久。持锁 Guard 只能在获取锁的同一线程移动和析构，不能把它搬到另一线程解锁。不可在同一线程对同页递归获取 Guard、升级读锁，或持有 Guard 调用 flush／flush_all；这些可能递归锁死。跨页嵌套获取必须由调用者固定顺序。底层 fetch/unpin 只作为低层接口，不能绕过 Guard 手动 unpin 它持有的 pin，也不能无页锁访问字节；不要让 data 引用逃出作用域。

没有后台刷新、WAL、页分配、自动析构写回和崩溃恢复。修改由显式 flush 或淘汰写回，流 flush 不等于 fsync，不保证断电持久性。构造锁失败的回滚在源码显式处理，但标准库没有可移植的强制锁失败注入接口，测试覆盖实际用户异常和 fetch 失败，不伪造该系统故障。

## 构建与验证

仅需 C++17、CMake 和标准线程库，无第三方依赖。回到总目录：`git show main:README.md`。

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

若 macOS 的 AppleClang 报标准头文件找不到，在两条配置命令中各附加：
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`。
这是本机 SDK 搜索路径排错，不是源码依赖。CTest 失败抛异常返回非零，Release 不依赖 assert；测试超时 20 秒。
