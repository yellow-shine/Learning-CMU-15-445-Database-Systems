# 36：节点读写 Latch 与乐观 Crabbing

数据库的事务 Lock 管理逻辑冲突及隔离，通常跨语句持有；Latch 保护短时内存结构，不提供事务原子性。本例让多个 C++ 线程访问真实的 B+Tree，使用每节点 shared_mutex，不能把 get/put/erase 组合当事务。前置：33／34 的树平衡、RAII 锁、共享与排他模式。

## 两条下降路径

读者先在 root_gate 共享保护下获得当前根的共享 latch，然后释放 gate。下降先取得孩子共享 latch，再释放父亲，即 latch coupling，叶上完成查找。节点 leaf 标志一经创建不变。

写者先按同样方式共享下降，在叶层改取排他 latch，然后释放父亲。若不会分裂、下溢或改变最小键，就原地完成并释放叶锁，不锁其他叶。因此不同叶写者能真正并行。覆盖已有键总是安全；新键要求叶未满且插入位置非首位；删除要求非首键且删除前记录数大于 2。找不到待删键可直接返回 false。

不安全操作必须先退出当前路径（所有 RAII latch 释放），再以排他 root_gate 进入结构慢路径。慢路径递归保留祖先排他 latch，取得孩子后才修改；删除在父节点排他保护下按数组次序锁孩子与兄弟，借位／合并后释放。根分裂／收缩由 gate 保护。这个保守变体会串行化结构修改，但没有用全局互斥包住所有操作：安全写和读者释放根保护后在各自节点执行。

为保持精确分隔键，安全写不修改子树最小键；重建分隔键时仍共享锁住下探节点，避免并发 vector 移动的数据竞争。父独占阻止新操作进入，已经下降的叶操作先完成。节点用 shared_ptr 保证合并后节点的 mutex 在 RAII 解锁前仍存活。next 沿用单线程树支撑，没有暴露并发范围游标。

## 可复现实验

先插入 0..999，两个线程分别覆盖 100 和 900。在两个叶锁内部的测试 hook 上 rendezvous：两个线程必须同时进入才能退出，若用全局锁冒充实现则十秒内失败。随后五线程屏障同时启动：四个线程操作互不相交键域，另一个不断查找；每个写线程插入、检查并删除偶数键，join 后逐键核对。另有四线程争用相同 100 键的查插删压力测试，最终验证全树并删除至空树。

演示两个线程分别插入 0..99、100..199，join 后输出确定结果：

```text
key42=42 key142=142
```

不要在 leaf_hook 中回调树操作，否则可能自锁；hook 是确定性交错测试接口，不是业务回调机制。validate / height 只可在线程全部静止后调用。

## 源码导读与不变量

`src/bplus_tree.h` 的 try_leaf 是安全性判断和释放祖先的核心；get 展示共享 coupling；insert / erase 沿用 34 的实际递归分裂、借位、合并，并加入节点 latch。父分隔键等于右子树最小键，叶最多 4 条、非根至少 2，内部非根 3..5 个孩子；validate 检查排序、等深、范围、占用及叶链。`tests/index_test.cpp` 的 Barrier 以条件变量而非 sleep 构造交错，CTest 总时限 45 秒。

## 成本、保证与限制

安全操作 O(log n) 路由、每层 latch；慢路径沿用最小键重算，最坏 O(log² n)，树空间 O(n)。结构写串行会成为高分裂率工作负载瓶颈，可升级为释放安全祖先的完整悲观写 crabbing／顶向下预分裂；本例保留所有不安全路径祖先，突出乐观安全叶路径。标准 shared_mutex 不保证公平，不能承诺无饥饿。单次操作由叶排他锁或结构路径保护，但测试不是线性一致性证明，也未运行 ThreadSanitizer。无事务 Lock、持久化、分配失败回滚或并发迭代器。

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
