#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <zlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Project idea source:
// https://github.com/SheepChef/Abracadabra/issues/123

namespace {

#ifdef _WIN32
void setupUtf8Console() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

std::string ansiToUtf8(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    const int wideLen =
        MultiByteToWideChar(CP_ACP, 0, input.data(), static_cast<int>(input.size()), nullptr, 0);
    if (wideLen <= 0) {
        return input;
    }

    std::wstring wide(static_cast<size_t>(wideLen), L'\0');
    MultiByteToWideChar(CP_ACP, 0, input.data(), static_cast<int>(input.size()), wide.data(),
                        wideLen);

    const int utf8Len =
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0,
                            nullptr, nullptr);
    if (utf8Len <= 0) {
        return input;
    }

    std::string out(static_cast<size_t>(utf8Len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(),
                        utf8Len, nullptr, nullptr);
    return out;
}
#else
void setupUtf8Console() {}
#endif

const std::string kHeader = u8"呋";
const std::string kMultilineEndMarker = "::END::";

const std::vector<std::string> kDict = {
    u8"食", u8"性", u8"很", u8"雜", u8"既", u8"溫", u8"和", u8"會", u8"誘", u8"捕",
    u8"動", u8"物", u8"家", u8"住", u8"山", u8"洞", u8"沒", u8"有", u8"冬", u8"眠",
    u8"偶", u8"爾", u8"襲", u8"擊", u8"人", u8"類", u8"呱", u8"哞", u8"嗄", u8"哈",
    u8"嘍", u8"啽", u8"唬", u8"咯", u8"呦", u8"嗷", u8"嗡", u8"哮", u8"嗥", u8"嗒",
    u8"嗚", u8"吖", u8"吃", u8"嗅", u8"嘶", u8"噔", u8"咬", u8"噗", u8"嘿", u8"嚁",
    u8"噤", u8"囑", u8"非", u8"常", u8"喜", u8"歡", u8"堅", u8"果", u8"魚", u8"肉",
    u8"蜂", u8"蜜", u8"註", u8"取", u8"象", u8"發", u8"達", u8"你", u8"覺", u8"出",
    u8"更", u8"盜", u8"森", u8"氏", u8"我", u8"誒", u8"怎", u8"寶", u8"麼", u8"圖",
    u8"現", u8"破", u8"嚄", u8"告", u8"訴", u8"樣", u8"呆", u8"萌", u8"笨", u8"拙",
    u8"意"};

const std::unordered_map<std::string, int>& reverseDict() {
    static const std::unordered_map<std::string, int> dict = [] {
        std::unordered_map<std::string, int> m;
        for (size_t i = 0; i < kDict.size(); ++i) {
            m[kDict[i]] = static_cast<int>(i);
        }
        return m;
    }();
    return dict;
}

std::vector<std::string> splitUtf8Chars(const std::string& input) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < input.size()) {
        const unsigned char lead = static_cast<unsigned char>(input[i]);
        size_t len = 0;
        if ((lead & 0x80) == 0) {
            len = 1;
        } else if ((lead & 0xE0) == 0xC0) {
            len = 2;
        } else if ((lead & 0xF0) == 0xE0) {
            len = 3;
        } else if ((lead & 0xF8) == 0xF0) {
            len = 4;
        } else {
            throw std::runtime_error("Invalid UTF-8 lead byte.");
        }

        if (i + len > input.size()) {
            throw std::runtime_error("Truncated UTF-8 sequence.");
        }

        for (size_t j = 1; j < len; ++j) {
            const unsigned char c = static_cast<unsigned char>(input[i + j]);
            if ((c & 0xC0) != 0x80) {
                throw std::runtime_error("Invalid UTF-8 continuation byte.");
            }
        }

        out.emplace_back(input.substr(i, len));
        i += len;
    }
    return out;
}

