#define WIN32_LEAN_AND_MEAN 
#include <windows.h>


//this so retarded...

#define DLLEXPORT extern "C" __declspec(dllexport)

#ifdef _MSC_VER
/*
_M_IX86 is defined for x86 processor compilation.
_M_X64 is defined for x64 processor compilation.
_WIN64 is defined for 64-bit processor compilation, and undefined otherwise.
*/

#ifdef _M_IX86
#pragma comment(linker, "/export:ShellHook=_ShellHook@12")
#undef DLLEXPORT
#define DLLEXPORT extern "C"
#endif // !1
#endif


#define LENGTHOF(x) (sizeof(x)/sizeof(x[0]))

static wchar_t nameBuffer[100] = { 0 };

template <typename T>
inline static T abs(T val) {
    return (val < 0) ? (-val) : val;
}

DLLEXPORT LRESULT __stdcall ShellHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HSHELL_WINDOWCREATED) {
        const HWND hCreatedUnownedTopWindow = (HWND)wParam;
        RECT windowRect;
        if (GetWindowRect(hCreatedUnownedTopWindow, &windowRect)) {
            if (const HMONITOR hMonitor = MonitorFromWindow(hCreatedUnownedTopWindow, MONITOR_DEFAULTTONULL)) {
                MONITORINFO mi;
                mi.cbSize = sizeof(mi);
                if (GetMonitorInfoW(hMonitor, &mi)) {
                    const auto absMonitorWidth = abs(mi.rcWork.right - mi.rcWork.left);
                    const auto absWindowWidth = abs(windowRect.right - windowRect.left);
                    const auto middleX = (absMonitorWidth - absWindowWidth) / 2;
                    if (middleX > 0) {
                        //meaning window's width < than width of the monitor
                        windowRect.left = mi.rcWork.left + middleX;
                    }

                    const auto absMonitorHeight = abs(mi.rcWork.bottom - mi.rcWork.top);
                    const auto absWindowHeight = abs(windowRect.bottom - windowRect.top);
                    const auto middleY = (absMonitorHeight - absWindowHeight) / 2;
                    if (middleY > 0) {
                        //meaning window's height < height of the monitor
                        windowRect.top = mi.rcWork.top + middleY;
                    }

                    //this however does not solve my original problem with Emacs 
                    //(because it sets its size much later after creation of the window)
                    const bool success = SetWindowPos(
                                                     hCreatedUnownedTopWindow, NULL,
                                                     windowRect.left, windowRect.top, 0, 0,
                                                     SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE
                                                     );
                    if (success) {
                        OutputDebugStringW(L"----------SUCCESS----------");
                    }
                    else {
                        OutputDebugStringW(L"----------FAILURE-----------");
                    }
                }
            }
            GetWindowTextW(hCreatedUnownedTopWindow, nameBuffer, LENGTHOF(nameBuffer));
            OutputDebugStringW(nameBuffer);
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

