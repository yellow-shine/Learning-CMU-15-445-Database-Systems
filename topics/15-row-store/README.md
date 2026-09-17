# 15：NSM 行存与整行访问

## 问题与前置知识

交易型负载经常按记录取出所有属性。NSM（N-ary Storage Model）将一行的字段相邻放置，
避免每取一行都去多个独立列区。本节处于存储布局层；前置是数组、整型端序、12 元组布局。
16 DSM 与 17 PAX 是可比较的独立教程，本节无需它们的 checkout。

## 实际布局、访问算法和不变量

固定 schema 为三个非 NULL uint32：id、score、age。物理存储是一个 byte vector，
每行 12 字节，小端编码，没有结构体 padding，也没有指针。

```text
id0 score0 age0 | id1 score1 age1 | id2 score2 age2
0    4     8     12  16     20    24  28     32
```

行 i 从 12*i 开始。构造前检查容量乘法；get 先验证行位置，再显式解码三个字段。
逻辑 id 是普通值，可重复，不是数组下标。位置在只读快照内稳定。
select 是本节刻意展示的行式算子：逐行 get，再判断 score>=threshold，返回位置。
读取计数累加到调用者变量，每次整行 get 计三个标量读取；拒绝非法位置不增加计数。

## 具体轨迹与预期输出

输入 (1,10,20)、(2,90,30)、(3,80,40)，36 字节中第二行起点 12。
select(80) 读三行、九个标量，输出位置 [1,2]；取位置 1 又读三个标量。

```text
rows=3 bytes=36 selected=2 scan reads=9
first=2,90,30 row reads=3
```

第一行的 score 虽然不合格，整行算子仍读 id、age。这解释了列扫描为何可能节约解码工作，
但**不是 NSM 必然要读取所有列**：投影优化也能用 12 字节 stride 只解码 score。
实际缓存行成本还受缓存、预取和编译器影响，计数不是硬件性能测量。

## 源码导读与测试

`src/row_store.h` 的构造函数形成真正交错字节布局，field 解码小端，get 物化整行，
select 产生位置列表；bytes 只读接口让测试直接检查物理偏移。`src/demo.cpp` 是上述轨迹。
`tests/storage_test.cpp` 以逻辑 Row vector 为 oracle，核对 102 行、各种阈值、UINT32_MAX、
空输入和非法位置，同时验证字节数、第二行字段偏移和精确访问次数。不是只打印概念。

## 复杂度、取舍与边界

构建 O(N)，占 12N 数据字节；get 时间和结果空间 O(1)；select 时间 O(N)，结果 O(K)。
整行相邻有利于点查；只扫某列时字段有 stride，不能像 DSM 一样形成紧凑单列数组。
计数是算法层字段解码数，不是页 I/O、缓存 miss 或运行时基准。
不支持 NULL、变长值、更新、删除、并发、持久化及坏磁盘缓冲区导入；所有 bytes 由构造器产生。
不按 id 排序，不强制唯一性。固定三列是布局实验，不是通用 SQL 表框架。

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
