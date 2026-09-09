CMU **15-445/645: Intro to Database Systems** 不是“教你怎么写 SQL”的数据库课，而是偏 **Database Internals / DBMS Implementation**：

> **一条 SQL 从进入数据库，到读取磁盘、走索引、执行 Join、并发事务、写 WAL、宕机恢复，数据库内部到底发生了什么？**

CMU 官方对课程目标的描述也基本如此：实现一个 disk-oriented DBMS，理解索引、查询执行、查询优化、并发控制、恢复以及分布式数据库架构。当前 Fall 2026 课程依然沿着这条主线展开。([CMU 15-445/645][1])

---

# 一张图先理解 15-445

可以把整门课理解成：

```text
                SQL
                 │
                 ▼
        ┌─────────────────┐
        │ Parser / Binder │
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │ Query Optimizer │
        │   查询优化器     │
        └────────┬────────┘
                 │
           Execution Plan
                 │
                 ▼
        ┌─────────────────┐
        │ Query Executor  │
        │ Scan/Join/Agg   │
        │ Sort/Filter     │
        └────────┬────────┘
                 │
        ┌────────┴────────┐
        ▼                 ▼
      Table             Index
                         B+Tree
        │                 │
        └────────┬────────┘
                 ▼
        ┌─────────────────┐
        │ Buffer Pool     │
        │ Page / Frame    │
        └────────┬────────┘
                 ▼
             SSD / Disk


同时还有两条“横切”整个系统：

        Transactions / MVCC / Locks
                   │
                   ▼
        WAL / Logging / Recovery
```

所以你之前问过的：

* page / slot / frame
* buffer pool
* B+Tree
* aggregation pushdown
* query optimizer
* MVCC
* transaction isolation
* WAL
* concurrency control

基本全部在 **15-445 的核心范围**里。

---

# 1. Relational Model & Relational Algebra

课程一开始不会马上讲存储，而是先回答：

> **数据库到底在表示什么？Query 本质上是什么？**

主要内容包括：

```text
Relation
Tuple
Attribute
Schema
Primary Key
Foreign Key
Constraint
```

以及非常重要的：

## Relational Algebra

包括：

```text
Selection    σ   WHERE
Projection   π   SELECT columns
Union        ∪
Intersection ∩
Difference   -
Join         ⋈
Cartesian Product ×
```

例如：

```sql
SELECT name
FROM users
WHERE age > 18;
```

抽象成：

```text
π_name (
    σ_age>18(users)
)
```

为什么数据库工程师要学这个？

因为 Optimizer 操作的本质并不是 SQL 字符串，而是：

```text
Logical Operators
```

例如：

```text
Filter
  |
Scan
```

可以做：

```text
Filter Pushdown
```

因此你前面问的 **aggregation pushdown / predicate pushdown**，理论根基就在 relational algebra。

Fall 2026 第一讲就是 **Relational Model & Algebra**。([CMU 15-445/645][1])

---

# 2. SQL 与数据库逻辑层

第二部分是 Modern SQL。

但重点并不是：

> 怎么背 SELECT 语法。

而是理解 SQL 怎么映射到数据库内部。

例如：

```sql
SELECT dept, COUNT(*)
FROM employee
WHERE salary > 10000
GROUP BY dept;
```

可能形成：

```text
Projection
     │
Aggregation
     │
  Filter
     │
 SeqScan
```

你会逐渐开始从数据库内核角度理解：

```text
SQL
 ↓
Logical Plan
 ↓
Physical Plan
 ↓
Operators
```

Fall 2026 第二讲就是 Modern SQL。([CMU 15-445/645][1])

---

# 3. Database Storage

这是 15-445 真正开始进入数据库内核的地方。

首先学习：

> 数据到底怎么放到磁盘上？

---

## 3.1 Disk-Oriented DBMS

15-445 主要研究：

```text
Disk-Oriented Database
```

即：

```text
Database size >> Memory size
```

因此：

```text
Disk
 ↓
Page
 ↓
Memory
 ↓
CPU
```

成为基本模型。

官方课程和 BusTub Project #1 都明确以 disk-oriented architecture 为核心。([CMU 15-445/645][2])

---

# 4. Page / Tuple / Slot

这是你之前问 BusTub page/frame 时接触到的内容。

磁盘通常不是：

```text
read(tuple)
```

而是：

```text
read(page)
```

例如 BusTub：

```text
Page = 8 KB
```

