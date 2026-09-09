# 34：B+Tree 删除、借位与根收缩

删除不是在叶子数组中 erase 就结束：页面占用率下降会破坏高度界限，失效的分隔键会把未来查询送错子树。本分支独立附带 33 的查找与递归分裂实现，重点是维护删除后的结构。前置：二分路由、B+Tree 分裂、unique_ptr 所有权。后续 35 讨论叶链遍历。

## 节点与算法

唯一 int 键映射到 int 值，put 覆盖重复键。叶容量 4，非根最少 2；内部最多 5 个孩子，非根最少 3。父键必须精确等于右孩子子树最小键，所有叶等深、有序、不重叠且叶链完整。空树是一个叶根，高度 1。

先递归删除。没有找到键则返回 false，不调整结构。孩子下溢时依次尝试：左兄弟借一条、右兄弟借一条、与左兄弟合并、没有左兄弟则与右兄弟合并。内部节点移动的是孩子所有权，不是叶键值。每层结束重建分隔键，因此删除恰好位于分隔键的记录也不会保留旧最小值。父节点因合并少了一个孩子时递归向上传播；根只剩一个孩子则将孩子提升为根。

## 手算轨迹

插入 1..5 后：根 `[3]`，叶 `[1,2] -> [3,4,5]`。删除 1，左叶只剩 `[2]`，右叶有 3 条，借右边的 3，得到 `[2,3] -> [4,5]`，根更新为 `[4]`。再删 2，左右都无法借，合成 `[3,4,5]`，原根失去一个孩子并收缩为该叶。合并时必须将 left.next 改成 right.next 后再释放 right，否则产生悬挂指针。

反向借位由左边富余叶提供最大键，插入右边最前端；内部合并同理拼接孩子，重新推导分隔键。demo 插入 1..15 后依次删除，实际输出：

```text
before=3 after=1 missing=1
```

## 源码与测试

`src/bplus_tree.h`：erase(Node*, key) 递归下溢处理；move_last_to_front / move_first_to_back 分开表达借位方向；merge 维护叶链或内部孩子；公开 erase 处理根收缩。validate 从结构本身推导所有范围与叶深，再比对链，而不是相信父键正确。rebalance_counts 记录实际路径以防随机测试没有触发某个分支。

`tests/index_test.cpp` 保留插入 oracle，增加固定种子 16000 次混合查插删与 std::map 差分，每次比较所有 400 个候选键并检查不变量。强制要求左右借位、左右合并、内部递归合并、根收缩计数均非零；升序与降序删除各 700 条，最终为空。覆盖不存在键、重复覆盖、整数极值、单叶和多层树。测试不使用会随 NDEBUG 消失的断言。

## 成本及教学边界

查询 O(log n)，空间 O(n)。节点容量固定；因每层 separators 下探寻找最小键，更新最坏 O(log² n)，不是生产版最优界。可在页中缓存最小键或递归返回新边界来消除额外下探。这里模拟内存页，无持久化、WAL、并发或分配失败的强异常保证。删除会释放节点；不能保留裸节点指针。标准容器只是页内数组和测试 oracle，没有代替树算法。

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
