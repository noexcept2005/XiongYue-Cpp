# XiongYue-Cpp
[简体中文](./README.md) | English

This C++ project demonstrates the reverse-engineered pipeline of the newer "Bear Talk (XiongYue)" ciphertext format, whose online service was taken down in March 2025. It supports decrypting post-2020 XiongYue ciphertext samples.

## Source Attribution

The overall project approach is based on:
https://github.com/SheepChef/Abracadabra/issues/123

See [`Abracadabra-Issue-123.md`](./Abracadabra-Issue-123.md) in this repository for the referenced analysis material.

## Legal and Security Risk Notice (Must Read)

This project is provided only for cryptography/encoding workflow research, reverse-engineering study, and security education demonstrations. It does not provide any online encryption/decryption service and must not be used as a public anonymous "encode/decode" utility. You are solely responsible for lawful and compliant use.

Please understand the following risks and boundaries:

- Do not use this project for any illegal or non-compliant activity, including but not limited to hiding unlawful content, bypassing platform moderation, distributing malicious links, cyberattacks, fraud, infringement, harassment, data theft, spam campaigns, money laundering, or other unlawful conduct.
- Do not process data without proper authorization. Unauthorized decryption, analysis, or restoration of third-party ciphertext may violate privacy, communication secrecy, data rights, or intellectual property laws.
- This project reproduces an encoding/compression pipeline, not modern secure encryption. Treating it as a security-grade encryption method may lead to sensitive data exposure, evidence retention, audit visibility, and operational risk.
- Any data you input, store, transmit, or publish may create compliance risk. Avoid using real personal data, production secrets, account credentials, internal documents, or customer data in examples.
- Regulatory requirements differ across jurisdictions (countries/regions), especially for cryptography, data processing, cybersecurity, platform governance, and content distribution. You must verify and comply with applicable laws in your jurisdiction.
- If your use, modification, or redistribution causes administrative penalties, civil disputes, criminal exposure, platform bans, data leaks, financial loss, or reputational damage, responsibility rests with the actual user/operator.

Maintainers and contributors provide no guarantee regarding:

- fitness for any specific purpose;
- controllable consequences of misuse;
- regulatory safety or compliance bypass;
- downstream third-party deployments and their outcomes;
- any direct or indirect losses caused by abuse, misuse, or illegal use.

Use this project only in isolated test environments with public non-sensitive sample data. Do not connect it to production systems or illicit scenarios.

## Features

- Encrypt: `UTF-8 -> Raw Deflate -> Base91 (0-90) -> 91-character mapping -> reverse -> prepend "呋"`
- Decrypt: inverse pipeline of the above
- Noise-tolerant decryption: ignores non-mapping characters mixed into ciphertext (spaces, punctuation, ASCII, etc.)
- Command-line argument mode for both text and file operations
- Interactive menu mode supports single-line encryption, decryption, multi-line encryption, and file encrypt/decrypt
- Built-in `--self-test`

## Example Code

Main example file: [`xiongyue.cpp`](./xiongyue.cpp)

## Clone (with submodule)

Recommended way to include zlib via submodule:

```bash
git clone --recurse-submodules <your-repo-url>
```

If you already cloned the repository, run:

```bash
git submodule update --init --recursive
```

## Build (g++)

### Option A: Use bundled zlib submodule (recommended, no system zlib required)

```bash
g++ -std=c++17 -O2 xiongyue.cpp ^
  third_party/zlib/adler32.c third_party/zlib/crc32.c ^
  third_party/zlib/deflate.c third_party/zlib/inflate.c ^
  third_party/zlib/inffast.c third_party/zlib/inftrees.c ^
  third_party/zlib/trees.c third_party/zlib/zutil.c ^
  -Ithird_party/zlib -o xiongyue
```

Note: the multiline example uses Windows `cmd` line continuation (`^`). On Linux/macOS, use `\` or put it on one line.

### Option B: Use system zlib

Requires zlib.

```bash
g++ -std=c++17 -O2 xiongyue.cpp -o xiongyue -lz
```

## Run

### 1) Command-line mode

```bash
# Encrypt
./xiongyue -e "magnet:?xt=urn:btih:00722F040871F58F011A38EA42FC206B494C1CF1"

# Decrypt (noise characters are tolerated)
./xiongyue -d "xxx 呋食食很捕既嗡嗄嗥...嗥氏 !!!"

# Encrypt file (stdout)
./xiongyue -ef input.txt

# Encrypt file (write to file)
./xiongyue -ef input.txt encrypted.txt

# Decrypt file (stdout)
./xiongyue -df encrypted.txt

# Decrypt file (write to file)
./xiongyue -df encrypted.txt decrypted.txt

# Quick self-test
./xiongyue --self-test
```

### 2) Menu mode (no arguments)

```bash
./xiongyue
```

The program shows:

- `1) Encrypt text (single line)`
- `2) Decrypt text (noise tolerant)`
- `3) Encrypt text (multi-line, end with ::END::)`
- `4) Encrypt file`
- `5) Decrypt file`
- `6) Self-test`
- `0) Exit`

