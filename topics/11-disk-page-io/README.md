# 11：固定页文件 I/O

## 问题与前置知识

磁盘导向数据库不为每条记录独立创建文件，而以固定大小页在文件与内存之间搬运数据。
本节位于存储引擎底层；需要知道二进制文件、数组和异常。后续 13 的槽页决定页内部格式，
14 的堆文件决定记录放在哪页；本节刻意不解释记录。

## 格式、算法与不变量

教学页大小为 256 字节，Page ID 为无符号 64 位整数，从 0 开始。
`文件 = page0 | page1 | ...`，偏移为 `id * 256`。已有页只能原位覆盖；
append 是唯一分配入口，不能通过 write 跳过页号制造洞。打开文件时检查大小是 256 的倍数。
读操作先验证页号，再 seek/read，并检查实际读取字节数；文件打开后被截短也必须报错。
偏移乘法在执行前验证 streamoff 范围，页数只在写成功后增加。

## 具体轨迹与预期输出

1. 在自动清理的私有临时目录创建空文件，页数是 0。
2. append 一个首字节为 42 的页，返回 ID 0，文件长 256。
3. append 零页，返回 ID 1，文件长 512。
4. 析构关闭后重新打开，读取两个页，demo 输出：

```text
pages=2 page0[0]=42 page1[0]=0
```

write(2) 在此时非法，不改变页数。合法写 ID 1 后，ID 0 的内容不变。

## 源码导读与测试反例

`src/page_file.h` 的 offset 是整数边界，构造函数是文件边界，read/write_at 是 I/O 边界。
`src/temp_dir.h` 只负责隔离实验文件；demo 展示关闭后重开。
`tests/storage_test.cpp` 覆盖空文件、末字节、零初始化、非法 ID、全页相等、覆盖、
重开、重复创建拒绝以及外部截断后的短读和非整页文件。测试不读取用户数据。

## 成本与局限

读写一页的时间和空间为 O(P)，P=256；定位计算 O(1)，每次操作一次逻辑页 I/O。
这不是设备物理 I/O 测量，系统缓存可能命中。无缓存池、校验和、锁、WAL 或崩溃原子性。
flush 只刷新 C++ 流缓冲到操作系统，**不是 fsync，不保证断电持久化**。
显式写入检查流状态；底层部分写入失败可能已破坏文件，不承诺事务回滚。
create 的存在性检查不是跨进程原子创建，因此只允许单所有者、无并发外部修改；
外部截断仅作为故障注入。使用标准 C++17 文件 API，不依赖 POSIX。

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
