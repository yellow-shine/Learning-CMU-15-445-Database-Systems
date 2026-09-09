# 38：向量距离与精确 Top-K 基线

向量索引按相似程度而不是键的大小检索。判断近似索引是否“召回得好”，必须先有精确答案。本例扫描所有向量并真正计算距离，再选择 Top-K，作为 39 IVF 的独立正确性基线。前置：向量、点积、排序与浮点数。

## 距离与接口语义

ExactIndex 构造时固定正维度和 Metric；add 分配从零递增的稳定 id，允许重复向量，没有用户自定义重复 id 的歧义。search 返回按 `(distance,id)` 严格排序的 Hit 数组，同距离时较小 id 优先。不用 epsilon 作为排序相等条件，以免破坏严格弱序。

- squared_l2：各维差的平方和，省略开平方不改变最近邻顺序；返回值不是欧氏距离本身。
- cosine：`1 - dot(a,b)/(|a||b|)`，范围 [0,2]，零向量无定义，添加与查询时拒绝。先按各向量最大绝对分量缩放，防止巨大／微小有限坐标的范数溢出；舍入造成的余弦越界夹紧到 [-1,1]。

维度错误、NaN／无穷、未知 metric 抛 invalid_argument；L2 中间溢出抛 overflow_error 而不是让 inf 或 NaN 进入比较器。k 为 size_t，0 返回空，超过记录数截到记录数，空库返回空。但即使 k=0 或空库也检查查询有效性。没有负 k 的语义，调用方不可把负整数强转成 size_t。

## 手算轨迹

插入 id0=(1,0)、id1=(-1,0)、id2=(0,2)，查 q=(0,0)，平方 L2 分别为 1、1、4；k=2 返回 0、1，以 id 打破并列。demo 输出：

```text
id=0 squared_l2=1
id=1 squared_l2=1
```

余弦例子：(1,0) 与 (0,1) 距离 1，与 (-1,0) 距离 2，与 (3,0) 距离 0。余弦忽略幅度，L2 不忽略；应由应用决定度量，不能把两者的召回答案混用。

## 源码导读与验证

`src/vector_search.h`：validate 在边界拒绝坏向量；distance 实现两种度量；ExactIndex 保存行并扫描；top_k 使用 nth_element 裁剪后对选中部分排序。标准选择算法没有代替距离计算，也没有声称近似搜索。`src/demo.cpp` 展示并列结果。

`tests/index_test.cpp` 独立测试空库、k=0/1/N/>N、维度、非有限数、零向量、距离数值、并列 id、巨大坐标余弦与 L2 溢出。固定种子生成 300 个三维向量、60 个查询，用独立平方和加全排序 oracle 对比五种 k，避免仅用 demo 的三个点说明正确性。

## 成本与限制

存储 O(Nd)。每次查询 O(Nd) 计算，nth_element 平均 O(N) 选择，结果排序 O(k log k)，额外 O(N) Hit 缓冲；标准库 nth_element 不承诺所有输入上的最坏线性界。小 k 生产实现可用 O(k) 堆节省临时空间。没有 SIMD、GPU、压缩、持久化、删除或并发保护。浮点“精确”指穷举所有记录，不是实数数学精确；相近距离受舍入影响，有限坐标的 L2 仍可能溢出并被拒绝。

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

C++17，无第三方依赖。测试独立于 demo，以异常使进程非零退出，Release 不会关闭检查。
macOS 若编译器找不到标准头文件，仅配置时增加以下参数（不要写死 SDK 路径）：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

Release 的配置可同样附加该参数。回总目录：`git show main:README.md`。
