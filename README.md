# 06 · 有界工作队列与并发页目录

## 问题与前置知识

数据库读取线程把页号交给处理线程。生产者快于消费者时，无界缓存可能耗尽内存；
关机时只停止生产、不唤醒消费者则 join 永远不能结束。本课用真实 C++17 线程实现
有界 FIFO，并用读写锁保护一个内存页目录。前置为 RAII、STL、引用捕获与生命周期。
所有组件在本分支；不是事务调度模拟器，也不依赖其它教程。

## 状态机与同步不变量

`WorkQueue(capacity)` 拒绝 0。mutex 同时保护队列、closed 和等待计数。
任何时刻 `0 <= size <= capacity`；push 满时等待，pop 空时等待。
condition_variable 的 wait 原子地释放锁并睡眠，返回时重获锁；谓词重检处理虚假唤醒。
通知不是一张可以保存的通行证，真正决定能否继续的是锁内状态。

| 状态 | push | pop |
| --- | --- | --- |
| 开放且有空间/数据 | 入队 true | 出队一个 int |
| 开放但满/空 | 等 writable | 等 readable |
| 已关闭 | 返回 false | 有数据继续排空，空则 nullopt |

close 在锁内单向设置 closed，通知所有两类等待者，可重复调用。若 push 先获得锁完成
入队，它会被消费；若 close 先完成，push 被拒绝。pop 以 optional 区分页号 0 和退出。
析构不是关闭协议：拥有者必须 close 并 join 所有调用线程，再销毁队列。
不能一边析构对象一边让其它线程访问其 mutex。

`PageDirectory::set` 使用 unique_lock<shared_mutex>，get 使用 shared_lock，
允许多个读者并行、写者独占。get 在锁内复制 optional<int>，不返回解除锁后可能悬空的
map 引用。两个 get 不是一个原子快照；读锁不能原地升级为写锁。

## 具体执行轨迹与输出

容量为 2，生产者依次 push 1、2、3。若消费者尚未运行，第三次 push 会等待；
消费者取出 1 通知 writable，生产者才能继续。实际调度不固定，FIFO 数据次序固定。
close 后消费者把剩余页号写入目录，看到空且 closed 后退出。主线程 join 之后读目录：

```text
page 1 offset 100
page 2 offset 200
page 3 offset 300
closed and drained: 1
```

目录只是示例 `page -> offset`，没有真实磁盘读写。演示输出在 join 后集中打印，
不依赖多线程 cout 的交错顺序。

## 源码与确定性验收

- `src/work_queue.h`：互斥锁、两条工作条件变量、close/drain 协议。
- `src/page_directory.h`：共享读锁、独占写锁和安全复制返回。
- `src/demo.cpp`：真实生产消费到页目录的完整生命周期。
- `tests/topic_tests.cpp`：零容量、FIFO、重复 close、关闭后 push 拒绝和排空；
  两个空队列消费者、两个满队列生产者确实进入等待后 close，验证全部唤醒；
  4 个生产者与 3 个消费者传递 2000 个不同编号，每个编号恰好出现一次；
  2 个目录写者和 3 个读者由同一关闭屏障放行，检查值范围及最终值。

教学观察接口 wait_for_waiters 在同一 mutex 下观察等待计数，最多等 2 秒；计数通知发生
在 wait 释放锁之前，观察者取得锁时等待线程已释放它。空/满条件在测试协调阶段保持不变，
因此不是“线程启动了就猜它睡着了”。超时也会先 close/join 再报告失败；CTest 总超时 20 秒
兜底检测漏通知死锁。没有 sleep。独立 CHECK 不受 NDEBUG 影响。
观察接口不是生产队列必要 API，只用于可复现的关闭验收，不是性能监控系统。

## 成本、异常与局限

队列 push/pop 摊销 O(1)，空间 O(capacity)，但等待时间无上界，OS 调度与锁公平性不保证。
目录查找/设置平均 O(1)、哈希碰撞最坏 O(n)，空间 O(n)。无 I/O，无稳定存储承诺。
队列故意只传 int 页号，避免泛型移动异常掩盖同步主题；分配失败会抛异常，锁由 RAII
释放，调用者仍需安排 close/join。演示不提供线程创建失败、内存耗尽的恢复协议。
一把队列锁限制并行吞吐；这是标准有界队列，不是无锁队列，也不承诺公平、取消或超时 push。
shared_mutex 可能导致读/写者饥饿，读写锁不自动比 mutex 快。
测试验证具体交错和真实多线程结果，不声称穷举所有调度或证明无任何数据竞争。

## 构建与验证

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

CMake Threads 链接系统线程库，无第三方依赖，C++17，扩展关闭。成功显示 `100% tests passed`。
若 macOS AppleClang 找不到标准头，在两个配置命令中附加
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`。
本机验收使用该条件性 workaround；项目不写死 SDK 路径。目录：`git show main:README.md`。
