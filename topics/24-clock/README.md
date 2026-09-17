# 24：Clock —— 用引用位给页面第二次机会

## 解决的问题

LRU 需要记录访问顺序。Clock 用环形 Frame 数组、一个指针和每项一位引用信息近似近期使用，属于缓冲池替换层。前置知识：数组、环形索引、pin/unpin；相关主题为 22 缓冲池和 23 LRU。本分支独立实现策略，不依赖磁盘或其他分支。

## 原理与不变量

每项包含 present、referenced、evictable。access 设置存在位与引用位，但不会自动解除 pin 保护。首次登记不可淘汰，调用者必须显式允许替换。

淘汰从 hand 开始逐个检查，检查后 hand 总前进到下一槽：

1. 空槽或不可淘汰槽跳过；本变体保留 pinned 槽的引用位。
2. 可淘汰但引用位为 1，清为 0，给一次机会。
3. 可淘汰且引用位为 0，清空项并返回 Frame ID。

单线程下最多两圈足够：第一圈会清掉所有候选的引用位，第二圈必定找到一个；无候选则返回空。容量零不进入循环，不会模零。淘汰后旧项的引用和保护状态不会遗留给新页。

## 手工轨迹

容量 3，hand=0，依次访问 `0,1,2,0`，全部可淘汰，引用位为 `[1,1,1]`。

|步骤|检查行为|引用位|hand|
|---|---|---|---|
|evict 第一圈|0、1、2 各给一次机会|000|0|
|evict 第二圈|淘汰 0|—00|1|
|access(1)|再次引用 1|—10|1|
|evict|清 1，淘汰 2|—0—|0|

demo 输出两行：`victim: 0`、`after access(1): 2`。与同轨迹 LRU 的第一个 victim=1 不同：Clock 不是精确 LRU，位无法表达多次访问或准确时间先后。

## 源码、测试与成本

先读 `src/clock.h` 的 Entry，再读 access、evict 的双层有界扫描；`src/demo.cpp` 对应上表。`tests/tests.cpp` 验证指定轨迹、保护条目的引用位保留、全保护空结果、空容量、单项、越界及多次复用后指针仍可选择 victim。

access 与保护设置 O(1)，淘汰最坏 O(F)，最多 2F 次检查；空间 O(F)，不记录时间戳或访问链表。未知 Frame 的保护设置和越界 ID 抛异常，不偷偷创建候选。

## 限制

仅单线程，不实现真实页 I/O，也不自行维护 pin count；缓冲池负责把 pin 状态翻译为 evictable。长扫描依然可能冲掉热点；频繁引用只能保证下一次扫描的一次机会，不能保证页面永久驻留。不同教材是否清 pinned 位、起始 hand 的约定可能不同，本教程以明确轨迹固定该变体。

## 构建与验证

仅需 C++17、CMake 和标准线程库，无第三方依赖。回到总目录：`git show main:README.md`。

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

若 macOS 的 AppleClang 报标准头文件找不到，在两条配置命令中各附加：
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`。
这是本机 SDK 搜索路径排错，不是源码依赖。CTest 失败抛异常返回非零，Release 不依赖 assert；测试超时 20 秒。
