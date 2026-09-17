# 82-parallel-execution：真实线程分区执行与合并

## 问题与前置知识

OLAP 扫描可以把独立行分给多个 CPU 工作线程；OLTP 的短事务未必能摊薄调度成本。前置知识：线程生命周期、原子计数器、异常与数据竞争；81 的本地消息交换并不自动产生 CPU 并行，本主题真正使用 std::thread。

## 算法与不变量

parallel_map 将输入划分为连续 chunk，原子 next 让工作线程动态领取 chunk。每块只领取一次，每个输出槽只有一个写者，最终按原始行位置返回结果，不需要排序或锁住整张表。启动线程数是 min(workers,块数)，workers 限定 1..256 防止意外无限创建线程，chunk 必须大于零。空输入不创建线程。每线程处理计数单独存储，join 后求和等于输入行数。

函数对象按值复制给各线程，副本内部可维护线程局部状态；若捕获共享引用，调用者负责同步且输入不能被并发修改。代码在任何返回或异常路径上 join 所有已启动线程，包括线程创建或回调复制失败。工作线程捕获异常到自己的 exception_ptr，设置 stop 请求，主线程全部 join 后重新抛出一个原异常。已经执行的外部副作用不会回滚；其他线程可能完成手中 chunk，故 stop 不是即时取消。多个异常时按线程编号选择首个，而不是按实际时间排序。

## 具体轨迹与预期输出

输入 [1,2,3,4,5,6,7,8]，chunk=2，得到四块 [0,2)、[2,4)、[4,6)、[6,8)。四个工作线程竞争领取；某个线程可能领取多块，调度不影响输出。计算每行平方，合并为 [1,4,9,16,25,36,49,64]，和为 204。
demo 输出 `threads=4 rows=8 sum=204 serial_equal=1`。threads 指真实启动数，不是承诺每个线程处理相同行数；测试另用条件变量屏障确保四个不同线程都真正进入回调。

## 源码导读与测试

`src/parallel_execution.hpp` 包含 JoinThreads 生命周期保护、parallel_map 调度与 square 示例；`src/demo.cpp` 对比串行输出。`tests/tests.cpp` 检查多种块长/线程数、空输入、余数块、极端整数、固定种子随机数据与串行等价；用条件变量而非 sleep 构造四线程相遇，超时会失败；构造回调抛错后验证异常类型与已启动工作已全部退出。

## 复杂度与工作负载选择

n 行、p 线程、块长 c：工作 O(n)，调度 O(ceil(n/c)) 次原子竞争，结果 O(n)，线程元数据 O(p)，额外线程栈由平台决定。理想运行时间 O(n/p) 只适合足够重且均匀的回调；现实受内存带宽、缓存、调度和最慢块限制。本实现先物化 n 个输出，不是流水线引擎。

块越小越利于负载均衡但竞争越大；块越大越省调度却易产生长尾。长扫描/独立表达式常可受益，短点查/有锁写入的 OLTP 可能因启动和争用更慢。生产系统应测量后采用复用线程池与 NUMA 布局，本例不声称加速比，不用不稳定墙钟计时作正确性断言。

## 边界

仅单机共享内存，不是跨机器部署、事务隔离或线程池。回调必须终止；C++17 无法安全强杀卡住的线程。失败不返回部分结果，不回滚回调副作用。square 把 int 转 int64 后相乘，在常见 32 位 int 平台安全，源码静态约束该输入宽度。

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
