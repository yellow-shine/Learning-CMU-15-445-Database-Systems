# 29：Robin Hood 哈希

这是数据库内存访问路径的独立 C++17 教学实验，不依赖 BusTub 或其他分支。
先了解数组、取模、键相等与哈希冲突；相关主题为 28–32 哈希系列。
返回完整目录：`git show main:README.md`（本分支不维护目录状态）。

## 问题与机制

线性探测中，有些键离 home 很远，有些却只走一步。Robin Hood 让“走得更远”的新键优先占位，降低探测距离的方差，并允许查找提早停止。
每槽存 key、value 和 distance；空槽用 optional 表示。home 为 uint32_t(key) % capacity。
插入时若 incoming.distance 大于 resident.distance，交换两者，继续安置被驱逐键。
重复键更新值；插入前先检查重复及容量，满表失败不会在交换中丢键。

查找从 home 开始，遇空槽或 resident.distance 小于已走距离时判不存在。
删除不用墓碑：把后继 distance>0 的项前移并将 distance 减一，到空槽或 distance=0 停止。
满表删除也最多移动 C-1 次；容量 1 同样有效。

## 具体轨迹与输出

容量 5，插入 0、1，再插 5。5 的 home=0；走到槽 1 时距离 1，大于键 1 的距离 0，因此交换。键 1 移到槽 2。
```text
slot=0 key=0 distance=0
slot=1 key=5 distance=1
slot=2 key=1 distance=1
after erase(0)
slot=0 key=5 distance=0
slot=1 key=1 distance=0
```
删除 0 后依次把 5、1 前移，槽 2 清空，而不是留下墓碑。

## 不变量与反例

distance 必须等于从 home 到物理槽的环形距离。每个键的查找路径不能被提早停止条件遮蔽；`valid()` 逐槽验证实际距离、唯一可达位置和 size。
**不能声称整个簇的 distance 全局递增**：例如 0、1、0 这样的相邻距离可以合法存在。
测试固定种子 2901 差分 12000 次操作，并逐次检查不变量；专门验证交换后的具体槽、backward-shift、回绕碰撞、满表失败、重复覆盖和容量 1。

## 成本与限制

C 槽空间 O(C)；分布均匀、负载较低时插入与查找期望 O(1)，最坏 O(C)。删除可移动整个簇，最坏 O(C)，消除了墓碑的长期累积。
`valid()` 是教学诊断，最坏 O(C²)，不在生产操作热路径中调用。
固定容量，无扩容；只降低探测方差而非消除冲突。整数键可为负，值按复制语义返回；不暴露可变槽引用。

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
