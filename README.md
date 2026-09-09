# CMU 15-445 数据库内核：独立教学实现

依据 [`spec.md`](spec.md) 拆解数据库内核知识，用 **C++17 独立教学实现**解释其机制。课程介绍中的重复内容与 BusTub P0–P4 项目内容已去重；本仓库不依赖 BusTub，也不是任何单一学期的官方项目答案。

## 已确认的组织方式

- 一个可独立学习、验证的算法或机制，对应一个 `topic/NN-name` Git 分支。
- 各分支独立构建，包含中文讲解、可运行实现、演示和测试；不需要切换其他分支取代码。
- 紧密关联的概念合并讲解，例如 Page ID、Slot ID、RID；具有独立机制的算法分别实现。
- `main` 保留本目录、原始资料和设计文档，不累积全部知识点实现。
- 实现目标为“教学级机制完整”，不是生产级数据库；简化、适用范围和不支持的能力必须写清楚。

设计与验收细节见 [`docs/superpowers/specs/2026-09-09-database-topics-design.md`](docs/superpowers/specs/2026-09-09-database-topics-design.md)。

## 当前状态

**82 个知识点已规划；0 个实现完成；尚未创建知识点分支。**

表中的分支名是计划名称，不代表分支已经存在。只有实现、讲解及测试验收通过并提交后，才标记为“已验证”。

## 知识点总目录

### 1. C++ 基础

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/01-cpp-raii` | 对象生命周期、RAII、资源释放；文件资源守卫 | 已规划 |
| `topic/02-cpp-smart-pointers` | unique_ptr、shared_ptr、weak_ptr、所有权与循环引用 | 已规划 |
| `topic/03-cpp-move-semantics` | 左值／右值、移动构造、移动赋值；可移动缓冲区 | 已规划 |
| `topic/04-cpp-templates` | 模板、泛型；简单的泛型数据库组件 | 已规划 |
| `topic/05-cpp-stl` | 容器、迭代器、算法、迭代器失效；数据库场景示例 | 已规划 |
| `topic/06-cpp-threading` | 线程、互斥锁、条件变量、读写锁；生产者／消费者队列 | 已规划 |

### 2. 关系模型与 SQL 逻辑层

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/07-relational-model` | Relation、Tuple、Attribute、Schema；类型化关系与元组 | 已规划 |
| `topic/08-relational-constraints` | 主键、外键、非空、检查约束；约束验证 | 已规划 |
| `topic/09-relational-algebra` | 选择、投影、并、交、差、笛卡尔积、连接；集合语义关系代数 | 已规划 |
| `topic/10-sql-planning` | SQL、Parser、Binder、逻辑计划、物理计划；限定语法范围的查询转换 | 已规划 |

### 3. 存储引擎

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/11-disk-page-io` | 磁盘导向数据库、文件、固定大小 Page、Page ID；页读写 | 已规划 |
| `topic/12-tuple-layout` | 元组序列化、定长／变长字段、NULL 位图、元数据 | 已规划 |
| `topic/13-slotted-page` | 页头、Slot、RID、空闲空间；页内增删改查与整理 | 已规划 |
| `topic/14-heap-file` | 堆表、跨页存储、RID 定位、堆扫描 | 已规划 |
| `topic/15-row-store` | NSM 行存布局、整行访问 | 已规划 |
| `topic/16-column-store` | DSM 列存布局、列扫描、晚物化基础 | 已规划 |
| `topic/17-pax-layout` | PAX 页内分列、行列混合布局的取舍 | 已规划 |

### 4. 数据压缩

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/18-rle` | 游程编码、解码、适用数据分布 | 已规划 |
| `topic/19-bit-packing` | 位宽计算、整数打包与解包 | 已规划 |
| `topic/20-dictionary-encoding` | 字典构建、编码、解码、编码上的等值过滤 | 已规划 |
| `topic/21-delta-encoding` | 差分编码、还原、边界处理 | 已规划 |

