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

struct HooksInstallationConfiguration {
    bool InstallShellHook;
    bool InstallKeyboardHook;
};


bool GetHooksInstallationConfiguration(HooksInstallationConfiguration& result) {
    //set default values
    result.InstallKeyboardHook = true;
    result.InstallShellHook = true;

    const auto commandLine = GetCommandLineW();
    int numArgs = 0;
    wchar_t** argsArray = CommandLineToArgvW(commandLine, &numArgs);
    if (NULL != argsArray) {
        if (numArgs != 2) {
            //this is not entirely correct, since not only "no arguments" (numArgs == 1) but more than 1 will be accepted
            //and default configuration for hooks will be used.
            return true;
        }
        const wchar_t* InstallOnlyKeyboardHookFlag = L"keyboard-only";
        const wchar_t* InstallOnlyShellHookFlag = L"shell-only";

        const wchar_t* userArgument = argsArray[1];
        if (CSTR_EQUAL == CompareStringOrdinal(InstallOnlyKeyboardHookFlag, -1, userArgument, -1, true)) {
            result.InstallKeyboardHook = true;
            result.InstallShellHook = false;
        } else if (CSTR_EQUAL == CompareStringOrdinal(InstallOnlyShellHookFlag, -1, userArgument, -1, true)){
            result.InstallKeyboardHook = false;
            result.InstallShellHook = true;
        } else {
            //neither was supplied. Assume default then.
            //TODO: show proper error here
        }
        return true;
    }
    return false;
}

void entry_point() {
    HooksInstallationConfiguration config;
    if (GetHooksInstallationConfiguration(config)) {
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

                if (config.InstallKeyboardHook) {
                    UnhookWindowsHookEx(hKeyboardHook);
                }

                if (config.InstallShellHook) {
                    UnhookWindowsHookEx(hShellHook);
                }

                ExitProcess(0);
            }
        }
    }


    display_last_error();
    ExitProcess(0);
}