官方 Project #1 就使用 8KB page。([CMU 15-445/645][2])

一页可能长成：

```text
┌────────────────────────────┐
│ Page Header                │
├────────────────────────────┤
│ Slot 0 → Tuple A           │
│ Slot 1 → Tuple B           │
│ Slot 2 → Tuple C           │
├────────────────────────────┤
│ Free Space                 │
├────────────────────────────┤
│ Tuple C                    │
│ Tuple B                    │
│ Tuple A                    │
└────────────────────────────┘
```

这里会涉及：

```text
Page ID
Slot ID
RID = (Page ID, Slot ID)

Slotted Page
Tuple Layout
Variable-length data
Metadata
```

你最近问过的：

> page / slot / frame 到底有什么区别？

在这里会系统讲清楚。

---

# 5. Storage Models

接下来非常重要：

> **一行数据内部怎么组织？**

---

## NSM / Row Store

N-ary Storage Model：

```text
Row1:
[id][name][age][salary]

Row2:
[id][name][age][salary]
```

典型：

```text
OLTP
PostgreSQL
MySQL
```

---

## DSM / Column Store

Decomposition Storage Model：

```text
id:
1
2
3

age:
20
30
40

salary:
100
200
300
```

更适合：

```text
OLAP
Analytics
Aggregation
Compression
Vectorized Execution
```

还会讨论：

```text
PAX
Hybrid storage
```

---

# 6. Compression

Column Store 为什么通常压缩特别好？

例如：

```text
China
China
China
China
USA
USA
```

可以变成：

```text
China × 4
USA   × 2
```

课程会涉及一系列数据库压缩思想：

```text
Run-Length Encoding
Bit Packing
Dictionary Encoding
Delta Encoding
```

当前课程把 **Storage Models & Compression** 单独作为一讲。([CMU 15-445/645][1])

这对理解：

```text
ClickHouse
DuckDB
Parquet
Snowflake
BigQuery
```

非常有帮助。

---

# 7. Buffer Pool

这是整个课程最值得系统工程师学习的一块之一。

核心问题：

```text
数据库 1TB
RAM 只有 32GB

怎么办？
```

答案：

```text
Disk

Page 1
Page 2
Page 3
...
        ↓ read
┌────────────────┐
│ Buffer Pool    │
├────────────────┤
│ Frame 0 Page17 │
│ Frame 1 Page32 │
│ Frame 2 Page91 │
└────────────────┘
        ↓
       CPU
```

于是有：

```text
Page
Frame
Pin
Unpin
Dirty Page
Flush
Eviction
```

---

## Page Replacement

内存满了：

```text
Page A
Page B
Page C
```

现在要读取 D。

谁出去？

涉及：

```text
LRU
Clock
LRU-K
```

BusTub Project #1 就要求实现 Buffer Pool Manager。([CMU 15-445/645][2])

---

# 8. Hash Tables

数据库 Hash Table 可不是普通 LeetCode HashMap 就结束了。

主要会研究：

```text
Static Hashing
Dynamic Hashing

Linear Probing
Robin Hood Hashing
Cuckoo Hashing
Extendible Hashing
```

以及：

```text
Hash Index
Hash Join
Hash Aggregation
```

Fall 2026 第 7 讲专门讲 Hash Tables。([CMU 15-445/645][1])

---

# 9. Database Index

这是课程非常重要的一大块。

核心数据结构：

# B+ Tree

例如：

```text
                  [20 | 40]
                 /    |    \
                /     |     \
        [1 5 10] [20 30] [40 50 60]
```

学习：

```text
Search
Insert
Delete

Split
Merge
Redistribution
```

以及：

```text
Leaf node
Internal node
Fan-out
Tree height
```

---

# 10. Concurrent B+Tree

15-445 并不会停留在：

> 我能写一棵 B+ Tree。

还要解决：

```text
Thread A insert(20)

Thread B delete(30)

Thread C search(40)
```

怎么办？

因此学习：

```text
Latch
Latch Crabbing
Latch Coupling
Read Latch
Write Latch
```

Spring 2026 的 B+Tree Project 明确包含 concurrent index 和 latch crabbing。([CMU 15-445/645][3])

这一步已经非常接近真正数据库内核开发了。

---

# 11. Filters 与现代索引

现在课程也不只是传统 B+Tree。

官方 syllabus 已经明确包含：

```text
trees
hash tables
vector indexes
filters
```

([CMU 15-445/645][4])

所以会帮助你建立更大的索引体系：

