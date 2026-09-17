# 45：排序归并连接

## 问题与前置知识

本分支是数据库查询执行层的独立 C++17 教程：计算两个关系在一个可空整数键上的 inner equijoin。先了解 vector、optional、RID（行身份）与 SQL 三值逻辑。相关主题是 43–46 的其他连接算法；不需要检出其他分支。总目录查看 `git show main:README.md`。

排序归并连接先把非 NULL 行投影为 `(key, 原始RID)` 并分别排序，再用两个游标对齐键。小键一侧前进；相等时找出两侧完整等键组，输出笛卡尔积，然后同时跳过这两组。普通逐行 zip 会把 2×3 个重复键错误缩成 2 条，这是本主题重点。

不变量：游标之前的行已经处理完；小键不可能与另一侧当前或后续更大键匹配；同键组只处理一次，组内每对 RID 恰好输出一次。排序后仍携带原始 RID，所以不会把排序位置误作输入行身份。`merge_comparisons` 只计主归并循环的键对比较轮次，不包括排序和组边界检查；`matched_groups` 计两侧共同拥有的不同非 NULL 键数。

## 输入、输出与语义

`Table` 是 `vector<optional<int>>`，每行只有一个连接键，`nullopt` 表示 SQL NULL。输出 `Match` 为两个输入位置组成的 RID 对；需要载荷时可据此回取两行。本实现不额外要求主键唯一。即使两行键值一样，它们的 RID 不同，必须保留多重集重数。NULL 与 NULL **不匹配**，NULL 与任何值也不匹配。空输入输出为空。负数、0、INT_MIN、INT_MAX 都是普通合法键，不作哨兵。

输入在调用期间不修改；返回结果引用的是原始快照的行位置，调用方若之后重排或删除输入，旧 RID 不再指向原行。结果不承诺排序，也不是集合去重；测试对 RID 对排序后比较整个向量，因此不会掩盖重复丢失。

## 手算轨迹

固定 demo 输入（位置从 0 开始）：

| RID | 左键 | 右键 |
| --- | --- | --- |
| 0 | 2 | 2 |
| 1 | NULL | 3 |
| 2 | 1 | 2 |
| 3 | 2 | NULL |
| 4 | 不存在 | 2 |

- 左排序流 `(1,2),(2,0),(2,3)`，右排序流 `(2,0),(2,2),(2,4),(3,1)`。
- 首轮比较 1 与 2，只推进左游标。
- 次轮比较 2 与 2：左组大小 2，右组大小 3，输出 6 对。
- 两侧越过 2 组后左侧耗尽，结束；主归并比较 2 轮、匹配组 1 个。

运行 demo 的实际输出（先规范化 RID 对顺序）：

```text
0,0
0,2
0,4
3,0
3,2
3,4
merge_comparisons=2 matched_groups=1
```

## 源码导读

- `src/join.h`：数据契约、统计指标与唯一入口 `tutorial::join`。
- `src/join.cpp`：从输入到结果的完整排序归并连接算法，按上面不变量阅读循环。
- `src/demo.cpp`：固定输入、结果规范化和可观察成本；不是正确性测试。
- `tests/join_test.cpp`：独立 nested-loop oracle、显式 2×3 重复组答案、空表／无匹配／全 NULL／整数边界，以及打乱顺序和随机差分。统计计数也有检查。
- `CMakeLists.txt`：无第三方库，关闭编译器扩展，开启警告，CTest 独立测试入口。

## 成本与边界

设左右总行数为 L、R，输出对数为 K。排序 O(L log L + R log R)，归并及重复组展开 O(L+R+K)，辅助排序副本 O(L+R)，输出 O(K)。全部同键时 K=L×R 无法靠归并消除这个输出下界。这里是内存排序，未实现外部排序、有序输入直通或磁盘溢写。

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
