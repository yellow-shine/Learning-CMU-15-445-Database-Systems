# 82 个数据库知识点分支实施计划

## 授权与设计

用户已审阅并确认 `docs/superpowers/specs/2026-09-09-database-topics-design.md`，允许开始实现。知识点精确名称及范围以根目录 README 的 82 条清单为准；原始 `spec.md` 不修改。

当前环境：macOS、Apple Clang 17、CMake 4.2.3。本地 Git 已初始化为 main，无需外部服务或安装依赖。

## Global Constraints

- 每个 `topic/NN-name` 从共同文档基线建立，独立包含全部运行代码，不依赖另一个分支的 checkout。依赖组件允许以必要的源码快照附带，并注明来源和教学焦点。
- 各分支使用 C++17、CMake、CTest，提供 `demo`，无第三方依赖。构建选项关闭 C++ 扩展，启用适用的编译警告。
- 根目录 README 必须是中文教程，不是通用目录或几行运行说明；包含问题、前置知识、原理、不变量、具体轨迹、源码导读、复杂度、测试方法、预期输出和局限。
- 实现算法本身，禁止 TODO 核心代码、打印流程充数、用 std::map 冒充 B+Tree 或标准排序冒充外部排序。
- 正确性测试独立于 demo，失败返回非零；不能仅使用会被 NDEBUG 消除的 assert。
- 每个分支配置、编译并运行 Debug 与 Release 的 CTest 和 demo，记录实际结果。专项故障场景按设计文档验收。
- 一个 worktree 一个写入者；禁止修改其他工作区、main、他人 topic 分支、用户 Git 配置或原始资料。禁止推送、创建远端、破坏性清理或启动子代理。
- 在自己的 worktree 中创建并提交所分配的 topic 分支，结束时回到初始管理分支。不得强制更新已存在分支；如果出现同名分支先检查并报告。
- 只有通过测试并独立审查的主题才可标记已验证。中断时报告真实完成列表和剩余项，不降低要求凑齐数量。

## 通用交付步骤

每个任务对应一个模块，模块内部的知识点依次实现；下文的专项测试是最低要求，不代替边界验证。

1. 阅读设计、当前任务 brief 和目录中分配的条目，检查工作区与共同基线。
2. 在自己的工作区创建本主题独立分支，编写最小核心实现、独立测试和 demo；复用已验证的必要支撑代码，而非无关模块。
3. 测试优先覆盖核心不变量与最容易写错的反例；边实现边验证，不把全部测试拖到最后。
4. 编写中文讲解，所有例子与输出来自实际代码；注明不支持的输入、算法变体及平台边界。
5. LSP 工具可用时先检查源码，再执行构建。以干净 Debug／Release 构建运行 CTest 与 demo；完成自查并提交。
6. 保存实际分支名、完整提交 SHA、测试命令与输出、局限以及本模块的基线到任务报告。审查用 diff 包包含每个主题相对于共同基线的变更，不使用 HEAD~1 猜测范围。
7. 独立 reviewer 检查每个提交的规范符合性与代码质量；存在实证问题时由该模块唯一 writer 修复、重测，再针对修复复审。
8. 主代理确认分支及证据后更新 main 的状态；不合并实现代码到 main。

统一构建命令：

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

构建目录不提交。演示与测试不修改用户数据，文件测试使用独立临时目录；涉及进程、线程的测试要有完成条件及合理超时。

## Task 1: C++ 基础，01–06

交付 01-cpp-raii、02-cpp-smart-pointers、03-cpp-move-semantics、04-cpp-templates、05-cpp-stl、06-cpp-threading。

- RAII 文件守卫测试正常关闭、异常退出、打开失败；不得让析构函数抛出异常。
- 智能指针演示独占、共享和弱引用，验证析构次数及弱引用失效，解释悬空引用。
- 可移动缓冲区验证移动后所有权、移动赋值、空状态；不得双重释放或读取已释放内存。
- 模板使用实际泛型数据库例子，STL 演示正确容器与算法选择、失效规则，避免执行未定义行为来“证明”失效。
- 线程队列支持有界容量或明确的容量契约、关闭与消费者退出；测试真实多生产者／消费者、无丢失／重复、关闭唤醒，不能靠 sleep 碰巧通过。

