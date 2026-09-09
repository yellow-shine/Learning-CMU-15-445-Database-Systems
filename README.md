# 58 LIMIT 下推：前缀不是集合

## 问题与前置知识

少物化一些行很诱人，但 LIMIT 的含义是当前执行序列的前 n 行，不能随便越过算子。
需要理解投影、过滤和 ORDER BY。这里明确使用序列语义，测试直接比较向量，
而不是把结果排序后比较；重复行保留，NULL 作为普通空单元在投影中保持。

## 合法规则

纯列投影既不改变行数也不改变顺序，因此 `Limit(Project(X),n)` 可以变成
`Project(Limit(X,n))`。嵌套 LIMIT 合并为较小值。规则递归穿过连续投影。
Filter、Sort、Join、LeftJoin 都是边界，计划保留原状；没有推测唯一性或排序性质。
原节点不变，复制投影并重建 LIMIT，新的计划由解释器真实执行。

## 轨迹与反例

扫描 x 序列 [3,1,2]，投影 x 后 LIMIT 2 得 [3,1]；先 LIMIT 再投影也是 [3,1]。
排序后 LIMIT 1 得 [1]，先 LIMIT 1 再排序则为 [3]，不能改写。
demo 输出 `safe prefix: 3 1; sorted first=1`。
过滤反例使用 [3,1,2,2]，x>1 后 LIMIT 3 得 [3,2,2]，先取三行则只剩 [3,2]。
连接反例首行键 3 无匹配，后面键 2 有匹配，先裁连接左侧会遗漏答案。

## 源码导读与测试

`src/plan.hpp` 附带内存多算子解释器；Sort 为稳定排序，NULL 排在非 NULL 前。
`src/optimizer.hpp` 包含 `limit` 构造器和 `push_limit` 合法性判断。
`tests/tests.cpp` 覆盖 0、超长 LIMIT、空输入、多层投影、嵌套 LIMIT，
以及过滤、排序、连接的拒绝与实际错误结果。测试和 demo 为独立入口。

## 成本与局限

改写 O(计划链长度)，向量物化复制 O(输入行数)，排序 O(n log n)。
投影宽度 c 时，安全下推可把投影工作从 O(nc) 降为 O(min(n,k)c)。
扫描解释器仍全量物化，因此不是惰性执行器，也不声称减少真实磁盘 I/O。
不支持 OFFSET、WITH TIES、Top-N、DISTINCT、窗口函数和有副作用的表达式。
无 ORDER BY 的真实 SQL 不保证顺序；这里固定扫描输入顺序以便精确演示规则。

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
