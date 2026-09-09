# 40 外部归并排序：让磁盘承担中间结果

## 问题与前置知识

数据库的 ORDER BY、排序连接与排序聚合都可能面对比内存大的输入。
本教程只排序定长 int64 记录，保留重复值；理解 vector、文件流、RAII 和小根堆即可。
相关主题为 41 排序聚合、42 哈希聚合；本分支不依赖它们。总目录可用 `git show main:README.md` 查看。

## 算法与不变量

`external_sort(input, workspace, B, K)` 要求 `2 <= K <= B`。
B 是生成阶段的**记录槽数预算**，不是整个进程的字节上限；K 是归并扇入。
输入按二进制流读取，每次至多 B 条，用 std::sort 排成一个真实磁盘 run。
这不是把完整输入交给 std::sort：生成缓冲在归并开始前释放。

每轮最多打开 K 个 run 与一个输出，堆只保存每个非空输入的最小未消费记录及来源编号。
弹出最小值，写出，再只从该来源补一个值。于是堆顶总是所有未输出记录的最小值。
每组输出保持有序且多重集不变；归并轮数重复到只有一个 run。
旧组在新组成功关闭后删除。用 `(轮号, 段号)` 计算文件名，内存里不保存随 N 增长的文件清单。
空输入也生成一个空文件，因此调用者不需要特殊路径约定。

## 手算轨迹与 demo

输入 `9 -1 5 5 2 8 0 -3 7 4`，B=3，K=2：

|阶段|磁盘有序段|
|---|---|
|生成|[-1,5,9] [2,5,8] [-3,0,7] [4]|
|第 1 轮|[-1,2,5,5,8,9] [-3,0,4,7]|
|第 2 轮|[-3,-1,0,2,4,5,5,7,8,9]|

第一组最初堆中为 (-1,0)、(2,1)，输出 -1 后从段 0 补 5，下一次输出 2。
demo 实际输出：

```text
-3 -1 0 2 4 5 5 7 8 9
runs=4 passes=2 peak_records=3 open_files=3
```

指标统计算法记录槽和显式文件流数，不包括堆弹出时常数个局部变量、流对象内部缓冲、库及操作系统缓存。
堆元素还有来源索引，K 个流各有实现定义的缓冲；总算法内存 O(B+K)，与总输入 N 无关。
所以不能把 B=3 宣称为只用 24 字节内存；若需要精确页预算，应替换 iostream 为显式页缓冲。

## 源码导读

- `src/external_sort.h`：Workspace 独占临时目录；read/write 检查短记录及 I/O 错误；external_sort 的生成循环与多轮堆归并是核心。
- `src/demo.cpp`：另建输入目录与排序目录，展示结果与统计。
- `tests/external_sort_test.cpp`：独立非 assert 检查，与 std::sort 比较所有记录；固定种子 1003 条、多种预算/扇入确保多轮合并。

测试还覆盖空输入、单记录、全重复、负数、int64 两端、非法预算、缺失输入、损坏尾记录、坏读写流、非空工作目录拒绝、成功/异常后的目录清理以及原输入保留。

## 成本

设 R=ceil(N/B)，归并轮数 P=ceil(log_K R)，空文件例外计一个 run。
生成 CPU 为 O(N log B)，归并为 O(N log K × P)，内存 O(B+K)，最多 K+1 个数据文件同时打开。
生成及每轮均读写全部 N 条，逻辑数据传输约 `2N(1+P)` 条；实际块 I/O 取决于流缓冲与文件系统。
磁盘同时保留尚未删除的旧 run 和已生成的新 run，临时数据上界约 2N 条，不计目录元数据。

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

若 macOS Command Line Tools 找不到 `<vector>` 等标准头，仅在配置时附加（两种构建都适用）：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

CTest 预期 1/1 通过，失败返回非零，Release 不会消除检查。

## 生命周期、错误与局限

返回文件属于传入 Workspace，仅在其存活期间可读；Workspace 必须为空，且不可复制。
目录通过 create_directory 原子地新建，绝不接管已有目录；析构仅清理自有目录。
输入永不覆盖。失败抛异常，调用者应销毁该 Workspace 后重试；本教程不提供事务式用户输出替换。
析构无法报告清理错误（权限变化等可能残留目录），不保证 kill -9 后自动清理。
检查写入和 close，但没有 fsync，不声称断电持久化。
二进制文件是本机 int64 表示，不跨字节序，不带校验和；可检测残缺记录但不能检测完整记录的位翻转。
不支持可变长元组、稳定排序、并行归并、磁盘满故障注入或恶意并发篡改工作目录。
