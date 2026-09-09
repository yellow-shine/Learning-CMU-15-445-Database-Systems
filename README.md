# 04 · 用模板表达泛型内存表

## 问题与前置知识

销售行和整数页号都需要保存、过滤和归约。复制两套扫描代码容易让边界行为分叉。
本课属于执行器的 C++ 基础，不实现 SQL 解析器。前置是值语义、vector、函数调用和
03 的移动语义；全部源码在本分支，无需检出前置主题。

## 原理与不变量

`Table<Row>` 是类模板，实例化时决定行布局；`select<Predicate>` 和
`sum<Value, Project>` 是成员函数模板，实例化时决定谓词、投影和累加类型。
编译器检查调用是否合法，不需要继承、虚函数或运行时类型标签。
定义放在头文件，使调用方实例化时可见。这里的泛型并不是任意输入都有效：
insert 需要行能放入 vector；select 需要可复制的行、可用 const Row 调用且返回可转 bool
的谓词；sum 要求投影结果可以通过 += 累加到 Value。违反要求是编译期错误。

表使用多重集语义，保存插入顺序，重复行不消除。select 总是完整扫描，返回独立拥有的
行副本，绝不暴露 vector 迭代器；结果修改和后续 insert 不会使旧结果悬空。
谓词和投影只能观察 const 行；若抛异常，局部结果析构，表不变（调用方外部副作用不回滚）。
`std::invoke` 同时支持 lambda 和数据成员指针。sum 的初值明确指定结果类型和空表单位元，
不是偷偷用 int 累加所有类型。

## 手算轨迹与输出

输入依次为 `(east,120), (west,80), (east,120)`，金额单位为分。
过滤 region == east：第一行命中，第二行跳过，第三行命中，保留两条重复行。
归约从 `0LL` 开始，投影 cents：0 → 120 → 200 → 320。
第二个实例 `Table<int>` 保存 7、9，不改变算法，只改变行类型。

```text
east rows: 2
total cents: 320
id sum: 16
```

## 源码导读与测试

- `src/table.h`：类模板保存 vector；两个成员模板实现真实过滤和归约循环。
- `src/demo.cpp`：Sale 和 int 两种实例；lambda 和成员指针两种可调用对象。
- `tests/topic_tests.cpp`：空表、全命中/全不命中、重复、独立结果、调用次数、异常后表不变，
  以及第二种类型的结果。CHECK 抛异常，Release 也不会去掉验证。

测试中的副本改为 0 后，原表仍求和为 320；这是结果拥有权的反例检查，不能只测行数。

## 复杂度与边界

插入摊销 O(1)，扩容 O(n) 次行移动/复制；过滤 O(n) 次谓词、O(k) 个结果行空间；
归约 O(n) 次投影和累加、额外 O(1) 个 Value。成本假设行复制和调用本身 O(1)，
字符串的实际字节复制需另计。表占 O(n) 行，无磁盘 I/O。

这是内存扫描组件，不是索引、事务、持久化或动态 schema。没有 NULL/SQL 三值逻辑。
泛型 sum 遵从 Value 的 += 规则：调用方必须选择足够宽的数值类型并保证不会溢出；
整数测试数据在范围内，不能把此例当作安全财务聚合库。select 不支持不可复制行；
状态谓词的外部副作用与并发写入不受保护。模板还可能增加编译时间和多实例机器码体积。

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

独立测试成功显示 `100% tests passed`，失败非零，超时 20 秒。使用 C++17、无第三方依赖。
若 macOS AppleClang 找不到标准头文件，在两次配置命令中附加
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`；
本机验收使用该 SDK 搜索路径绕行，正常工具链无需添加，不写死进项目。
返回总目录：`git show main:README.md`。
