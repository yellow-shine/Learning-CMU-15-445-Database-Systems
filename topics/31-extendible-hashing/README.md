# 31：可扩展哈希：目录与桶分裂

这是数据库内存访问路径的独立 C++17 教学实验，不依赖 BusTub 或其他分支。
先了解数组、取模、键相等与哈希冲突；相关主题为 28–32 哈希系列。
返回完整目录：`git show main:README.md`（本分支不维护目录状态）。

## 问题与机制

固定哈希表满了往往要迁移全部记录；可扩展哈希用目录间接指向桶，只分裂溢出的桶。
目录长度是 2^global_depth，取 uint32_t(key) 的低 global_depth 位索引。
桶有 local_depth 和至多 B 条键值。多个目录项可以指向同一个 shared_ptr 桶，这种别名是算法的核心，不是意外共享。

插入先更新重复键，未满就追加。满桶按第 local_depth 位划分成两个新桶，局部深度加一。
若原 local_depth 等于 global_depth，先将目录复制一遍、全局深度加一；否则只重定向已有别名。
所有指向原桶的目录入口必须按新位选择左右桶，不能只改触发插入的那一项。分裂后重新定位，再尝试插入；可能连续分裂空侧桶。

## 不变量

0<=local_depth<=global_depth<=max_depth；每桶有 2^(global-local) 个目录别名。
同桶入口的低 local_depth 位相同；每个键根据目录路由到实际拥有它的桶。键唯一，更新不改变 size。
`valid()` 检查深度、别名数、前缀、容量、键可达性及重复键；目录接口只暴露深度/是否同桶，不允许外部篡改。
禁止复制对象，避免默认浅拷贝导致两个独立表共享可变桶。

## 轨迹与预期输出

桶容量 2：先插 0、2，初始唯一桶满。插 1 时分裂为偶数桶 {0,2} 与奇数桶 {1}，global=1。
插 4 时偶数桶再满，目录加倍：低二位 00 的桶为 {0,4}，10 的桶为 {2}；奇数桶保持 local=1，被入口 01 和 11 共享。

```text
global=2 directory=4
entry=0 local=2
entry=1 local=1
entry=2 local=2
entry=3 local=1
alias(1,3)=1 key4=40
```

再插 3、5 会分裂奇数桶，但 global 仍为 2，测试显式覆盖这一不加倍路径。

## 测试、成本和限制

固定种子 3101 做 7000 次 std::map 差分，每步全键查找与不变量检查；另测空表、深度 0、无效参数、极值、连续分裂和深度耗尽。
默认 max_depth=16，允许 0..16；B 必须大于 0。达到深度上限且桶满时返回 false、不丢原键，但之前成功的分裂可能已经增大目录。这是保留逻辑内容、非结构回滚的失败语义。
单次 split 先分配新桶与目录再 swap，分配失败不会发布半个分裂；异常向调用者传播。

目录 D=2^g，占 O(D) 指针；桶记录空间 O(N)，桶元数据 O(桶数)。查找/删除 O(B)，删除仅桶内移动，不做合并或目录收缩。
分裂复制整个目录，因此 O(D+B) 而非严格局部 O(B)；一次插入最多 max_depth 次分裂。
低位 identity 哈希方便手算，但低位集中会膨胀目录并提前失败；实际索引应使用混合哈希、溢出策略和磁盘页。本实现没有磁盘 I/O，不能把指针访问数直接当磁盘读次数。

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
