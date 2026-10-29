# Building Blaze IDE

## Frontend (both platforms)

Install Node.js LTS for development, then compile the static TypeScript frontend:

```sh
cd frontend
npm install
npm run build
cd ..
```

The app loads `frontend/dist/main.js`; CMake intentionally fails if it has not been generated. Node.js is not needed to run the desktop app after compilation.

## Windows

Prerequisites: Visual Studio 2022 with the Desktop development with C++ workload and Windows SDK, CMake 3.21+, Node.js, and the Microsoft.Web.WebView2 SDK NuGet package. The WebView2 Runtime must be installed on the machine that runs Blaze IDE.

Download/extract the WebView2 SDK package using the NuGet workflow approved by your organization. Set `WEBVIEW2_SDK_ROOT` to the extracted package root, which contains `build/native/include/WebView2.h` and `build/native/x64/WebView2LoaderStatic.lib` (or the matching architecture library).

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DWEBVIEW2_SDK_ROOT="C:/path/to/Microsoft.Web.WebView2"
cmake --build build --config Release
./build/Release/blaze_ide.exe
ctest --test-dir build -C Release --output-on-failure
```

If the WebView2 Runtime is missing, the host displays a setup error and does not fall back to another browser. Use the official Microsoft Edge WebView2 Runtime installer after user approval.

## macOS

Prerequisites: macOS 11 or later, Xcode Command Line Tools (including the macOS SDK), CMake 3.21+, and Node.js. The system AppKit and WebKit frameworks are used; no WebView framework download is required.

```sh
cmake -S . -B build -G Xcode
cmake --build build --config Release
open build/Release/Blaze\ IDE.app
ctest --test-dir build -C Release --output-on-failure
```

For a command-line generator, use `cmake -S . -B build -G Ninja` followed by `cmake --build build`; launch the generated `.app` bundle under the build tree. Code signing and notarization are release-stage work and are not configured in this milestone.

## Verification status

The commands above are setup guidance, not claimed test results. Windows must be built and launched on Windows with the WebView2 SDK/runtime; macOS must be built and launched on macOS with Xcode tools. Neither host has been built in the current environment. No cross-platform build result should be inferred from the source files alone.
