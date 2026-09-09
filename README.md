# 13：Slotted Page 与稳定 RID

## 问题与前置知识

变长记录不能用行号乘固定长度定位。槽页在固定大小页中增加一层间接寻址：
RID=(page_id, slot_id)，槽保存记录偏移与长度，移动记录只改槽，不改 RID。
需要了解 11 的固定页与 12 的元组字节格式；本节记录是非空 byte string，不绑定 schema。

## 布局与不变量

```text
0       8              lower         upper                 256
| header | slot0 slot1 ... | free space | payload ... payload |
```

页头 8 字节：魔数 S、版本 1、槽数 u16、lower u16、upper u16。
每个槽 4 字节，包含 offset u16 与 length u16，均小端。
lower=8+4*槽数，lower≤upper≤256；所有存活记录在 [upper,256) 内且互不重叠。
长度零、偏移零表示删除，因此拒绝空记录。删除留下空洞；compact 重建 payload，保留槽号。
**不复用墓碑槽号**，避免旧 RID 指向新记录；因此即使记录全部删除，槽目录也会耗尽空间。

插入先在候选副本上紧缩，再检查 payload+4 字节目录是否能放下。更新在候选副本上删除旧值、
紧缩后放入新值。只有成功才交换页字节，所以页满失败必须保持原页逐字节不变。
get 对非法槽抛异常，对已删除槽返回 nullopt；erase/update 已删除槽抛异常。

## 具体轨迹与输出

插入 cat 得槽 0、偏移 253；插入 elephant 得槽 1、偏移 245。
此时 lower=16，upper=245，连续空闲 229。删除 cat 不立即移动其他数据。
紧缩后 elephant 移到 248，槽 1 仍然有效，连续空闲增加 3。再缩短更新成 ox。

```text
slot=1 value=elephant free=229->232
slot=1 value=ox
```

## 源码导读与测试

`src/page.h` 定义 256 字节页；`src/slotted_page.h` 的 word 显式读写小端，place 从页尾分配，
compact 复制到新页，validate 检查页头、槽边界、墓碑与记录重叠。
`tests/storage_test.cpp` 验证 RID 在紧缩与变长更新后稳定、删除语义、满页失败无损、244 字节
极限记录；400 步确定性操作以 optional 字符串数组为 oracle，每步检查所有记录并重载字节页。
损坏测试覆盖魔数、目录边界、记录越界与重叠，不将非法磁盘数据当作指针使用。

## 成本与局限

get 复制 L 字节，时间/额外空间 O(L)。删除 O(1)。紧缩、插入和更新为 O(P+S)，P=256，
S 为槽数；临时空间 O(P)。validate 用 P 个布尔标记检测重叠，是教学级线性页检查。
没有 overflow page、跨页更新、并发、日志或 generation RID。最多 62 个目录槽，
墓碑永不回收是为了明确 RID 生命周期；生产系统可用 generation 或外部引用规则安全复用。
本节只操作内存页；页字节可序列化，但不声称提供持久化，14 堆文件会附带此组件快照。

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
