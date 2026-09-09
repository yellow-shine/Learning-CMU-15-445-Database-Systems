# 62 连接顺序：子集动态规划与穷举 oracle

## 问题与支持范围
连接顺序改变中间结果大小。前置知识是内等值连接、NDV、动态规划和位集合。
本实现明确只支持 1–12 张表，同一个非 NULL 整数键的等价类：所有表键必须相等。
这相当于连通的同键内等值连接，不是任意图、多键或外连接优化器。
Query 的非等值和左外连接语义会被拒绝，不能不加检查地利用结合律。
重复键采用多重集语义；每个输入行带表内行号，答案保留完整行号组合，避免只比较行数。

## 算法与成本不变量
每个子集 mask 保存一个最优树。叶子成本 0；对每个非平凡子集枚举左右分割，
候选成本 C(L)+C(R)+N(L)N(R)，代表物化嵌套循环的比较次数。
左右交换成本相同，因此固定最小表编号在左边，去掉对称重复。
proper subset 数值小于 mask，按 mask 递增时子问题已经完成。

子集基数是各表行数乘积除以最大 NDV 的 (表数-1) 次方。
这是假设同一均匀值域的教学估计，独立于树形，因此同子集可安全只保留最低成本树。
它可能高估不相交值域，最终执行仍逐键比较，不靠估计产生答案。
`exhaustive` 不做 memo 或最优剪枝，返回所有无左右对称的二叉树，仅允许最多六表。
DP 与穷举比较的是这个模型的最优成本，不是现实硬件上的最优执行时间。

## 手算轨迹
T0=[0,1,0,1,0,1,0,1]，T1=[0,1]，T2=[0]。
先 T1⋈T2 花 2 次比较、得到一行；再和 T0 比较 8 次，总计 10。
左深 (T0⋈T1)⋈T2 先花 16 次，得到八行，再花 8 次，总计 24。
最终只有键 0，保留四个 T0 行号组合。
demo 输出树 `(T0 join (T1 join T2))`，随后
`dp=10 exhaustive=10 left-deep=24 actual comparisons=10 rows=4`。

## 源码导读与验证
`src/ordering.hpp` 的 cardinalities 计算子集估计，combine 构造计划和可解释成本，
dynamic_program 枚举分割并保存最优树，enumerate 是不剪枝的独立搜索策略。
execute 真正执行二叉连接并计数，result 将完整行号组合规范化。
`tests/tests.cpp` 对 12 组数据、1–5 表比较所有穷举树、DP、左深答案和最优成本，
覆盖空表、重复键、无匹配、单表、范围检查及不支持语义拒绝。

## 复杂度与局限
DP 的分割枚举 O(3^n)，存储最优根和基数 O(2^n)，共享树节点可能占 O(n·2^n)。
穷举随树数超指数增长，六表上限用于保护教学实验。执行空间与物化中间结果成正比。
单次嵌套连接 O(|L||R|)，不含真实 I/O。行号采用 int，输入规模假设不超过 INT_MAX；
教学内存规模也远小于计数溢出边界。没有 Hash Join、排序性质、并行、索引或笛卡尔积。
不同物理性质不能一般只保留一个最优子计划，本例因成本与性质简化才满足最优子结构。

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

若 macOS 的 AppleClang 找不到标准库头文件，仅在本机配置时追加
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`，
不需要修改源码或安装依赖。测试使用显式异常检查，Release 不会删除断言。
本快照为独立 C++17 教学程序，无运行时分支依赖。总目录见 `git show main:README.md`。
