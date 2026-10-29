#include "app/application.hpp"

namespace blaze {

ApplicationInfo Application::info() {
#if defined(_WIN32)
    constexpr auto platform = "windows";
#elif defined(__APPLE__)
    constexpr auto platform = "macos";
#else
    constexpr auto platform = "unsupported";
#endif
    return {"Blaze IDE", "0.1.0", platform};
}

} // namespace blaze
