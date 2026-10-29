#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace blaze::bridge {

// Request wire format: decimal-request-id|operation-name.
// A response is a UTF-8 JSON string, or nullopt for a malformed frame.
[[nodiscard]] std::optional<std::string> handle_message(std::string_view message);

} // namespace blaze::bridge
