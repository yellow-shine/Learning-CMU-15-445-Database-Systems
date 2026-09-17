# 60 统计信息、直方图与基数估计

## 问题与前置知识

优化器尚未执行查询时，需要预测中间行数。需要了解概率、选择率、不同值数 NDV。
这里从真实整数/NULL 数据扫描统计，构建等宽直方图，不把估计当作真实答案。

## 数据结构和假设

Statistics 保存总行数、NULL 数、精确 NDV、可空最小/最大值及半开桶 [lo,hi)。
整数域用 [min,max+1) 表示，边界计算先提升到 long double，避免 INT_MAX+1 溢出。
请求桶数必须正，实际桶数不超过非 NULL 行数。全空或全 NULL 表没有桶。
`less(x)` 在每桶按覆盖宽度插值，再除以总行数；NULL 永不满足普通比较。
`equal(x)` 为 CDF(x+1)-CDF(x)，允许整数单位区间跨越分数桶边界。
它是桶内均匀密度模型，不是准确频率表，甚至原数据没有的值也可能得到正估计。

AND 估计乘积依赖谓词独立性。等值连接使用非 NULL 行数乘积除以两侧较大 NDV，
假设值均匀、较小值域被包含；不相交最小最大范围直接估零。
它不是任意重叠分布的精确估计，也不依赖结果去重。

## 具体轨迹

数据 0..9，五桶 [0,2)、[2,4)、[4,6)、[6,8)、[8,10)，每桶两行。
x<4 覆盖前两桶，选择率 4/10，基数估计 4。
demo 第一行 `rows=10 ndv=10 P(x<4)=0.4 estimate=4`。
若另一列 y=x，x<4 AND y<4 的独立估计是 10×0.4×0.4=1.6，实际 4，
demo 第二行明确同时输出独立估计与实际计数，而不是声称模型精确。
偏斜反例 [0,0,0,9] 的单桶把 x=0 估成 0.1，而真实是 0.75。

## 源码导读与测试

`src/statistics.hpp` 的 analyze 扫描数据与建桶，less/equal 做插值，
conjunction/equijoin 是基数模型。`tests/tests.cpp` 验证空表、全 NULL、常量列、
INT 边界、桶边界、CDF 单调、桶总数守恒、超多桶、非法参数及明确不准的偏斜案例。

## 成本与局限

精确 NDV 用 set，建统计 O(n log NDV + buckets)，额外空间 O(NDV+buckets)。
每个 CDF 查询 O(buckets)，连接估计 O(1)。全部来自内存扫描，无采样或持久化统计。
浮点插值存在舍入误差，测试用容限；输入阈值拒绝 NaN/无穷。
不支持字符串、联合直方图、相关性修正、过期统计和反馈学习。统计只是计划选择依据，
执行器必须仍检查真实谓词，不能依据估计删除数据。

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
