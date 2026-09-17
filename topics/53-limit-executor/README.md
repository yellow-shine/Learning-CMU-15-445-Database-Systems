# 53-limit-executor：LIMIT/OFFSET 与提前终止

## 问题与前置知识

分页的 LIMIT/OFFSET 不该先拉取整个结果再切片，也不应在达到上限后为了探测 EOF 多消费一行。
这是执行层的拉取算子；前置为 47 的 Init/Next 协议和无符号整数边界。本分支带最小 Source/Scan，
思路来自逐行执行主题但源码独立，不依赖其它 checkout。

## 状态机与不变量

Limit 保存 child 引用、limit、offset、skipped、emitted 和 done。必须保证子节点生存期覆盖父节点。
Init 递归重置子节点及本地状态，不读取数据。Next：

1. done 或 emitted==limit 时立即 false，尤其 LIMIT 0 不执行 OFFSET 跳过。
2. skipped < offset 时逐行丢弃；若子节点 false，立刻锁存 done。
3. 恰好拉一行作为结果，成功后 emitted 加一；不预取下一条。

skipped <= offset、emitted <= limit。实现不计算 offset+limit，所以 SIZE_MAX 边界不会溢出；
大 offset 的循环仍会在实际 EOF 停止，不会空转到那个巨大数字。false 不修改调用者输出，重复 false 不再访问子节点。
API 接收 size_t 非负计数；没有 SQL 文本解析，负数不是接口支持的输入域，不要把有符号负值隐式转换后传入。

## 具体轨迹与预期输出

Scan 为 `[0,1,2,3,4,5]`，LIMIT 2 OFFSET 2。
第一次 Next 丢弃 0、1 并返回 2，共读 3 行；第二次返回 3，共读 4 行；第三次直接 false。
Demo 输出 `2 3`，随后 `reads=4 calls=4`。Scan 没有收到 EOF 探测，也没有读到 4。
若 OFFSET 100，则真实读 6 行加一次 EOF 探测；若 LIMIT 0 OFFSET 100，则读 0 行、调用 0 次。
LIMIT 不创建顺序：本例保留输入顺序和重复值；生产 SQL 想稳定分页需显式 ORDER BY，没有 NULL 处理。

## 复杂度与验证

输入 n，offset=o，limit=l；l=0 时 O(1)，否则消费 min(n,o+l) 行（数学表达式，代码不做可能溢出的加法）。
额外空间 O(1)，输入 Scan 快照另算 O(n)。如果孩子是排序等阻塞算子，第一次拉取仍可能触发孩子读取全表，
本算子只能保证“不多调用直接孩子”，不能穿透 pipeline breaker 省掉排序。
测试穷举 n<8、offset<10、limit<10，对照切片答案并精确检查调用数；另测 SIZE_MAX、空输入、
重复耗尽、重新 Init、LIMIT 0 和嵌套 LIMIT 的不预取属性。

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

非负 size_t API、内存整数流，无 SQL/NULL/WITH TIES。提前终止只约束对子节点的拉取，不保证阻塞子节点内部也少读。