```text
             Index
               │
      ┌────────┼─────────┐
      │        │         │
    B+Tree    Hash     Vector
```

这部分对你理解 **Milvus** 尤其有帮助。

传统数据库：

```text
B+Tree:
salary > 10000
```

向量数据库：

```text
Vector Index:
nearest(vector)
```

虽然课程主体仍然是传统 DBMS internals，但近年来内容已经明显纳入 vector index。

---

# 12. Sorting

接下来进入 Query Execution。

一个看起来简单的：

```sql
ORDER BY salary;
```

实际上当数据大于内存：

```text
1 TB table
32 GB RAM
```

不能：

```text
全部塞 RAM → sort()
```

于是出现：

# External Merge Sort

```text
Disk
 ↓
chunk 1 → sort ┐
chunk 2 → sort ├─→ Merge → sorted result
chunk 3 → sort ┘
```

你会学到：

```text
External Sorting
Run Generation
K-way Merge
Memory/Disk I/O Cost
```

---

# 13. Aggregation

例如：

```sql
SELECT country, COUNT(*)
FROM users
GROUP BY country;
```

数据库怎么执行？

两条经典路线：

## Sort-based

```text
Sort
 ↓
Group
 ↓
Aggregate
```

## Hash-based

```text
Hash Table

China → 1000
USA   → 800
Japan → 500
```

于是你最近问的：

> Elasticsearch aggregation 是怎么工作的？

这里的很多数据库基本功是相通的。

---

# 14. Join Algorithms

这大概是整个 query execution 里最重要的章节之一。

例如：

```sql
SELECT *
FROM orders o
JOIN users u
ON o.user_id = u.id;
```

数据库到底怎么 Join？

---

## Nested Loop Join

```text
for each order:
    for each user:
        if match:
```

复杂度近似：

```text
O(N × M)
```

---

## Index Nested Loop Join

如果：

```text
users.id
```

有 index：

```text
for order:
    index.lookup(order.user_id)
```

---

## Sort Merge Join

```text
Sort A
Sort B

      ↓

Merge
```

---

## Hash Join

```text
Build:
Users → HashTable

Probe:
Orders → HashTable lookup
```

当前课程专门安排 **Joins Algorithms**。([CMU 15-445/645][1])

---

# 15. Query Execution Engine

把前面这些 operator 组合起来。

例如：

```sql
SELECT department, AVG(salary)
FROM employee
WHERE age > 30
GROUP BY department;
```

可能变成：

```text
       Projection
            │
       Aggregation
            │
          Filter
            │
         SeqScan
```

这就是：

```text
Physical Query Plan
```

课程会讲：

```text
Iterator Model
Materialization Model
Vectorized Execution
Pipeline
Pipeline Breaker
```

---

# 16. Volcano / Iterator Model

经典数据库执行模型：

```cpp
operator->Init();

while (operator->Next(&tuple)) {
    ...
}
```

形成：

```text
Aggregation.Next()
       ↓
Filter.Next()
       ↓
Scan.Next()
```

也就是经典：

```text
open()
next()
close()
```

这套设计深刻影响了 PostgreSQL 等数据库。

---

# 17. Query Optimization

如果说 15-445 有三个最核心部分，我会选：

```text
Storage
Query Processing
Transactions
```

Optimizer 正好在 Query Processing 的中心。

同一句：

```sql
SELECT *
FROM A, B, C
WHERE ...
```

可能：

```text
(A Join B) Join C
```

也可能：

```text
A Join (B Join C)
```

成本可能相差几个数量级。

---

# 18. Logical Optimization

例如：

```text
Filter
   │
 Join
 /  \
A    B
```

可以变成：

```text
 Join
 /  \
Filter B
  |
  A
```

这就是：

```text
Predicate Pushdown
```

同一类还有：

```text
Projection Pushdown
Aggregation Pushdown
Limit Pushdown
```

所以你昨天问：

> pushdown 属于数据库里的哪类技术？

非常准确地说：

> **属于 Query Optimization / Query Rewrite 技术。**

---

# 19. Cost-Based Optimization

真正复杂的是：

```text
哪个 Plan 最便宜？
```

要估算：

```text
I/O Cost
CPU Cost
Cardinality
Selectivity
```

例如：

```text
100M rows
WHERE country='China'
```

Optimizer 必须估计：

```text
结果是 10 rows？

还是 50M rows？
```

于是涉及：