## Task 2: 关系模型与 SQL，07–10

交付 07-relational-model、08-relational-constraints、09-relational-algebra、10-sql-planning。

- 类型与 schema 验证字段数量、类型、NULL 许可；约束失败不能留下部分修改。
- 约束测试主键重复、外键缺失、非空和检查失败，明确支持的插入／删除操作范围。
- 关系代数涵盖七类操作，测试兼容 schema、去重、空集、重复输入与连接列处理。
- 最小 SQL 需要真实词法／语法解析和名称绑定，而不是按硬编码 SQL 匹配输出；支持明确的 SELECT/FROM/WHERE 子集和逻辑到物理转换，拒绝非法、歧义及尾随内容。说明集合与 SQL bag 语义区别。

## Task 3: 存储引擎，11–17

交付 11-disk-page-io 至 17-pax-layout 的七个分支。

- 页 I/O 使用真实固定页文件，验证边界、短读、非法 page ID；不混淆 flush 与 fsync。
- 元组格式支持定长、变长、NULL；验证编码解码、长度溢出及畸形缓冲区。
- Slotted Page 支持插入、删除、更新和紧缩，检查 RID 稳定性、空闲区不重叠、页满失败无损。
- Heap File 展示跨页持久化、关闭后重开、RID 定位与扫描。
- NSM、DSM、PAX 使用实际不同数据布局及访问计数解释取舍，不只更改类名；验证读出相同逻辑数据，PAX 包含页内 mini-page 边界。

## Task 4: 压缩，18–21

交付 18-rle、19-bit-packing、20-dictionary-encoding、21-delta-encoding。

- 每种编码独立实现 encode/decode，并提供实际压缩前后大小比较。
- 测试空输入、重复数据、单项数据、最大值、非法编码，以及固定种子的随机往返。
- Bit Packing 覆盖跨字节边界、0 位／满位宽及不能容纳的数；Delta 处理有符号差分溢出，不触发 UB。
- Dictionary 在编码 ID 上执行等值过滤并与原字符串过滤比较。

## Task 5: 缓冲池，22–27

交付 22-buffer-pool 至 27-page-guard 的六个分支。

- Buffer Pool 包含真实页 I/O、固定 Frame 容量、page table、pin count、dirty writeback；测试全部 pinned 时失败、淘汰脏页后重载、重复 fetch 不占两个 frame。
- LRU、Clock、LRU-K 各自验证指定访问轨迹、淘汰顺序、不可淘汰条目、空集与单项；明确 LRU-K 历史不足 K 的优先级及 tie-break。
- Disk Scheduler 真实后台工作线程处理读写及 promise/future 结果，传播 I/O 异常并安全关闭。
- Page Guard 包含必要 Buffer Pool 支撑、移动与异常路径下 unpin、读写锁生命周期；验证释放恰好一次与 dirty 标记。

## Task 6: 哈希，28–32

交付 28-linear-probing 至 32-hash-index 的五个分支。

- 不能用 unordered_map 实现所学哈希算法；标准容器只用于 oracle 或辅助目录。
- Linear Probing 测试碰撞链中删除后仍可找到后续键、满表与重复更新。
- Robin Hood 测试探测距离不变量及 backward-shift 删除。
- Cuckoo 测试驱逐、环检测及重建失败的有界行为，不能无限循环或丢键。
- Extendible Hashing 测试目录加倍、局部／全局深度、多个目录入口指向同桶、分裂后全部键可查。
- Hash Index 支持重复 key 的多个 RID 与精确删除；各算法用固定种子差分测试。

## Task 7: 索引，33–39

交付 33-bplus-tree-insert 至 39-ivf-index 的七个分支。

