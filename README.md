# 44：索引嵌套循环连接

## 问题与前置知识

本分支是数据库查询执行层的独立 C++17 教程：计算两个关系在一个可空整数键上的 inner equijoin。先了解 vector、optional、RID（行身份）与 SQL 三值逻辑。相关主题是 43–46 的其他连接算法；不需要检出其他分支。总目录查看 `git show main:README.md`。

索引嵌套循环用外表键探测内表的二级索引，避免为每个外行全表扫描。这里的索引是按 `(key, RID)` 排序的只读数组：二分定位第一个不小于探测键的条目，然后顺序走完所有同键候选，通过 RID 回表读取并验证。标准排序／二分只是支撑索引，核心是逐外行探测、枚举候选、回表连接；本分支不声称实现 B+Tree。

不变量：索引包含每个非 NULL 内行且恰好一次，RID 指向当前调用的不可变输入；lower_bound 之前不存在匹配，等键区间之外不存在匹配。重复外键必须再次探测，重复内键必须全部访问。`index_entries` 是索引条目数，`probes` 是非 NULL 外行数，`candidate_visits` 是实际按 RID 读取内行次数，不是二分键比较数或磁盘 I/O 次数。

## 输入、输出与语义

`Table` 是 `vector<optional<int>>`，每行只有一个连接键，`nullopt` 表示 SQL NULL。输出 `Match` 为两个输入位置组成的 RID 对；需要载荷时可据此回取两行。本实现不额外要求主键唯一。即使两行键值一样，它们的 RID 不同，必须保留多重集重数。NULL 与 NULL **不匹配**，NULL 与任何值也不匹配。空输入输出为空。负数、0、INT_MIN、INT_MAX 都是普通合法键，不作哨兵。

输入在调用期间不修改；返回结果引用的是原始快照的行位置，调用方若之后重排或删除输入，旧 RID 不再指向原行。结果不承诺排序，也不是集合去重；测试对 RID 对排序后比较整个向量，因此不会掩盖重复丢失。

## 手算轨迹

固定 demo 输入（位置从 0 开始）：

| RID | 左键 | 右键 |
|---|---|---|
| 0 | 2 | 2 |
| 1 | NULL | 3 |
| 2 | 1 | 2 |
| 3 | 2 | NULL |
| 4 | 不存在 | 2 |

- 右索引排序后为 `(2,0),(2,2),(2,4),(3,1)`，NULL 不入索引。
- 左 RID 0 探测 2，访问三个候选；RID 1 是 NULL 不探测。
- RID 2 探测 1，二分停在 2，但无等键候选；RID 3 探测 2，再访问三个。
- 索引 4 条，探测 3 次，候选回表 6 次；若内表没有匹配，探测仍发生但回表为零。

运行 demo 的实际输出（先规范化 RID 对顺序）：

```text
0,0
0,2
0,4
3,0
3,2
3,4
index_entries=4 probes=3 candidate_visits=6
```

## 源码导读

- `src/join.h`：数据契约、统计指标与唯一入口 `tutorial::join`。
- `src/join.cpp`：从输入到结果的完整索引嵌套循环连接算法，按上面不变量阅读循环。
- `src/demo.cpp`：固定输入、结果规范化和可观察成本；不是正确性测试。
- `tests/join_test.cpp`：独立 nested-loop oracle、显式 2×3 重复组答案、空表／无匹配／全 NULL／整数边界，以及打乱顺序和随机差分。统计计数也有检查。
- `CMakeLists.txt`：无第三方库，关闭编译器扩展，开启警告，CTest 独立测试入口。

## 成本与边界

设左右总行数为 L、R，输出对数为 K。本次索引构建 O(R log R) 时间、O(R) 空间；连接 O(L log(R+1)+K) 时间，输出 O(K) 空间。复用已有索引时可摊掉构建成本。本实现每次 join 自建索引快照，不能把总成本只说成探测成本。RID 访问计数是逻辑元组访问，不等于物理页读；没有缓存或随机磁盘 I/O 模型。

只支持内存、单线程、单键整数内等值连接；不含 outer/semi/anti join、非等值谓词、复合键、SQL 解析、并发更新或事务。输出全部物化，极大重复组可能耗尽内存；标准容器的分配异常向调用者传播，不会返回伪造的部分成功结果。计数器用 size_t，受进程地址空间／计数范围限制；不是生产级资源预算执行器。

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

预期两个配置均 `100% tests passed`；直接运行测试可看到边界、200 次置换和 1000 次固定种子差分通过的提示。检查使用抛异常／非零返回，不依赖 Release 会移除的 assert。空表分别放左右，重复键显式验证六个 RID 对；随机数据含 NULL、负数、重复键，且覆盖左右不同大小与平局。输入顺序变化后重新按新 RID 计算 oracle，而不是误用旧 RID。

若 macOS 的 AppleClang 报标准头文件找不到，可仅在本机配置时附加 SDK 搜索路径（Release 同理）：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

此参数是工具链排错，不是项目依赖；其他平台无需设置，也没有写死本机 SDK 路径。
