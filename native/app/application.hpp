#pragma once

#include <string>

namespace blaze {

struct ApplicationInfo {
    std::string name;
    std::string version;
    std::string platform;
};

class Application final {
public:
    [[nodiscard]] static ApplicationInfo info();
};

} // namespace blaze
