#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <unordered_map>
#include <atomic>

#pragma comment(lib, "user32.lib")

constexpr int ID_TRAY_APP_ICON = 1001;
constexpr int ID_TRAY_EXIT_CONTEXT_MENU_ITEM = 3000;
constexpr UINT WM_TRAYICON = WM_USER + 1;

struct KeyState {
    std::atomic<bool> pressed{ false };
};

// Global variables
constexpr int KEY_A = 'A';
constexpr int KEY_D = 'D';
std::unordered_map<int, KeyState> keyStates;
std::atomic<int> activeKey{ 0 };
std::atomic<int> previousKey{ 0 };
HHOOK hHook = nullptr;
NOTIFYICONDATA nid{};
std::atomic<bool> running{ true };
std::atomic<bool> isCode1{ true };

// Function declarations
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void InitNotifyIconData(HWND hwnd);
void RunCode1();
void RunCode2();

void handleKeyDown(int keyCode) {
    if (keyCode == KEY_A || keyCode == KEY_D) {
        auto& keyState = keyStates[keyCode];
        if (!keyState.pressed.exchange(true)) {
            int expected = 0;
            if (activeKey.compare_exchange_strong(expected, keyCode) || activeKey.load() == keyCode) {
                // Do nothing
            }
            else {
                previousKey.store(activeKey.exchange(keyCode));
                INPUT input = {};
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = static_cast<WORD>(previousKey);
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }
}

void handleKeyUp(int keyCode) {
    if (keyCode == KEY_A || keyCode == KEY_D) {
        auto& keyState = keyStates[keyCode];
        if (previousKey == keyCode && !keyState.pressed) previousKey = 0;
        if (keyState.pressed.exchange(false)) {
            if (activeKey == keyCode && previousKey != 0) {
                activeKey = previousKey.exchange(0);
                INPUT input = {};
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = static_cast<WORD>(activeKey);
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        const auto* kbdStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        DWORD wVirtKey = kbdStruct->vkCode;
        WORD wScanCode = static_cast<WORD>(kbdStruct->scanCode);

        if (isCode1) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                handleKeyDown(wVirtKey);
            }
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                handleKeyUp(wVirtKey);
            }
        }
        else { // Code 2
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                if (wVirtKey == KEY_A) {
                    keyStates[KEY_A].pressed = true;
                    if (keyStates[KEY_D].pressed) {
                        keybd_event(KEY_D, wScanCode, KEYEVENTF_KEYUP, 0);
                    }
                    keybd_event(KEY_A, wScanCode, 0, 0);
                }
                else if (wVirtKey == KEY_D) {
                    keyStates[KEY_D].pressed = true;
                    if (keyStates[KEY_A].pressed) {
                        keybd_event(KEY_A, wScanCode, KEYEVENTF_KEYUP, 0);
                    }
                    keybd_event(KEY_D, wScanCode, 0, 0);
                }
            }
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                if (wVirtKey == KEY_A) {
                    keyStates[KEY_A].pressed = false;
                    if (keyStates[KEY_D].pressed) {
                        keybd_event(KEY_D, wScanCode, 0, 0);
                    }
                }
                else if (wVirtKey == KEY_D) {
                    keyStates[KEY_D].pressed = false;
                    if (keyStates[KEY_A].pressed) {
                        keybd_event(KEY_A, wScanCode, 0, 0);
                    }
                }
            }
        }

    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// ... (keep InitNotifyIconData and WndProc as they were)

void RunCode1() {
    isCode1 = true;
    HANDLE hMutex = CreateMutex(nullptr, TRUE, TEXT("WootingMutex"));
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "Wooting mode is already running!" << std::endl;
        return;
    }

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);
    if (hHook == nullptr) {
        std::cout << "Failed to install hook!" << std::endl;
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return;
    }

    std::cout << "Wooting mode is running. Press Ctrl+C to exit." << std::endl;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) && running) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
}

void RunCode2() {
    isCode1 = false;
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(nullptr), 0);
    if (hHook == nullptr) {
        std::cout << "Failed to install keyboard hook!" << std::endl;
        return;
    }

    std::cout << "Razer SnapTap mode enabled. Press Ctrl+C to exit." << std::endl;

    MSG msg;
    while (running && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
}


int main() {
    int choice;

    std::cout << "Select an option:\n"
        << "1. Run Wooting Mode (Recommended)\n"
        << "2. Run Razer SnapTap mode\n"
        << "Enter your choice (1 or 2): ";
    std::cin >> choice;


    switch (choice) {
    case 1:
        RunCode1();
        break;
    case 2:
        RunCode2();
        break;
    default:
        std::cout << "Invalid choice. Exiting." << std::endl;
        return 1;
    }

    return 0;
}
