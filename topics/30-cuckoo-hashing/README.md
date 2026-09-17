# 30：Cuckoo 哈希：驱逐与有界重建

这是数据库内存访问路径的独立 C++17 教学实验，不依赖 BusTub 或其他分支。
先了解数组、取模、键相等与哈希冲突；相关主题为 28–32 哈希系列。
返回完整目录：`git show main:README.md`（本分支不维护目录状态）。

## 问题与数据结构

Cuckoo 以较昂贵的插入换取固定两个候选位置的等值查找。两个长度 C 的数组各有一个候选槽，使用带种子的 64 位混合函数；不是用 map 包装算法。
一个键只能占其中一个槽。更新重复键只覆盖值，删除清空候选槽即可，不需要探测链墓碑。

插入先尝试第一张表；冲突则把原住户换出，原住户去另一张表的候选位置，以此交替。
`place` 记录 (被安置键, 目标表)；重复则保守认定存在循环风险并停止。这不是对整个表状态的严格环证明，可能提前重建。
此外每次安置最多 8C 步，即使检测漏过也保证终止。失败后换种子、容量翻倍（不超过 max_capacity）并重新安置**原表全部键和新键**。

## 失败原子性与上限

正常插入也在表副本上驱逐，只有完整成功才 swap 发布；失败副本中的被驱逐键绝不用于恢复原表。
最多 max_rebuilds 次重建，仍失败则 put 返回 false，所有原键和值、size、容量均保留。统计记录的是最近一次 put 的尝试，包括失败工作。
构造要求 1<=capacity<=max_capacity<=65536、max_rebuilds<=16；无无限增长、无 stash。
这些上限是教学资源边界，不是通用最大数据库大小。内存分配异常传播给调用者，但不会发布半张表。

## 可复现轨迹

每表容量 1 时，所有键只能去 A[0] 或 B[0]：插入 1 占 A；插入 2 驱逐 1 到 B。
第三个键无法放进两个槽，驱逐检测/预算阻止死循环，两次重建也无法改变容量上限 1。

```text
second insertion kicks=1
third insertion accepted=0
rebuilds=2 preserved=10,20
growing size=20 key20=200
```

demo 后半用默认允许增长的表插入 20 键，展示重建成功路径，而不是仅演示失败。

## 不变量、测试和成本

每个 resident 位于其候选位置且不在另一表重复出现；`valid()` 验证位置、唯一性和计数。
专项测试覆盖真实驱逐、循环风险检测、零次重试、两次重建耗尽不丢键、更新与极值；固定种子 3001 的 5000 次差分逐次检查全键空间与不变量，并要求确实发生成功重建。
查找/删除最坏两个槽，O(1)；空间 O(C)。本实现为失败原子性复制数组，普通插入至少 O(C)，**不宣称插入均摊 O(1)**。
用线性 seen 检测使单次安置最坏 O(C²)；R 次重建、N 个键时保守 O(RNCmax²)，临时空间 O(Cmax+N)。适合小规模观察机制，不用于吞吐基准；高性能版本可用撤销日志和更快的驱逐检测。

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
