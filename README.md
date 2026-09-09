# 61 可解释成本模型：预测值与执行计数

## 问题和前置知识
合法计划可能有不同代价，优化器需要比较。前置知识是页、嵌套循环、Hash Join 和基数估计。
本教程不是秒表微基准：模型单位是逻辑页访问及算子工作次数，权重可调。
所有数据在内存，页访问由按页容量的真实循环计数，不能称为实际磁盘读次数。

## 模型与执行约定
设输入行数 n,m，每页容纳 c 行，P(x)=ceil(x/c)。嵌套循环左侧扫描一次，
每个左行重新扫描右表，页数 P(n)+nP(m)，比较次数 nm。
Hash Join 右侧 build、左侧 probe，页数 P(n)+P(m)，操作次数 n+m。
两者每输出一行增加一次输出成本。总成本为 pages×io+(operations+outputs)×cpu。
默认 io=10、cpu=1，非有限或负权重拒绝；页容量必须正。
Hash 操作把一次容器插入/查找视为一个单位，不计算桶冲突和分配细节。

`choose` 生成含算法、页容量、预测计数的真实 Plan，`execute` 执行它并独立计数。
不拿预测值填实测值。右侧重复键保留所有配对，无 NULL，结果为多重集。
空左表时嵌套循环不扫描右表，但哈希仍 build 右表；公式与执行必须一致。

## 手工轨迹
A=B=[1,2,3,4]，页容量 2，输出 4 行。
嵌套循环页数 2+4×2=10，操作 16，成本 100+16+4=120。
哈希页数 2+2=4，操作 8，成本 40+8+4=52。
demo 两行分别输出 `nested pages=10 ops=16 output=4 estimated cost=120 measured cost=120`
和 `hash pages=4 ops=8 output=4 estimated cost=52 measured cost=52`。
只有一行对一行时，两者同样页数，嵌套一次比较比哈希两次操作便宜。
错误基数反例：三个键 1 对三个键 1 输出 9 而不是 3，预测输出成本低估。

## 源码与测试
`src/cost.hpp` 的 estimate 是公式，execute 的 read/emit 在循环中记录计数，
choose 比较代价并在平局选择 NestedLoop。`tests/tests.cpp` 穷举 0–7 行、三种页容量，
比较精确页数/操作/输出计数，比较两种连接答案，并验证小大输入选择、偏斜估计及非法参数。

## 复杂度与局限
估计与选择 O(1)；执行嵌套 O(nm)，哈希平均 O(n+m+输出)，空间 O(m+输出)。
不模拟缓存、预取、随机 I/O、spill 或操作系统；循环计数是实验模型内的实测。
输出估计由调用方提供，本主题不重复实现直方图；不声称估计总是准确。
权重不是硬件校准结果，更不能用 52/120 预测真实加速倍数。

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
