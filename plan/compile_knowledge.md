# 🛠️ 软件工程师专用：编译相关知识清单（Notion 模板）

> 适用于追求工程能力与系统调试能力的程序员。聚焦于 "理解编译器生成什么"、"如何优化"、"如何调试" 三大核心。

---

## 🌟 必须掌握的知识点（核心工程所需）

### 🔧 编译流程基本结构

* [ ] 编译（compile） vs 汇编（assemble） vs 链接（link）

  * 实验：`gcc -v main.c -o main`
  * 工具：`gcc -E`（预处理） / `-S`（生成汇编） / `-c`（只编译）

### 🧩 链接与库

* [ ] 静态链接（.a） vs 动态链接（.so/.dll）

  * 工具：`ldd`, `nm`, `objdump`, `readelf`
  * 分析 ELF 文件结构与符号表
* [ ] 动态加载失败排查技巧

### 🧠 符号与调试信息

* [ ] gdb 与 -g 调试符号的关系
* [ ] addr2line / objdump -S 实验
* [ ] Go 中 symbol 和函数内联对调试的影响

### 🚀 优化等级与行为

* [ ] -O0 / -O2 / -O3 / -Og 的区别

  * 优化 vs 可调试性
  * 实验：对比函数是否被 inline

### 🔍 宏与预处理器

* [ ] `#define`、`#ifdef`、`#include` 的展开逻辑

  * 实验：查看 `gcc -E` 生成结果
  * 理解头文件保护、条件编译

### 🧪 Go 编译器构建过程

* [ ] `go build -x` 查看详细构建
* [ ] go tool compile / asm / link 拆解
* [ ] 模块缓存与依赖管理机制（go.mod）

### 🧵 调用约定 / 参数传递（ABI 层面）

* [ ] x86\_64 系统调用传参方式
* [ ] 汇编视角下的函数调用结构
* [ ] FFI 调用（如 C ↔ Python、C ↔ Go）时常见问题

---

## 📘 拓展理解：建议了解

### 🧾 AST 与语法结构

* 抽象语法树（Abstract Syntax Tree）
* 用于 LSP、代码生成、静态分析

### 🏗️ 中间表示（IR）

* LLVM IR：类似汇编的中间层
* SSA 形式（静态单赋值）
* Go 的 SSA 可视化工具（debug/ssa）

### 📦 GC 与逃逸分析

* 逃逸到堆的变量会导致 GC 压力
* Go 中：`go build -gcflags=-m` 查看逃逸信息

---

## ✅ 推荐实验任务（10 小时打通）

* [ ] 用 `gcc -E/-S/-c` 拆解编译阶段
* [ ] 分析 `-O0` 与 `-O2` 汇编输出差异
* [ ] `nm` + `objdump` + `readelf` 分析 ELF 文件结构
* [ ] `gdb` + `addr2line` 进行函数溯源
* [ ] 写一个小型 tokenizer（用现成工具如 `lex` / `Go scanner`）
* [ ] 查看 Go build 的编译路径（`go build -x`）并注释

---

## 🎓 推荐资源

* 📘 [CS\:APP Chapter 7 - Linking](https://csapp.cs.cmu.edu/3e/ch7-preview.pdf)
* 📘 [Go SSA 设计文档](https://golang.org/s/go-ssa)
* 📘 [LLVM IR 简介](https://llvm.org/docs/LangRef.html)
* 📘 [用 GCC 看懂编译过程](https://eli.thegreenplace.net/2013/07/09/input-to-gcc-the-hidden-language-of-the-compiler/)
* 🧰 工具：[godbolt.org](https://godbolt.org/)（可视化编译器输出）
* 🧰 工具：`gdb`, `perf`, `strace`, `valgrind`, `go tool compile`

---

## 📁 建议笔记结构

```
/compiler-notes
├── 01_gcc_stages.md
├── 02_debug_symbols.md
├── 03_linking_issues.md
├── 04_go_build.md
├── 05_macro_expansion.md
└── cheatsheet.md
```

---

> 📌 提醒：编译原理不必“精通”，但必须“理解 + 能分析 + 能解释你写的代码如何被编译”。
