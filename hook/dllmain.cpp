#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstddef>

#define LENGTHOF(x) (sizeof(x)/sizeof(x[0]))

template <typename T>
inline static T abs(T val) {
    return (val < 0) ? (-val) : val;
}


static void WindowTakesRightHalfOfAMontitor(HWND hCreatedUnownedTopWindow) {
    if (const HMONITOR hMonitor = MonitorFromWindow(hCreatedUnownedTopWindow, MONITOR_DEFAULTTONULL)) {
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(hMonitor, &mi)) {
            const RECT& monitorWorkRect = mi.rcWork;
            const auto absMonitorWidth = abs(monitorWorkRect.right - monitorWorkRect.left);
            const auto absMonitorHeight = abs(monitorWorkRect.bottom - monitorWorkRect.top);

            SetWindowPos(
                hCreatedUnownedTopWindow, NULL,
                monitorWorkRect.left + absMonitorWidth / 2, monitorWorkRect.top,
                absMonitorWidth / 2, absMonitorHeight,
                SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER
            );
        }
    }
};


static void WindowTakesLeftHalfOfAMontitor(HWND hCreatedUnownedTopWindow) {
    if (const HMONITOR hMonitor = MonitorFromWindow(hCreatedUnownedTopWindow, MONITOR_DEFAULTTONULL)) {
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(hMonitor, &mi)) {
            const RECT& monitorWorkRect = mi.rcWork;
            const auto absMonitorWidth = abs(monitorWorkRect.right - monitorWorkRect.left);
            const auto absMonitorHeight = abs(monitorWorkRect.bottom - monitorWorkRect.top);

            SetWindowPos(
                hCreatedUnownedTopWindow, NULL,
                monitorWorkRect.left, monitorWorkRect.top,
                absMonitorWidth / 2, absMonitorHeight,
                SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER
            );
        }
    }
};


static void CenterWindow(HWND hCreatedUnownedTopWindow) {
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
                SetWindowPos(
                    hCreatedUnownedTopWindow, NULL,
                    windowRect.left, windowRect.top, 0, 0,
                    SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE
                );
            }
        }
    }
};

static bool is_button_down(int virtual_key){
    const int BTN_DOWN = 0x8000;
    return BTN_DOWN == (GetKeyState (virtual_key) & BTN_DOWN);
}

//https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
const int VIRTUAL_KEY_C = 0x43;
const int VIRTUAL_KEY_H = 0x48;
const int VIRTUAL_KEY_S = 0x53;

LRESULT CALLBACK KeyboardHook(int code, WPARAM wParam,LPARAM lParam){
    if (code < 0) return CallNextHookEx(NULL, code, wParam, lParam);

    //https://msdn.microsoft.com/en-us/library/windows/desktop/ms644984(v=vs.85).aspx
    const bool altIsDown = (lParam & (1 << 29)) > 0;
    const bool buttonIsDown = (lParam & (1 << 31)) == 0;

    if (is_button_down(VK_CONTROL) && altIsDown && buttonIsDown){
        const HWND foregroundWindow = GetForegroundWindow();
        if (foregroundWindow) {
            switch (wParam) {
                case VIRTUAL_KEY_C: {
                    CenterWindow(foregroundWindow);
                } break;

                case VIRTUAL_KEY_H: {
                    WindowTakesLeftHalfOfAMontitor(foregroundWindow);
                } break;

                case VIRTUAL_KEY_S: {
                    WindowTakesRightHalfOfAMontitor(foregroundWindow);
                } break;
            }
        }
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}

static DWORD __stdcall CenterWindowDelayed(void* param) {
    HWND windowToCenter = (HWND)param;
    Sleep(600);
    CenterWindow(windowToCenter);
    return 0;
}

static bool is_windows_explorer_class(const wchar_t* pName) noexcept {
    return ( (0 == lstrcmpW(pName, L"Explorer­WClass")) || (0 == lstrcmpW(pName, L"Cabinet­WClass")) );
}

template<typename T, std::size_t n>
static constexpr std::size_t array_length(T(&anArray)[n]) noexcept {
    return n;
}

struct Callback_State final {
    HWND second_explorer_window;
    HWND first_explorer_window;
};

static BOOL CALLBACK window_enumeration_callback(HWND hwnd, LPARAM lParam) {
    Callback_State* state = reinterpret_cast<Callback_State*>(lParam);
    if (hwnd != state->second_explorer_window) {
        wchar_t class_name_buffer[256];
        GetClassNameW(hwnd, class_name_buffer, array_length(class_name_buffer));

        if (is_windows_explorer_class(class_name_buffer)) {
            if (state->first_explorer_window == nullptr) { // first non-second Explorer window
                state->first_explorer_window = hwnd;
            } else {
                // this is a third (at least) Explorer window.
                // set first_explorer_window to nullptr to signal "not two windows situation" and stop enumeration
                state->first_explorer_window = nullptr;
                return FALSE;
            }
        }
    }
    return TRUE;
}


LRESULT CALLBACK ShellHook(int nCode, WPARAM wParam, LPARAM lParam) {
    wchar_t buffer[256]; //
    if (nCode == HSHELL_WINDOWCREATED) {
        const HWND hCreatedUnownedTopWindow = (HWND)wParam;
        GetClassName(hCreatedUnownedTopWindow, buffer, sizeof(buffer) / sizeof(buffer[0]));
        if (0 == lstrcmpW(buffer, L"Emacs")) {
            //give it a time to load completely.
            CreateThread(nullptr, 0, &CenterWindowDelayed, hCreatedUnownedTopWindow, 0, nullptr);
        } else if (is_windows_explorer_class(buffer)) {
            // if this is second explorer window, put the first one to the left half of the monitor
            // and the second one on the right
            Callback_State state{};
            state.second_explorer_window = hCreatedUnownedTopWindow;

            EnumWindows(window_enumeration_callback, reinterpret_cast<LPARAM>(&state));

            if (state.first_explorer_window) { // if there is exactly two Explorer windows
                WindowTakesLeftHalfOfAMontitor(state.first_explorer_window);
                WindowTakesRightHalfOfAMontitor(state.second_explorer_window);
            } else {
                CenterWindow(hCreatedUnownedTopWindow);
            }

        } else {
            CenterWindow(hCreatedUnownedTopWindow);
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
    } break;

    case DLL_THREAD_ATTACH: {
    } break;

    case DLL_THREAD_DETACH: {
    } break;

    case DLL_PROCESS_DETACH: {

    } break;

    default:{

    } break;

    }
    return TRUE;
}
