# 32：哈希索引：Key 到多个 RID

这是数据库内存访问路径的独立 C++17 教学实验，不依赖 BusTub 或其他分支。
先了解数组、取模、键相等与哈希冲突；相关主题为 28–32 哈希系列。
返回完整目录：`git show main:README.md`（本分支不维护目录状态）。

## 数据库中的问题

非唯一索引不能只存 key→value：多条元组可以有相同索引键，必须返回所有匹配位置。
RID 由无符号 32 位 page、slot 两部分组成；二者都相等才是同一记录位置。
本分支实现内存索引，不实现堆表；调用者拿 RID 访问对应页槽并验证元组可见性。没有 RID 生命周期管理，不能保证 RID 永不失效。
前置主题为哈希冲突、页与槽；不需要检出这些分支才能运行。

## 数据结构与接口契约

固定桶数组，按 uint32_t(key) % bucket_count 选择桶；桶内是显式的 Entry 序列，每项保存 key 和 RID posting list。
这是自实现 separate chaining 的连续容器变体，不是 unordered_map；数组容器只负责内存管理，冲突匹配与操作由本代码完成。
查找必须比较完整键，而不是把哈希相等当键相等。

- `insert(key,rid)`：允许相同 key 的不同 RID，拒绝完全相同的 (key,RID) 对，返回是否新插入。
- `lookup(key)`：返回所有 RID 的副本，缺失为空；不承诺排序，调用者不能通过返回值修改索引。
- `erase(key,rid)`：只删精确对，不删同键其他 RID，也不删碰撞桶里的其他键；最后一个 RID 删除后移除 key 项。
- `size()` 统计对数，`key_count()` 统计不同键数，二者不能混用。

RID 全部 uint32_t 取值（含 0、最大值）都合法。本索引允许同一 RID 出现在不同键下，不验证基础表一致性；更新键时应由上层协调删除旧对和插入新对。

## 具体轨迹与输出

容量 5 时，键 7 与 12 都路由到桶 2。插入 7→(1,0)、7→(1,1)、12→(2,0)。桶里有两个 Entry，而不是覆盖掉第一个 7。
删除 (7,(1,0)) 后只剩第二条 7，12 不受影响：

```text
key=7 (1,0) (1,1)
key=7 (1,1)
key=12 (2,0)
pairs=2 keys=2
```

如果只按 slot 比较，(1,0) 与 (2,0) 会被误认为同一 RID；如果只按 key 删除则会误删所有重复键元组，测试专门覆盖两种反例。

## 不变量、测试与复杂度

每个 key 只在其桶出现一次，posting list 非空且内部 RID 唯一；全体列表长度和等于 size。
`valid()` 验证路由、重复键、重复 RID 与计数。固定种子 3201 的 8000 次操作对照 map<int,set<RID>>，每步比较完整结果集，最后精确删除至空。
专项覆盖重复对幂等、同页不同槽/同槽不同页、碰撞隔离、缺失删除、列表副本、极值与删空后重插。

K 个不同键、P 个对、M 个桶的空间 O(M+K+P)。均匀哈希下桶内查找期望 O(1+K/M)；返回 r 个 RID 另需 O(r) 拷贝。
插入/精确删除还扫描该键的 r 个 RID，最坏 O(K+P)；单键热点不会因增加桶数而消失。
诊断 valid 用朴素两两比较，最坏 O(K²+P²)，不放入操作热路径。
固定桶数不自动重哈希，列表无磁盘溢出页、不支持范围扫描、复合键、NULL 或唯一约束；大规模场景需要扩容/页式 posting list，而不是据此宣称索引能替代 B+Tree。

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
