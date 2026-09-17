# 79-replication：主从复制与确认边界

## 问题与前置知识

复制将主节点写入送到从节点，读者必须区分“主已接受”“从已应用”和“客户端已确认”。前置知识：追加日志、FIFO 消息队列。相关主题是 80 两阶段提交；复制不是跨分片事务。

## 本地节点与消息模型

`Replication` 是单线程、确定性的本地消息模拟器，一个主、一个从，内存顺序日志保存整数值。write 分配从 1 开始的序号并发送 Apply；deliver_apply 在从节点顺序应用后发送 Ack；deliver_ack 只有主收到确认才推进 confirmed。每次显式投递代表一次网络步骤，不使用 sleep 假装网络延迟。链路断开或接收节点失败时，投递返回 false，消息留在队列。

异步写在主接受后即可 acknowledged；同步写必须等确认水位达到其序号。等待不是回滚：同步写未确认时数据可能已到达从节点，因此超时是结果未知，不是必然失败。复制延迟 lag = 主日志长度 - 从日志长度，确认延迟还可能多一次 Ack 投递。read replica 允许陈旧结果。每条 Apply 按序号校验，日志与确认水位不会倒退。

## 具体轨迹与输出

异步写 7：立即确认，lag=1，从节点仍为空；投递 Apply 后 lag=0。再投递 Ack，主知道从已应用。
同步写 9：未确认；投递 Apply 后从已有 [7,9]，但仍未确认；投递 Ack 后才确认。
demo 输出：

```
async ack=1 lag=1
sync before=0
after apply=0 lag=0
after ack=1
```

## 源码与复杂度

`src/replication.hpp` 定义 Entry、消息队列和节点故障开关；`src/demo.cpp` 展示正常路径；`tests/tests.cpp` 检查链路断开、主失败前消息未送达导致异步已确认数据无法从从节点读取、Ack 丢失式延迟、从停机恢复投递、非法序号和空队列。
每次写入和投递摊还 O(1)，日志与积压消息空间 O(n)。日志只追加，不截断；这是机制演示而非资源有界的服务。

## 故障边界

故障开关模拟 fail-stop 节点不可处理消息；主失败会清除在途消息，不再接受请求。从节点可以恢复服务，保留内存日志。没有主重启、自动选主、重试客户端去重、持久化、真实 socket、磁盘同步或多数派共识。同步确认仅保证此模型的两个内存副本已经应用，不保证两机同时掉电后的持久性。单从故障会阻塞同步确认；异步降低延迟，但主在复制前失败可能丢失已确认写入。

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
