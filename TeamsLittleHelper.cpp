#include <windows.h>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <string>
#include <cmath>

std::atomic<bool> isActive(false);
HWND hwndStatus;
HWND hwndHelpText;

void jiggleMouse() {
    POINT originalPos;
    GetCursorPos(&originalPos);

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distX(-20, 20);
    std::uniform_int_distribution<int> distY(-20, 20);

    while (isActive) {
        for (int i = 0; i < 20 && isActive; ++i) { // Move for 2 seconds
            POINT newPos;
            GetCursorPos(&newPos);
            newPos.x += distX(generator);
            newPos.y += distY(generator);
            SetCursorPos(newPos.x, newPos.y);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (isActive) std::this_thread::sleep_for(std::chrono::seconds(3)); // Pause for 3 seconds
    }
    SetCursorPos(originalPos.x, originalPos.y); // Reset position
}

void toggleCapsLock() {
    for (int i = 0; i < 2 && isActive; ++i) {
        keybd_event(VK_CAPITAL, 0, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        keybd_event(VK_CAPITAL, 0, KEYEVENTF_KEYUP, 0);
    }
}

void performActions() {
    while (isActive) {
        jiggleMouse();
        if (isActive) std::this_thread::sleep_for(std::chrono::seconds(3));
        toggleCapsLock();
        if (isActive) std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        hwndStatus = CreateWindow(
            "STATIC", "STATUS: OFF",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 50, 200, 30,
            hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        hwndHelpText = CreateWindow(
            "STATIC", "Press Ctrl + Shift + Alt + P",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 90, 200, 30,
            hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        break;
    }
    case WM_HOTKEY: {
        if (wParam == 1) {
            isActive = !isActive;
            SetWindowText(hwndStatus, isActive ? "STATUS: ON" : "STATUS: OFF");
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); // Keep window on top
            HDC hdc = GetDC(hwndStatus);
            if (isActive) {
                SetBkColor(hdc, RGB(255, 200, 200)); // Light red background
            } else {
                SetBkColor(hdc, RGB(255, 255, 255)); // White background
            }
            ReleaseDC(hwndStatus, hdc);

            if (isActive) {
                std::thread(performActions).detach();
            }
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        if (hwndStatic == hwndStatus && isActive) {
            SetTextColor(hdcStatic, RGB(255, 0, 0)); // Red text for ON status
            SetBkColor(hdcStatic, RGB(255, 200, 200)); // Light red background
            static HBRUSH hBrush = CreateSolidBrush(RGB(255, 200, 200));
            return (INT_PTR)hBrush;
        } else {
            SetTextColor(hdcStatic, RGB(0, 0, 0)); // Black text for OFF status
            SetBkColor(hdcStatic, RGB(255, 255, 255)); // White background
            static HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
            return (INT_PTR)hBrush;
        }
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "MouseCapsApp";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Little Helper",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Register hotkey Ctrl + Shift + Alt + P
    if (!RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT | MOD_ALT, 0x50)) { // 0x50 is 'P'
        MessageBox(NULL, "Failed to register hotkey!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterHotKey(hwnd, 1);
    return 0;
}