### 5. Buffer Pool

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/22-buffer-pool` | Page 与 Frame、页表、Pin／Unpin、Dirty、Flush、Eviction | 已规划 |
| `topic/23-lru` | LRU 替换策略、访问轨迹实验 | 已规划 |
| `topic/24-clock` | Clock 替换策略、引用位 | 已规划 |
| `topic/25-lru-k` | LRU-K、访问历史、淘汰选择 | 已规划 |
| `topic/26-disk-scheduler` | 异步磁盘请求、后台线程、完成通知、错误传播 | 已规划 |
| `topic/27-page-guard` | RAII 页守卫、Pin 生命周期、读写保护 | 已规划 |

### 6. 哈希表

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/28-linear-probing` | 静态哈希、开放寻址、冲突探测、删除标记 | 已规划 |
| `topic/29-robin-hood-hashing` | 探测距离、交换规则、删除处理 | 已规划 |
| `topic/30-cuckoo-hashing` | 多候选位置、驱逐链、重建 | 已规划 |
| `topic/31-extendible-hashing` | 动态哈希、目录、全局／局部深度、桶分裂 | 已规划 |
| `topic/32-hash-index` | Key → RID 索引、重复键、等值查找 | 已规划 |

### 7. 树索引、过滤器与向量索引

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/33-bplus-tree-insert` | 叶子／内部节点、扇出、树高、查找、插入、递归分裂 | 已规划 |
| `topic/34-bplus-tree-delete` | 删除、下溢、借位、合并、根收缩 | 已规划 |
| `topic/35-bplus-tree-iterator` | 叶链、范围查询、迭代器 | 已规划 |
| `topic/36-concurrent-bplus-tree` | Lock 与 Latch 区别、读写 latch、Latch Coupling／Crabbing | 已规划 |
| `topic/37-bloom-filter` | 概率过滤器、假阳性、无假阴性的适用条件 | 已规划 |
| `topic/38-vector-search` | 距离度量、精确 Top-K 检索；近似检索的正确性基线 | 已规划 |
| `topic/39-ivf-index` | 简化 IVF、向量索引、候选裁剪、召回率取舍 | 已规划 |

### 8. 排序与聚合

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/40-external-merge-sort` | 有界内存、生成有序段、K 路归并、磁盘 I/O | 已规划 |
| `topic/41-sort-aggregation` | 排序分组、COUNT／SUM／AVG／MIN／MAX | 已规划 |
| `topic/42-hash-aggregation` | 哈希分组、聚合状态、结果生成 | 已规划 |

### 9. Join 算法

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/43-nested-loop-join` | 嵌套循环连接、比较次数 | 已规划 |
| `topic/44-index-nested-loop-join` | 使用索引探测内表 | 已规划 |
| `topic/45-sort-merge-join` | 排序归并连接、重复键匹配 | 已规划 |
| `topic/46-hash-join` | Build／Probe、重复键、构建侧选择 | 已规划 |

### 10. 查询执行引擎

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/47-volcano-execution` | Iterator／Volcano、Init／Next、Scan → Filter → Projection | 已规划 |
| `topic/48-materialized-execution` | 算子完整物化中间结果、内存开销 | 已规划 |
| `topic/49-vectorized-execution` | 批量数据、批量算子、选择向量 | 已规划 |
| `topic/50-pipelines` | Pipeline、Pipeline Breaker、算子执行边界 | 已规划 |
| `topic/51-access-executors` | SeqScan、IndexScan、访问路径对比 | 已规划 |
| `topic/52-modification-executors` | Insert、Update、Delete、表／索引维护 | 已规划 |
| `topic/53-limit-executor` | LIMIT、OFFSET、提前终止 | 已规划 |
| `topic/54-window-functions` | 分区、排序、窗口；排名和累计聚合的明确子集 | 已规划 |

