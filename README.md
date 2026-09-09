# 35：B+Tree 叶链、游标与范围扫描

索引不仅服务单点查询，也服务 `WHERE key >= low AND key < high ORDER BY key`。对每条结果从根查起会重复付出树高成本。本例一次定位下界，然后沿叶链扫描；独立携带 33／34 的插入、删除与结构校验源码。前置：B+Tree 分隔键、半开区间与指针生命周期。

## 游标契约

Iterator 保存非拥有的叶指针及槽位，解引用返回键值副本，递增走下一个槽；页尾切到 next。end 是空指针与零槽位，空树 begin==end。解引用或递增 end 抛 out_of_range。游标不是完整 STL ForwardIterator（没有后缀递增与 iterator_traits），只提供显式循环所需的最小接口，不应声称所有标准算法均适用。

所有 put / erase 都使既有游标失效，哪怕只是覆盖值或删除不存在的键也按此保守契约处理；销毁或移动树亦失效。必须在修改后重新取得游标，不能使用悬挂指针。无并发扫描或快照隔离保证。

叶最多 4 条、非根至少 2 条；内部最多 5 个孩子、非根至少 3 个，分隔键等于右子树最小键，叶子等深。叶链必须与 DFS 叶序一一对应，尾指针为空。删除合并释放右叶前修复 next。

## 一次跨叶轨迹

插入 1..15 得到叶 `[1,2] -> [3,4] -> [5,6] -> [7,8] -> ...`。删除 7 会借位或合并，但扫描调用者不依赖具体页边界。range(5,10) 从 lower_bound(5) 开始，依次输出 5、6、8、9，到键 10 停止，不包含上界。demo 的实际输出：

```text
5:50 6:60 8:80 9:90
```

lower_bound 落在叶尾时必须切到下一叶，不能把当前叶末尾误当整棵树的 end。low==high 或 low>high 返回空结果；超出最大键的下界直接 end。区间用比较而非 high+1，避免整数溢出；range 的 int 上界无法表示 INT_MAX+1，若要包含最大整数可从 lower_bound 一直迭代到 end。

## 源码导读与验证

`src/bplus_tree.h` 的 Iterator 构造函数统一规范化叶尾；begin 定位最左叶，lower_bound 先按内部 upper_bound 路由，再叶内二分；range 仅用游标，绝不重复调用 get。其余平衡逻辑来自删除主题，是本例独立运行的支撑。

`tests/index_test.cpp` 保留查插删 map 差分和每次结构校验；额外对 101 个偶数键枚举 211 个下界及多种上界（包含反向区间），与 map::lower_bound 扫描比较。覆盖空树、end 错误、缺失键边界、超过最大键、跨多叶；再删掉一半键后整链扫描，验证合并后的叶链。检查代码在 Release 中同样运行。

## 成本与局限

下界定位 O(log n)，游标递增 O(1)，输出 r 条为 O(log n+r)，range 额外保存 O(r) 副本；直接游标循环只需 O(1) 附加空间。树 O(n) 内存，更新沿用重建分隔键的 O(log² n) 教学实现。没有页缓存、I/O、WAL、事务或并发保证；内存分配失败不承诺回滚。本例讲清叶链扫描而非完整工业迭代器库。

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