std::vector<std::string> splitUtf8CharsLoose(const std::string& input) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < input.size()) {
        const unsigned char lead = static_cast<unsigned char>(input[i]);
        size_t len = 0;
        if ((lead & 0x80) == 0) {
            len = 1;
        } else if ((lead & 0xE0) == 0xC0) {
            len = 2;
        } else if ((lead & 0xF0) == 0xE0) {
            len = 3;
        } else if ((lead & 0xF8) == 0xF0) {
            len = 4;
        } else {
            ++i;
            continue;
        }

        if (i + len > input.size()) {
            break;
        }

        bool valid = true;
        for (size_t j = 1; j < len; ++j) {
            const unsigned char c = static_cast<unsigned char>(input[i + j]);
            if ((c & 0xC0) != 0x80) {
                valid = false;
                break;
            }
        }

        if (valid) {
            out.emplace_back(input.substr(i, len));
            i += len;
        } else {
            ++i;
        }
    }
    return out;
}

std::string rawDeflate(const std::string& input, int level = 1) {
    z_stream zs{};
    if (deflateInit2(&zs, level, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("deflateInit2 failed.");
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    zs.avail_in = static_cast<uInt>(input.size());

    std::string output;
    char buffer[4096];
    int ret = Z_OK;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(buffer);
        zs.avail_out = sizeof(buffer);
        ret = deflate(&zs, Z_FINISH);
        output.append(buffer, sizeof(buffer) - zs.avail_out);
    } while (ret == Z_OK);

    deflateEnd(&zs);
    if (ret != Z_STREAM_END) {
        throw std::runtime_error("Raw deflate failed.");
    }
    return output;
}

std::string rawInflate(const std::string& input) {
    z_stream zs{};
    if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) {
        throw std::runtime_error("inflateInit2 failed.");
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    zs.avail_in = static_cast<uInt>(input.size());

    std::string output;
    char buffer[4096];
    int ret = Z_OK;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(buffer);
        zs.avail_out = sizeof(buffer);
        ret = inflate(&zs, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&zs);
            throw std::runtime_error("Raw inflate failed.");
        }
        output.append(buffer, sizeof(buffer) - zs.avail_out);
    } while (ret != Z_STREAM_END);

    inflateEnd(&zs);
    return output;
}

std::vector<int> base91EncodeValues(const std::string& data) {
    uint32_t b = 0;
    uint32_t n = 0;
    std::vector<int> out;

    for (unsigned char byte : data) {
        b |= static_cast<uint32_t>(byte) << n;
        n += 8;
        if (n > 13) {
            uint32_t v = b & 8191;
            if (v > 88) {
                b >>= 13;
                n -= 13;
            } else {
                v = b & 16383;
                b >>= 14;
                n -= 14;
            }
            out.push_back(static_cast<int>(v % 91));
            out.push_back(static_cast<int>(v / 91));
        }
    }

    if (n) {
        out.push_back(static_cast<int>(b % 91));
        if (n > 7 || b > 90) {
            out.push_back(static_cast<int>(b / 91));
        }
    }

    return out;
}

std::string base91DecodeValues(const std::vector<int>& values) {
    uint32_t b = 0;
    uint32_t n = 0;
    int v = -1;
    std::string out;

    for (int x : values) {
        if (x < 0 || x > 90) {
            throw std::runtime_error("Base91 value out of range.");
        }

        if (v < 0) {
            v = x;
        } else {
            v += x * 91;
            b |= static_cast<uint32_t>(v) << n;
            n += ((v & 8191) > 88) ? 13 : 14;
            while (n > 7) {
                out.push_back(static_cast<char>(b & 255));
                b >>= 8;
                n -= 8;
            }
            v = -1;
        }
    }

    if (v != -1) {
        out.push_back(static_cast<char>((b | (static_cast<uint32_t>(v) << n)) & 255));
    }

    return out;
}

std::string encryptXiongYue(const std::string& plaintext) {
    const std::string deflated = rawDeflate(plaintext, 1);
    const std::vector<int> vals = base91EncodeValues(deflated);

    std::vector<std::string> mapped;
    mapped.reserve(vals.size());
    for (int v : vals) {
        mapped.push_back(kDict[static_cast<size_t>(v)]);
    }

    std::reverse(mapped.begin(), mapped.end());
    std::string out = kHeader;
    for (const auto& ch : mapped) {
        out += ch;
    }
    return out;
}

