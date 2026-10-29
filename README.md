# Blaze IDE

Blaze IDE is a lightweight native desktop shell for a future C/C++ development environment. The first milestone establishes the cross-platform host, a deliberately narrow native bridge, and a small TypeScript interface. It does not yet include an editor, filesystem access, compiler integration, or project management.

## Architecture

- Shared application and bridge logic is standard C++20 with no third-party runtime dependency.
- Windows uses a Win32 host and the installed Microsoft WebView2 Runtime. It links against the WebView2 SDK's static loader library; the SDK itself is not copied into the application.
- macOS uses a native AppKit host and WKWebView, with only the platform entry point written in Objective-C++.
- The frontend is HTML, CSS, and TypeScript compiled to a small browser script. Node.js is a development/build prerequisite only, never an application runtime.
- The bridge accepts a framed string protocol and exposes only the explicitly allowlisted `app.getInfo` operation. It grants no filesystem or process access.

See [docs/architecture.md](docs/architecture.md) for the bridge contract and [docs/building.md](docs/building.md) for exact platform setup and build commands.

## Project tree

```text
CMakeLists.txt
README.md
docs/
	architecture.md
	building.md
frontend/
	index.html
	package.json
	tsconfig.json
	src/main.ts
	src/styles.css
native/
	app/application.cpp
	app/application.hpp
	bridge/bridge.cpp
	bridge/bridge.hpp
	platform/windows/main.cpp
	platform/macos/main.mm
tests/
	unit/bridge_test.cpp
```

## Milestone 1 checklist

- [x] CMake project and shared C++20 application layer
- [x] Windows Win32/WebView2 host source
- [x] macOS AppKit/WKWebView host source
- [x] Validated, operation-allowlisted native bridge
- [x] Focused native bridge unit test
- [x] Responsive IDE shell and persistent dark/light theme switch
- [ ] Native builds verified on Windows and macOS (requires each platform's SDK/toolchain)
- [ ] Editor, project explorer, filesystem operations, toolchains, build/run/debug, SDL2, LSP, terminal, Git, and Gemini (later milestones)

## Status

The host implementations are source scaffolding, not verified platform binaries. This environment currently has CMake but does not have Node.js on PATH, so frontend compilation and a complete native build have not been run here. The app does not claim that Windows validation implies macOS validation, or vice versa.
