# 22：Buffer Pool —— 用有限内存管理真实磁盘页

## 问题与前置知识

数据库不能把每次取元组都变成一次读盘，也不能让内存无限增长。缓冲池在存储层将页号映射到固定数量的 Frame。先了解数组、哈希表、异常和文件偏移；相关主题为 11 页 I/O、23–25 替换策略、27 页守卫。本分支独立附带真实文件页 I/O，不依赖其他 checkout。

## 原理与不变量

磁盘页固定 256 字节，偏移为 `id * 256`，文件必须是完整页的整数倍；页号越界和短读抛异常，不把不存在的页伪装成零页。Frame 保存页内容、可空页号、pin count 和 dirty 位，页表保存页号到 Frame 索引。

- 同一页最多驻留一次；命中只增加 pin，不分配新 Frame。
- pin 大于零的 Frame 不能淘汰；每次成功 fetch 对应一次 unpin。
- dirty 在 unpin 时做逻辑或，其他读者不能清掉写者的标记。
- 缺页先选第一个未 pin 的 Frame，读入临时页，再写回旧脏页；任何 I/O 失败不会丢失旧页映射。
- flush 成功才清 dirty；全部 pinned 时明确失败，而非偷偷扩容。

这是教学替换策略，不是 LRU。`flush_all` 是显式提交写回的调用点，析构不自动吞掉 I/O 异常；遗漏它会丢掉仍在内存的修改。

## 一步一步的轨迹

容量 1，初始磁盘三页全零：

|操作|驻留页|pin|dirty|I/O|
|---|---|---|---|---|
|fetch(0)，写 A|0|1|尚未登记|读 0|
|unpin(0,true)|0|0|true|无|
|fetch(1)|1|1|false|读 1，写回 0|
|unpin(1)，fetch(0)|0|1|false|读 0|

最后得到 A 而不是零。demo 的实际输出为 `reload page 0: A`。如果在第一次 fetch 后重复 fetch(0)，pin 变成 2；fetch(1) 必须等两次 unpin 后才可成功。

## 源码导读与成本

`src/disk.h` 校验页号并执行 seek/read/write；`src/buffer_pool.h` 的 fetch 按命中、选 victim、I/O、更新页表顺序阅读。`src/temp_file.h` 为 demo 和测试原子创建独立临时目录，退出清理。`src/demo.cpp` 展示脏页重载。

命中期望 O(1)，缺页选择 O(F)，空间 O(F×256)，页表 O(F)；一次缺页通常一次读，脏 victim 增加一次写。flush_all 扫描 O(F)，最多 F 次写。标准流开关文件成本未优化。

## 反例、测试和限制

`tests/tests.cpp` 独立验证全 pinned 失败、重复 fetch、双重 unpin、脏页淘汰重载、显式 flush、越界、短文件、写失败保留 dirty 后重试。它不是只检查打印结果。

仅单线程；拿到的 Frame 引用只在 pin 期间有效，调用者修改后必须标 dirty。没有页分配、删除、WAL、校验和、崩溃恢复和后台刷盘；固定容量不能动态伸缩。文件 flush 只是流缓冲写到操作系统，不是 fsync，不承诺断电持久性。外部进程不能并发修改页文件。

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
