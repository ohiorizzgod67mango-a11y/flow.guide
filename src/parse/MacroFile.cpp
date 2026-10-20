#include "MacroFile.hpp"

#include "parse/Gdr2Parser.hpp"
#include "parse/JsonMacroParser.hpp"
#include "parse/MsgpackDecoder.hpp"
#include "parse/NestingGuard.hpp"
#include "parse/ZbotParser.hpp"

#include <cctype>
#include <cstdint>
#include <fstream>
#include <span>
#include <system_error>
#include <vector>

namespace cgv {

namespace {

constexpr size_t kMagicLength = 3;

enum class MacroFlavour {
    Gdr2,
    Json,
    Zbot,
    LegacyMsgpack,
};

bool startsWithGdrMagic(std::vector<uint8_t> const& bytes) {
    if (bytes.size() < kMagicLength) return false;
    return bytes[0] == 'G' && bytes[1] == 'D' && bytes[2] == 'R';
}

bool startsWithJsonToken(std::vector<uint8_t> const& bytes) {
    for (uint8_t byte : bytes) {
        if (std::isspace(static_cast<unsigned char>(byte))) continue;
        return byte == '{' || byte == '[';
    }
    return false;
}

MacroFlavour sniffFlavour(std::vector<uint8_t> const& bytes) {
    if (startsWithGdrMagic(bytes)) return MacroFlavour::Gdr2;
    if (startsWithJsonToken(bytes)) return MacroFlavour::Json;
    if (looksLikeZbot(std::span<const uint8_t>(bytes.data(), bytes.size()))) return MacroFlavour::Zbot;
    return MacroFlavour::LegacyMsgpack;
}

std::variant<std::vector<uint8_t>, std::string> readAllBytes(std::filesystem::path const& path) {
    std::error_code code;

    if (!std::filesystem::exists(path, code) || code) {
        return std::string("File does not exist");
    }
    if (std::filesystem::is_directory(path, code)) {
        return std::string("Path is a directory, not a macro file");
    }

    auto size = std::filesystem::file_size(path, code);
    if (code) return std::string("Could not determine file size");
    if (size == 0) return std::string("File is empty");
    if (size > kMaxMacroFileBytes) return std::string("File is too large to be a macro");

    std::ifstream stream(path, std::ios::binary);
    if (!stream) return std::string("Could not open file for reading");

    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    if (stream.gcount() != static_cast<std::streamsize>(size)) {
        return std::string("File read was truncated");
    }

    return bytes;
}

MacroLoadResult parseLegacyMsgpack(std::vector<uint8_t> const& bytes) {
    auto decoded = decodeMsgpack(std::span<const uint8_t>(bytes.data(), bytes.size()));
    if (auto* error = std::get_if<std::string>(&decoded)) {
        return std::string("Not a recognised macro format (" + *error + ")");
    }

    auto parsed = parseJsonMacro(std::get<matjson::Value>(decoded));
    if (auto* replay = std::get_if<ReplayData>(&parsed)) {
        if (replay->botName == "JSON") replay->botName = "GDR";
        return std::move(*replay);
    }
    return std::get<std::string>(parsed);
}

MacroLoadResult parseJsonBytes(std::vector<uint8_t> const& bytes) {
    if (!jsonNestingWithinLimit(bytes)) {
        return std::string("Macro is nested too deeply to be read safely");
    }

    std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return std::visit([](auto&& value) -> MacroLoadResult { return std::move(value); },
                      parseJsonMacroText(text));
}

MacroLoadResult dispatchByFlavour(MacroFlavour flavour, std::vector<uint8_t> const& bytes) {
    switch (flavour) {
        case MacroFlavour::Gdr2:
            return std::visit([](auto&& value) -> MacroLoadResult { return std::move(value); },
                              parseGdr2(std::span<const uint8_t>(bytes.data(), bytes.size())));
        case MacroFlavour::Json:
            return parseJsonBytes(bytes);
        case MacroFlavour::Zbot:
            return std::visit([](auto&& value) -> MacroLoadResult { return std::move(value); },
                              parseZbot(std::span<const uint8_t>(bytes.data(), bytes.size())));
        case MacroFlavour::LegacyMsgpack:
            return parseLegacyMsgpack(bytes);
    }
    return std::string("Unsupported macro format");
}

} // namespace

MacroLoadResult loadMacroFile(std::filesystem::path const& path) {
    auto read = readAllBytes(path);
    if (auto* error = std::get_if<std::string>(&read)) return *error;

    auto const& bytes = std::get<std::vector<uint8_t>>(read);
    auto result = dispatchByFlavour(sniffFlavour(bytes), bytes);

    if (auto* replay = std::get_if<ReplayData>(&result)) {
        replay->sourceName = path.filename().string();
    }
    return result;
}

} // namespace cgv
