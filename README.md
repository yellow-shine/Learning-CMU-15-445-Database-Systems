# 41 排序聚合：把相同键放在一起

## 解决的问题

执行器如何把明细行转换为每组一行的 COUNT/SUM/AVG/MIN/MAX？排序聚合先按分组键排序，再扫描相邻的等键区间。
这是 SQL GROUP BY 的教学子集，不含 SQL 解析器。前置知识：排序、optional、整数溢出、vector。
相关主题 40 外部排序、42 哈希聚合。本分支可独立构建，不依赖其他 checkout；总目录见 `git show main:README.md`。

## 数据与精确语义

`Row` 包含 `optional<int>` 键和 `optional<int64_t>` 值。输入为多重集，重复行不会去重。
所有 NULL 键属于同一组，排序时位于非 NULL 键之前，其他键升序输出。

|函数|规则|
|---|---|
|COUNT(*)|包括 NULL 值的全部行数|
|COUNT(value)|仅非 NULL 值个数|
|SUM/MIN/MAX|忽略 NULL；没有非 NULL 值时为 NULL|
|AVG|非 NULL 的总 SUM / COUNT(value)，返回 long double|

`sort_aggregate({})` 没有组，返回空 vector；`global_aggregate({})` 表示无 GROUP BY 的全局聚合，返回一个状态，两个 COUNT 为 0，其他为 NULL。
全 NULL 组的 SUM 不是 0；含实际数值 0 的组则 SUM=0。聚合状态始终保留这个区别。

## 算法、不变量与轨迹

`sort_aggregate` 拷贝输入，按键 std::sort，然后扫描：键变化就创建新组，否则只更新当前组。
不变量：已输出的组不会再出现；当前 State 精确概括已扫描的当前键行。
排序可以用标准库，因为教学焦点是组边界与聚合状态，不是内排序实现。

示例原始输入：`(2,10),(1,-4),(2,NULL),(1,6),(NULL,3),(2,20),(3,NULL)`。
排序得到键序列 `NULL,1,1,2,2,2,3`（组内顺序不作保证）：

1. NULL 组读到 3：rows=1，count=1，sum=3。
2. 键 1 从 -4 到 6：rows=2，count=2，sum=2，min=-4，max=6。
3. 键 2 的 NULL 仅增加 rows：最后 rows=3，count=2，sum=30，avg=15。
4. 键 3 的唯一值为 NULL：rows=1，count=0，SUM/AVG/MIN/MAX 全为 NULL。

实际 demo 输出：

```text
key count(*) count(v) sum avg min max
NULL 1 1 3 3.000000 3 3
1 2 2 2 1.000000 -4 6
2 3 2 30 15.000000 10 20
3 1 0 NULL NULL NULL NULL
empty global: 0 NULL
```

## 为什么不能平均局部平均值

State 保留 rows、非 NULL count、整数 sum、可空 min/max；`merge` 合并这些充分统计量。
分区 A=[10]，B=[20,30,40,NULL]：局部均值 10 和 30 的简单平均 20 是错误的；合并后 sum=100、count=4，AVG=25。
`add` 把单行转为状态后调用同一个 merge，因此 NULL 和溢出逻辑只实现一次。
COUNT 使用 uint64，SUM 使用 int64，更新之前检查溢出；拒绝的 add/merge 不改变原状态。
即使最终数学和可表示，中间和溢出也会拒绝，故数值极端时重排或分区可能影响是否成功。
正常范围内 SUM 精确，AVG 最后一次转换为 long double，仍有平台相关浮点舍入。

## 源码导读与成本

- `src/aggregation.h`：State::merge 的原子检查、global_aggregate、sort_aggregate 的组边界。
- `src/demo.cpp`：同一输入覆盖负值、NULL 键、NULL 值、全 NULL 组。
- `tests/aggregation_test.cpp`：朴素 O(NG) 独立 oracle，另用 unordered_map 分组交叉验证排序结果。

N 行、G 组：排序 O(N log N)，扫描 O(N)，输入副本 O(N)，输出 O(G)，当前聚合状态 O(1)。
如果上游已保证同键连续，扫描可以直接进行；当前 API 为保持最小接口仍会排序。
本实现完全内存化，不宣称有外部排序的内存边界；大输入应接入主题 40 的元组版本，而不是把本程序当作磁盘算子。

## 构建、测试、预期

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

macOS 若标准头找不到，可为对应构建目录配置添加条件性参数：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

CTest 预期 1/1 通过，独立测试失败返回非零，不依赖 assert。
测试覆盖空分组/全局空输入、重复、负数、全 NULL、NULL 键、零值、固定种子 2000 行及反序、两种分组方法、不同大小分区合并、SUM 两端溢出和 COUNT 自合并溢出（不需要真的存储海量行）。

## 局限

不支持复合键、字符串排序规则、DISTINCT 聚合、HAVING、窗口、DECIMAL、输入浮点 NaN、磁盘 spill 或并发。
一个函数抛异常时不返回部分组；State 自身的失败更新保证不变，但没有外部事务系统。
这不是完整 SQL 标准实现，也不是生产执行引擎。组内顺序未定义，不能用它实现依赖行顺序的聚合。
