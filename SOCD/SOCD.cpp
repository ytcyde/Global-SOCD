#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#pragma comment(lib, "user32.lib")

using namespace std;

#define ID_TRAY_APP_ICON                1001
#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM  3000
#define WM_TRAYICON                     (WM_USER + 1)

struct KeyState
{
    bool pressed = false;
};

// Global variables
int keyA_code = 'A';
int keyD_code = 'D';
unordered_map<int, KeyState> keyStates;
int activeKey = 0;
int previousKey = 0;
HHOOK hHook = NULL;
NOTIFYICONDATA nid;
bool running = true;
bool isCode1 = true; // Flag to determine which code is running

// Function declarations
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void InitNotifyIconData(HWND hwnd);
void RunCode1();
void RunCode2();
BOOL WINAPI ConsoleHandler(DWORD signal);

void handleKeyDown(int keyCode)
{
    if (keyCode == keyA_code || keyCode == keyD_code)
    {
        auto& keyState = keyStates[keyCode];
        if (!keyState.pressed)
        {
            keyState.pressed = true;
            if (activeKey == 0 || activeKey == keyCode) activeKey = keyCode;
            else
            {
                previousKey = activeKey;
                activeKey = keyCode;

                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = static_cast<WORD>(previousKey);
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }
}

void handleKeyUp(int keyCode)
{
    if (keyCode == keyA_code || keyCode == keyD_code)
    {
        auto& keyState = keyStates[keyCode];
        if (previousKey == keyCode && !keyState.pressed) previousKey = 0;
        if (keyState.pressed)
        {
            keyState.pressed = false;
            if (activeKey == keyCode && previousKey != 0)
            {
                activeKey = previousKey;
                previousKey = 0;

                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = static_cast<WORD>(activeKey);
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        DWORD wVirtKey = kbdStruct->vkCode;
        WORD wScanCode = static_cast<WORD>(kbdStruct->scanCode);

        if (isCode1)
        {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
            {
                handleKeyDown(wVirtKey);
            }
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            {
                handleKeyUp(wVirtKey);
            }
        }
        else // Code 2
        {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
            {
                if (wVirtKey == 'A')
                {
                    keyStates['A'].pressed = true;
                    if (keyStates['D'].pressed)
                    {
                        keybd_event('D', wScanCode, KEYEVENTF_KEYUP, 0);
                    }
                    keybd_event('A', wScanCode, 0, 0);
                }
                else if (wVirtKey == 'D')
                {
                    keyStates['D'].pressed = true;
                    if (keyStates['A'].pressed)
                    {
                        keybd_event('A', wScanCode, KEYEVENTF_KEYUP, 0);
                    }
                    keybd_event('D', wScanCode, 0, 0);
                }
            }
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            {
                if (wVirtKey == 'A')
                {
                    keyStates['A'].pressed = false;
                    if (keyStates['D'].pressed)
                    {
                        keybd_event('D', wScanCode, 0, 0);
                    }
                }
                else if (wVirtKey == 'D')
                {
                    keyStates['D'].pressed = false;
                    if (keyStates['A'].pressed)
                    {
                        keybd_event('A', wScanCode, 0, 0);
                    }
                }
            }
        }

        if (wVirtKey == 'Q' && GetAsyncKeyState(VK_CONTROL) & 0x8000)
        {
            running = false;
            PostQuitMessage(0);
        }
    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// ... (keep InitNotifyIconData and WndProc as they were)

void RunCode1()
{
    isCode1 = true;
    // Create a named mutex
    HANDLE hMutex = CreateMutex(NULL, TRUE, TEXT("WootingMutex"));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        std::cout << "Wooting mode is already running!" << std::endl;
        return;
    }

    // Set the hook
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (hHook == NULL)
    {
        std::cout << "Failed to install hook!" << std::endl;
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return;
    }

    std::cout << "Wooting mode is running. Press Ctrl+Q to exit." << std::endl;

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) && running)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Unhook the hook
    UnhookWindowsHookEx(hHook);

    // Release and close the mutex
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
}

void RunCode2()
{
    isCode1 = false;
    // Set the keyboard hook for Snap Key mode
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    if (hHook == NULL)
    {
        std::cout << "Failed to install keyboard hook!" << std::endl;
        return;
    }

    std::cout << "Razer SnapTap mode enabledd. Press Ctrl + Q to exit." << std::endl;

    MSG msg;
    while (running && GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
}

BOOL WINAPI ConsoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT)
    {
        std::cout << "Exiting SOCD..." << std::endl;
        running = false;
        PostQuitMessage(0);
        return TRUE;
    }
    return FALSE;
}

int main()
{
    int choice;

    std::cout << "Select an option:" << std::endl;
    std::cout << "1. Run Code 1" << std::endl;
    std::cout << "2. Run Code 2" << std::endl;
    std::cout << "Enter your choice (1 or 2): ";
    std::cin >> choice;

    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    switch (choice)
    {
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
