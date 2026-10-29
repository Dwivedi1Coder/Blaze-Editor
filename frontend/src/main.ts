type AppInfo = {
  name: string;
  version: string;
  platform: string;
};

type BridgeResponse = {
  id: number;
  ok: boolean;
  result?: AppInfo;
  error?: string;
};

type WebViewMessageEvent = MessageEvent<string>;

declare global {
  interface Window {
    chrome?: {
      webview?: {
        postMessage(message: string): void;
        addEventListener(type: "message", listener: (event: WebViewMessageEvent) => void): void;
      };
    };
    webkit?: {
      messageHandlers?: {
        nativeBridge?: {
          postMessage(message: string): Promise<string>;
        };
      };
    };
  }
}

const pendingRequests = new Map<number, (response: BridgeResponse) => void>();
let nextRequestId = 1;

function parseResponse(value: string): BridgeResponse {
  const parsed: unknown = JSON.parse(value);
  if (typeof parsed !== "object" || parsed === null || !("id" in parsed) || !("ok" in parsed)) {
    throw new Error("Native bridge returned an invalid response.");
  }
  const response = parsed as BridgeResponse;
  if (typeof response.id !== "number" || typeof response.ok !== "boolean") {
    throw new Error("Native bridge response fields are invalid.");
  }
  return response;
}

function invokeNative(operation: "app.getInfo"): Promise<AppInfo> {
  const requestId = nextRequestId++;
  const payload = `${requestId}|${operation}`;
  const webview = window.chrome?.webview;
  const webkit = window.webkit?.messageHandlers?.nativeBridge;

  if (webview) {
    return new Promise<AppInfo>((resolve, reject) => {
      const timeout = window.setTimeout(() => {
        pendingRequests.delete(requestId);
        reject(new Error("Native bridge timed out."));
      }, 4000);
      pendingRequests.set(requestId, (response) => {
        window.clearTimeout(timeout);
        if (!response.ok || response.result === undefined) {
          reject(new Error(response.error ?? "Native request failed."));
          return;
        }
        resolve(response.result);
      });
      webview.postMessage(payload);
    });
  }

  if (webkit) {
    return webkit.postMessage(payload).then((value) => {
      const response = parseResponse(value);
      if (!response.ok || response.result === undefined) {
        throw new Error(response.error ?? "Native request failed.");
      }
      return response.result;
    });
  }

  return Promise.reject(new Error("Blaze IDE native bridge is unavailable."));
}

window.chrome?.webview?.addEventListener("message", (event) => {
  try {
    const response = parseResponse(event.data);
    const resolveRequest = pendingRequests.get(response.id);
    if (resolveRequest) {
      pendingRequests.delete(response.id);
      resolveRequest(response);
    }
  } catch {
    setBridgeStatus("NATIVE BRIDGE RESPONSE INVALID", false);
  }
});

function setBridgeStatus(label: string, connected: boolean): void {
  const status = document.getElementById("panel-status");
  const pip = document.querySelector<HTMLElement>(".state-pip");
  if (status) status.textContent = label;
  pip?.classList.toggle("connected", connected);
}

function initializeTheme(): void {
  const root = document.documentElement;
  const toggle = document.getElementById("theme-toggle");
  let theme = "dark";
  try {
    const storedTheme = localStorage.getItem("blaze.theme");
    if (storedTheme === "dark" || storedTheme === "light") theme = storedTheme;
  } catch {
    // Theme switching remains available when storage is disabled.
  }

  const applyTheme = (): void => {
    root.dataset.theme = theme;
    toggle?.setAttribute("aria-label", `Switch to ${theme === "dark" ? "light" : "dark"} theme`);
    toggle?.setAttribute("title", `Switch to ${theme === "dark" ? "light" : "dark"} theme`);
  };

  applyTheme();
  toggle?.addEventListener("click", () => {
    theme = theme === "dark" ? "light" : "dark";
    applyTheme();
    try {
      localStorage.setItem("blaze.theme", theme);
    } catch {
      // The selected theme still applies for the current session.
    }
  });
}

function initializePanelTabs(): void {
  const message = document.getElementById("panel-message");
  document.querySelectorAll<HTMLButtonElement>(".bottom-tab").forEach((tab) => {
    tab.addEventListener("click", () => {
      document.querySelectorAll(".bottom-tab").forEach((item) => {
        item.classList.remove("selected");
        item.setAttribute("aria-selected", "false");
      });
      tab.classList.add("selected");
      tab.setAttribute("aria-selected", "true");
      if (message) {
        message.textContent = tab.dataset.panel === "problems"
          ? "No compiler diagnostics are available before a project build."
          : "No build has run. Compiler output will appear here in a later milestone.";
      }
    });
  });
}

async function initializeNativeStatus(): Promise<void> {
  try {
    const info = await invokeNative("app.getInfo");
    const version = document.getElementById("app-version");
    const platform = document.getElementById("platform-label");
    if (version) version.textContent = `${info.name} · ${info.version}`;
    if (platform) platform.textContent = info.platform === "windows" ? "Windows" : "macOS";
    setBridgeStatus("NATIVE BRIDGE CONNECTED", true);
  } catch {
    setBridgeStatus("NATIVE BRIDGE UNAVAILABLE", false);
  }
}

initializeTheme();
initializePanelTabs();
void initializeNativeStatus();