### 11. 查询优化器

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/55-predicate-pushdown` | 谓词下推、引用列分析、不能下推的情况 | 已规划 |
| `topic/56-projection-pushdown` | 列裁剪、保留后续算子所需列 | 已规划 |
| `topic/57-aggregation-pushdown` | 局部／最终聚合、AVG 分解、安全改写条件 | 已规划 |
| `topic/58-limit-pushdown` | LIMIT 下推合法条件、错误改写反例 | 已规划 |
| `topic/59-physical-plan-selection` | 逻辑／物理算子选择、等值连接转换为 Hash Join | 已规划 |
| `topic/60-statistics-estimation` | 统计信息、直方图、选择率、基数估计 | 已规划 |
| `topic/61-cost-model` | I/O 与 CPU 成本模型、估计值和实测计数对比 | 已规划 |
| `topic/62-join-ordering` | 连接顺序枚举、动态规划、计划成本比较 | 已规划 |

### 12. 事务与并发控制

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/63-transaction-acid` | 事务状态、提交／中止、转账；区分内存回滚与持久性保证 | 已规划 |
| `topic/64-conflict-serializability` | Schedule、冲突、优先图、环检测 | 已规划 |
| `topic/65-two-phase-locking` | S／X 锁、兼容矩阵、升级、2PL、Strict 2PL | 已规划 |
| `topic/66-deadlocks` | 等待图、死锁检测、牺牲者选择、中止释放 | 已规划 |
| `topic/67-timestamp-ordering` | 读／写时间戳、顺序检查、事务中止 | 已规划 |
| `topic/68-optimistic-concurrency-control` | Read／Validate／Write、读写集、冲突验证 | 已规划 |
| `topic/69-mvcc-versioning` | 时间戳、Undo Log、版本链、历史元组重建 | 已规划 |
| `topic/70-snapshot-isolation` | 快照可见性、写写冲突、提交检查 | 已规划 |
| `topic/71-isolation-anomalies` | 隔离级别、脏读、不可重复读、幻读、丢失更新、写偏斜 | 已规划 |
| `topic/72-serializable-mvcc` | 多版本上的保守串行化验证、阻止写偏斜 | 已规划 |

### 13. WAL 与恢复

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/73-write-ahead-logging` | 日志记录、LSN、日志先行规则、提交持久化 | 已规划 |
| `topic/74-buffer-recovery-policies` | Steal／No-Steal、Force／No-Force、Undo／Redo 需求 | 已规划 |
| `topic/75-checkpointing` | 检查点、事务表、脏页表、恢复起点 | 已规划 |
| `topic/76-redo-undo` | 已提交事务重做、未提交事务撤销、重复恢复 | 已规划 |
| `topic/77-aries-recovery` | 教学子集的 Analysis／Redo／Undo、PageLSN、CLR、恢复中再次崩溃 | 已规划 |

### 14. 分布式与并行数据库入门

| 分支 | 讲解与实现 | 状态 |
| --- | --- | --- |
| `topic/78-partitioning` | 哈希／范围分区、路由、数据倾斜 | 已规划 |
| `topic/79-replication` | 主从复制、同步／异步确认、复制滞后、故障边界 | 已规划 |
| `topic/80-distributed-transactions` | 两阶段提交、跨节点事务、持久化决策、阻塞问题 | 已规划 |
| `topic/81-distributed-query` | 数据交换、广播／重分区连接、局部聚合 | 已规划 |
| `topic/82-parallel-execution` | 分区并行、工作分配、结果合并、OLTP／OLAP 工作负载取舍 | 已规划 |

Bloom Filter、IVF 和两阶段提交是为原始资料中的“过滤器”“向量索引”“分布式事务”选择的具体教学算法，并非原文指定的算法。

## 建议学习顺序

```text
C++ 基础 → 关系模型与 SQL
                 ↓
存储布局 → 压缩 → 缓冲池
                 ↓
哈希表 → B+Tree → 并发索引 → 过滤器与向量索引
                 ↓
排序／聚合／Join → 执行模型 → 查询优化
                 ↓
事务基础 → 并发控制 → MVCC 与隔离
                 ↓
WAL → 检查点 → 恢复
                 ↓
分布式与并行数据库入门
```

编号是稳定的目录标识，不是严格的依赖顺序。例如学习 Buffer Pool 前可先读替换策略，学习并发 B+Tree 前应掌握线程同步。每个知识点的 README 会列出精确前置知识。

## 每个知识点的使用方式

以下是实现分支须提供的统一命令约定；当前 `main` 只有目录与设计，不能执行这些构建命令。

```sh
# 仅对总目录中已标记为“已验证”的分支执行
# git switch topic/NN-name
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/demo
```

测试必须在 Debug 与 Release 构建下都有效，不得依赖会被 NDEBUG 删除的断言作为唯一正确性检查。较复杂知识点可以提供额外命令，但应保留上述基本入口。

## 原始资料与适用范围

- 原始 `spec.md` 保留不变，作为需求来源。
- 原文引用 Fall 2025、Spring 2026 与 Fall 2026，不构成统一的课程版本锁定。
- Page 大小等具体参数由对应实现说明，不将某个 BusTub 版本的参数说成通用规定。
- 概念联系 PostgreSQL、DuckDB、TiDB、Milvus 等系统，不表示这些系统使用完全相同的实现。
- 不声称生产可用、不提供官方课程评分保证，也不以进程崩溃测试代替真实断电安全证明。
