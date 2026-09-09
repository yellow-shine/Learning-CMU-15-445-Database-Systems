# 56 Projection 下推与列活跃性

## 为什么不能只留下 SELECT 的列

优化器希望减少行宽，但过滤、连接和聚合需要不出现在最终输出中的列。
前置知识是关系投影、多重集、树遍历。这里投影不去重，零列的两行仍然是两行。
整数和 NULL 的内存表使用限定列名，连接双方列名不能冲突。

## 算法与不变量

`prune(plan, needed)` 自顶向下传递集合，再自底向上重建计划。
Filter 把谓词引用加入输入集合；Join 按孩子 schema 分配需要的列，并补上两边连接键；
Aggregate 保留分组键与 SUM 输入，即使最终只输出 sum；Sort 保留排序键。
最后投影隐藏内部保留列，保证节点对父亲提供恰好 needed。
Scan 下放一个真实 Project，减少后续行宽，不声称实现列式磁盘读取。
非法 required 列会抛异常，而不是静默产生 NULL。原计划保持不变。

## 具体轨迹

输入 (a,v,unused)=(1,10,99),(1,20,88)。先过滤 v>5，再 GROUP BY a SUM(v)，
最后只选 sum。最终需要 {sum}，聚合输入需要 {a,v}，过滤仍需要 {a,v}，
扫描输出由三列裁成两列。两行进入分组并输出 30。
demo 输出 `sum=30 scan columns=3 required=2`。
若扫描只保留最终输出列，sum 尚未生成，a/v 都丢失，执行不可能正确。
测试明确验证这种不安全裁剪被构造器拒绝。

## 源码和验证路径

`src/plan.hpp` 为本分支附带的内存计划解释器；新增 Aggregate 实现按键 SUM，
全 NULL 组 SUM 为 NULL，空输入没有组。`src/optimizer.hpp` 是集合活跃性递归。
`tests/tests.cpp` 验证 join/filter 依赖、重复键乘法、aggregate 依赖、只输出分组键、
空表、零列、非法裁剪和排序键保留。demo 独立于测试。

## 复杂度与边界

计划遍历 O(nodes *columns log columns)；物化投影 O(rows* columns log columns)。
连接使用嵌套循环 O(nm)，分组使用有序容器 O(n log groups)，空间包括中间结果。
整数 SUM 假设不溢出 int；教学数据很小，不适用于大数生产聚合。
只支持具名列、SUM、单列分组，无 DISTINCT、HAVING、表达式别名、子查询或 SQL 解析。
规则保留所有聚合输入，没有消除未用聚合；更多优化应先补充语义证明。

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
