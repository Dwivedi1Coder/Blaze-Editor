#define UNICODE
#define _UNICODE

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <WebView2.h>
#include <wrl.h>

#include "bridge/bridge.hpp"

#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

constexpr wchar_t window_class_name[] = L"BlazeIDEWindow";
ComPtr<ICoreWebView2Controller> webview_controller;
ComPtr<ICoreWebView2> webview;

std::string to_utf8(std::wstring_view value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring to_wide(std::string_view value) {
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) return {};
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

void show_host_error(HWND window, const wchar_t* message) {
    MessageBoxW(window, message, L"Blaze IDE", MB_OK | MB_ICONERROR);
}

std::wstring webview_user_data_path() {
    PWSTR local_app_data = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &local_app_data))) {
        return {};
    }
    std::filesystem::path path(local_app_data);
    CoTaskMemFree(local_app_data);
    path /= L"BlazeIDE";
    path /= L"WebView2";
    std::error_code error;
    std::filesystem::create_directories(path, error);
    return error ? std::wstring{} : path.wstring();
}

std::wstring frontend_url(HWND window) {
    const auto index_path = (std::filesystem::path(BLAZE_FRONTEND_DIR) / L"index.html").wstring();
    wchar_t url[32768]{};
    DWORD url_length = static_cast<DWORD>(std::size(url));
    if (UrlCreateFromPathW(index_path.c_str(), url, &url_length, 0) != S_OK) {
        show_host_error(window, L"Could not create a URL for the Blaze IDE frontend.");
        return {};
    }
    return url;
}

void navigate_to_frontend(HWND window) {
    const auto url = frontend_url(window);
    if (url.empty()) return;

    webview->add_NavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [url](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                LPWSTR requested_url = nullptr;
                if (FAILED(args->get_Uri(&requested_url))) {
                    args->put_Cancel(TRUE);
                    return S_OK;
                }
                const bool trusted = url == requested_url;
                CoTaskMemFree(requested_url);
                if (!trusted) args->put_Cancel(TRUE);
                return S_OK;
            }).Get(), nullptr);

    webview->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                LPWSTR message = nullptr;
                if (FAILED(args->TryGetWebMessageAsString(&message))) return S_OK;
                const auto response = blaze::bridge::handle_message(to_utf8(message));
                CoTaskMemFree(message);
                if (response) {
                    const auto response_wide = to_wide(*response);
                    if (!response_wide.empty()) webview->PostWebMessageAsString(response_wide.c_str());
                }
                return S_OK;
            }).Get(), nullptr);
    webview->Navigate(url.c_str());
}

void initialize_webview(HWND window) {
    const auto user_data_path = webview_user_data_path();
    if (user_data_path.empty()) {
        show_host_error(window, L"Blaze IDE could not create its per-user WebView2 data directory.");
        return;
    }
    const HRESULT result = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, user_data_path.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [window](HRESULT status, ICoreWebView2Environment* environment) -> HRESULT {
                if (FAILED(status) || environment == nullptr) {
                    if (IsWindow(window)) {
                        show_host_error(window,
                            L"WebView2 could not start. Install the Microsoft Edge WebView2 Runtime, then restart Blaze IDE.");
                    }
                    return S_OK;
                }
                environment->CreateCoreWebView2Controller(
                    window,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [window](HRESULT controller_status, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(controller_status) || controller == nullptr) {
                                if (IsWindow(window)) show_host_error(window, L"Blaze IDE could not create its WebView2 controller.");
                                return S_OK;
                            }
                            if (!IsWindow(window)) {
                                controller->Close();
                                return S_OK;
                            }
                            webview_controller = controller;
                            webview_controller->get_CoreWebView2(&webview);
                            ComPtr<ICoreWebView2Settings> settings;
                            if (SUCCEEDED(webview->get_Settings(&settings))) {
                                settings->put_AreDevToolsEnabled(FALSE);
                                settings->put_IsStatusBarEnabled(FALSE);
                            }
                            RECT bounds{};
                            GetClientRect(window, &bounds);
                            webview_controller->put_Bounds(bounds);
                            navigate_to_frontend(window);
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());

    if (FAILED(result)) show_host_error(window, L"Blaze IDE could not initialize WebView2.");
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:
        if (webview_controller) {
            RECT bounds{};
            GetClientRect(window, &bounds);
            webview_controller->put_Bounds(bounds);
        }
        return 0;
    case WM_DESTROY:
        webview.Reset();
        if (webview_controller) webview_controller->Close();
        webview_controller.Reset();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wparam, lparam);
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) return 1;

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.hInstance = instance;
    window_class.lpfnWndProc = window_proc;
    window_class.lpszClassName = window_class_name;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassExW(&window_class)) {
        CoUninitialize();
        return 1;
    }

    HWND window = CreateWindowExW(0, window_class_name, L"Blaze IDE",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1366, 850,
        nullptr, nullptr, instance, nullptr);
    if (window == nullptr) {
        CoUninitialize();
        return 1;
    }

    ShowWindow(window, show_command);
    initialize_webview(window);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    CoUninitialize();
    return static_cast<int>(message.wParam);
}
