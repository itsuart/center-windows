#include <Windows.h>


static void display_last_error() {
    DWORD last_error = GetLastError();
    wchar_t* reason = NULL;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, last_error, 0, (LPWSTR)&reason, 1, NULL);
    MessageBoxW(NULL, reason, NULL, MB_OK);
    HeapFree(GetProcessHeap(), 0, reason);
}

static bool installHooks(HMODULE dll, HHOOK& hShellHook, HHOOK& hKeyboardHook) {
    void* proc = GetProcAddress(dll, "ShellHook");
    if (proc == NULL) {
        return false;
    }

    hShellHook = SetWindowsHookExW(WH_SHELL, (HOOKPROC)proc, dll, 0);
    if (hShellHook == NULL) {
        return false;
    }

    const void* keyboardProc = GetProcAddress(dll, "KeyboardHook");
    if (keyboardProc == NULL) {
        return false;
    }

    hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD, (HOOKPROC)keyboardProc, dll, 0);
    return hKeyboardHook != NULL;
}

#ifdef _MSC_VER
/*
_M_IX86 is defined for x86 processor compilation.
_M_X64 is defined for x64 processor compilation.
_WIN64 is defined for 64-bit processor compilation, and undefined otherwise.
*/

#ifdef _M_IX86
#define HOOK_DLL L"center-window-hook32.dll"
#elif _M_X64
#define HOOK_DLL L"center-window-hook64.dll"
#endif

#endif

void entry_point() {
    const HMODULE dll = LoadLibraryW(HOOK_DLL);
    if (dll) {
        HHOOK hShellHook = 0;
        HHOOK hKeyboardHook = 0;
        if (installHooks(dll, hShellHook, hKeyboardHook)) {
            MSG msg;

            // Main message loop:
            while (GetMessage(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            UnhookWindowsHookEx(hShellHook);
            UnhookWindowsHookEx(hKeyboardHook);
            ExitProcess(0);
        }
    }

    display_last_error();
    ExitProcess(0);
}