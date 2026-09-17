# 52-modification-executors：原子修改与表索引一致性

## 问题与前置知识

修改执行器不仅改表，还必须维护索引；批量语句最后一行失败，前面的行不能偷偷提交。
本主题位于执行层，前置是 RID、唯一索引、异常保证和值拷贝。52 聚焦语句原子性，不声称实现事务管理器。

## 数据结构与语义

表为 vector<optional<Row>>：RID 稳定，删除留下空槽，插入总是追加，不复用 RID。
map<key,RID> 是明确标注的辅助唯一索引，不能据此声称实现 B+Tree。key 唯一，value 可重复，无 NULL。
Insert 接收整批行；Update 接收目标 RID 和完整替换行；Delete 接收整批 RID。
不存在或已删除的 RID 拒绝，重复修改同一 RID 拒绝，空批成功。Update 的唯一性按语句最终状态验证，
因此两个现有 key 可在同一批中交换，不受逐行临时冲突影响。

## 原子性算法与不变量

1. 复制当前表为 candidate，只修改这个私有副本；任何校验、分配或用户注入异常均不影响已发布状态。
2. 从 candidate 重新构建唯一索引；重复 key 抛异常，销毁整个候选。
3. 全部成功后用两次 noexcept swap 发布表和索引，期间没有可能抛出的操作。

这是单线程语句级 copy-on-write，不是多线程可见性协议。索引不变量是每个活行恰有一个同 key 的 RID 条目，
且无指向死行的多余条目。Consistent 扫表并检查映射和条目数。返回的 RID 只有语句成功才交给调用者。
DuringIndex 故障点在候选索引第一次实际插入后触发；无活行就不经过该故障点。

## 具体轨迹与预期输出

插入 `(1,10),(2,20)` 得 RID 0、1；更新 RID 0 为 `(3,30)` 删除旧 key 1 的映射；
删除 RID 1 同时删除 key 2 映射。再插入 `(4,40),(3,99)`，候选表先追加两行，但构建索引发现 key 3 重复，
整个候选被丢弃，包括合法的 key 4。
Demo 输出：

```text
rollback: duplicate key
RID=0 key=3 value=30
consistent=1 key4_exists=0
```

测试独立检查重复新键、与旧键冲突、部分有效后缺失 RID、重复目标、键交换、空批、墓碑及不复用 RID。
对 Insert/Update/Delete 分别在表修改后、索引插入后注入异常，比较完整表快照并检查每个索引映射，
不是只检查返回码。分配异常同样发生在发布前，但测试没有替换全局 allocator 来强制 bad_alloc。

## 复杂度与代价

当前槽位数 n、语句输入 m、活行数 l：复制表 O(n)，候选修改 O(m log m)（目标去重），
重建索引 O(n+l log l)，额外空间 O(n+m+l)。墓碑长期积累也计入 n。
这是有意选择的教学上限；大表应改用撤销日志和增量索引维护，不能把这里的成本称作每行 O(log n)。

## 构建与验证

本分支完全独立，C++17 标准库足够，无需 BusTub 或其它分支。使用 CMake 3.16+：

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

如果 macOS 的 AppleClang 报找不到标准库头文件，给上述两个配置命令附加
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`；
这是本机 SDK 搜索路径排错，不是算法的依赖，不应硬编码进 CMake。

## 源码导读与测试入口

先读 `src/engine.h` 的数据结构，再沿调用方向读算子，最后读 `src/demo.cpp` 的装配。
`tests/topic_tests.cpp` 是独立正确性程序，CHECK 失败抛异常并返回非零，Release 不会移除检查。
CTest 运行它，demo 只用于可见轨迹，不代替测试。实现为单头文件，便于对照循环和状态变化；
没有 SQL 解析器，也没有依赖隐藏的运行时或文件数据。总目录请通过 `git show main:README.md` 查看。

## 范围与局限

单线程、内存、语句级复制发布；无事务隔离、WAL、持久化或崩溃恢复。唯一索引用 std::map 辅助，重建成本 O(n+l log l)，墓碑不回收。故障注入覆盖发布前，不模拟断电。
