# 26：Disk Scheduler —— 后台磁盘请求与完成通知

## 问题与前置

调用线程不应亲自阻塞在每次页读写上。调度器把真实磁盘请求交给后台线程，通过 future 告知结果或异常。先了解 mutex、条件变量、promise/future、线程 join 与页偏移。相关 11 页 I/O、22 Buffer Pool；本分支从 22 附带 `disk.h` 与临时文件支撑快照，不依赖其他分支运行。

## 队列和生命周期

请求保存操作类型、页号、**自有的 256 字节页副本**和 promise。submit 在队列互斥锁内检查关闭状态并入队，然后通知条件变量。后台线程以谓词等待“队列非空或关闭”，取出队首后释放锁，再进行真实文件 I/O；不会持队列锁等待磁盘。

read 返回 `future<Page>`；write 同样返回 `future<Page>`，成功值为写入页副本，作为确认。失败使用 set_exception，get 在调用线程重新抛出。一个请求出错不杀死工作线程。FIFO 顺序是取得入队锁的顺序，不保证并发提交者的墙钟先后。

close 将 stopping 置真，拒绝新请求，唤醒工作线程，**排空已接受的请求后 join**。关闭用另一把锁串行化，因此多个 close 不会重复 join；队列锁不跨 join 持有。析构调用 close。后台线程成员最后构造，启动时其他状态都已存在。

不变量：每个已入队请求只被取走一次，promise 只完成一次；I/O 执行期间调用者页缓冲区不需要存活；Disk 必须比 Scheduler 活得久。并发 close 是安全的，但析构不能与其他成员调用并发发生。

## 具体轨迹和输出

初始两页零数据：提交 write(0,'S') → 提交 read(0) → 工作线程写页 0 → 完成写 promise → 读页 0 → 完成读 promise。即使提交读前未等待写 future，同一个 FIFO 也保证读到 S。

再提交 read(99)，Disk 检查越界，异常穿过 promise 到 get。demo 输出：

```text
async read: S
I/O error delivered
```

## 源码导读与测试

`src/disk_scheduler.h` 的 submit、run、close 分别对应生产者、消费者、停止协议；`src/disk.h` 执行 seek/read/write 并检查流状态；`src/temp_file.h` 创建独立临时目录，避免修改用户文件。

`tests/tests.cpp` 验证 FIFO、提交数据副本、越界异常后继续处理、8 个真实并发生产者、两个并发 close、关闭后拒绝、100 个未等待请求析构排空、短读、实际写失败和空队列关闭。同步依靠 future 和 join，而不是 sleep 猜测时序。CTest 20 秒超时约束死锁。

## 成本与限制

入队与出队均摊 O(1)，每项固定一页复制，积压 Q 个请求占 O(Q×256)；一个后台线程串行执行磁盘操作，可让提交者与 I/O 重叠但不增加磁盘并行度。没有优先级、取消、批处理、背压或关闭超时；无限提交可耗尽内存，真实设备永久阻塞也会阻塞 close。不要把这个队列当生产级资源隔离设施。

Disk 只操作已有固定页文件，拒绝越界与损坏长度，外部不得并发修改文件。流 flush 不等于 fsync；完成通知不是断电持久性保证，没有 WAL 或恢复。本实现用标准 C++17 线程和文件设施，无额外线程池框架。

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
