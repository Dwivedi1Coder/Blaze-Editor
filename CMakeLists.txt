cmake_minimum_required(VERSION 3.21)
project(BlazeIDE VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(BLAZE_FRONTEND_DIST "${CMAKE_CURRENT_SOURCE_DIR}/frontend/dist/main.js")
if(NOT EXISTS "${BLAZE_FRONTEND_DIST}")
    message(FATAL_ERROR "Frontend output is missing. Run `npm install` and `npm run build` in frontend/ first.")
endif()

add_library(blaze_core STATIC
    native/app/application.cpp
    native/bridge/bridge.cpp
)
target_include_directories(blaze_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/native")

if(BUILD_TESTING)
    add_executable(blaze_bridge_tests tests/unit/bridge_test.cpp)
    target_link_libraries(blaze_bridge_tests PRIVATE blaze_core)
    add_test(NAME blaze_bridge_tests COMMAND blaze_bridge_tests)
endif()

if(WIN32)
    set(WEBVIEW2_SDK_ROOT "" CACHE PATH "Root of the extracted Microsoft.Web.WebView2 NuGet package")
    if(NOT WEBVIEW2_SDK_ROOT)
        message(FATAL_ERROR "Set WEBVIEW2_SDK_ROOT to the extracted Microsoft.Web.WebView2 package directory. See docs/building.md.")
    endif()
    find_path(WEBVIEW2_INCLUDE_DIR WebView2.h
        HINTS "${WEBVIEW2_SDK_ROOT}/build/native/include"
        REQUIRED
    )
    find_library(WEBVIEW2_LOADER_LIBRARY
        NAMES WebView2LoaderStatic
        HINTS
            "${WEBVIEW2_SDK_ROOT}/build/native/${CMAKE_GENERATOR_PLATFORM}"
            "${WEBVIEW2_SDK_ROOT}/build/native/x64"
            "${WEBVIEW2_SDK_ROOT}/build/native/arm64"
            "${WEBVIEW2_SDK_ROOT}/build/native/x86"
        REQUIRED
    )
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/frontend")
    configure_file(frontend/index.html "${CMAKE_CURRENT_BINARY_DIR}/frontend/index.html" COPYONLY)
    configure_file(frontend/src/styles.css "${CMAKE_CURRENT_BINARY_DIR}/frontend/styles.css" COPYONLY)
    configure_file("${BLAZE_FRONTEND_DIST}" "${CMAKE_CURRENT_BINARY_DIR}/frontend/main.js" COPYONLY)

    add_executable(blaze_ide WIN32 native/platform/windows/main.cpp)
    target_include_directories(blaze_ide PRIVATE "${WEBVIEW2_INCLUDE_DIR}")
    target_compile_definitions(blaze_ide PRIVATE
        BLAZE_FRONTEND_DIR=L"${CMAKE_CURRENT_BINARY_DIR}/frontend"
    )
    target_link_libraries(blaze_ide PRIVATE
        blaze_core
        "${WEBVIEW2_LOADER_LIBRARY}"
        ole32
        shell32
        shlwapi
    )
elseif(APPLE)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    find_library(WEBKIT_FRAMEWORK WebKit REQUIRED)

    add_executable(blaze_ide MACOSX_BUNDLE native/platform/macos/main.mm)
    target_compile_options(blaze_ide PRIVATE
        $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
    )
    target_link_libraries(blaze_ide PRIVATE
        blaze_core
        "${APPKIT_FRAMEWORK}"
        "${WEBKIT_FRAMEWORK}"
    )
    set_target_properties(blaze_ide PROPERTIES
        OUTPUT_NAME "Blaze IDE"
        MACOSX_BUNDLE_BUNDLE_NAME "Blaze IDE"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.blaze-ide.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
    add_custom_command(TARGET blaze_ide POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_DIR:blaze_ide>/Contents/Resources/frontend"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/frontend/index.html"
            "$<TARGET_BUNDLE_DIR:blaze_ide>/Contents/Resources/frontend/index.html"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/frontend/src/styles.css"
            "$<TARGET_BUNDLE_DIR:blaze_ide>/Contents/Resources/frontend/styles.css"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${BLAZE_FRONTEND_DIST}"
            "$<TARGET_BUNDLE_DIR:blaze_ide>/Contents/Resources/frontend/main.js"
        VERBATIM
    )
else()
    message(FATAL_ERROR "Blaze IDE desktop targets are Windows and macOS only.")
endif()
