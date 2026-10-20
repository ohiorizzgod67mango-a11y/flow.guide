#include "JsonMacroParser.hpp"

#include "parse/Framerate.hpp"
#include "parse/JsonAccess.hpp"

#include <algorithm>
#include <cmath>

namespace cgv {

namespace {

constexpr size_t kMaxInputs = 4'000'000;

const matjson::Value* findInputArray(matjson::Value const& root) {
    if (root.isArray()) return &root;
    static constexpr std::initializer_list<const char*> kArrayKeys = {
        "inputs", "macro", "events", "actions", "clicks", "replay", "data", "frames"
    };
    if (auto* found = json::firstMember(root, kArrayKeys)) {
        if (found->isArray()) return found;
    }
    return nullptr;
}

bool readHoldFlag(matjson::Value const& entry) {
    static constexpr std::initializer_list<const char*> kDownKeys = {
        "down", "hold", "isDown", "pressed", "press", "holding", "state"
    };
    if (auto* found = json::firstMember(entry, kDownKeys)) {
        return json::toBool(*found, false);
    }
    return false;
}

bool readPlayer2Flag(matjson::Value const& entry) {
    static constexpr std::initializer_list<const char*> kP2Keys = {
        "player2", "p2", "isPlayer2", "second", "player_2"
    };
    if (auto* found = json::firstMember(entry, kP2Keys)) {
        return json::toBool(*found, false);
    }
    static constexpr std::initializer_list<const char*> kP1Keys = { "player1", "p1", "isPlayer1" };
    if (auto* found = json::firstMember(entry, kP1Keys)) {
        return !json::toBool(*found, true);
    }
    return false;
}

uint8_t readButton(matjson::Value const& entry) {
    static constexpr std::initializer_list<const char*> kButtonKeys = { "button", "btn", "key", "action" };
    double raw = json::numberAt(entry, kButtonKeys, 1.0);
    int rounded = static_cast<int>(std::lround(raw));
    if (rounded < 1 || rounded > 3) rounded = 1;
    return static_cast<uint8_t>(rounded);
}

uint64_t readFrame(matjson::Value const& entry, bool& ok) {
    static constexpr std::initializer_list<const char*> kFrameKeys = {
        "frame", "f", "step", "tick", "x", "time"
    };
    auto* found = json::firstMember(entry, kFrameKeys);
    if (!found) {
        ok = false;
        return 0;
    }
    double raw = json::toNumber(*found, -1.0);
    if (!std::isfinite(raw) || raw < 0.0) {
        ok = false;
        return 0;
    }
    ok = true;
    return static_cast<uint64_t>(raw);
}

bool readPosition(matjson::Value const& entry, float& xOut, float& yOut) {
    static constexpr std::initializer_list<const char*> kXKeys = { "xPos", "x_position", "xPosition", "posX" };
    static constexpr std::initializer_list<const char*> kYKeys = { "yPos", "y_position", "yPosition", "posY" };

    const matjson::Value* xNode = json::firstMember(entry, kXKeys);
    const matjson::Value* yNode = json::firstMember(entry, kYKeys);

    if (!xNode || !yNode) {
        if (auto* nested = json::firstMember(entry, { "position", "pos", "physics", "phys" })) {
            if (!xNode) xNode = json::firstMember(*nested, { "x", "xPos", "xPosition" });
            if (!yNode) yNode = json::firstMember(*nested, { "y", "yPos", "yPosition" });
        }
    }

    if (!xNode || !yNode) return false;

    double x = json::toNumber(*xNode, 0.0);
    double y = json::toNumber(*yNode, 0.0);
    if (!std::isfinite(x) || !std::isfinite(y)) return false;

    xOut = static_cast<float>(x);
    yOut = static_cast<float>(y);
    return true;
}

double readDeclaredRate(matjson::Value const& root) {
    static constexpr std::initializer_list<const char*> kRateKeys = {
        "framerate", "fps", "tps", "tickRate", "frameRate", "FPS", "physicsFPS"
    };
    if (auto* found = json::firstMember(root, kRateKeys)) {
        return json::toNumber(*found, 0.0);
    }
    if (auto* meta = json::firstMember(root, { "meta", "info", "header", "settings" })) {
        return json::numberAt(*meta, kRateKeys, 0.0);
    }
    return 0.0;
}

std::string readBotName(matjson::Value const& root) {
    static constexpr std::initializer_list<const char*> kBotKeys = { "bot", "botName", "botInfo", "author", "creator" };
    if (auto* found = json::firstMember(root, kBotKeys)) {
        if (found->isObject()) return json::textAt(*found, { "name", "bot" }, "JSON");
        return json::toText(*found, "JSON");
    }
    return "JSON";
}

void appendEntry(matjson::Value const& entry, ReplayData& out) {
    if (!entry.isObject()) return;

    bool frameOk = false;
    uint64_t frame = readFrame(entry, frameOk);
    if (!frameOk) return;

    ReplayInput input;
    input.frame = frame;
    input.down = readHoldFlag(entry);
    input.player2 = readPlayer2Flag(entry);
    input.button = readButton(entry);
    input.hasPosition = readPosition(entry, input.xPos, input.yPos);

    out.inputs.push_back(input);
}

} // namespace

JsonParseResult parseJsonMacro(matjson::Value const& root) {
    const matjson::Value* array = findInputArray(root);
    if (!array) return std::string("No input array found in JSON macro");

    auto entries = array->asArray();
    if (!entries) return std::string("Macro input list is not an array");

    auto const& list = entries.unwrap();
    if (list.empty()) return std::string("Macro contains no inputs");
    if (list.size() > kMaxInputs) return std::string("Macro input count is implausibly large");

    ReplayData data;
    data.inputs.reserve(list.size());
    for (auto const& entry : list) {
        appendEntry(entry, data);
    }

    if (data.inputs.empty()) return std::string("No usable inputs in macro");

    std::stable_sort(data.inputs.begin(), data.inputs.end(), [](ReplayInput const& a, ReplayInput const& b) {
        return a.frame < b.frame;
    });

    data.botName = readBotName(root);
    data.duration = static_cast<float>(json::numberAt(root, { "duration", "length", "seconds" }, 0.0));
    data.framerate = recoverTickRate(readDeclaredRate(root), data.inputs.back().frame, data.duration);
    data.recomputePositionCoverage();

    return data;
}

JsonParseResult parseJsonMacroText(std::string const& text) {
    auto parsed = matjson::parse(text);
    if (!parsed) return std::string("Invalid JSON: " + parsed.unwrapErr().message);
    return parseJsonMacro(parsed.unwrap());
}

} // namespace cgv
