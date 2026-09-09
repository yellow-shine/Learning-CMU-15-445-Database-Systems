# 01 · 用 RAII 管理数据库文件

## 问题与前置知识

页文件打开以后，执行器可能正常返回，也可能因解析错误抛出异常。
在每个返回点手写 `fclose` 容易遗漏；复制一个拥有文件的对象又容易关闭两次。
本主题属于数据库资源管理基础。先理解栈对象、作用域、构造函数与异常展开；
下一主题讨论智能指针，03 讨论如何安全转移所有权。

## 原理与不变量

`FileGuard` 在构造函数中获取 FILE，失败立即抛出异常，此时没有完整对象需要析构。
成功后只有这个不可复制、不可移动的对象拥有句柄。作用域结束调用不抛异常的析构函数。
`borrow()` 只借用，不转交所有权：调用方不能 fclose，也不能在守卫销毁后使用指针。
显式 `close()` 先清空内部指针，再关闭句柄，因此即使关闭报告错误，也不会重试关闭已失效句柄。
关闭后的写入被拒绝；重复 close 是无操作。

文件缓冲区写入可能延迟报错。需要确认结果时显式 close 并处理异常；析构只负责尽力释放，
不能在异常展开途中再抛异常。RAII 是释放协议，不是事务回滚，也不保证数据完整。

## 具体执行轨迹

测试在独立临时目录写入 `page 7\n`：

| 步骤 | 所有权状态 | 可观察结果 |
| --- | --- | --- |
| 构造 | 持有一个 FILE | fileno 得到有效描述符 |
| write | 仍持有 | 字节可能仍在 stdio 缓冲区 |
| 离开作用域 | 关闭 | fcntl 返回 EBADF，重新读取内容一致 |
| 第二次构造并抛异常 | 栈展开关闭 | 不依赖手写 catch 内的 fclose |
| 打开 missing/page | 构造失败 | 抛异常，无对象泄漏 |

演示在独立临时目录中真实写入并关闭，再触发异常展开，预期输出：

```text
normal: page 7
exception: unwound
open failure: no owned resource
```

正常与异常释放还由独立测试通过描述符验证，不只是打印说明。

## 源码导读与测试

- `src/file_guard.h`：获取、借用、短写检查、显式释放与析构协议。
- `src/demo.cpp`：正常写入、异常展开、打开失败的调用方。
- `src/temp_directory.h`：演示与测试共享的 POSIX 临时目录守卫。
- `tests/topic_tests.cpp`：正常关闭后读回字节；异常关闭；失败构造；重复关闭；关闭后写入拒绝。
  还用 POSIX fcntl 直接检查描述符已释放，而不是把“数据能读取”等同于“资源已关闭”。
- 临时目录使用 mkdtemp，离开测试作用域自动删除，仅删除本测试创建的目录。

## 成本与局限

守卫额外空间 O(1)，构造/关闭各一个标准 I/O 调用；写 n 字节时间 O(n)。
实际系统调用次数取决于 stdio 缓冲，不宣称一次 write 对应一次磁盘 I/O。
`fclose` 不是 fsync，不保证断电持久性。析构错误被忽略，因此重要写入必须显式 close。
核心为标准 C++17/C stdio，描述符验收与临时目录测试面向 macOS/Linux POSIX。
没有移动、随机页访问、重试、故障注入或并发共享文件协议；这些不是本课的承诺。

## 构建、运行与验证

本机 AppleClang 默认缺少 libc++ 搜索路径，验收额外使用
`-DCMAKE_CXX_FLAGS="-isystem $(xcrun --show-sdk-path)/usr/include/c++/v1"`；正常工具链无需此项。

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