- B+Tree 单线程三个主题附带完整所需支撑，检查排序、等深叶子、分隔键、占用率与叶链；查找／插入／删除与 std::map 差分验证。
- 删除覆盖左右借位、左右合并、递归合并及根收缩；迭代覆盖空树、范围边界及跨叶。
- 并发树实现真正的节点读写 latch 与 crabbing；测试并发查插删及最终不变量，不能用全局互斥包装冒充节点级算法。
- Bloom Filter 测试插入元素无假阴性、明确不支持直接删除、固定种子观察假阳性而不把概率结果当绝对保证。
- 向量精确 Top-K 验证维度、空集、k 范围、距离与并列排序。
- IVF 包含实际粗聚类／倒排列表／nprobe 搜索，与精确结果比较 recall；全部列表探测应与精确结果一致。固定随机种子，处理空簇与非法参数。

## Task 8: 排序与聚合，40–42

交付 40-external-merge-sort、41-sort-aggregation、42-hash-aggregation。

- 外部排序真实磁盘 run generation 与 K 路多轮 merge，输入大于预算，多轮归并限制内存和打开文件数；验证与 std::sort 一致，清理自有临时文件。
- 两种聚合支持 COUNT/SUM/AVG/MIN/MAX，明确 NULL、COUNT(*)/COUNT(column)、空输入和分组语义，AVG 不取局部平均的简单平均。
- 用重复键、负数、空输入、多组及合理数值范围做交叉验证。

## Task 9: Join，43–46

交付 43-nested-loop-join、44-index-nested-loop-join、45-sort-merge-join、46-hash-join。

- 各分支独立实现所学连接，标准参考 nested-loop 可作 oracle。
- 验证空表、无匹配、两侧重复键的 m×n 输出、多种输入顺序、NULL 不等值匹配以及输出多重集一致。
- Index NLJ 展示 RID 候选访问计数，Hash Join 展示 build/probe 与小表选择，Sort Merge 处理重复键组，不能丢掉同键匹配。

## Task 10: 执行引擎，47–54

交付 47-volcano-execution 至 54-window-functions 的八个分支。

- 三种执行模型运行相同的 Scan/Filter/Projection 思路，暴露实际逐元组、完整物化和批量调用差异，测试空输入及耗尽行为。
- Pipeline 通过带阻塞算子的执行轨迹展示真正的阶段边界。
- SeqScan/IndexScan 返回相同结果并展示访问差异；修改执行器维持表与索引一致，失败不留下部分写入。
- LIMIT/OFFSET 验证 0、越界、大 offset、提前停止不多消费输入。
- 窗口实现明确子集：ROW_NUMBER、RANK、DENSE_RANK 和显式 ROWS 累计 SUM；处理分区、并列值和输入排序，拒绝未支持的 frame，不暗示完整 SQL 窗口支持。

## Task 11: 优化器，55–62

交付 55-predicate-pushdown 至 62-join-ordering 的八个分支。

- 各下推规则有真实计划结构、合法性检查和执行前后结果对比。至少一个安全和一个不安全案例，不能硬编码恒等答案。
- Predicate 检查引用列和外连接等语义边界；Projection 保留 join/filter/aggregate 后续所需列。
- Aggregation 展示 partial/final、SUM/COUNT 表示 AVG、重复连接键影响；Limit 不越过过滤／排序等不安全边界。
- 物理选择真实匹配等值条件生成 Hash Join，非等值保留可行备选。
- 统计／直方图支持空表与边界，估计说明独立性等假设；成本为可解释教学模型并有计数实验。
- Join Ordering 使用明确支持的 inner equijoin 计划及 DP 枚举，和小规模穷举比较最优成本；验证执行结果等价。

## Task 12: 事务，63–72

交付 63-transaction-acid 至 72-serializable-mvcc 的十个分支。

