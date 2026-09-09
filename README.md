# 49-vectorized-execution：批量执行与选择向量

## 问题与前置知识

Volcano 的每行调用开销如何摊薄？本主题在执行层以一批行作为处理单位，
仍执行与 47、48 相同的 Scan → value >= 10 → key 投影。前置是数组索引、游标和向量擦除。
“向量化”在此指批处理接口和紧密循环，不保证编译器生成 SIMD 指令。

## 数据与不变量

Batch.rows 保存本批双列行；selection 是合法行下标的有序子集。Scan 初始化全选向量，
Filter 只删除未匹配下标，不搬动原始 Row；Projection 按选择向量访问 key。
Next 的 false 只表示整个输入耗尽，不表示某批全被过滤；Query 必须继续取下一批。
每次 Next 清空输出，成功时输出非空且不超过批宽。Init 重置扫描和调用统计；引用暴露的 scan 只读。
批宽 0 明确拒绝；尾批可不满。重复行和原输入顺序保留，不实现 NULL。

## 可复算轨迹

输入 `(1,5),(2,20),(3,10),(2,20)`，批宽 2。

| 批 | 初始 selection | 过滤后 | key |
|---|---|---|---|
| 1 | [0,1] | [1] | [2] |
| 2 | [0,1] | [0,1] | [3,2] |
| 3 | 空 / EOF | 不调用过滤 | 无 |

Demo 输出 `batch: 2`、`batch: 3 2`，然后
`scan calls=3 reads=4 filter calls=2 projection calls=2`。
相比逐行版，真实读取数相同，调用粒度不同；相比物化版，首批结果不等待全表。
测试对批宽 1、2、3、99 与固定标量答案比较，覆盖尾批、空表、全拒绝、全拒绝批之后仍有匹配、
重复 EOF、重启和 0 批宽。单独检查选择向量证明 Filter 没有压缩 Row 数组。

## 成本

n 行、批宽 B：扫描批调用 ceil(n/B)+1 次（含 EOF），时间 O(n)，
输入快照外额外 O(min(n,B)) 行、下标和投影空间。选择向量增加间接寻址但避免复制整个 Row。
这里使用 AoS 行批，未采用列向量、掩码寄存器或表达式融合，不能声称模拟工业 SIMD 性能。

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

内存 AoS 批量执行；无 SIMD 保证、列式存储、SQL、NULL 或磁盘。批宽控制单批行数，不是总输入内存预算。
