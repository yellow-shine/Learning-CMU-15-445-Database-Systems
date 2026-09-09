# 28：线性探测与墓碑

这是数据库内存访问路径的独立 C++17 教学实验，不依赖 BusTub 或其他分支。
先了解数组、取模、键相等与哈希冲突；相关主题为 28–32 哈希系列。
返回完整目录：`git show main:README.md`（本分支不维护目录状态）。

## 问题与机制

开放寻址把键值直接放入连续槽数组，避免每条记录单独分配链节点。
本实现使用 uint32_t(key) % capacity 作为起点，冲突后逐槽前进并回绕。
槽有 empty、live、deleted 三态。empty 证明键不存在，deleted 只表示可复用，不能截断探测链。

`put` 返回成功或满表失败；重复键覆盖值、不增加大小。即使先遇见墓碑，也必须继续寻找重复键。
`get` 返回 optional 值；`erase` 返回是否真的删到键。每条探测路径最多走 capacity 步，因此全墓碑或满表仍终止。
容量必须正数；负数键先转无符号，避免负下标，不假定整数哈希无冲突。

## 具体轨迹与预期输出

容量 5，插入 1→10、6→60、11→110，三者起点都为 1：

| 操作 | 槽 1 | 槽 2 | 槽 3 |
|---|---|---|---|
| 三次插入 | 1 | 6 | 11 |
| 删除 6 | 1 | 墓碑 | 11 |
| 更新 11 | 1 | 墓碑 | 11→111 |
| 插入 16 | 1 | 16 | 11→111 |

因此不能把删除槽改成 empty：查找 11 会提前停止。demo 输出：
```text
after erase(6): 11=110
11=111 size=3
```

## 不变量、成本与测试

每个 live 键恰好出现一次，size 只数 live 槽；从其 home 到当前位置没有 empty。
固定种子 2801 的 12000 次操作逐次对比 std::map，另外覆盖满表更新、回绕碰撞链、全删除与整数极值。
容量 C 的空间 O(C)，低装载且分布均匀时操作期望 O(1)，最坏 O(C)。墓碑累积会降低性能，即使 size 很小也可能扫描全表。

本主题刻意固定容量，不自动扩容、不清理墓碑；需要长期运行时可在装载率或墓碑率过高时重建。它不支持范围查询，也不保证物理槽次序等于键顺序。

## 源码导读

1. `src/hash_table.h`：真实的数据结构与查找、插入、删除实现；从成员布局开始，沿查找路径阅读。
2. `src/demo.cpp`：固定输入的可复现轨迹；不是正确性测试的替代品。
3. `tests/hash_test.cpp`：专项反例与固定种子差分测试；标准映射只作测试 oracle，不实现被学算法。
4. `CMakeLists.txt`：无依赖构建；CTest 失败返回非零，检查不受 NDEBUG 影响。

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

如果 macOS SDK 的 libc++ 头文件搜索异常（例如找不到 vector），仅在本机配置时追加：
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```
Release 同理；此环境修复未硬编码进工程。正常工具链无需此参数。

## 实验边界

单线程、纯内存、整数键，不提供磁盘页、WAL、锁、事务或 SQL NULL 语义。
哈希均用于教学而非抵抗恶意输入；不能据平均常数时间宣称最坏常数时间。
测试覆盖本实现约定，不构成生产数据库正确性或性能认证。
