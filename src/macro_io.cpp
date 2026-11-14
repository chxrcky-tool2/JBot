#include "macro_io.hpp"
#include <Geode/Geode.hpp>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>
#include "replay.hpp"

using namespace geode::prelude;

namespace jbot {
    namespace {
        constexpr char kMagic[4] = {'J', 'B', 'M', '1'};
        constexpr uint32_t kHeaderSize = 16;
        constexpr uint32_t kEntrySize = 8;
        constexpr uint32_t kMaxEntries = 0x1000000u;

        std::filesystem::path macroDir() {
            return Mod::get()->getSaveDir() / "macros";
        }

        std::filesystem::path macroPath(std::string const& clean) {
            return macroDir() / (clean + ".jb");
        }
    }

    std::string sanitizeName(std::string const& name) {
        std::string out;
        for (char c : name) {
            unsigned char u = static_cast<unsigned char>(c);
            if (std::isalnum(u) || c == '_' || c == '-' || c == ' ') out.push_back(c);
            if (out.size() >= 64) break;
        }
        size_t first = out.find_first_not_of(' ');
        if (first == std::string::npos) return {};
        size_t last = out.find_last_not_of(' ');
        return out.substr(first, last - first + 1);
    }

    bool saveMacro(std::string const& name) {
        auto clean = sanitizeName(name);
        if (clean.empty()) return false;

        std::error_code ec;
        std::filesystem::create_directories(macroDir(), ec);

        std::ofstream out(macroPath(clean), std::ios::binary | std::ios::trunc);
        if (!out) return false;

        uint32_t count = jbot_rep_count();
        float tps = jbot_rep_tps_get();
        uint32_t reserved = 0;

        out.write(kMagic, 4);
        out.write(reinterpret_cast<char const*>(&tps), 4);
        out.write(reinterpret_cast<char const*>(&count), 4);
        out.write(reinterpret_cast<char const*>(&reserved), 4);
        if (count > 0) {
            out.write(
                static_cast<char const*>(jbot_rep_data()),
                static_cast<std::streamsize>(count) * kEntrySize
            );
        }
        out.flush();
        return static_cast<bool>(out);
    }

    bool loadMacro(std::string const& name) {
        auto clean = sanitizeName(name);
        if (clean.empty()) return false;

        std::ifstream in(macroPath(clean), std::ios::binary | std::ios::ate);
        if (!in) return false;

        std::streamoff size = in.tellg();
        if (size < static_cast<std::streamoff>(kHeaderSize)) return false;
        in.seekg(0);

        std::vector<uint8_t> buf(static_cast<size_t>(size));
        if (!in.read(reinterpret_cast<char*>(buf.data()), size)) return false;

        if (std::memcmp(buf.data(), kMagic, 4) != 0) return false;

        float tps = 0.f;
        uint32_t count = 0;
        std::memcpy(&tps, buf.data() + 4, 4);
        std::memcpy(&count, buf.data() + 8, 4);

        if (count > kMaxEntries) return false;
        if (static_cast<uint64_t>(size) != kHeaderSize + static_cast<uint64_t>(count) * kEntrySize) return false;

        uint32_t prev = 0;
        for (uint32_t i = 0; i < count; i++) {
            uint32_t frame = 0;
            std::memcpy(&frame, buf.data() + kHeaderSize + static_cast<size_t>(i) * kEntrySize, 4);
            if (frame < prev) return false;
            prev = frame;
        }

        if (!(tps >= 1.f && tps <= 100000.f) || std::isnan(tps)) tps = 240.f;

        if (!jbot_rep_load(buf.data() + kHeaderSize, count)) return false;
        jbot_rep_tps_set(tps);
        return true;
    }
}
