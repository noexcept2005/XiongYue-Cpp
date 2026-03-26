#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <zlib.h>

// Project idea source:
// https://github.com/SheepChef/Abracadabra/issues/123

namespace {

const std::string kHeader = u8"呋";

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

std::string decryptXiongYue(const std::string& ciphertext) {
    if (ciphertext.rfind(kHeader, 0) != 0) {
        throw std::runtime_error("Ciphertext must start with header '呋'.");
    }

    const std::string body = ciphertext.substr(kHeader.size());
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

std::string joinArgs(int argc, char* argv[], int startIndex) {
    std::string out;
    for (int i = startIndex; i < argc; ++i) {
        if (i > startIndex) {
            out.push_back(' ');
        }
        out += argv[i];
    }
    return out;
}

void printUsage(const char* program) {
    std::cout << "Usage:\n";
    std::cout << "  " << program << " -e <plaintext>\n";
    std::cout << "  " << program << " -d <ciphertext>\n";
    std::cout << "  " << program << " --menu\n";
    std::cout << "  " << program << " --help\n";
}

int runMenu() {
    while (true) {
        std::cout << "\n===== XiongYue Demo =====\n";
        std::cout << "1) Encrypt\n";
        std::cout << "2) Decrypt\n";
        std::cout << "0) Exit\n";
        std::cout << "Select: ";

        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }

        if (choice == "0") {
            return 0;
        }

        if (choice != "1" && choice != "2") {
            std::cout << "Invalid selection.\n";
            continue;
        }

        std::cout << "Input text: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            return 0;
        }

        try {
            if (choice == "1") {
                std::cout << "Ciphertext:\n" << encryptXiongYue(input) << "\n";
            } else {
                std::cout << "Plaintext:\n" << decryptXiongYue(input) << "\n";
            }
        } catch (const std::exception& ex) {
            std::cout << "Error: " << ex.what() << "\n";
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc == 1) {
            return runMenu();
        }

        const std::string mode = argv[1];

        if (mode == "--help" || mode == "-h") {
            printUsage(argv[0]);
            return 0;
        }

        if (mode == "--menu") {
            return runMenu();
        }

        if ((mode == "-e" || mode == "--encrypt") && argc >= 3) {
            std::cout << encryptXiongYue(joinArgs(argc, argv, 2)) << "\n";
            return 0;
        }

        if ((mode == "-d" || mode == "--decrypt") && argc >= 3) {
            std::cout << decryptXiongYue(joinArgs(argc, argv, 2)) << "\n";
            return 0;
        }

        printUsage(argv[0]);
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 1;
    }
}