std::string normalizeCiphertext(const std::string& ciphertext) {
    const auto& rev = reverseDict();
    const size_t headerPos = ciphertext.find(kHeader);
    const std::string view = (headerPos == std::string::npos)
                                 ? ciphertext
                                 : ciphertext.substr(headerPos + kHeader.size());

    const std::vector<std::string> chars = splitUtf8CharsLoose(view);
    std::string cleaned = kHeader;
    size_t kept = 0;

    for (const auto& ch : chars) {
        if (rev.find(ch) != rev.end()) {
            cleaned += ch;
            ++kept;
        }
    }

    if (kept == 0) {
        throw std::runtime_error("No valid mapped ciphertext characters found.");
    }

    return cleaned;
}

std::string decryptXiongYue(const std::string& ciphertext) {
    const std::string normalized = normalizeCiphertext(ciphertext);
    const std::string body = normalized.substr(kHeader.size());

    std::vector<std::string> chars = splitUtf8Chars(body);
    std::reverse(chars.begin(), chars.end());

    const auto& rev = reverseDict();
    std::vector<int> vals;
    vals.reserve(chars.size());
    for (const auto& ch : chars) {
        auto it = rev.find(ch);
        if (it == rev.end()) {
            throw std::runtime_error("Ciphertext contains unknown mapped character.");
        }
        vals.push_back(it->second);
    }

    const std::string compressed = base91DecodeValues(vals);
    return rawInflate(compressed);
}

std::vector<std::string> collectArgs(int argc, char* argv[]) {
    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc));

    for (int i = 0; i < argc; ++i) {
#ifdef _WIN32
        args.push_back(ansiToUtf8(argv[i]));
#else
        args.push_back(argv[i]);
#endif
    }

    return args;
}

std::string joinArgs(const std::vector<std::string>& args, int startIndex) {
    std::string out;
    for (size_t i = static_cast<size_t>(startIndex); i < args.size(); ++i) {
        if (i > static_cast<size_t>(startIndex)) {
            out.push_back(' ');
        }
        out += args[i];
    }
    return out;
}

std::string readFileAll(const std::string& path) {
    std::ifstream in(std::filesystem::u8path(path), std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open file for reading: " + path);
    }

    std::ostringstream oss;
    oss << in.rdbuf();
    if (!in.good() && !in.eof()) {
        throw std::runtime_error("Failed while reading file: " + path);
    }
    return oss.str();
}

void writeFileAll(const std::string& path, const std::string& data) {
    std::ofstream out(std::filesystem::u8path(path), std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open file for writing: " + path);
    }

    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!out) {
        throw std::runtime_error("Failed while writing file: " + path);
    }
}

std::string promptLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    if (!std::getline(std::cin, value)) {
        throw std::runtime_error("Input aborted.");
    }
    return value;
}

std::string readMultilineInput(const std::string& endMarker) {
    std::cout << "Enter multi-line text. End with a single line: " << endMarker << "\n";

    std::string line;
    std::string all;
    bool first = true;
    while (std::getline(std::cin, line)) {
        if (line == endMarker) {
            return all;
        }

        if (!first) {
            all.push_back('\n');
        }
        all += line;
        first = false;
    }

    return all;
}

bool runSelfTest() {
    const std::string sample = u8"line1\n中文line2";
    const std::string enc = encryptXiongYue(sample);
    const std::string noisy = std::string("noise-prefix\n") + enc + "\n***";
    const std::string dec = decryptXiongYue(noisy);
    return dec == sample;
}

void printUsage(const char* program) {
    std::cout << "Usage:\n";
    std::cout << "  " << program << " -e <plaintext>\n";
    std::cout << "  " << program << " -d <ciphertext-with-optional-noise>\n";
    std::cout << "  " << program << " -ef <input_file> [output_file]\n";
    std::cout << "  " << program << " -df <input_file> [output_file]\n";
    std::cout << "  " << program << " --menu\n";
    std::cout << "  " << program << " --self-test\n";
    std::cout << "  " << program << " --help\n";
}

