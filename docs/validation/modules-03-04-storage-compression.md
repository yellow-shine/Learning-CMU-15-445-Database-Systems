# 模块 3–4：存储与压缩验收

11–21 共 11 个主题已完成实现、中文讲解、Debug／Release 构建、CTest、demo 和独立源码审查。主代理核对精确提交和证据后接受交付。

| 分支 | 已验证提交 |
| --- | --- |
| `topic/11-disk-page-io` | `b53e39448a0fde41b54cdf297ddd951a7304c424` |
| `topic/12-tuple-layout` | `0c1055614fddcbe82fc86e989f13c8d54a686b3e` |
| `topic/13-slotted-page` | `8d92f354680ec971a77224a3f3de4a9517e48cff` |
| `topic/14-heap-file` | `dc2d132fdb8e103f3a3b0489d5ba17015f3bc6f3` |
| `topic/15-row-store` | `b8ec8deb204c866c1274c6a7277d227227ad2f26` |
| `topic/16-column-store` | `e791a4be11edb1c6077bf464f418518fcfcedb39` |
| `topic/17-pax-layout` | `2fa5079d41d5dbd5355c7aae6914fdb7fa726bb9` |
| `topic/18-rle` | `8f93b800982e562a919106f50b19b6eec58f0426` |
| `topic/19-bit-packing` | `0e4b8a686c305f26e7609ef2edc69bb298f94662` |
| `topic/20-dictionary-encoding` | `f28be1c276ccb5a890d0684785679b0a9e233f5e` |
| `topic/21-delta-encoding` | `4c20685681b36db024a3682dd275f98699ef5a69` |

## 存储验证与修复

- 页文件执行真实固定页 I/O，检查非法页、偏移、短读；Heap File 验证跨页、重开、RID 访问与扫描。
- 元组解码验证定长／变长／NULL 元数据和损坏长度；Slotted Page 检查不重叠、不足空间失败原子性、紧缩及 RID 稳定性。
- NSM／DSM／PAX 使用不同实际布局，验证相同逻辑行及 PAX mini-page 边界。访问计数是教学算法计数，不是假称硬件实测。
- 初审在列存 `project_ids` 发现参数别名导致越界：当计数器引用位置数组元素时，循环中更新计数会改变后续已验证索引。主代理在旧提交 `28de0075e8896224ee30c365d4e248389685e333` 用 AddressSanitizer 复现 heap-buffer-overflow。
- 修复 `e791a4b` 将计数更新移到全部读取之后，并加入别名调用的回归测试，验证结果 `{1,1}` 与最终计数 2。七个主题重新完成 Debug／Release 验证；列存完整测试及原始复现用例通过 ASan。主代理独立使用原始未改动的失败用例对修复快照重跑，退出 0、无 sanitizer 报告。
- 独立复审确认修复及影响范围，无剩余问题。其他六个存储提交保持不变。

存储限制：教学固定 schema；Slotted Page 不复用墓碑槽；Heap File 为 first-fit、同页更新，不提供 WAL、fsync、并发或 I/O 失败回滚。行／列／PAX 展示以只读内存布局为主，不提供压缩和持久性保证。

## 压缩验证

四个主题实现实际编码／解码及字节大小比较。测试包括空输入、单项、重复值、边界、非法编码和固定种子的往返验证。

- RLE：验证规范游程及展开预算，明确可能膨胀。
- Bit Packing：覆盖 0–64 位、跨字节、尾部 padding、精确长度，避免满位宽移位 UB。
- Dictionary：字符串字节保真、合法 ID、解析／展开预算、在整数 ID 上过滤；不存在的过滤键也不跳过损坏 ID 验证。
- Delta：有符号加减边界、ZigZag、规范 varint，超出 int64 差值范围时拒绝。Debug／Release 外还通过 UBSan 测试与 demo。

压缩限制：全块内存算法；位打包是标量教学实现；字典使用固定 uint32 ID，查询会完整解析；差分没有绝对值 escape 或随机访问索引。详见各分支教程。

## 提交与证据

全部验证使用已披露的本机 SDK 头文件 workaround。主代理验证 refs、基线祖先关系、完整变更范围和审查包字节一致，并导出精确源码快照。没有额外构建产物提交。结论仅覆盖表中 SHA，不自动适用于未来提交。

- 存储初审：`4d114879-378c-4922-9fe0-edb4fb41a903`，一项 P1。
- 存储复审：`e936dcc7-d03c-4358-84fd-8e6879f302c8`，pass，零 findings。
- 压缩审查：`a7e563a5-9918-44b9-a358-80e1a6d49537`，pass，零 findings。
- 临时日志包：`/tmp/cmu445-module-3-fix-review.WV0A7x`、`/tmp/cmu445-module-4-review.WYXejq`。
- 精确源码／Git 证据临时目录：`cmu445-module3-fix-exact-denez_41`、`cmu445-module4-exact-review-ekm2kjki`。

临时目录不属于运行依赖；清理后可按表中 SHA 重新导出并运行 CMake、CTest 与 demo。
