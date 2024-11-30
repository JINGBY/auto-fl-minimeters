#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <string>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

// Process names
const wchar_t* flStudioProcessName = L"FL64.exe";
const wchar_t* miniMetersProcessName = L"MiniMeters.exe";

NOTIFYICONDATAW nid;
HWND hwnd;
UINT WM_TASKBAR = 0;

// Logs a message to the console
void Log(const std::string& message) {
    std::cout << message << std::endl;
}

// Retrieves the process ID of a given process name
DWORD GetProcessId(const wchar_t* processName) {
    DWORD processId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (_wcsicmp(pe32.szExeFile, processName) == 0) {
                    processId = pe32.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    return processId;
}

// Checks if a process with the given name is running
bool IsProcessRunning(const wchar_t* processName) {
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return false;
    }
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    if (!Process32FirstW(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return false;
    }
    do {
        if (wcscmp(pe32.szExeFile, processName) == 0) {
            CloseHandle(hProcessSnap);
            return true;
        }
    } while (Process32NextW(hProcessSnap, &pe32));
    CloseHandle(hProcessSnap);
    return false;
}

// Check if FL Studio window is the active window
bool IsFLStudioWindowActive() {
    HWND foreground = GetForegroundWindow();
    if (!foreground) return false;

    DWORD foregroundProcessId;
    GetWindowThreadProcessId(foreground, &foregroundProcessId);

    return foregroundProcessId == GetProcessId(flStudioProcessName);
}

// Manage MiniMeters window positioning and visibility
void ManageMiniMetersWindow() {
    HWND miniMetersWindow = FindWindowW(NULL, L"MiniMeters");

    if (miniMetersWindow) {
        if (IsFLStudioWindowActive()) {
            // Show and set to top-most when FL Studio is active
            ShowWindow(miniMetersWindow, SW_SHOW);
            SetWindowPos(miniMetersWindow, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE);
            Log("MiniMeters set to top of FL Studio");
        }
        else {
            // Hide when another window is active
            ShowWindow(miniMetersWindow, SW_HIDE);
            Log("MiniMeters hidden");
        }
    }
}

// Starts MiniMeters application if it's not already running
void StartMiniMeters() {
    if (GetProcessId(miniMetersProcessName) == 0) {
        system("start \"\" \"C:\\Program Files\\MiniMeters\\MiniMeters.exe\"");
        Log("Opened MiniMeters");
        Sleep(1000); // Give some time for the app to start
    }
}

// Window procedure for handling messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        ZeroMemory(&nid, sizeof(NOTIFYICONDATAW));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_APP + 1;
        nid.hIcon = (HICON)LoadImage(NULL, TEXT("C:\\Users\\Hannes\\OneDrive - Gingby\\Documents\\AutoFLMiniMeters\\icon2.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_SHARED);
        wcscpy_s(nid.szTip, L"MiniMeters Monitor");
        Shell_NotifyIconW(NIM_ADD, &nid);
        break;
    case WM_APP + 1:
        switch (lParam) {
        case WM_RBUTTONUP:
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 1, L"Exit");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
            break;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            Shell_NotifyIconW(NIM_DELETE, &nid);
            PostQuitMessage(0);
        }
        break;
    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Entry point of the application
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // Uncomment these lines for console debugging
    /*
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    */

    const wchar_t CLASS_NAME[] = L"MiniMeters Monitor Window Class";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    hwnd = CreateWindowExW(0, CLASS_NAME, L"Hidden Window", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_HIDE);

    WM_TASKBAR = RegisterWindowMessageW(L"TaskbarCreated");

    MSG msg = {};
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return 0;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Main logic for managing MiniMeters
        if (IsProcessRunning(flStudioProcessName)) {
            if (IsFLStudioWindowActive()) {
                StartMiniMeters();
            }
            ManageMiniMetersWindow();
        }
        else {
            // Hide MiniMeters if FL Studio is not running
            HWND miniMetersWindow = FindWindowW(NULL, L"MiniMeters");
            if (miniMetersWindow) {
                ShowWindow(miniMetersWindow, SW_HIDE);
            }
        }

        Sleep(500); // Check every half second
    }

    return 0;
}