#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>

#include "bridge/bridge.hpp"

#include <string_view>

@interface BlazeAppDelegate : NSObject <NSApplicationDelegate, WKScriptMessageHandlerWithReply, WKNavigationDelegate>
@property(nonatomic, strong) NSWindow *window;
@property(nonatomic, strong) WKWebView *webView;
@property(nonatomic, strong) NSURL *frontendURL;
@end

@implementation BlazeAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    const NSRect frame = NSMakeRect(0, 0, 1366, 850);
    self.window = [[NSWindow alloc] initWithContentRect:frame
        styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                  NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
        backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"Blaze IDE";

    WKWebViewConfiguration *configuration = [[WKWebViewConfiguration alloc] init];
    [configuration.userContentController addScriptMessageHandlerWithReply:self name:@"nativeBridge"];
    self.webView = [[WKWebView alloc] initWithFrame:frame configuration:configuration];
    self.webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.webView.navigationDelegate = self;
    self.window.contentView = self.webView;

    self.frontendURL = [[[NSBundle mainBundle] URLForResource:@"index"
        withExtension:@"html" subdirectory:@"frontend"] standardizedFileURL];
    if (self.frontendURL == nil) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Blaze IDE frontend is missing";
        alert.informativeText = @"Rebuild the application so its frontend resources are copied into the app bundle.";
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }
    [self.webView loadFileURL:self.frontendURL
        allowingReadAccessToURL:[self.frontendURL URLByDeletingLastPathComponent]];
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)userContentController:(WKUserContentController *)userContentController
          didReceiveScriptMessage:(WKScriptMessage *)message
                     replyHandler:(void (^)(id _Nullable reply, NSString * _Nullable errorMessage))replyHandler {
    (void)userContentController;
    if (![message.body isKindOfClass:[NSString class]]) {
        replyHandler(nil, @"Bridge messages must be strings.");
        return;
    }

    NSData *payload = [(NSString *)message.body dataUsingEncoding:NSUTF8StringEncoding];
    const std::string_view request(static_cast<const char *>(payload.bytes), payload.length);
    const auto response = blaze::bridge::handle_message(request);
    if (!response.has_value()) {
        replyHandler(nil, @"Malformed bridge request.");
        return;
    }
    NSString *result = [[NSString alloc] initWithBytes:response->data()
        length:response->size() encoding:NSUTF8StringEncoding];
    replyHandler(result, nil);
}

- (void)webView:(WKWebView *)webView
 decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction
 decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    (void)webView;
    NSURL *target = navigationAction.request.URL.standardizedFileURL;
    const BOOL trusted = target != nil && [target isEqual:self.frontendURL];
    decisionHandler(trusted ? WKNavigationActionPolicyAllow : WKNavigationActionPolicyCancel);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    (void)sender;
    return YES;
}

@end

int main(int argc, const char *argv[]) {
    (void)argc;
    (void)argv;
    @autoreleasepool {
        NSApplication *application = [NSApplication sharedApplication];
        application.activationPolicy = NSApplicationActivationPolicyRegular;
        BlazeAppDelegate *delegate = [[BlazeAppDelegate alloc] init];
        application.delegate = delegate;
        [application run];
    }
    return 0;
}
