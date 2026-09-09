# 模块 2：关系模型与 SQL 验收

以下四个精确提交通过 Debug／Release 构建、CTest、demo 和独立源码审查，主代理核对提交与证据后接受交付。

| 分支 | 已验证提交 |
| --- | --- |
| `topic/07-relational-model` | `615e7c43c098f91e7800765a4491ab6d3d338890` |
| `topic/08-relational-constraints` | `91ced6416ebcd6bd0f9b0b546860982a631bda02` |
| `topic/09-relational-algebra` | `b520854a4d69fc9a21d2d87cdf4006801b79243c` |
| `topic/10-sql-planning` | `df4110ae0dc2e7100b04baf47e3cd516fbfa74d4` |

## 验证范围

- 主代理确认四个 topic ref 等于报告 SHA，共同文档基线是祖先；完整变更清单仅含 README.md、CMakeLists.txt、src/、tests/。
- 每个提交经 `git archive` 导出为独立源码快照，实际 Git diff 与报告包字节一致。
- 独立 reviewer 检查四个完整源码快照、中文教程、示例和八组 Debug／Release 测试日志，规范符合性及代码质量均通过，无待修复发现。
- 全部测试在 NDEBUG 下仍有效。使用已披露的 macOS SDK 头文件 workaround，不在 CMake 中写死路径。

## 教学内容与边界

- **07**：类型、元组数量、NULL 许可在插入前校验；覆盖空关系、重复值与零度关系。采用集合语义和整数／文本类型，不是 SQL 三值逻辑。
- **08**：主键、外键、非空和 CHECK；通过候选数据库验证后交换实现原子发布，支持插入和 RESTRICT 删除。限单列主键、无环外键模式，CHECK 回调需为纯函数。
- **09**：选择、投影、并、交、差、笛卡尔积、等值连接；覆盖 schema 不兼容、去重、空输入和连接列前缀。采用集合语义，NULL 按明确说明的字面值处理，不假称 SQL 等值语义。
- **10**：真实词法解析、完整输入语法检查、名称绑定、逻辑／物理转换及执行；支持声明的 SELECT/FROM/WHERE 等值 AND 子集，保留重复行并正确过滤 NULL 比较。拒绝歧义名称、非法语法、尾随内容；不支持别名、DISTINCT、ORDER BY、DML 或优化。

运行方式与其他主题一致：切换目标分支，CMake 配置、编译，运行 CTest 和 demo；实际示例、复杂度和限制详见各分支 README。

## 可追溯性

- 独立审查 run：`977ba801-a9cf-4a98-8a73-3292c26523aa`，结果 pass，无 findings。
- 实现日志包：`/tmp/cmu445-module-2-review.dGeFOm`。
- 主代理精确提交验证及源码快照：本机会话临时目录 `cmu445-module2-exact-review-0j22jng6/verification.json`。

临时文件仅用于本次审查，不是运行依赖。本记录不覆盖未来提交，也不代表后续存储等模块已经完成。