```text
Statistics
Histogram
Selectivity Estimation
Cardinality Estimation
Cost Model
```

这也是现代数据库优化器最难的问题之一。

---

# 20. Transactions

然后进入数据库另一座大山：

```text
Transactions
```

首先学习：

# ACID

```text
Atomicity
Consistency
Isolation
Durability
```

例如：

```text
BEGIN;

A -= 100;
B += 100;

COMMIT;
```

必须保证：

```text
不会只执行一半
不会看到奇怪的中间状态
宕机后数据不会丢
```

---

# 21. Serializability

课程会进一步问：

```text
T1:
read A
write A

T2:
read A
write A
```

什么执行顺序才是正确的？

因此学习：

```text
Conflict Serializability
Precedence Graph
Schedules
```

---

# 22. Two-Phase Locking — 2PL

经典并发控制：

```text
Transaction

Growing Phase:
   acquire locks

Shrinking Phase:
   release locks
```

涉及：

```text
Shared Lock
Exclusive Lock

S Lock
X Lock

Lock Upgrade
Deadlock
```

Fall 2026 有专门的 **Two-Phase Locking Concurrency Control** 课程。([CMU 15-445/645][1])

---

# 23. Timestamp Ordering

另一套思路：

```text
T1 timestamp = 10
T2 timestamp = 20
```

系统按照 timestamp 控制事务之间的先后关系。

涉及：

```text
Read Timestamp
Write Timestamp
Timestamp Ordering
Optimistic Concurrency Control
```

---

# 24. MVCC

这是现代数据库最重要的技术之一。

例如：

```text
Tuple:

V1: salary = 100
       ↑
V2: salary = 200
       ↑
V3: salary = 300
```

不同 transaction：

```text
T1 → V1
T2 → V2
T3 → V3
```

于是：

```text
Reader
```

不一定需要阻塞：

```text
Writer
```

Spring 2026 BusTub Project #4 已经直接要求实现 **optimistic multi-version concurrency control (MVOCC)**，包括 timestamps、undo logs、version chains、snapshot isolation，进一步支持 serializable。([CMU 15-445/645][5])

这部分非常值得你深入。

---

# 25. Isolation Levels

围绕 MVCC/事务自然就会理解：

```text
Read Uncommitted
Read Committed
Repeatable Read
Snapshot Isolation
Serializable
```

以及：

```text
Dirty Read
Non-repeatable Read
Phantom
Write Skew
Lost Update
```

这也是理解 PostgreSQL / MySQL / TiDB / CockroachDB 的基础。

---

# 26. Logging / WAL

数据库：

```text
UPDATE account SET balance = 100;
```

不能简单：

```text
直接修改 page
```

因为突然：

```text
💥 power failure
```

怎么办？

于是：

# Write-Ahead Logging

核心规则：

```text
Log first
Data later
```

即：

```text
修改 page
   ↓
生成 log
   ↓
log flush disk
   ↓
page 可以稍后 flush
```

当前 Fall 2026 专门有 **Database Logging**。([CMU 15-445/645][1])

---

# 27. Database Recovery

数据库重启：

```text
CRASH
 ↓
Restart
 ↓
???
```

必须恢复。

经典思想：

```text
Analysis
Redo
Undo
```

并学习：

```text
Checkpoint
LSN
Dirty Page
Transaction Table
Redo
Undo
```

即经典 ARIES 思想。

课程把 Logging 和 Recovery 分成连续两讲。([CMU 15-445/645][1])

---

# 28. Distributed Databases

最后才从：

```text
single-node DB
```

走到：

```text
distributed DB
```

会进入：

```text
Partitioning
Replication
Distributed Transactions
Distributed Query Processing
Parallel Execution
```

以及 OLTP / OLAP 的架构取舍。官方课程目标也明确要求学生能够比较 distributed / parallel alternatives。([CMU 15-445/645][4])

注意：

**15-445 的重点仍然是单机 DBMS internals。**

Distributed DB：

```text
不是主体
```

如果想进一步深入：

```text
15-445
   ↓
CMU 15-721 Advanced Database Systems
   ↓
distributed / OLAP / modern DB architecture papers
```

会更自然。

---

# 29. BusTub Projects 才是这门课真正的精华

15-445 和普通“看 PPT 的数据库课程”最大的区别就是：

> **你真的要造数据库。**

官方课程强调 programming projects，且所有项目都基于 BusTub。([CMU 15-445/645][6])

大致是：

