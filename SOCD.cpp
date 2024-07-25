#include <windows.h>
#include <iostream>

#pragma comment(lib, "user32.lib")

HHOOK keyboardHook;
bool aPressed = false;
bool dPressed = false;
bool running = true;

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
            // Check for exit key combination (Ctrl + Q)
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

int main()
{
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    if (keyboardHook == NULL)
    {
        std::cout << "Failed to install keyboard hook!" << std::endl;
        return 1;
    }

    std::cout << "Keyboard hook installed. Press Ctrl + Q to exit." << std::endl;

    MSG msg;
    while (running && GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(keyboardHook);
    return 0;
}