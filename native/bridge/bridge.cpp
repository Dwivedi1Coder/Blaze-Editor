#include "bridge/bridge.hpp"

#include "app/application.hpp"

#include <charconv>
#include <string>

namespace blaze::bridge {
namespace {

std::string json_string(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (const char character : value) {
        switch (character) {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (static_cast<unsigned char>(character) < 0x20) {
                constexpr char digits[] = "0123456789abcdef";
                const auto byte = static_cast<unsigned char>(character);
                escaped += "\\u00";
                escaped.push_back(digits[byte >> 4]);
                escaped.push_back(digits[byte & 0x0f]);
            } else {
                escaped.push_back(character);
            }
        }
    }
    escaped.push_back('"');
    return escaped;
}

std::string success_response(unsigned int request_id) {
    const auto app = Application::info();
    return "{\"id\":" + std::to_string(request_id) +
        ",\"ok\":true,\"result\":{\"name\":" + json_string(app.name) +
        ",\"version\":" + json_string(app.version) +
        ",\"platform\":" + json_string(app.platform) + "}}";
}

std::string error_response(unsigned int request_id, std::string_view message) {
    return "{\"id\":" + std::to_string(request_id) +
        ",\"ok\":false,\"error\":" + json_string(message) + "}";
}

} // namespace

std::optional<std::string> handle_message(std::string_view message) {
    if (message.size() > 256) {
        return std::nullopt;
    }
    const auto separator = message.find('|');
    if (separator == std::string_view::npos || separator == 0 || separator > 10 ||
        message.find('|', separator + 1) != std::string_view::npos) {
        return std::nullopt;
    }

    unsigned int request_id = 0;
    const auto id_text = message.substr(0, separator);
    const auto parsed = std::from_chars(id_text.data(), id_text.data() + id_text.size(), request_id);
    if (parsed.ec != std::errc{} || parsed.ptr != id_text.data() + id_text.size() || request_id == 0) {
        return std::nullopt;
    }

    const auto operation = message.substr(separator + 1);
    if (operation == "app.getInfo") {
        return success_response(request_id);
    }
    return error_response(request_id, "Unsupported operation");
}

} // namespace blaze::bridge