| Project | 你实现什么                      |
| ------- | -------------------------- |
| P0      | C++ Primer                 |
| P1      | Buffer Pool Manager        |
| P2      | B+Tree Index               |
| P3      | Query Execution            |
| P4      | Concurrency Control / MVCC |

---

## P0 — C++ Primer

因为 BusTub 使用：

```text
C++17
```

需要掌握：

```text
RAII
smart pointer
move semantics
templates
STL
threading
```

Spring 2026 官方 Project #0 也是专门检查 C++ 基础。([CMU 15-445/645][7])

---

# 30. P1 — Buffer Pool

实现：

```text
Disk Scheduler
Buffer Pool
Page Replacement
Page Guard
```

你真正会理解：

```text
Page ≠ Frame
```

```text
Page
=
磁盘上的逻辑块

Frame
=
内存里装 Page 的槽位
```

这一点看十页 PPT 都不如自己实现一次。

---

# 31. P2 — B+Tree

你会真正实现：

```text
GetValue()

Insert()
  ↓
leaf full
  ↓
split
  ↓
parent insert
  ↓
parent full
  ↓
split recursively

Remove()
  ↓
underflow
  ↓
redistribute / merge
```

然后再加：

```text
Concurrent B+Tree
Latch Crabbing
```

Spring 2026 Project #2 明确覆盖 split、delete、coalesce、iterator 和 concurrent index。([CMU 15-445/645][3])

---

# 32. P3 — Query Execution

这一部分我认为对你会特别有意思。

你需要实现真实 operator：

```text
SeqScan
IndexScan

Insert
Update
Delete

Aggregation

NestedLoopJoin
HashJoin

Sort
Limit
Window Function
```

以及 optimizer rule。

Spring 2026 Project #3 官方任务明确包括 access method executors、aggregation/join executors、hash join optimization、external merge sort、limit 和 window functions。([CMU 15-445/645][8])

所以：

```sql
SELECT department, COUNT(*)
FROM employee
GROUP BY department;
```

不再只是 SQL。

你会真正写：

```text
AggregationExecutor
```

---

# 33. P4 — MVCC

Spring 2026 版本尤其值得学习。

BusTub 已经不是简单：

```text
lock everything
```

而是要求实现：

```text
Timestamp
      │
Transaction Manager
      │
Version Chain
      │
Undo Log
      │
Tuple Version
```

例如：

```text
Latest tuple
 salary=300
      │
    undo
      ▼
 salary=200
      │
    undo
      ▼
 salary=100
```

一个事务：

```text
read_ts = 50
```

沿 version chain 找到：

```text
timestamp <= 50
```

的版本。

这已经和：

```text
PostgreSQL
DuckDB
TiDB
OceanBase
CockroachDB
```

里面很多核心思想形成联系了。

---

# 34. 整门课真正建立的是这几个 Mental Model

学完以后，看数据库不应该再是：

```text
Database
    =
SQL Server
```

而应该是：

```text
DBMS
│
├── Storage Engine
│    ├── File
│    ├── Page
│    ├── Tuple
│    └── Buffer Pool
│
├── Access Methods
│    ├── Heap Scan
│    ├── B+Tree
│    ├── Hash
│    └── Vector Index
│
├── Query Processor
│    ├── Parser
│    ├── Binder
│    ├── Optimizer
│    └── Executor
│
├── Execution Operators
│    ├── Scan
│    ├── Join
│    ├── Aggregate
│    ├── Sort
│    └── Filter
│
├── Transaction Manager
│    ├── Locks
│    ├── Timestamp
│    └── MVCC
│
├── Recovery
│    ├── WAL
│    ├── Checkpoint
│    └── Undo / Redo
│
└── Distributed Layer
     ├── Partition
     ├── Replication
     └── Distributed Query
```

---

# 35. 对你来说，我认为最值得重点学的章节

结合你最近在学的内容，我会把优先级排成：

| 优先级   | 内容                           | 原因                 |
| ----- | ---------------------------- | ------------------ |
| ⭐⭐⭐⭐⭐ | Storage / Page / Buffer Pool | DB 内核基础            |
| ⭐⭐⭐⭐⭐ | B+Tree / Index               | Storage Engine 核心  |
| ⭐⭐⭐⭐⭐ | Query Execution              | 理解算子非常关键           |
| ⭐⭐⭐⭐⭐ | Join / Aggregation / Sort    | 查询引擎基本功            |
| ⭐⭐⭐⭐⭐ | Query Optimization           | pushdown、plan、cost |
| ⭐⭐⭐⭐⭐ | MVCC                         | 现代数据库核心            |
| ⭐⭐⭐⭐⭐ | WAL / Recovery               | DB durability 核心   |
| ⭐⭐⭐⭐  | Compression / Column Store   | OLAP 基础            |
| ⭐⭐⭐⭐  | Concurrency B+Tree           | 系统并发能力             |
| ⭐⭐⭐   | Distributed DB               | 15-445 只算入门        |

