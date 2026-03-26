# XiongYue-Cpp

用于演示“与熊论道（熊曰）”新版密文流程的 C++ 示例项目。

## 来源说明

本项目整体思路来源于：
https://github.com/SheepChef/Abracadabra/issues/123

仓库内的 [`Abracadabra-Issue-123.md`](./Abracadabra-Issue-123.md) 为参考分析材料。

## 功能

- 支持加密：`UTF-8 -> Raw Deflate -> Base91(0-90) -> 91字映射 -> 倒序 -> "呋"标头`
- 支持解密：上述流程的逆过程
- 支持命令行参数模式
- 无参数时进入交互菜单，可选择加密/解密

## 示例代码

主示例文件：[`xiongyue_example.cpp`](./xiongyue_example.cpp)

## 编译（g++）

需要 zlib。

```bash
g++ -std=c++17 -O2 xiongyue_example.cpp -o xiongyue_example -lz
```

## 运行

### 1) 命令行参数模式

```bash
# 加密
./xiongyue_example -e "magnet:?xt=urn:btih:00722F040871F58F011A38EA42FC206B494C1CF1"

# 解密
./xiongyue_example -d "呋食食很捕既嗡嗄嗥呱哞家嘿呦嗚咬很冬怎常發誘和更嚄喜沒性果嚁冬唬很擊覺咬喜溫擊發呦襲既哈噔非嗚山人訴你盜誘告既噔囑吖囑非歡蜜洞現象既果偶怎嗒和堅常嗄我偶嗥氏"
```

### 2) 菜单模式（无参数）

```bash
./xiongyue_example
```

程序会显示菜单：

- `1) Encrypt`
- `2) Decrypt`
- `0) Exit`