- ACID 转账具备提交／回滚原子性，说明内存例子不具备崩溃持久性。
- 冲突可串行化构建真实依赖图并检测环；2PL 支持 S/X、升级与阶段规则；死锁实现 waits-for cycle 与牺牲者释放。
- Timestamp Ordering 实现 read_ts/write_ts 冲突规则；OCC 实现读／验证／写与冲突中止。
- MVCC 使用 undo/version chain 重建、事务提交可见性；Snapshot Isolation 检查并发写冲突和快照稳定性。
- 隔离异常包含可重复的五类异常调度，区分 RR 与 SI，不笼统宣称 MVCC 阻止全部异常。
- Serializable MVCC 可用保守读集／谓词验证，必须测试 write skew 和幻读型冲突；如跟踪全表版本，需要说明额外中止代价。
- 调度模拟器明确标注模拟；承诺锁等待／并发的实现必须真实运行线程并避免不确定 sleep。

## Task 13: 持久性与恢复，73–77

交付 73-write-ahead-logging 至 77-aries-recovery 的五个分支。

- WAL 真实文件、递增 LSN、数据页刷盘前日志同步、提交记录同步；测试未提交与已提交进程中断后差异。
- Buffer policies 用可执行模型显示 steal/force 四种组合对 undo/redo 的需求。
- Checkpoint 真实保存所用事务表、脏页表与安全恢复起点，测试有活跃事务的检查点。
- Redo/Undo 处理重复恢复、已提交重做和未提交撤销；日志尾部截断与损坏有明确校验及失败策略。
- ARIES 教学子集具备 Analysis/Redo/Undo、PageLSN、CLR、undoNextLSN 与恢复中再次中断，不能仅顺序打印三个阶段。
- 使用子进程退出／被终止后新进程恢复，覆盖关键阶段；说明 fsync 及进程崩溃不等于断电。

## Task 14: 分布式与并行，78–82

交付 78-partitioning 至 82-parallel-execution 的五个分支。

- 分区展示 hash/range 路由、边界值与倾斜统计。
- 主从复制用明确的本地节点／消息模型展示同步／异步确认、延迟与故障边界；不声称自动选主和共识。
- 2PC 具备 coordinator/participant 状态机、prepare/commit/abort、持久化决策、重启重放与 in-doubt 阻塞；不宣称解决网络分区可用性。
- 分布式查询实现 broadcast/repartition 数据交换与 partial aggregate，对比集中式结果，重复键不丢失。
- 并行执行真实线程分区执行、结果合并、异常传播；比较串行结果，说明线程并行不等于跨机器部署。

## 批次与审查安排

每个模块是一个独立 writer lane；同一模块中的多个主题顺序完成，互不依赖的模块使用独立管理 worktree。最大并发为 4。

| 批次 | 任务 | 主题数 | 下一道验收 |
| --- | --- | --- | --- |
| A | 1–4：C++、关系、存储、压缩 | 21 | 每模块独立审查，再修复实证问题 |
| B | 5–7：缓冲池、哈希、索引 | 18 | 并发／结构不变量专项审查 |
| C | 8–11：排序聚合、Join、执行、优化 | 23 | 结果等价、预算约束、语义反例审查 |
| D | 12–14：事务、恢复、分布式 | 20 | 隔离、故障恢复、持久化保证审查 |

复用只从已验证提交取必要源码；如果独立教学主题无需某前置分支的具体实现，可自行提供最小支撑并明确其边界。批次不会改变任何知识点的独立使用要求。

## 最终验收

- 对总目录的 82 个分支逐一检查存在、提交、中文教程、C++17 构建入口、src/tests 和不含构建产物。
- 在干净检出／导出的快照上执行统一 Debug／Release 命令，并记录每个分支的准确 SHA 与结果。
- 汇总模块独立审查和修复记录，执行跨分支完整性审查。
- main 更新真实完成数量与验证记录，保留未完成或未验证项，不虚报全量完成。
- 不推送、不把所有 topic 合并到 main、不删除仍用于交付或复审的 worktree。