特别是你最近连续问了：

```text
Milvus operators
aggregation pushdown
database page/frame
MVCC
OceanBase/TiDB
```

15-445 恰好能把这些零散知识串成一个完整体系。

---

# 36. 我建议不要单纯按 Lecture 1 → 25 学

如果是你现在这个基础，我更推荐按照 **数据库组件** 来学：

```text
阶段 1：Storage Engine
────────────────────────
Storage
Page / Tuple
Buffer Pool
Storage Model
Compression

        ↓

阶段 2：Access Methods
────────────────────────
Hash Table
B+Tree
Index
Concurrent Index

        ↓

阶段 3：Query Engine
────────────────────────
Sort
Aggregation
Join
Execution Model

        ↓

阶段 4：Optimizer
────────────────────────
Logical Plan
Physical Plan
Pushdown
Join Ordering
Cardinality
Cost Model

        ↓

阶段 5：Transaction Engine
────────────────────────
Serializability
2PL
Timestamp
MVCC

        ↓

阶段 6：Durability
────────────────────────
WAL
Checkpoint
Recovery

        ↓

阶段 7：Distributed DB
```

这其实就是在逐层回答：

```text
数据怎么存？
    ↓
数据怎么找？
    ↓
SQL 怎么执行？
    ↓
SQL 怎么执行得更快？
    ↓
多人同时操作怎么办？
    ↓
宕机怎么办？
    ↓
一台机器不够怎么办？
```

这就是 **CMU 15-445 的完整故事线**。

官方 Fall 2026 schedule 也基本按照这个顺序展开：Relational Model → Storage → Memory → Storage Models → Hash/Indexes → Sort/Aggregation/Join → Execution → Optimization → Concurrency/MVCC → Logging/Recovery → Distributed Databases。([CMU 15-445/645][1])

如果只选一门课来建立**数据库内核全景图**，15-445 是目前非常合适的一门；而如果你的目标进一步偏向 **“我想真正懂 TiDB / PostgreSQL / DuckDB / Milvus 这些系统为什么这么实现”**，比较自然的路线是：

**15-445 → BusTub Projects → CMU 15-721 → 阅读 PostgreSQL/DuckDB/TiDB/Milvus 源码。**

[1]: https://15445.courses.cs.cmu.edu/fall2026/schedule.html?utm_source=chatgpt.com "Schedule | CMU 15-445/645 :: Intro to Database Systems (Fall 2026)"
[2]: https://15445.courses.cs.cmu.edu/fall2025/project1/?utm_source=chatgpt.com "Project #1 - Buffer Pool Manager | CMU 15-445/645 :: Intro to Database Systems (Fall 2025)"
[3]: https://15445.courses.cs.cmu.edu/spring2026/project2/?utm_source=chatgpt.com "Project #2 - B+Tree | CMU 15-445/645 :: Intro to Database Systems (Spring 2026)"
[4]: https://15445.courses.cs.cmu.edu/spring2026/syllabus.html?utm_source=chatgpt.com "Syllabus | CMU 15-445/645 :: Intro to Database Systems (Spring 2026)"
[5]: https://15445.courses.cs.cmu.edu/spring2026/project4/?utm_source=chatgpt.com "Project #4 - Concurrency Control | CMU 15-445/645 :: Intro to Database Systems (Spring 2026)"
[6]: https://15445.courses.cs.cmu.edu/fall2025/syllabus.html?utm_source=chatgpt.com "Syllabus | CMU 15-445/645 :: Intro to Database Systems (Fall 2025)"
[7]: https://15445.courses.cs.cmu.edu/spring2026/project0/?utm_source=chatgpt.com "Project #0 - C++ Primer | CMU 15-445/645 :: Intro to Database Systems (Spring 2026)"
[8]: https://15445.courses.cs.cmu.edu/spring2026/project3/?utm_source=chatgpt.com "Project #3 - Query Execution | CMU 15-445/645 :: Intro to Database Systems (Spring 2026)"
