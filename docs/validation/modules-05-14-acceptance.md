# 模块 5–14 验收

独立审查工作流 `404de29f-b615-4966-a348-dded4872c05d` 对 22–82 给出最终 pass。P1 均已修复并复审；剩余 P2 不阻塞验收，记录于文末。实现仍在各 `topic/` 分支，未合并进 `main`。

## 精确提交

| 分支 | 已验证提交 |
| --- | --- |
| `topic/22-buffer-pool` | `6bcf49bb6502cc4319aa9cfc6592db2f3026d336` |
| `topic/23-lru` | `92c83846fd0451ed45216933f3a4477c83ac84cb` |
| `topic/24-clock` | `6006eb0fa285b1a70aa4f82c117324911a2603af` |
| `topic/25-lru-k` | `8ec02864ec6662bd4aaa5437c81acc408895cb49` |
| `topic/26-disk-scheduler` | `c06e5fe6d13ff6bf39bdf5d934b4da34d75e0ddb` |
| `topic/27-page-guard` | `f13dd82e00bd2905543f93bef78c928da49259d3` |
| `topic/28-linear-probing` | `03e15f3f1ff27a9039d16024bb91fedb1d04e48c` |
| `topic/29-robin-hood-hashing` | `2c1484640f6153cf0facbd817d328d58b907dac8` |
| `topic/30-cuckoo-hashing` | `ff4787a6a02e3ac53e728aa9bc2fcffde285cdda` |
| `topic/31-extendible-hashing` | `4608fe83b84fdda64ed6e52e1a26a0df5cb80c30` |
| `topic/32-hash-index` | `488fda1e58e9a2ce76c57c536f78daefb5d447a9` |
| `topic/33-bplus-tree-insert` | `928ce88c3e972745a312cc94cdf27e95d7cdc9bd` |
| `topic/34-bplus-tree-delete` | `1c3f48f6eb4d276f4875fa621c457f239ad09176` |
| `topic/35-bplus-tree-iterator` | `5f039309effb85c72eef4e64b790551c6a6c0c1a` |
| `topic/36-concurrent-bplus-tree` | `34874ab5f0346279b7de64140fc17d0ddb4299d7` |
| `topic/37-bloom-filter` | `616d3e9b2e06abe57b8d80726400acfda0957168` |
| `topic/38-vector-search` | `46fa10df5f869dd42bc9972c705f53cae85705a5` |
| `topic/39-ivf-index` | `bc01a78d042bbe212b20e4d5b4fe8af6e7efbb57` |
| `topic/40-external-merge-sort` | `79eec85371f97b7b2b8d1a132aca98a23bcc8290` |
| `topic/41-sort-aggregation` | `76e74af2ad3141816761818d92d0da3e09633bf5` |
| `topic/42-hash-aggregation` | `c9a31d10781e93543c347f5fbe8d6575172b5088` |
| `topic/43-nested-loop-join` | `93a7a56a756e943cd85341de65ccf4f9f00f27a6` |
| `topic/44-index-nested-loop-join` | `3b4e8a887561d1d3c1da1cb09075e0d29df2e38c` |
| `topic/45-sort-merge-join` | `32fc6599f8f7459366b3a5f4a0df82504cdf3a74` |
| `topic/46-hash-join` | `740e4ffb2886e17d298af495cfd1af085021b5e7` |
| `topic/47-volcano-execution` | `04592d6d6a0708904e570d9039505f03b00e2988` |
| `topic/48-materialized-execution` | `6259e87cd71763e5034d61cac6a30ba3404aebd0` |
| `topic/49-vectorized-execution` | `01202c7a43edd322e20916e83ce80705b8671f76` |
| `topic/50-pipelines` | `fed8923fc84ebef8de6aeb05ccab6d42ad1c2498` |
| `topic/51-access-executors` | `cb36dea4fe506d85704871e94504404ad06b0b07` |
| `topic/52-modification-executors` | `b7464035dce27d26724f4b16da98c4f8d3f78241` |
| `topic/53-limit-executor` | `3e9d46d76347280564db7cc9253f6848d79bc5cd` |
| `topic/54-window-functions` | `3a423e2695a3f20dc3b85f136b401930bdf5424a` |
| `topic/55-predicate-pushdown` | `b88c4678d9d76bd3e10dd041254dd6a949e5fe7e` |
| `topic/56-projection-pushdown` | `3436da6dbabc932f219e95fae0f8cb2e3979937c` |
| `topic/57-aggregation-pushdown` | `1529618adef869a001c5ff2c1252aaa2fefed233` |
| `topic/58-limit-pushdown` | `da981c24c6b4f3412b88f0de8c925904b0526682` |
| `topic/59-physical-plan-selection` | `01b2601ab6c67b57dc6c73bd59cef6716a7ed4cf` |
| `topic/60-statistics-estimation` | `27e5231179f89945c9d0ef0c4d0d7d435a1a193f` |
| `topic/61-cost-model` | `1bb60f7a2b93f64c28c1a52dc1a2c2aed7004c19` |
| `topic/62-join-ordering` | `46e98927fdf050e017f78306b9c16bffeec1d1c9` |
| `topic/63-transaction-acid` | `3cb3e7c4af492ae95eed9677ccc016c023e15d82` |
| `topic/64-conflict-serializability` | `042d37126d29d49990cbc16cf4589f49e3dbefa9` |
| `topic/65-two-phase-locking` | `5440aad1b5fbde1bb36fd1c993e8aec5b58eccbd` |
| `topic/66-deadlocks` | `fd394a90f7e224aa2108985a445300203ff314bf` |
| `topic/67-timestamp-ordering` | `2f7dde2fd3564f5eec833b4d2e50a66bf87baaa7` |
| `topic/68-optimistic-concurrency-control` | `c782d8575a30883da0f356746ddf1ce5a9f4f585` |
| `topic/69-mvcc-versioning` | `721bd2cd11cd92682b2b322b8c6f8f4d31a57c52` |
| `topic/70-snapshot-isolation` | `8c1a8536c51b0c2248b3f84e00913d95642fdd9b` |
| `topic/71-isolation-anomalies` | `df7dda17cee11d2b339a42bdd6dc068677f932c0` |
| `topic/72-serializable-mvcc` | `7926bd7d4829a6b0c2639021b873bb0741e3910d` |
| `topic/73-write-ahead-logging` | `9df1c50d1b4799a569085d6960f562124eae4811` |
| `topic/74-buffer-recovery-policies` | `e1d0b728208c58e7ece3488799aeb58c0f75074e` |
| `topic/75-checkpointing` | `d8d8794aa01690b2839af36ba609b955c5b88df3` |
| `topic/76-redo-undo` | `8ea7fe3ebca9fe39810f80ac356d1289961a199d` |
| `topic/77-aries-recovery` | `682111ca39c6d1cff8d542f2b5620a5e77803738` |
| `topic/78-partitioning` | `38231f4bf3709f53dd450528ea007d447811b83e` |
| `topic/79-replication` | `50c6abfd69cb9da4040aa5fc5fe9039f0a81628e` |
| `topic/80-distributed-transactions` | `c9868c546271040d3997c52fbf0c14d48e22bc86` |
| `topic/81-distributed-query` | `78fb14905dac161cf86cd2e810dfeccdb20f9198` |
| `topic/82-parallel-execution` | `4a58188399c18c0d6b84e65d4fd8224c00348505` |

