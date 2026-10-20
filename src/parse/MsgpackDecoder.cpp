#include "MsgpackDecoder.hpp"

#include <cstring>

namespace cgv {

namespace {

constexpr size_t kMaxNestingDepth = 64;
constexpr size_t kMaxContainerEntries = 8'000'000;

class Cursor {
public:
    explicit Cursor(std::span<const uint8_t> data) : m_data(data) {}

    bool remaining(size_t count) const { return m_offset + count <= m_data.size(); }
    bool atEnd() const { return m_offset >= m_data.size(); }

    bool takeByte(uint8_t& out) {
        if (!remaining(1)) return false;
        out = m_data[m_offset++];
        return true;
    }

    bool takeUnsigned(size_t width, uint64_t& out) {
        if (!remaining(width)) return false;
        uint64_t value = 0;
        for (size_t i = 0; i < width; ++i) {
            value = (value << 8) | static_cast<uint64_t>(m_data[m_offset + i]);
        }
        m_offset += width;
        out = value;
        return true;
    }

    bool takeBytes(size_t count, const uint8_t*& out) {
        if (!remaining(count)) return false;
        out = m_data.data() + m_offset;
        m_offset += count;
        return true;
    }

    bool skip(size_t count) {
        if (!remaining(count)) return false;
        m_offset += count;
        return true;
    }

private:
    std::span<const uint8_t> m_data;
    size_t m_offset = 0;
};

bool decodeValue(Cursor& cursor, size_t depth, matjson::Value& out, std::string& error);

bool decodeString(Cursor& cursor, size_t length, matjson::Value& out, std::string& error) {
    const uint8_t* bytes = nullptr;
    if (!cursor.takeBytes(length, bytes)) {
        error = "Truncated string";
        return false;
    }
    out = std::string(reinterpret_cast<const char*>(bytes), length);
    return true;
}

bool decodeArray(Cursor& cursor, size_t count, size_t depth, matjson::Value& out, std::string& error) {
    if (count > kMaxContainerEntries) {
        error = "Array length out of range";
        return false;
    }
    std::vector<matjson::Value> items;
    items.reserve(count < 1024 ? count : 1024);
    for (size_t i = 0; i < count; ++i) {
        matjson::Value item;
        if (!decodeValue(cursor, depth + 1, item, error)) return false;
        items.push_back(std::move(item));
    }
    out = std::move(items);
    return true;
}

bool decodeMap(Cursor& cursor, size_t count, size_t depth, matjson::Value& out, std::string& error) {
    if (count > kMaxContainerEntries) {
        error = "Map length out of range";
        return false;
    }
    matjson::Value object = matjson::Value::object();
    for (size_t i = 0; i < count; ++i) {
        matjson::Value key;
        if (!decodeValue(cursor, depth + 1, key, error)) return false;
        matjson::Value value;
        if (!decodeValue(cursor, depth + 1, value, error)) return false;

        std::string keyText;
        if (key.isString()) {
            keyText = key.asString().unwrapOr("");
        } else if (key.isNumber()) {
            keyText = std::to_string(key.asDouble().unwrapOr(0.0));
        } else {
            continue;
        }
        object.set(keyText, std::move(value));
    }
    out = std::move(object);
    return true;
}

bool decodeFloat32(Cursor& cursor, matjson::Value& out, std::string& error) {
    uint64_t raw = 0;
    if (!cursor.takeUnsigned(4, raw)) {
        error = "Truncated float";
        return false;
    }
    uint32_t bits = static_cast<uint32_t>(raw);
    float value = 0.f;
    std::memcpy(&value, &bits, sizeof(value));
    out = static_cast<double>(value);
    return true;
}

bool decodeFloat64(Cursor& cursor, matjson::Value& out, std::string& error) {
    uint64_t bits = 0;
    if (!cursor.takeUnsigned(8, bits)) {
        error = "Truncated double";
        return false;
    }
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    out = value;
    return true;
}

bool decodeExtension(Cursor& cursor, size_t length, matjson::Value& out, std::string& error) {
    uint8_t type = 0;
    if (!cursor.takeByte(type) || !cursor.skip(length)) {
        error = "Truncated extension";
        return false;
    }
    out = matjson::Value(nullptr);
    return true;
}

bool decodeValue(Cursor& cursor, size_t depth, matjson::Value& out, std::string& error) {
    if (depth > kMaxNestingDepth) {
        error = "Nesting too deep";
        return false;
    }

    uint8_t tag = 0;
    if (!cursor.takeByte(tag)) {
        error = "Unexpected end of data";
        return false;
    }

    if (tag <= 0x7f) {
        out = static_cast<double>(tag);
        return true;
    }
    if (tag >= 0xe0) {
        out = static_cast<double>(static_cast<int8_t>(tag));
        return true;
    }
    if ((tag & 0xf0) == 0x80) return decodeMap(cursor, tag & 0x0f, depth, out, error);
    if ((tag & 0xf0) == 0x90) return decodeArray(cursor, tag & 0x0f, depth, out, error);
    if ((tag & 0xe0) == 0xa0) return decodeString(cursor, tag & 0x1f, out, error);

    uint64_t length = 0;
    switch (tag) {
        case 0xc0: out = matjson::Value(nullptr); return true;
        case 0xc2: out = false; return true;
        case 0xc3: out = true; return true;
        case 0xc4:
        case 0xc5:
        case 0xc6: {
            size_t width = tag == 0xc4 ? 1 : (tag == 0xc5 ? 2 : 4);
            if (!cursor.takeUnsigned(width, length) || !cursor.skip(static_cast<size_t>(length))) {
                error = "Truncated binary blob";
                return false;
            }
            out = matjson::Value(nullptr);
            return true;
        }
        case 0xc7:
        case 0xc8:
        case 0xc9: {
            size_t width = tag == 0xc7 ? 1 : (tag == 0xc8 ? 2 : 4);
            if (!cursor.takeUnsigned(width, length)) {
                error = "Truncated extension header";
                return false;
            }
            return decodeExtension(cursor, static_cast<size_t>(length), out, error);
        }
        case 0xca: return decodeFloat32(cursor, out, error);
        case 0xcb: return decodeFloat64(cursor, out, error);
        case 0xcc:
        case 0xcd:
        case 0xce:
        case 0xcf: {
            size_t width = static_cast<size_t>(1) << (tag - 0xcc);
            if (!cursor.takeUnsigned(width, length)) {
                error = "Truncated unsigned integer";
                return false;
            }
            out = static_cast<double>(length);
            return true;
        }
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3: {
            size_t width = static_cast<size_t>(1) << (tag - 0xd0);
            uint64_t raw = 0;
            if (!cursor.takeUnsigned(width, raw)) {
                error = "Truncated signed integer";
                return false;
            }
            int64_t value = 0;
            switch (width) {
                case 1: value = static_cast<int8_t>(raw); break;
                case 2: value = static_cast<int16_t>(raw); break;
                case 4: value = static_cast<int32_t>(raw); break;
                default: value = static_cast<int64_t>(raw); break;
            }
            out = static_cast<double>(value);
            return true;
        }
        case 0xd4:
        case 0xd5:
        case 0xd6:
        case 0xd7:
        case 0xd8: {
            size_t payload = static_cast<size_t>(1) << (tag - 0xd4);
            return decodeExtension(cursor, payload, out, error);
        }
        case 0xd9:
        case 0xda:
        case 0xdb: {
            size_t width = tag == 0xd9 ? 1 : (tag == 0xda ? 2 : 4);
            if (!cursor.takeUnsigned(width, length)) {
                error = "Truncated string header";
                return false;
            }
            return decodeString(cursor, static_cast<size_t>(length), out, error);
        }
        case 0xdc:
        case 0xdd: {
            size_t width = tag == 0xdc ? 2 : 4;
            if (!cursor.takeUnsigned(width, length)) {
                error = "Truncated array header";
                return false;
            }
            return decodeArray(cursor, static_cast<size_t>(length), depth, out, error);
        }
        case 0xde:
        case 0xdf: {
            size_t width = tag == 0xde ? 2 : 4;
            if (!cursor.takeUnsigned(width, length)) {
                error = "Truncated map header";
                return false;
            }
            return decodeMap(cursor, static_cast<size_t>(length), depth, out, error);
        }
        default:
            error = "Unknown msgpack tag";
            return false;
    }
}

} // namespace

MsgpackResult decodeMsgpack(std::span<const uint8_t> data) {
    if (data.empty()) return std::string("Empty msgpack payload");

    Cursor cursor(data);
    matjson::Value root;
    std::string error;
    if (!decodeValue(cursor, 0, root, error)) return error;

    return root;
}

} // namespace cgv
