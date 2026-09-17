# 33：B+Tree 插入与递归分裂

数据库索引要在有序键中快速定位记录，且不能让数据页无限长。这里用内存节点模拟固定容量页，真正实现叶节点与内部路由节点；`std::map` 只作为测试答案。前置知识：二分搜索、独占所有权、递归。相关主题：34 删除、35 迭代、36 并发。

## 布局与不变量

键和值都是 int，唯一键，重复 put 是覆盖值。叶子最多四条记录，非根至少两条；内部节点最多五个孩子，非根至少三个。内部键不保存值，`keys[i]` 恰好等于 `children[i+1]` 子树最小键。相等键必须走右侧，因此路由用 upper_bound，而叶中查找用 lower_bound。

所有叶子同深，next 按键序连接叶子且末尾为空。空树保留一个空叶根，高度定义为节点层数（一层而非零层）。根可以低于普通节点最小占用。unique_ptr 管理孩子，next 只是非拥有链接。禁止复制树，移动使用编译器规则；不要使用已移动对象。

## 具体轨迹

按顺序插入 1..5：叶 `[1,2,3,4,5]` 溢出，分成 `[1,2] -> [3,4,5]`，新根分隔键为 3。再插 6、7，右叶分裂，新根变为 `[3,5]`。继续到 13 时根有六个孩子，内部节点按三／三个孩子分裂，再长出新根。插至 15 后 demo 输出：

```text
height=3 key7=70
```

分裂叶子复制右叶第一键到父节点；分裂内部节点重新从孩子最小键构造分隔键，不能把叶子键值数组当内部节点处理。每次插入至多向上传播一次分裂，只有根分裂会增加高度。

## 源码导读

- `src/bplus_tree.h` 的 Node 表达两种节点；route / get 完成路由。
- insert 递归下降并返回新右兄弟；put 处理根提升。
- separators 从孩子计算严格最小分隔键，避免多个更新公式不一致。
- validate 独立递归检查排序、范围、占用率、等深和整个叶链，而非只验证能查到键。
- `tests/index_test.cpp`：固定种子 4000 次 upsert 与 map 差分；升序、降序制造多层分裂，另测空树与 int 极值。每次更新都检查结构。

## 成本与边界

扇出固定为 5，查找 O(log n)，空间 O(n)。本教学版本重算分隔键会向子树底部寻找最小值，插入最坏 O(log² n)，节点内移动最多常数个元素。生产页可缓存最小键或直接传递分隔键，使更新 O(log n)。这里没有磁盘页、WAL、异常分配失败回滚或多线程保障；next 只是为后续叶链主题铺设必要结构。不支持删除，不能把本例的内存访问当作实测磁盘 I/O。

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

C++17，无第三方依赖。测试独立于 demo，以异常使进程非零退出，Release 不会关闭检查。
macOS 若编译器找不到标准头文件，仅配置时增加以下参数（不要写死 SDK 路径）：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"
```

Release 的配置可同样附加该参数。回总目录：`git show main:README.md`。
