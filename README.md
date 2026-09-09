# 55 谓词下推：先少读行，再连接

## 问题与前置知识
查询优化器用等价规则减少中间结果，而不是改变用户答案。需要理解选择、内连接、
左外连接和 SQL NULL。本教程实现树形逻辑计划和解释器；不是 SQL 字符串替换。

## 规则与不变量
`Filter(a.v > 10, Join(A,B))` 可以改写为 `Join(Filter(A),B)`。
`Predicate::references` 提供引用集合，优化器检查集合是否完全属于某一个孩子。
名字必须限定且两边不能重名；构造器拒绝不存在的列。改写复制节点，不修改原计划。
执行采用多重集语义，重复键产生全部配对；NULL 比较不为真。结果比较排序后逐行比较，
因此保留重复次数但不要求无 ORDER BY 的物理顺序。

左外连接是保守边界：不推任何谓词。尤其右侧过滤不能简单挪到右输入，
因为连接之后原本被过滤的行会变成补 NULL 行。本例不尝试证明更多合法外连接规则。

## 手工轨迹
A=(1,5),(2,20)，B=(1),(2)。原计划先生成两行再过滤，只剩 (2,20,2)。
改写先把 A 从两行变一行，再连接，仍然一行。demo 输出
`before=1 after=1 left input=1`。
反例 A 包含键 1，过滤 B.key>1：外连接之后过滤不保留键 1，
先过滤 B 再外连接却保留键 1 的 NULL 补齐行。

## 源码导读和测试
`src/plan.hpp` 是本分支附带的小型内存算子解释器；Scan 保存显式 schema，
Filter 处理 NULL，Join 枚举配对，LeftJoin 补空值。其它小算子仅为计划结构支撑。
`src/optimizer.hpp` 是核心合法性与单层改写。`src/demo.cpp` 展示安全路径。
`tests/tests.cpp` 独立覆盖左右引用、重复键、空输入、NULL、非法引用、阈值差分，
以及外连接的真实错误改写。失败抛异常，CTest 返回非零。

## 成本与局限
引用集合检查约 O(c log c)，连接解释器 O(|A||B|)，物化空间与结果大小成正比。
下推减少连接左输入但未必减少扫描读取；这里没有索引或磁盘 I/O。
只支持单列大于整数常量、整数/NULL、单次内连接改写；不支持 OR、函数副作用、
相关子查询、表达式列重命名及完整 SQL 三值表达式树。规则不是生产优化器。

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
