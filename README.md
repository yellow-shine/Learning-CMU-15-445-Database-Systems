# 59 从逻辑连接选择物理算法

## 问题与前置知识

逻辑 Join 指定匹配关系，物理 Join 决定怎样找匹配。前置知识是内连接、哈希表、
嵌套循环和 NULL。不能看到 Join 就替换成 Hash Join，必须检查真正的连接条件。
本例使用枚举表示 Equal/Less/Greater，不依赖字符串猜测。

## 选择和执行

`select_physical` 递归复制计划，对内连接的 Equal 条件选择 HashJoin。
非等值保留嵌套循环，外连接也保守保留；两者都有真正可执行的路径。
HashJoin 构建右输入的 unordered_multimap，再逐个左键 equal_range 探测。
重复键的每一个 build 行都输出，不覆盖、不去重。NULL 不插入也不探测。
哈希执行器自己再次检查 Equal，错误的物理计划会被拒绝。
结果遵循多重集语义；哈希表遍历不保证顺序，等价检查比较排序后的完整多重集。

## 手算轨迹

A=[1,2,2]，B=[2,2,3]。等值连接键 2 的两个左行各匹配两个右行，输出 4 行。
小于连接：键 1 匹配三行，每个键 2 匹配键 3，共 5 行。
demo 输出 `HashJoin rows=4`、`NestedLoop rows=5`。
若把小于强行解释为等值，(1,2) 这一对会从真变假，测试展示答案差异。

## 源码导读与测试

`src/plan.hpp` 附带独立计划结构、schema 校验和两种连接解释器，支持具名整数/NULL 行。
`src/optimizer.hpp` 仅负责算法匹配和递归选择，不实现基数估算或成本排序。
`tests/tests.cpp` 在多种输入大小上比较物理与朴素路径，包含重复键、NULL、空表、
三种比较、外连接、Filter 包裹、错误等值替换与 HashJoin 拒绝非等值。

## 成本与局限

嵌套循环 O(nm)，Hash Join 平均 O(n+m+输出)，哈希最坏退化仍可达 O(nm)。
哈希构建空间 O(m)，所有执行器物化结果，输出本身可能平方增长。
这里固定右侧 build，不比较内存预算、磁盘 spill、索引或 Merge Join。
选择依据是合法性和教学默认偏好，不宣称 Hash Join 总更快；小输入可能相反。
不支持复合键、残余谓词、排序保证和生产级成本优化。

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
