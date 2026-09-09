# 12：元组的字节布局

## 问题与前置知识

C++ 对象含有指针、padding 与宿主端序，不能直接写入数据库文件。本节位于逻辑记录与页之间，
把固定 schema `(id: nullable uint32, name: nullable byte string)` 编码成自包含字节序列。
前置知识是移位、数组、optional；相关主题为 11 页文件、13 槽页，本实现不依赖它们。

## 格式与不变量

| 字节 | 内容 |
|---|---|
| 0 | 版本号 1 |
| 1 | NULL 位图：bit0=id，bit1=name，其余必须为 0 |
| 2–3 | 总长度，uint16 小端 |
| 4–7 | id，uint32 小端；NULL 时必须为零 |
| 8–9 | name 起始偏移，固定为 12 |
| 10–11 | name 字节数，uint16 小端 |
| 12… | name 字节，不加结束符 |

NULL 字符串与空串长度同为零，靠位图区分。字符串可含零字节和 UTF-8，长度是字节数，
不是字符数。解码先检查最小头长再访问字段；总长度必须等于缓冲区大小，禁止尾随垃圾。
校验 offset、length 和 NULL 的规范表示，不接受重叠元数据、超界 payload 或未知版本。
最大名字为 65535−12=65523 字节，编码在分配之前拒绝更大长度，避免 uint16 截断。

## 手工轨迹与预期输出

输入 `(42, Ada)`：位图 0，id 字节为 `2a 00 00 00`，偏移 12，长度 3，
payload 是 `41 64 61`，总长 15。两个 NULL 的元组只有头部，位图为 3。

```text
bytes=15 bitmap=0 id=42 name=Ada
null bytes=12 bitmap=3
```

若将长度字段改成 255，但只交给 decoder 15 字节，则抛异常，而不是读取缓冲区外内存。

## 源码导读与验证

`src/tuple.h` 的 Tuple 是逻辑模型，encode/decode 是物理格式边界，put16/get16 显式处理端序。
不使用结构体强制转换，也没有对齐要求。`src/demo.cpp` 展示普通值与全 NULL。
`tests/storage_test.cpp` 检查所有 NULL 组合、0、UINT32_MAX、空串、内嵌零、中文、
最大合法长度、编码溢出，以及每个截断前缀、未知版本、坏位图、坏偏移、坏长度、尾随数据。

## 复杂度与局限

编码与解码时间 O(L)，输出空间 O(L)，L 为名字字节数；固定字段读取 O(1)。
名字会复制；没有零拷贝视图、schema evolution、类型目录、多列通用编码或字符集验证。
这是明确的单 schema 格式，不假装为任意 SQL 类型序列化框架。uint32 不表示负数。
无校验和：合法范围内的位翻转可能变成另一条有效记录，格式校验不等于完整性保护。

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
