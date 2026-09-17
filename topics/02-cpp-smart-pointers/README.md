# 02 · 页与执行计划中的智能指针

## 问题与前置知识

一个页由谁释放？多个执行器读同一个对象时，最后一个读者何时离开？
计划树的孩子需要访问父节点，是否意味着孩子也拥有父节点？这些都是所有权问题，
不是“把所有指针改成 shared_ptr”就能解决。前置为 01 的 RAII、堆/栈生命周期与引用。
03 将进一步实现自己的可移动资源；本课直接使用标准库智能指针。

## 三种所有权及不变量

- `unique_ptr<Page>`：唯一强所有者，不可复制，move 转移后源指针为空。
- `shared_ptr<Page>`：复制增加强计数；最后一个强所有者释放对象。
- `weak_ptr<Page>`：只观察控制块，不维持对象存活。`lock()` 一次性取得临时强所有权，
  失败返回空指针。不要先检查 expired 再解引用裸指针，那不是并发安全的获取协议。

`Page::lifetime` 与 `PlanNode::lifetime` 持有计数对象，保证计数器比被观察对象活得久。
构造/析构计数是教学观测数据，不是线程安全监控器。
裸指针 `get()` 仅借用，释放最后一个强所有者后变成悬空指针；我们不再读取它，
也不通过重新构造 shared_ptr 装入相同裸地址，否则产生两个控制块和双重释放。

计划树的强边向下，弱边向上：`filter --shared--> scan --weak--> filter`。
只有向下边保持树形时此模型才无强环；结构体是教学数据模型，不提供通用图编辑验证器。

## 具体轨迹与输出

页 7 首先由 unique_ptr 持有，移动保持对象地址；再转成 shared_ptr。
复制一个 reader 时强计数从 1 到 2，reader 离开回到 1；reset 最后一个拥有者后析构一次。
weak 不会阻止析构。随后构造 filter/scan，外部继续持有 scan，释放 filter 后父弱引用失效。

```text
borrowed page: 7
strong owners: 2
expired: 1, destroyed: 1
scan parent expired: 1
plan nodes destroyed: 2
```

反例不是执行未定义行为：测试让两个 CycleNode 相互持有 shared_ptr，释放外部拥有者后
析构数仍为 0。通过预先保存的 weak 探针锁定对象，手动断开两条强边后析构数为 2。
因此反例不会给测试留下真正的泄漏。weak_parent 模型则根本不产生这条强环。

## 源码与验证导读

`src/ownership.h` 定义真实页对象与计划节点，拒绝空计数器，并删除对象复制以保持计数语义。
`src/demo.cpp` 展示独占到共享的转移、借用、读者作用域和弱父边。
`tests/topic_tests.cpp` 独立验证移动后地址不变、空源、临时 lock 增加强计数、
析构恰好一次、weak 失效、弱父边释放和强环的保留/拆解。没有解引用任何悬空地址。

## 成本、选择与局限

unique_ptr 释放/移动 O(1)，通常只存一个地址；共享复制/销毁需更新控制块，O(1)，
但有原子计数成本。make_shared 通常合并分配；weak 活着时控制块可能仍然占空间。
树整体空间 O(n)，释放子树 O(n)，极深树的递归释放可能耗尽调用栈。
共享指针只保障控制块操作，不保护 Page 内容；本课计数器与图编辑只在单线程使用。
没有实现缓冲池 pin/unpin、垃圾收集、弱缓存淘汰或并发对象访问协议。
选择规则是先确定拥有关系，独占够用就不用共享，反向导航优先弱引用。

## 构建、运行与验证

本机 AppleClang 默认缺少 libc++ 搜索路径，验收配置额外传入
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`。
这是本机 SDK 配置绕行，正常工具链不需要；未修改系统安装。

无第三方依赖，使用 C++17、CMake 和 CTest。两种配置均运行独立测试，
测试使用抛异常的 `CHECK`，不是会被 `NDEBUG` 删除的 `assert`。

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

CTest 成功应显示 `100% tests passed`；失败退出非零，超时为 20 秒。
每个主题从共同文档基线独立建立，不需要检出其它主题来运行。
返回完整主题目录：`git show main:README.md`（目录验收状态以 main 为准）。
