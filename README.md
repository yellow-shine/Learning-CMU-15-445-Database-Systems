# 21 · Delta Encoding：存相邻差分，而不是重复存大整数

## 问题与前置知识

有序时间戳、递增序号的绝对值可能很大，而相邻变化很小。差分编码保留首值和后续变化量；
但若变化量仍用固定 64 位保存，本身并不压缩。本教程把有符号差分经 ZigZag 转成无符号数，
再用七位一组的变长整数真正输出字节。前置知识是补码范围、无符号移位、前缀和、字节数组。
相关主题是 18 RLE、19 位打包、20 字典编码；本分支不依赖其他 checkout。

## 算法与格式

输入是 `vector<int64_t>`。输出先写 4 字节小端行数 n；若非空，再依次写首值和 n-1 个差分。
首值也用 ZigZag+varint，没有额外标志或索引。空输入就是四个零字节。

| 步骤 | 规则 | 不变量 |
| --- | --- | --- |
| 差分 | d[i]=x[i]-x[i-1] | 数学结果必须能放进 int64_t |
| ZigZag | 非负 x 映到 2x，负数映到 -2x-1 | 小绝对值对应小无符号值 |
| varint | 每字节低 7 位存数据，高位表示还有后续 | 低位组先写，最多 10 字节 |
| 还原 | 首值直接读，后续 previous+delta | 每次加法前检查有符号范围 |

例如 `0→0, -1→1, 1→2, -2→3`；INT64_MIN 映到 UINT64_MAX，INT64_MAX 映到 UINT64_MAX-1。
公式仅表示数学含义，代码不会直接执行 `-2*x-1`：负数先做 `-(x+1)` 再转无符号乘二加一。
解码 `u>>1` 总能放入 int64_t，奇数返回 `-1-half`，避免把过大的无符号值强转有符号。

## 溢出不是可以忽略的边界

相邻输入都是合法 int64_t，并不代表差分合法。`[INT64_MIN,0]` 需要正的 2^63，必须拒绝。
编码在减法前检查：previous>0 时 current 不能小于 MIN+previous；previous<0 时 current
不能大于 MAX+previous。阈值本身都可表示。解码同理：delta>0 时 previous<=MAX-delta；
delta<0 时 previous>=MIN-delta，然后才执行加法。任何失败都抛 `std::invalid_argument`，
不返回部分编码，不做环绕、不饱和、不使用非标准 `__int128`。

这是明确的格式上限：不支持所有可能的 int64_t 序列，但每个单项、重复最大／最小值都支持。
若生产应用需要任意跳变，可以增加绝对值逃逸标记；这里不隐式引入另一种格式。

## 逐步轨迹与预期输出

输入 `[1000,1001,1003,1002,1002]`：

| 项 | 首值或差分 | ZigZag | varint 字节（十进制） |
| ---: | ---: | ---: | --- |
| 0 | 1000 | 2000 | 208 15 |
| 1 | 1 | 2 | 2 |
| 2 | 2 | 4 | 4 |
| 3 | -1 | 1 | 1 |
| 4 | 0 | 0 | 0 |

2000=80+15×128，第一字节 80|128=208，第二字节 15 不置续位。
还原前缀和依次为 1000→1001→1003→1002→1002，不要求输入单调递增。

```text
first=1000 deltas: 1 2 -1 0
raw=40 encoded=10 bytes
payload: 208 15 2 4 1 0
```

大小比较为原始 5×8 字节整数负载与真实 Bytes.size()（含 4 字节行数）。
差分后小值各只用一字节；单个 INT64_MIN 编码需要 4+10=14 字节，比原始 8 字节更大。
不适合随机大幅跳动的数据，更不承诺所有合法输入都压缩。

## 源码导读、成本与错误测试

- `src/codec.h`：仅暴露 encode/decode 和 Values；接口注明差分溢出拒绝。
- `src/codec.cpp`：局部 ZigZag／varint 支撑，编码先检查减法，解码先检查加法。
- `src/bytes.h`：自带的小端读取支撑快照，源自 19，不依赖该分支运行。
- `src/demo.cpp`：手算示例的实际字节和还原一致性检查。
- `tests/codec_test.cpp`：独立的 CHECK；种子 21445，500 组有界随机游走和全范围随机单项。

编码／解码时间 O(n)，每项最多十字节，工作空间 O(1)，输出空间 O(n)。
编码尺寸为 `4+L(zigzag(x[0]))+ΣL(zigzag(delta))`（非空时）；L 为 varint 长度。
访问第 k 项需累加前 k 项，不能直接从第 k 个字节读取第 k 行，没有分块跳表或 SIMD。

测试包括空、单项、重复最值、递减、正负差分、最大行数，以及恰好为 MIN/MAX 的合法差分。
编码溢出和解码还原溢出分别构造，不让两者相互掩盖。手算字节 oracle 防止往返测试互相抵消。
解码拒绝所有截断点、尾随字节、过大行数、非最短 varint、超过 64 位或十字节的 varint。
第十字节只能是 0 或 1，且多字节末组不能是零，所以规范十字节表示最终只能以 1 结束。
没有版本号、校验和和文件 I/O；合法位翻转无法保证被发现。

## 构建与验证

无第三方依赖；需要 CMake 3.16+ 和支持 C++17 的编译器。独立检出本分支即可运行，
不需要检出相关主题。返回总目录使用 `git show main:README.md`，无需合并实现。

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

本次验证环境为 macOS AppleClang 17。该主机 CommandLineTools 默认未找到 SDK 中的
libc++ 头文件，配置时实际额外传入下面参数（正常工具链不需要）：

```sh
-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

测试程序与 demo 分离；`tests/check.h` 失败时抛异常，使 CTest 返回失败，
不使用会在 Release 中消失的 `assert`。构建启用标准 C++17、关闭语言扩展、开启警告。
`src/bytes.h` 是本分支自带的小端读写支撑，不依赖其他 checkout；越界读取、
非法数量、残余字节等格式错误抛 `std::invalid_argument`，内存分配失败由标准库报告。

## 共通边界

这是内存块编码实验，不是数据库文件格式：没有版本号、校验和、页 I/O、并行压缩或
自动选择最优编码器。每块最多 1,000,000 个元素；这是明确的资源约束，不是类型本身的上限。
不能检测“损坏后恰好仍合法”的字节流，也不声称压缩总是节省空间。
大小比较使用真正生成的 `Bytes::size()`，包含本格式元数据，不是 C++ 容器 `sizeof`；
不计分配器容量、对象头、临时工作空间。教学时间成本是 RAM 模型分析，不是磁盘实测。
