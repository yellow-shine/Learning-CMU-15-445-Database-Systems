# 03 · 移动页缓冲区，而不是复制页字节

## 问题与前置知识

页读取函数返回一个缓冲区，执行器再把它放入槽位。如果每次传递都复制 4 KiB，
复制成本会掩盖真正的查询工作。共享所有权又不一定符合“一个槽位负责释放”的协议。
本课实现独占的可移动字节缓冲区。前置为 01 RAII、02 unique_ptr 与引用；
不需要其它分支源码，unique_ptr 直接来自标准库。

## 值类别、所有权与不变量

有名字的变量表达式是左值，包括函数参数 `other`，即使它的类型是右值引用。
`std::move(other)` 只是转成允许移动的值类别，不会自己搬运任何字节。
真正转移由移动构造/移动赋值完成；const 对象通常不能转移独占资源。
本实现删除复制，避免把复制成本藏在普通赋值里。

缓冲区只有两种状态：`size == 0 && data == nullptr`，或拥有 size 个已初始化字节。
构造分配并清零；移动转移 unique_ptr，再用 exchange 把源长度置零。
只移动指针不清空长度是常见反例：源会看似非空，随后访问空指针。
移动赋值先让 unique_ptr 释放目标的旧分配，再接管源；自移动保持原值。
两个移动操作都是 noexcept，因此 vector 扩容可以安全移动而不必复制。

`at` 在访问前检查范围，空状态和越界都抛 out_of_range。
`data` 是只读借用地址，其有效期取决于当前所有者；所有者移动时地址本身不变，
但目标被覆盖/销毁后地址失效。本例从不读取已释放的旧目标地址。

## 具体轨迹与预期输出

`read_page()` 分配 4 字节并写第一字节 42。返回语句通常被 NRVO 消除，
不能把一次返回打印冒充“必定发生一次移动”。随后显式移动到 destination 才展示协议。

| 操作 | source | destination | slot |
| --- | --- | --- | --- |
| read_page | 4 字节 | 不存在 | 不存在 |
| 移动构造 | 空 | 原来的 4 字节 | 不存在 |
| 分配槽位 | 空 | 4 字节 | 8 个零字节 |
| 移动赋值 | 空 | 空 | 释放旧 8 字节，接管 4 字节 |

```text
construct move: source=0, destination=4
assign move: source=0, first=42
empty access rejected
```

## 源码导读和验证

`src/page_buffer.h` 的两个移动成员是重点，标准 unique_ptr 负责实际 delete[]，
不为演示重新写一个危险的裸指针分配器。`src/demo.cpp` 是返回值到槽位的完整使用链。
`tests/topic_tests.cpp` 验证地址保持、空源、覆盖非空目标、自移动、空对象移动、
移后对象重新赋值、const 越界读取拒绝、零初始化与 1000 个缓冲区的 vector 扩容。
测试不依赖具体 vector 扩容倍数，也不声称用一次成功运行证明没有任何内存错误。

## 成本与局限

分配/零初始化 n 字节 O(n)，访问 O(1)，移动构造 O(1)，移动赋值不复制页字节，
但释放旧目标的成本由分配器决定。空间 O(n)，对象额外空间为指针与长度 O(1)。
析构使用标准数组删除，不调用每字节业务逻辑。构造可能抛 bad_alloc，尚未形成对象时无泄漏。
这不是真实磁盘页读取：read_page 只制造测试字节；没有 I/O、页格式、对齐、pin 或并发。
生产环境通常直接使用 vector<byte>；自定义类型在此用于显式保证“移后必为空”的教学协议，
不能把本类型的空状态保证推广到所有标准库可移动对象。

## 构建、运行与验证

本机 AppleClang 默认缺少 libc++ 搜索路径，验收配置额外传入
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`。
这是本机 SDK 配置绕行，正常工具链不需要；未修改系统安装。

无第三方依赖，使用 C++17、CMake 和 CTest。两种配置均运行独立测试，
测试使用抛异常的 `CHECK`，不是会被 `NDEBUG` 删除的 `assert`。

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

CTest 成功应显示 `100% tests passed`；失败退出非零，超时为 20 秒。
每个主题从共同文档基线独立建立，不需要检出其它主题来运行。
返回完整主题目录：`git show main:README.md`（目录验收状态以 main 为准）。
