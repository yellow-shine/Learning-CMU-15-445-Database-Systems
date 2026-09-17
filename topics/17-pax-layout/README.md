# 17：PAX 页内分列与 mini-page 边界

## 问题与前置知识

NSM 把整行放在一起，DSM 把整列放在一起。PAX（Partition Attributes Across）
保留页作为分组单位，但在每个页内部按列组织 mini-page：单列扫描连续访问页内属性，
整行重建也只需同一页。前置是 11 固定页、15 NSM 和 16 DSM；此实现独立且仅用 C++17。
固定 schema 与另两节相同，为三个非 NULL uint32 (id,score,age)。

## 实际字节格式与不变量

每页 128 字节，可放 10 行，所有整数显式小端，不依赖 C++ 结构体布局。

| 区间（左闭右开） | 用途 |
| --- | --- |
| [0,4) | P A X 与版本 1 |
| [4,8) | 一字节行数与三个零保留字节 |
| [8,48) | id mini-page，10 个 uint32 |
| [48,88) | score mini-page，10 个 uint32 |
| [88,128) | age mini-page，10 个 uint32 |

column c 的局部行 r 在 8+40*c+4*r，c<3、r<10。最后一个字段占 [124,128)，
不与前列或下一页重叠。最后一页即使不足 10 行，列起点也不变；未使用项必须为零。
除最后页外都满 10 行；没有零行物理页，空表没有页。全局位置 i 对应页 i/10、局部行 i%10。
from_pages 校验魔数、版本、计数、保留位、非末页满载和未使用区，拒绝非规范输入。
固定 array 页类型本身保证长度为 128；这里没有从任意长度文件加载的 API。

## 手工轨迹与预期输出

输入 i=0..11 的 (i,10*i,20+i)，页 0 放 0..9，页 1 放 10..11。
select(90) 仅解码两个页内 score mini-page 的 12 项，返回全局位置 [9,10,11]。
取第二个命中位置 10，从页 1 的三个 mini-page 首项重建整行。

```text
pages=2 counts=10,2 scan reads=12
selected=3 row=10,100,30 row reads=3
mini-page starts=8,48,88
```

页 1 有 8 行空位，但不能将 score 起点前移到 id 的第二项之后，否则固定偏移公式失效。
这正是边界测试同时检查满页末项和部分页空白的原因。

## 源码导读与检查

`src/pax_store.h` 的 offset 是 mini-page 地址公式，构造器按列写真实字节页，
from_pages 验证导入格式，get 在同页重建行，select 跨页扫描 score 并返回全局位置。
`src/demo.cpp` 展示跨页边界。`tests/storage_test.cpp` 用逻辑 Row vector 作 oracle，
覆盖 0、1、9、10、11、20、21、101 行、全部和零命中、UINT32_MAX、每列首尾偏移、
端序、重载相等、非法坐标、坏头、坏行数、非末页不满及三列未用区损坏。
每次扫描计数恰为 N，整行重建恰为 3N，不仅验证 demo 文本。

## 复杂度与取舍

构建 O(N)，数据页空间 128*ceil(N/10)；最后一页的未使用字段空间最多为 108 字节。
get O(1) 解码三个标量且都在一页；select O(N)，选择向量 O(K)。from_pages O(B*128)
时间和 O(B*128) 拷贝空间。页内只扫单列具有连续性，但跨页不能像 DSM 一样形成全局连续列。
与 15 行式算子每行三个解码相比，本扫描每行一个；NSM 本身也允许 stride 投影，
因此这不是通用三倍加速结论。标量读取计数是算法工作量，不是缓存 miss 或磁盘 I/O。

只读内存布局教学，无文件持久化、NULL、变长列、压缩、更新、并发或校验和。
合法区域的位翻转可能成为另一条有效记录，格式校验不等于数据完整性保证。
小页大小方便手算，不用于预测真实数据库的缓存或设备性能。逻辑 id 不必唯一。

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