int runMenu() {
    while (true) {
        std::cout << "\n===== XiongYue Demo =====\n";
        std::cout << "1) Encrypt text (single line)\n";
        std::cout << "2) Decrypt text (noise tolerant)\n";
        std::cout << "3) Encrypt text (multi-line)\n";
        std::cout << "4) Encrypt file\n";
        std::cout << "5) Decrypt file\n";
        std::cout << "6) Self-test\n";
        std::cout << "0) Exit\n";
        std::cout << "Select: ";

        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }

        if (choice == "0") {
            return 0;
        }

        try {
            if (choice == "1") {
                const std::string input = promptLine("Input text: ");
                std::cout << "Ciphertext:\n" << encryptXiongYue(input) << "\n";
            } else if (choice == "2") {
                const std::string input = promptLine("Input ciphertext: ");
                std::cout << "Plaintext:\n" << decryptXiongYue(input) << "\n";
            } else if (choice == "3") {
                const std::string input = readMultilineInput(kMultilineEndMarker);
                std::cout << "Ciphertext:\n" << encryptXiongYue(input) << "\n";
            } else if (choice == "4") {
                const std::string inPath = promptLine("Input file path: ");
                const std::string outPath = promptLine("Output file path (empty=print): ");
                const std::string plaintext = readFileAll(inPath);
                const std::string ciphertext = encryptXiongYue(plaintext);

                if (outPath.empty()) {
                    std::cout << "Ciphertext:\n" << ciphertext << "\n";
                } else {
                    writeFileAll(outPath, ciphertext);
                    std::cout << "Saved encrypted output to: " << outPath << "\n";
                }
            } else if (choice == "5") {
                const std::string inPath = promptLine("Input file path: ");
                const std::string outPath = promptLine("Output file path (empty=print): ");
                const std::string ciphertext = readFileAll(inPath);
                const std::string plaintext = decryptXiongYue(ciphertext);

                if (outPath.empty()) {
                    std::cout << "Plaintext:\n" << plaintext << "\n";
                } else {
                    writeFileAll(outPath, plaintext);
                    std::cout << "Saved decrypted output to: " << outPath << "\n";
                }
            } else if (choice == "6") {
                if (runSelfTest()) {
                    std::cout << "Self-test passed.\n";
                } else {
                    std::cout << "Self-test failed.\n";
                }
            } else {
                std::cout << "Invalid selection.\n";
            }
        } catch (const std::exception& ex) {
            std::cout << "Error: " << ex.what() << "\n";
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        setupUtf8Console();
        const std::vector<std::string> args = collectArgs(argc, argv);

        if (args.size() == 1) {
            return runMenu();
        }

        const std::string mode = args[1];

        if (mode == "--help" || mode == "-h") {
            printUsage(args[0].c_str());
            return 0;
        }

        if (mode == "--menu") {
            return runMenu();
        }

        if (mode == "--self-test") {
            if (runSelfTest()) {
                std::cout << "Self-test passed.\n";
                return 0;
            }
            std::cerr << "Self-test failed.\n";
            return 1;
        }

        if ((mode == "-e" || mode == "--encrypt") && args.size() >= 3) {
            std::cout << encryptXiongYue(joinArgs(args, 2)) << "\n";
            return 0;
        }

        if ((mode == "-d" || mode == "--decrypt") && args.size() >= 3) {
            std::cout << decryptXiongYue(joinArgs(args, 2)) << "\n";
            return 0;
        }

        if (mode == "-ef" || mode == "--encrypt-file") {
            if (args.size() < 3 || args.size() > 4) {
                printUsage(args[0].c_str());
                return 1;
            }

            const std::string plaintext = readFileAll(args[2]);
            const std::string ciphertext = encryptXiongYue(plaintext);
            if (args.size() == 4) {
                writeFileAll(args[3], ciphertext);
            } else {
                std::cout << ciphertext << "\n";
            }
            return 0;
        }

        if (mode == "-df" || mode == "--decrypt-file") {
            if (args.size() < 3 || args.size() > 4) {
                printUsage(args[0].c_str());
                return 1;
            }

            const std::string ciphertext = readFileAll(args[2]);
            const std::string plaintext = decryptXiongYue(ciphertext);
            if (args.size() == 4) {
                writeFileAll(args[3], plaintext);
            } else {
                std::cout << plaintext << "\n";
            }
            return 0;
        }

        printUsage(args[0].c_str());
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 1;
    }
}
