# 37：Bloom Filter——先排除，再精确查找

LSM／索引访问之前可以用很小的位数组排除“不存在”的键，减少昂贵查找；返回可能存在时仍须访问真实索引，不能凭过滤器判定成员资格。前置知识是位运算、哈希与概率。这里键为 uint64_t，过滤器不保存值或原始键。

## 算法与适用条件

构造 m 位和 k 个确定性哈希位置，add 只把对应位设为 1；查询发现任一位为 0 就确定不存在，全部为 1 只能回答 maybe。位置由固定种子、键与哈希序号经 64 位混合得到，模 m；无第三方库或依赖实现定义的 std::hash。无符号溢出是定义明确的模 2^64 算术，不是有符号溢出。

无假阴性要求：同一实例／相同哈希参数、添加已完成、没有清位或数据损坏、无未同步并发写。本实现不支持直接删除，erase 被编译期删除；多个键可能共享某个位，清掉一个键的位会误删另一个键的证据。需删除时可以整体重建，或另学有溢出管理的 counting Bloom；本例没有偷偷实现计数版。

## 具体轨迹与概率

一位一哈希的过滤器从 `[0]` 开始，add(42) 变成 `[1]`，查询 42 返回 true；查询未插入的 999 也返回 true。这是确定性碰撞反例。重复 add(42) 不改变状态。

较实用例子：m=100003、k=7、seed=37，插入 0..9999，然后查询 10000..109999，统计实际假阳性。理想独立均匀哈希下近似概率是 `(1-exp(-kn/m))^k`，本参数约 0.0082。源码用 expm1 减少小指数相减误差；这是估算而非每次实验的保证，尤其哈希位置并非数学独立随机变量。demo 输出 inserted42=1、实际 false_positives 计数及 estimate=0.0082；日志保留本机实际计数。

## 源码导读及测试

`src/bloom_filter.h`：position 统一插入／查询的哈希方式，add 进行按位或，maybe_contains 提前返回；构造器拒绝零位、零哈希以及超过 64 的哈希数（教学接口上限）。字节数使用除法加余数，避免 m+7 溢出。

`tests/index_test.cpp` 检查 10000 个插入键全部无假阴性、重复插入、最大 uint64、非整字节 m、空过滤器、非法参数、一位饱和碰撞。固定种子假阳性实验只报告观察值，不写“计数必须等于理论概率”或用脆弱阈值让测试偶发失败。demo 与测试分离，失败返回非零。

## 成本与局限

每次 add／查询最多 O(k)，存储 ceil(m/8) 字节，无随插入数增长的键数组。n 增大但 m 不变时过滤器逐渐饱和；不会自动扩容，扩容必须重新插入所有真实键。普通字节读写无原子性，不可并发 add；无序列化、持久化、密码学抗对抗输入或按公式自动挑参。分配过大的位数组由标准容器抛异常。只能作为精确索引的前置过滤器，不是索引替代品。

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
