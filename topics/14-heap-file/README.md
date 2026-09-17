# 14：跨页堆文件与 RID

## 问题与前置知识

一页装不下整张表。堆文件不按键排序，而将记录放入第一个有空间的页，
用 RID=(page,slot) 定位。前置是 11 固定页文件和 13 槽页；本分支附带两者已测试的
源码快照，不需要其他 checkout。page_file.h 仅改为引用公共 page.h，避免重复定义。

## 结构、算法和不变量

文件是连续的 256 字节槽页，不另设表头；页内为 8 字节头、4 字节槽目录、变长 payload。
打开时检查文件整页对齐，并逐页验证魔数、版本、偏移、长度、目录边界和记录不重叠。
插入先在空页试装，拒绝空串和超过 244 字节的记录，然后 first-fit 扫描已有页，
通过候选副本紧缩和插入；所有页都装不下时追加新页。已有页失败不改变原字节。
槽号永不复用，删除返回 nullopt，非法页号或槽号抛异常。更新仅限原页，装不下返回 false，
绝不悄悄迁移而破坏 RID。扫描按页号、槽号递增跳过墓碑，不保证插入时间顺序。

## 手工轨迹与预期输出

插入 140 个 A：页 0 槽 0，剩余 104 字节；第二条 140 个 B 加 4 字节槽放不下，
分配页 1 槽 0。关闭所有流后重开，依然可以使用保存的 RID：

```text
pages=2 rid=1:0
reopen rows=2 second bytes=140
```

更新第二条为短串时它的槽号不变；删除第一条后扫描只返回第二条。
虽然删除产生空闲空间，但墓碑目录不回收，旧 RID 不会重新指向新记录。

## 源码导读和测试

`src/heap_file.h` 是本节重点：insert 实现 first-fit，get 两级定位，scan 跳墓碑，
update 在原页失败无损。`page_file.h` 负责真实 seek/read/write 与短读检查，
`slotted_page.h` 负责页内整理与验证；`temp_dir.h` 隔离 demo 和测试文件。
`tests/storage_test.cpp` 跨 40 条变长记录检查 RID 与原数据，关闭重开后逐条核对，
验证删除、更新失败、超长插入无损、坏 RID、损坏页头和截断文件。CHECK 在 Release 仍执行。

## 成本与边界

设页数 B、页大小 P、结果字节数 L。打开和 first-fit 最坏 O(BP)，各读取 B 个逻辑页；
get/update/erase 一页 I/O，扫描 B 页并复制 O(L) 字节。本教学 scan 收集全部结果，
空间 O(L+B条槽记录的元数据)，不是流式迭代器；单页工作区 O(P)。没有空闲空间索引。

这是单所有者、无并发写的文件。不支持 overflow page、跨页更新、事务、缓存池、WAL、
校验和及断电恢复。流 flush 不是 fsync，只保证关闭重开可读取，不保证断电持久化。
部分磁盘写失败可能破坏文件，不承诺 I/O 故障事务回滚。检测到损坏即拒绝打开，不自动修复。
槽目录耗尽的页仍保留，文件不收缩，创建操作不允许覆盖已有文件。

## 构建与验证

无需第三方库；C++17 与 CMake 3.16 即可。测试程序使用抛异常的 CHECK，
不会因 Release 的 NDEBUG 消失；CTest 设置 20 秒超时。独立 demo 不代替测试。

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

本分支是独立快照，不需要检出任何相关分支。知识点总目录见 `main:README.md`
（例如 `git show main:README.md`），不要把实现分支合并回目录分支。

### macOS 工具链排障

若默认 AppleClang 报 `<vector>` / `<array>` 不存在，使用已安装 SDK 的 libc++ 头文件，
无需安装依赖或修改 CMake。对 Debug 与 Release **都**传入：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

然后执行上述 build、CTest 和 demo。本机验收采用此参数；编辑器未加载编译配置时的
标准头文件缺失诊断属于同一工具链问题，不代表已经通过静态分析。
