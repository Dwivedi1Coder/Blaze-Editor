#include "bridge/bridge.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main() {
    const auto info = blaze::bridge::handle_message("7|app.getInfo");
    require(info.has_value(), "valid app.getInfo request should receive a response");
    require(info->find("\"id\":7") != std::string::npos, "response should preserve the request ID");
    require(info->find("\"name\":\"Blaze IDE\"") != std::string::npos, "response should identify Blaze IDE");
    require(info->find("\"ok\":true") != std::string::npos, "known operation should succeed");

    const auto unsupported = blaze::bridge::handle_message("8|process.run");
    require(unsupported.has_value(), "well-formed unknown operation should receive an error");
    require(unsupported->find("Unsupported operation") != std::string::npos, "unknown operation should be rejected");

    require(!blaze::bridge::handle_message("|app.getInfo"), "empty request ID should be rejected");
    require(!blaze::bridge::handle_message("0|app.getInfo"), "zero request ID should be rejected");
    require(!blaze::bridge::handle_message("1|app.getInfo|extra"), "extra frame delimiters should be rejected");
    require(!blaze::bridge::handle_message("12345678901|app.getInfo"), "oversized request ID should be rejected");
    require(!blaze::bridge::handle_message("nope|app.getInfo"), "non-numeric request ID should be rejected");
    require(!blaze::bridge::handle_message(std::string(257, 'x')), "oversized bridge message should be rejected");
    return EXIT_SUCCESS;
}