## 已关闭的 P1

- **27**：`flush` 持元数据锁等待页 latch，与嵌套 Guard 死锁。修复为 pinned/busy 立即拒绝。隔离复现原 SHA 挂起，修复后不再阻塞。
- **59**：LIMIT 下把等值 Join 改成 Hash Join，前缀截断不是包等价。修复为保留整个 LIMIT 子树。
- **63**：Bank 隐式拷贝／赋值复制 `busy_` 并允许重叠事务。修复为删除拷贝／移动。

另：**16** 列存计数器别名越界已在模块 3–4 记录中关闭。

## 未阻塞的 P2

- **39**：README 手算把 `(9,0)` 写成第三近邻，精确距离顺序是第二。代码排序正确。
- **73／75／76／77**：公共 `child` 助手在 `body()` 返回后才 `_exit`，基线崩溃矩阵会先跑析构。75 检查点 hook 与 76／77 恢复 hook 仍是存活期中断。隔离追踪已确认；把 `_exit` 放进存活 `Store` 后 73 矩阵仍通过。
- **82**：拷贝失败测试在启动任何 worker 之前抛出，没有覆盖“已启动线程后的清理”。实现看起来正确，缺回归深度。

## 使用

```sh
git switch topic/NN-name
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/demo
```

本机若缺 C++ 标准库头文件，按根目录 README 的 SDK `-isystem` workaround 配置 Debug 与 Release。不把本机路径写入项目 CMake。结论只覆盖表中 SHA。
