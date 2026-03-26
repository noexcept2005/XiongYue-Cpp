# XiongYue-Cpp
简体中文 | [English](./README-en.md)

用于演示 2025 年 3 月被下架的“与熊论道（熊曰）”新版密文流程的 C++ 示例项目。支持解密之前 2020 年以后的熊曰密文。
Copyright (C) 2026 Wormwaker, SheepChef.

## 来源说明

本项目整体思路来源于：
https://github.com/SheepChef/Abracadabra/issues/123

仓库内的 [`Abracadabra-Issue-123.md`](./Abracadabra-Issue-123.md) 为参考分析材料。

## 法律与安全风险声明（请务必阅读）

本项目仅用于密码学/编码流程研究、逆向分析学习与安全教育演示，不提供任何线上加解密服务，不面向匿名公众提供“代加密/代解密”能力。你在使用本项目时应自行确保完全合规，并独立承担全部法律与安全责任。

请明确以下风险与边界：

- 禁止用于任何违法违规用途，包括但不限于隐匿违法信息、规避平台审查、传播恶意链接、网络攻击、诈骗、侵权、骚扰、数据窃取、批量滥发、洗钱或其他违法活动。
- 禁止用于处理你无合法授权的数据。未经授权解密、分析、还原他人密文，可能构成对隐私、通信秘密、数据权益或知识产权的侵害。
- 本项目实现的是“编码/压缩流程复现”，不是现代安全加密方案。将其误当作安全加密手段，可能导致敏感信息泄露、证据留存、审计暴露与业务风险。
- 任何由你输入、保存、传输或发布的数据都可能产生合规风险。请避免在示例中使用真实个人信息、生产密钥、账号口令、内部文档、客户数据等敏感内容。
- 在不同司法辖区（国家/地区）中，关于密码技术、数据处理、网络安全、平台治理与内容发布的监管要求不同。你必须自行确认并遵守当地现行法律法规。
- 若因使用、修改、分发本项目导致行政处罚、民事纠纷、刑事风险、平台封禁、数据泄露、经济损失或声誉损害，责任由实际使用者承担。

维护者与贡献者不对以下事项提供保证：

- 不保证项目适用于任何特定目的；
- 不保证“误用后果可控”或“可规避监管风险”；
- 不为第三方二次开发、部署、传播及其后果背书；
- 不承担由滥用、误用或非法使用引发的直接或间接损失。

建议仅在隔离的测试环境中、使用可公开的非敏感样例数据进行学习验证；不要将本项目接入生产系统或灰黑产相关场景。

## 功能

- 支持加密：`UTF-8 -> Raw Deflate -> Base91(0-90) -> 91字映射 -> 倒序 -> "呋"标头`
- 支持解密：上述流程的逆过程
- 解密支持“噪声容错”：可自动忽略密文中夹杂的非映射字符（如空白、标点、英文等）
- 支持命令行参数模式（文本/文件）
- 无参数时进入交互菜单，支持单行加密、解密、多行加密、文件加解密
- 支持 `--self-test` 自检

## 示例代码

主示例文件：[`xiongyue.cpp`](./xiongyue.cpp)

## 获取仓库（含子模块）

推荐使用子模块方式获取 zlib：

```bash
git clone --recurse-submodules <your-repo-url>
```

如果你已经 clone 过仓库，再执行：

```bash
git submodule update --init --recursive
```

## 编译（g++）

### 方式 A：使用子模块内置 zlib（推荐，无需系统安装 zlib）

```bash
g++ -std=c++17 -O2 xiongyue.cpp ^
  third_party/zlib/adler32.c third_party/zlib/crc32.c ^
  third_party/zlib/deflate.c third_party/zlib/inflate.c ^
  third_party/zlib/inffast.c third_party/zlib/inftrees.c ^
  third_party/zlib/trees.c third_party/zlib/zutil.c ^
  -Ithird_party/zlib -o xiongyue
```

说明：上面使用的是 Windows `cmd` 换行符 `^`。如果你在 Linux/macOS，可把它写成一行或使用 `\` 换行。

### 方式 B：使用系统 zlib

需要 zlib。

```bash
g++ -std=c++17 -O2 xiongyue.cpp -o xiongyue -lz
```

## 运行

### 1) 命令行参数模式

```bash
# 加密
./xiongyue -e "magnet:?xt=urn:btih:00722F040871F58F011A38EA42FC206B494C1CF1"

# 解密（允许夹杂噪声字符）
./xiongyue -d "xxx 呋食食很捕既嗡嗄嗥...嗥氏 !!!"

# 加密文件（输出到 stdout）
./xiongyue -ef input.txt

# 加密文件（输出到文件）
./xiongyue -ef input.txt encrypted.txt

# 解密文件（输出到 stdout）
./xiongyue -df encrypted.txt

# 解密文件（输出到文件）
./xiongyue -df encrypted.txt decrypted.txt

# 快速自检
./xiongyue --self-test
```

### 2) 菜单模式（无参数）

```bash
./xiongyue
```

程序会显示菜单：

- `1) Encrypt text (single line)`
- `2) Decrypt text (noise tolerant)`
- `3) Encrypt text (multi-line, end with ::END::)`
- `4) Encrypt file`
- `5) Decrypt file`
- `6) Self-test`
- `0) Exit`

