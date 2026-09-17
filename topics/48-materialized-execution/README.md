# 48-materialized-execution：完整物化执行

## 问题与前置知识

执行层不一定逐行拉取。若每个算子一次返回完整数组，代码简单，但首行延迟和中间内存会怎样？
前置是 vector、值语义和关系选择／投影。与 47、49 使用相同查询：双列行中过滤 value >= 10，再输出 key。
重复行保留，无 NULL，顺序保留输入顺序。

## 算法与不变量

Scan 复制表到 scanned；Filter 遍历完整 scanned 建立 filtered；Projection 遍历完整 filtered 建立 projected。
只有前一算子完成后下一算子才开始。MaterializedQuery::Init 完成全部计算，Next 只是结果游标，
不向输入发起调用。必须先 Init；重新 Init 会重算并重置统计和游标，重复 EOF 不改变输出。
为观察内存，本例刻意保留三个中间数组；工业实现可以在最后一次使用后释放数组。
这不是把逐行接口改名：即使调用者只读一行，Init 仍会扫描全部输入。

## 手工轨迹与预期输出

输入 `(1,5),(2,20),(3,10),(2,20)`。

1. Scan 完成时 scanned 有 4 行，尚无任何投影结果。
2. Filter 完成时 filtered 有 `(2,20),(3,10),(2,20)`。
3. Projection 完成时 projected 为 `2,3,2`，此时才可调用 Next。

Demo 第一行 `2 3 2`；第二行 `operator calls=1,1,1 reads=4 intermediate bytes=68`
（常见 sizeof(int)=4、sizeof(Row)=8 平台）。源码按 sizeof 计算，不假定所有平台都是 68。
单次完整算子调用不意味着只处理一行：reads 仍是 4。测试检查这些不同计数、空表、全过滤、
重复 EOF 与重启；空间检查使用 sizeof 而非硬编码字节数。

## 复杂度与内存口径

n 条输入、k 条匹配：时间 O(n+k)，保留的中间逻辑载荷 n*sizeof(Row)+k*sizeof(Row)+k*sizeof(int)。
另有输入快照 O(n)，容器元数据、capacity 空闲以及重新赋值时短暂同时存在的旧新数组不计入展示值，
所以 intermediate_bytes 不是进程峰值内存。Next 为 O(1)，但首条结果之前已经付出 O(n) 工作。

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

完整内存物化，无预算、溢写或页 I/O；保留所有阶段便于教学，展示逻辑载荷而非 allocator/RSS 峰值。仅整数 >= 过滤及 key 投影。
