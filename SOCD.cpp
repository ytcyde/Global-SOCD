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

// Global variables for Code 1
int keyA_code = 'A'; // Default to 'A'
int keyD_code = 'D'; // Default to 'D'
unordered_map<int, KeyState> keyStates;
int activeKey = 0;
int previousKey = 0;
HHOOK hHook = NULL;
NOTIFYICONDATA nid;

// Global variables for Code 2
bool aPressed = false;
bool dPressed = false;
bool running = true;
HHOOK keyboardHook;

// Function declarations
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void InitNotifyIconData(HWND hwnd);

// Function for Code 1
void RunCode1()
{
    // Create a named mutex
    HANDLE hMutex = CreateMutex(NULL, TRUE, TEXT("SnapKeyMutex"));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBox(NULL, TEXT("SnapKey is already running!"), TEXT("Error"), MB_ICONINFORMATION | MB_OK);
        return;
    }

    // Create a window class
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = TEXT("SnapKeyClass");

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error"), MB_ICONEXCLAMATION | MB_OK);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return;
    }

    // Create a window
    HWND hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        TEXT("SnapKey"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, wc.hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error"), MB_ICONEXCLAMATION | MB_OK);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return;
    }

    // Initialize and add the system tray icon
    InitNotifyIconData(hwnd);

    // Set the hook
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (hHook == NULL)
    {
        MessageBox(NULL, TEXT("Failed to install hook!"), TEXT("Error"), MB_ICONEXCLAMATION | MB_OK);
        ReleaseMutex(hMutex); // Release the mutex before exiting
        CloseHandle(hMutex); // Close the handle
        return;
    }

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Unhook the hook
    UnhookWindowsHookEx(hHook);

    // Remove the system tray icon
    Shell_NotifyIcon(NIM_DELETE, &nid);

    // Release and close the mutex
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
}

// Function for Code 2
void RunCode2()
{
    // Set the keyboard hook for Code 2
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    if (keyboardHook == NULL)
    {
        std::cout << "Failed to install keyboard hook!" << std::endl;
        return;
    }

    std::cout << "Keyboard hook installed. Press Ctrl + Q to exit." << std::endl;

    MSG msg;
    while (running && GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(keyboardHook);
}

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
                input.ki.wVk = previousKey;
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
                input.ki.wVk = activeKey;
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
        DWORD wScanCode = kbdStruct->scanCode;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            if (wVirtKey == 'A')
            {
                aPressed = true;
                if (dPressed)
                {
                    keybd_event('D', wScanCode, KEYEVENTF_KEYUP, 0);
                }
            }
            else if (wVirtKey == 'D')
            {
                dPressed = true;
                if (aPressed)
                {
                    keybd_event('A', wScanCode, KEYEVENTF_KEYUP, 0);
                }
            }
            else if (wVirtKey == 'Q' && GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                running = false;
                PostQuitMessage(0);
            }
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            if (wVirtKey == 'A')
            {
                aPressed = false;
                if (dPressed)
                {
                    keybd_event('D', wScanCode, 0, 0);
                }
            }
            else if (wVirtKey == 'D')
            {
                dPressed = false;
                if (aPressed)
                {
                    keybd_event('A', wScanCode, 0, 0);
                }
            }
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void InitNotifyIconData(HWND hwnd)
{
    memset(&nid, 0, sizeof(NOTIFYICONDATA));

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_APP_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;

    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpy(nid.szTip, TEXT("SnapKey"));

    Shell_NotifyIcon(NIM_ADD, &nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONDOWN)
        {
            POINT curPoint;
            GetCursorPos(&curPoint);
            SetForegroundWindow(hwnd);

            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, TEXT("Exit SnapKey"));

            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, curPoint.x, curPoint.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_EXIT_CONTEXT_MENU_ITEM)
        {
            PostQuitMessage(0);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int main()
{
    int choice;

    cout << "Select an option:" << endl;
    cout << "1. Run Code 1" << endl;
    cout << "2. Run Code 2" << endl;
    cout << "Enter your choice (1 or 2): ";
    cin >> choice;

    switch (choice)
    {
    case 1:
        RunCode1();
        break;
    case 2:
        RunCode2();
        break;
    default:
        cout << "Invalid choice. Exiting." << endl;
        return 1;
    }

    return 0;
}
