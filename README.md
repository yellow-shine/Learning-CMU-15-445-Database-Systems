# 05 · 用 STL 做内存查询，而不是手写容器

## 问题与前置知识

对内存行过滤删除标记、获取不同页号、按区域计数，是执行器中常见的数据整理任务。
本课学习容器、迭代器和算法的搭配，不声称实现数据库索引或外部排序。
前置为 02/03 的值与拥有权、04 的模板与 lambda；所有运行代码独立存在。

## 容器选择与算法不变量

`vector<Row>` 连续存储，适合顺序扫描。`live_ids` 按值接收，因此允许整理副本而不改变
调用方：remove_if 把存活行移到前缀，返回逻辑末尾；erase 才真正缩小大小。
remove_if 单独调用不会缩小容器，尾部仍是有效但值未指定的对象，不应该把它当结果。
提取 id 后 sort 建立升序不变量，unique 把相邻重复合并，erase 删除尾段。
只有排序之后 binary_search 才有前提；这不是碰巧在乱序输入上找到了就正确。

区域分组使用 unordered_map：只要按键累加，无需维护键序。最后复制到 vector 并排序，
使输出确定，不依赖哈希桶顺序。DISTINCT id 使用集合语义；区域 COUNT 保留重复行，
两者都排除 deleted 行。没有 NULL，也没有按 id 强制唯一的表约束。

## 迭代器失效：避免反例中的未定义行为

| 操作 | 规则 | 本课做法 |
| --- | --- | --- |
| vector 扩容 | 所有指针、引用、迭代器失效 | 只保存索引，操作后用 at 重新获取 |
| vector erase/insert | 位置及其后迭代器失效；插入扩容则全部失效 | 使用 erase 返回的新迭代器 |
| unordered_map rehash | 迭代器失效，但元素引用/指针仍有效 | 重取 find；测试保留的元素引用 |
| unordered_map erase | 被删元素的引用也失效 | 不再访问被删元素 |

索引不是永久行标识：前面插入后，索引 0 代表新行。需要稳定身份时应保存键再查找。
我们不比较或解引用已失效迭代器来“证明”规则，也不依赖 vector 的具体扩容倍数。

## 具体轨迹

输入 `(3,east,live), (1,west,live), (3,east,live), (2,west,deleted)`。
过滤副本得到 id 3、1、3；排序为 1、3、3；unique+erase 得到 1、3。
分组遇到 east 两次、west 一次，排序键之后稳定输出：

```text
distinct live ids: 1 3
contains 3: 1
east: 2
west: 1
reacquired first id: 3
```

最后一次 reserve 强制扩大容量，重新通过下标获得第一行，不访问旧引用。

## 源码与验证

`src/query.h` 中的两条实际数据流水线是核心；`src/demo.cpp` 演示二分查找和安全重取。
`tests/topic_tests.cpp` 使用固定种子生成 100 组数据，与 set/map 参考结果比较；
测试空表、全部删除、重复、缺失键、原输入大小不变、插入后的索引身份变化、erase 返回值
和 rehash 引用稳定性。独立 CHECK 在 Release 仍执行。标准容器在此正是教学对象，
不是冒充 B+Tree；参考容器使用不同的有序机制验证结果。

## 成本与局限

n 行、k 个存活 id、g 个区域：复制/过滤 O(n)，排序 O(k log k)，unique O(k)，
binary_search O(log k)。哈希分组平均 O(n)，恶意碰撞最坏 O(n²)，结果排序 O(g log g)。
空间 O(n+g)，字符串复制与比较还需计入字符长度。无磁盘 I/O、并发访问、内存预算或事务。
这是刻意的内存算法，数据超过内存需另学外部排序，不能把 std::sort 称作外部排序。
输出行副本有额外开销，但避免将悬空迭代器作为查询接口返回。

## 构建与预期验证

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

C++17，无第三方库，CTest 成功为 `100% tests passed`，失败非零，20 秒超时。
若 macOS AppleClang 找不到标准头文件，两次配置均附加
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`。
本机验证使用此条件性 SDK 绕行；没有在 CMake 中硬编码路径。总目录：`git show main:README.md`。
