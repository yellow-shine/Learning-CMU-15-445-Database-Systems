# 39：IVF 粗聚类、倒排列表与召回率

精确向量搜索要读 N 个向量。IVF 先按粗聚类把向量分到倒排列表，查询仅访问靠近查询点的 nprobe 个列表，减少距离计算；漏掉的列表可能含真近邻，速度换召回率。本分支独立附带 38 的 ExactIndex／距离计算作为答案，不依赖其他 checkout。前置：平方 L2、均值、Top-K 与精确排序。

## 实际建索引流程

1. 校验维度与有限坐标；要求 N>0、1<=nlist<=N、iterations>0。
2. 用 seed=39 的 mt19937 打乱记录 id，取前 nlist 条作为初始中心。
3. 每轮对全部记录找到最近中心，等距选较小中心编号；按在线均值计算新中心。
4. 空簇保留上轮中心并计数，不除零。运行固定轮数（默认 12），不声称达到全局最优。
5. **最后一次更新中心后重新分配**，将每条 id 存进且仅存进一个列表。

search 计算查询到所有中心的平方 L2，选最近 nprobe 个列表，将这些列表记录的真实距离重新计算，再按 `(distance,id)` 取 Top-K。列表不是按记录关键词检索的全文倒排表，而是“粗中心 → 向量 id”的分桶。未做量化，候选内部排名仍是精确浮点距离。

## 手算与实验轨迹

若两中心收敛到 (0,0)、(10,0)，数据有 (-1,0)、(1,0)、(9,0)、(11,0)，前两点入左列表、后两点入右列表。查询 (4.9,0)，nprobe=1 只看左边两条；k=3 时最多返回两条，真实第三近邻 (9,0) 被裁掉。nprobe=2 则恢复全部四候选并与精确 Top-3 相同。

demo 以固定种子生成 500 个二维点，训练 12 个中心，查询 (50,50)，依次报告 nprobe=1/4/12 的候选数与 recall@10。最后一行必为：

```text
nprobe=12 candidates=500 recall@10=1.00
```

前两行是实际聚类的观测，不承诺任何非全探测的最低召回率。recall@k = 返回结果与精确 Top-K 的 id 交集数 / 精确结果数；空目标按 1 定义。全探测等价的原因不是“聚类足够好”，而是每个记录恰好属于一个列表、距离与并列规则完全相同。

## 源码导读和测试

`src/ivf_index.h` 构造器执行训练与最后分配；nearest_center 是确定的等距策略；search 显示候选裁剪及 candidates 统计；empty_updates 暴露空簇实际处理次数。索引构造后只读，无增删接口，记录 id 等于输入位置。

`src/vector_search.h` 是随本主题复制的精确支撑，保留平方 L2 和余弦基线，但 IVF 本身**只支持平方 L2**，不能把普通均值训练宣称为余弦 IVF。

`tests/index_test.cpp` 验证每条 id 恰好出现一次且属于最终最近中心，同 seed 重建一致；80 个查询分别探测 1/4/全部 12 个列表，比较 recall 与候选数，全探测逐 Hit 等于精确搜索（包括 k>N）。四条相同向量、四个中心制造三个空簇，三轮应有 9 次空簇保留；检查所有同距离结果按 id 排序。另测非法参数、单记录、坏维度、非有限查询与 k=0。`tests/exact_test.cpp` 单独保留精确距离与全排序 oracle。没有把随机召回值硬编码成概率保证。

## 成本与边界

训练 O(iterations × N × nlist × d)，存储 O(Nd+nlist×d+N)。查询粗打分 O(nlist×d)，候选打分 O(Cd)，选择平均 O(nlist+C)，结果排序 O(nprobe log nprobe+k log k)，临时内存 O(nlist+C)。nprobe 越大，嵌套候选集合越大，固定并列规则下 recall 不下降，但不是线性提高。

nprobe=0 或超过 nlist 拒绝；k=0 返回空但仍校验查询与 nprobe，k 超过候选数则返回全部候选。空数据无法训练而抛异常。有限输入的距离／均值仍可能算术溢出并抛 overflow_error。无增量训练、压缩 PQ、持久化、并发更新或磁盘列表；不同标准库的 shuffle 细节可能给出不同初始中心，因此固定种子只承诺同工具链可复现，全部列表等价保证不受此影响。

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
