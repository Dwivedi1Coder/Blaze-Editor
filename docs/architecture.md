# Architecture

## Milestone 1 boundaries

`blaze_core` is platform-independent C++20 and owns application identity plus bridge request validation. The Windows host owns Win32 window/WebView2 lifecycle; the macOS host owns AppKit/WKWebView lifecycle. The frontend is static HTML/CSS and TypeScript compiled to JavaScript. Node.js is only used to compile the frontend.

WebView navigation remains on the packaged local frontend. The frontend does not receive arbitrary native access: only the named bridge operations are dispatched by the shared core. Filesystem, process, URL-opening, and settings-write operations are intentionally absent in this milestone.

## Bridge contract

Requests are UTF-8 strings with the form `decimal-id|operation`. IDs must be positive decimal integers containing at most ten digits. The only supported operation is `app.getInfo`; any other well-formed operation returns an explicit unsupported-operation response. Malformed frames receive no Windows response and a rejected WebKit reply. The result uses JSON and contains the application name, version, and host platform.

The frontend checks the response shape and correlates Windows messages by request ID. WebKit's reply handler returns the same serialized response directly. New bridge operations must be added to the shared allowlist and validated before the platform adapter exposes them.

## Dependencies and trade-offs

- C++20 and CMake provide the shared native base without imposing a third-party runtime.
- WebView2 uses the installed Windows runtime; the app links the SDK's static loader. The SDK is a Windows build prerequisite, not a bundled browser.
- WKWebView and AppKit are system frameworks. The macOS entry point uses Objective-C++ only where Cocoa APIs require it. The reply-capable WebKit bridge requires macOS 11 or later.
- TypeScript has one development dependency, the TypeScript compiler. The built app runs emitted JavaScript in the system WebView; Node.js is not an application runtime.
- No editor widget is selected yet. Editor weight should be measured in the editor milestone before choosing Monaco or a smaller component.

## Lifecycle and security notes

Each host creates one native window and one WebView, loads only the packaged local frontend, and releases or terminates with the native application lifecycle. The bridge accepts string messages only and uses an operation allowlist. It does not evaluate frontend-provided code or launch commands. Further navigation restrictions, CSP, and a richer typed protocol should be added before introducing filesystem or process capabilities.

Windows stores its WebView2 profile under the current user's Local AppData directory rather than beside the executable, so browser-backed preferences do not require write access to the install location. WKWebView uses its persistent system-managed data store.
