# 54-window-functions：排名与显式 ROWS 累计窗口

## 问题与前置知识

GROUP BY 会合并行，窗口函数则保留每一行，同时给出分区内位置与累计值。
本主题位于执行层，前置为排序、分区和整数聚合。实现明确的函数子集，不是完整 SQL 窗口解释器。

## 支持的语义与不变量

输入为 `(partition,order,amount)`，按 partition ASC、order ASC 稳定排序；输出也采用这一排序，
input_position 记录原始位置。重复行保留，无 NULL。相同 order 是 peer，并列时稳定保留输入位置，
用于确定 ROW_NUMBER 和 ROWS 的逐行先后；输入位置不是 RANK 的额外排序键，不会拆散 peer。
支持 ROW_NUMBER、RANK、DENSE_RANK，以及显式
`ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW` 的 SUM。

调用者必须传 Frame，不能把 SQL 默认 RANGE 隐式当成 ROWS。Range、Groups、其它起止边界均抛 invalid_argument，
包括空输入时；不存在字符串解析、隐式 frame 或 frame 排除语法。排名函数不依赖 frame，
本 API 把四项一并计算，因此整次调用统一拒绝不支持的 SUM frame。

算法先排序，再线性走过输出：遇到新分区清零 number/rank/dense/sum；每行 number 加一；
遇到新的 order，rank=number、dense 加一，否则保留二者。SUM 每次增加当前 amount，
先做有符号溢出检查，失败抛 overflow_error，不返回部分结果且不修改输入。

## 具体轨迹与 demo 输出

输入顺序刻意打乱：`(2,2,7),(1,20,5),(1,10,3),(1,20,-2),(2,1,4),(1,30,1)`。
排序和计算后（demo 第一行为列名）：

```text
partition order amount row_number rank dense_rank sum
1 10 3 1 1 1 3
1 20 5 2 2 2 8
1 20 -2 3 2 2 6
1 30 1 4 4 3 7
2 1 4 1 1 1 4
2 2 7 2 2 2 11
```

并列 order=20 的 rank 都为 2，下一组 rank 跳到 4，dense_rank 只到 3。
两条 peer 的 SUM 分别为 8 和 6，这是 ROWS，不是 RANGE 的 peer 整组累计；输入稳定次序因此很重要。
partition=2 所有状态重新起算，负 amount 允许使累计值下降。

## 验证与成本

测试固定上述每项差异，另用 40 行乱序多分区数据，通过 O(n²) “计数更小键、相同键之前行、不同较小键”
参考算法逐行核对全部函数。还覆盖单行、空输入、非法 frame、正负溢出以及 INT64_MIN + INT64_MAX。

排序通常 O(n log n)，标准 stable_sort 低内存回退可为 O(n log² n) 比较；排序后计算 O(n)，
结果与排序额外内存 O(n)。这是完整物化算子，有首行阻塞延迟；不提供磁盘溢写或在线排序。
SQL 未指定 peer 次序时 ROW_NUMBER/ROWS 可能不确定，本例的稳定输入次序是明确的教学约定，不是 SQL 的普遍保证。

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

只支持整数分区/升序单键、ROW_NUMBER/RANK/DENSE_RANK 与显式累计 ROWS SUM；无 NULL、SQL 解析、RANGE/GROUPS、滑动 frame、降序或溢写。稳定输入次序确定 peer 内 ROWS 顺序，溢出明确拒绝。
