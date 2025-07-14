# 🗓️ 程序员成长计划：第 28 周（2025.7.9 - 2025.7.14）

## 🎯 目标概述

本周重点构建“老派有眼界程序员”的第一块地基：

- 扎实掌握 Linux 命令行
- 学习 C 语言并了解系统调用
- 初步掌握并发与网络编程（Go + C）
- 建立对工程文化和系统程序员的直觉

---

## 🧱 每日学习计划

### 📅 周二（7.9）Linux & Shell 基础

- **学习内容：**
  - Bash 变量、控制流（if, for, while）
  - 常用工具：`grep`, `find`, `xargs`, `cut`, `awk`, `sed`
  - man 手册：`man`, `man -k`, `man 2 open`
- **实践任务：**
  - 编写 `log_filter.sh`，支持按关键字、日期过滤日志并输出统计
- **资源推荐：**
  - [Shell 教程（鸟哥的 Linux 私房菜）](https://linux.vbird.org/linux_basic/)
  - [ShellCheck 工具](https://www.shellcheck.net/)

---

### 📅 周三（7.10）C 语言基础 + gdb 调试

- **学习内容：**
  - 指针、结构体、数组与内存模型
  - gdb 的使用：break、next、print、layout src
- **实践任务：**
  - 写一个解析字符串表达式的 C 程序（如“123+456” -> 579）
  - 用 gdb 调试运行、打印栈帧
- **资源推荐：**
  - [C语言现代教程](https://www.learn-c.org/)
  - [GDB 教程](https://darkdust.net/files/GDB%20Cheat%20Sheet.pdf)

---

### 📅 周四（7.11）系统调用与 mini 工具实现

- **学习内容：**
  - 系统调用原理：open/read/write/fork/exec/wait
  - C 中调用 `unistd.h`, `fcntl.h`
- **实践任务：**
  - 用 C 写 `mini_cat`, `mini_ls`
  - 用 `strace` 分析原版 `cat` 的行为
- **资源推荐：**
  - [man7.org Linux syscall 文档](https://man7.org/linux/man-pages/index.html)
  - [CS](http://csapp.cs.cmu.edu/)[:APP](http://csapp.cs.cmu.edu/)[ 第三章（程序的机器级表示）](http://csapp.cs.cmu.edu/)

---

### 📅 周五（7.12）网络编程与 epoll 初识

- **学习内容：**
  - TCP 协议基本流程（三次握手）
  - `socket`, `bind`, `listen`, `accept`, `recv`, `send`
  - epoll 的非阻塞模型
- **实践任务：**
  - 写一个 C 语言 echo server，支持多个客户端并发（使用 epoll）
- **资源推荐：**
  - [Beej’s 网络编程指南](https://beej.us/guide/bgnet/)
  - [epoll 教程](https://man7.org/linux/man-pages/man7/epoll.7.html)

---q

### 📅 周六（7.13）Go 并发编程实践

- **学习内容：**
  - goroutine、channel、select 的原理与用法
  - defer、panic 的机制理解
- **实践任务：**
  - 编写一个并发爬虫，带超时机制、最多同时 N 个请求
- **资源推荐：**
  - [Go by Example](https://gobyexample.com/)
  - [Go 并发模式 PDF](https://go.dev/talks/2012/concurrency.slide)

---

### 📅 周日（7.14）总结 + 编程文化

- **学习内容：**
  - 阅读《The Art of Unix Programming》的部分章节
  - 思考软件工具哲学、模块化、组合式设计的价值
- **实践任务：**
  - 写一篇博客《我这一周学到了什么》（总结学习内容、心得、未来方向）
- **资源推荐：**
  - [The Art of Unix Programming 在线版](http://catb.org/esr/writings/taoup/html/)
  - [GitHub Pages 教程](https://pages.github.com/)

---

## 📁 本周产出目录建议结构

```bash
week28/
├── log_filter.sh
├── mini_cat.c
├── mini_ls.c
├── echo_server.c
├── timeout_fetcher.go
├── week28-reflection.md
└── README.md
```

---

