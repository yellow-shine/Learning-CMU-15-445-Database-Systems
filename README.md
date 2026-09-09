# 16：DSM 列存、选择向量与晚物化

## 问题与前置知识

分析查询常常只读少数列。DSM（Decomposition Storage Model）把每个属性连续保存，
扫描 score 不必解码 id 与 age。本节位于物理布局和执行算子的交界；前置是数组索引、
15 NSM 的整行布局。固定 schema 与 15、17 相同：三个非 NULL uint32 (id,score,age)。
本分支独立实现，没有运行时依赖，不用给行存类改名来冒充列存。

## 真实结构与不变量

```text
column[0]: id0    id1    id2 ...
column[1]: score0 score1 score2 ...
column[2]: age0   age1   age2 ...
```

三个独立 vector<uint32_t>，每列连续，同一位置对应同一逻辑行。构造成功后列长始终相等，
接口只读，调用者不能破坏对齐。逻辑 id 允许重复，与行位置不是一回事。
select 仅扫描 score 列，返回合格位置列表（选择向量）；project_ids 只对这些位置读取 id。
直到需要输出列时才进行访问，是晚物化的最小例子。get 可以从三列重建整行。
位置列表允许重复和乱序，输出遵循列表；全部位置先验证，非法输入不增加计数。

## 手工轨迹与输出

输入 (1,10,20)、(2,90,30)、(3,80,40)，实际 score 数组为 [10,90,80]。
select(80) 读取三个 score，选择向量 [1,2]；再取 id 列的第 1、2 项，age 列完全不访问。

```text
rows=3 selected=2 scan reads=3
ids=2,3 total reads=5
```

与 15 的整行算子九次标量读取相比，本例只需五次，代价是保留选择向量。
这不是证明所有 NSM 算子都需要九次读取：行存也能按 stride 投影，只是列存具有连续访问优势。

## 源码导读与验证

`src/column_store.h` 构造三个独立列，select 产生位置，project_ids 按位置取另一列，
get 重建行；column 提供只读列用于物理布局检查。`src/demo.cpp` 展示选择与晚物化。
`tests/storage_test.cpp` 用 Row vector 作 oracle，逐列核对 102 行及整行重建，覆盖全选、
无普通行命中、极大阈值、重复 id、乱序与重复选择位置、空表、非法列号及 SIZE_MAX 位置。
每个阈值核对精确读取次数，检查输出与逻辑过滤相同，Release 不删除检查。

## 复杂度和局限

构造 O(N) 时间，原始字段占 12N 字节（不计 vector 容量与对象开销）。select O(N) 时间、
O(K) 选择向量；project_ids O(K) 时间和输出空间；get O(1)，但访问三个分离内存区域。
少列扫描局部性好，整行查询和更新多列则有聚合成本。计数是源码算法中的标量读取，
不是缓存 miss、设备 I/O 或实际 benchmark，未测量 CPU 推测执行与编译优化。
这里只读内存快照，不支持压缩、NULL、变长列、持久化、并发、更新删除或 SQL 解析。
原生 uint32 数组不构成跨端序文件格式，不能直接据此声称磁盘可移植。

## 构建与验证

无需第三方库；C++17 与 CMake 3.16 即可。测试程序使用抛异常的 CHECK，
不会因 Release 的 NDEBUG 消失；CTest 设置 20 秒超时。独立 demo 不代替测试。

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

本分支是独立快照，不需要检出任何相关分支。知识点总目录见 `main:README.md`
（例如 `git show main:README.md`），不要把实现分支合并回目录分支。

### macOS 工具链排障

若默认 AppleClang 报 `<vector>` / `<array>` 不存在，使用已安装 SDK 的 libc++ 头文件，
无需安装依赖或修改 CMake。对 Debug 与 Release **都**传入：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

然后执行上述 build、CTest 和 demo。本机验收采用此参数；编辑器未加载编译配置时的
标准头文件缺失诊断属于同一工具链问题，不代表已经通过静态分析。
