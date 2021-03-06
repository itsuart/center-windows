#include <Windows.h>
#include <Aclapi.h>
#include <Sddl.h>

static void display_last_error() {
    DWORD last_error = GetLastError();
    wchar_t* reason = NULL;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, last_error, 0, (LPWSTR)&reason, 1, NULL);
    MessageBoxW(NULL, reason, NULL, MB_OK);
    HeapFree(GetProcessHeap(), 0, reason);
}

// Add 'ALL APPLICATION PACKAGES' group permissions, so that UWP apps will load our hooks
static DWORD SetPermissions(wchar_t* wstrFilePath) {
    PACL pOldDACL = NULL, pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS eaAccess;
    SECURITY_INFORMATION siInfo = DACL_SECURITY_INFORMATION;
    DWORD dwResult = ERROR_SUCCESS;
    PSID pSID;
    // Get a pointer to the existing DACL
    dwResult = GetNamedSecurityInfo(wstrFilePath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDACL, NULL, &pSD);
    if (dwResult != ERROR_SUCCESS)
        goto Cleanup;
    // Get the SID for ALL APPLICATION PACKAGES using its SID string
    ConvertStringSidToSid(L"S-1-15-2-1", &pSID);
    if (pSID == NULL)
        goto Cleanup;
    ZeroMemory(&eaAccess, sizeof(EXPLICIT_ACCESS));
    eaAccess.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
    eaAccess.grfAccessMode = SET_ACCESS;
    eaAccess.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    eaAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    eaAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    eaAccess.Trustee.ptstrName = (LPWSTR)pSID;
    // Create a new ACL that merges the new ACE into the existing DACL
    dwResult = SetEntriesInAcl(1, &eaAccess, pOldDACL, &pNewDACL);
    if (ERROR_SUCCESS != dwResult)
        goto Cleanup;
    // Attach the new ACL as the object's DACL
    dwResult = SetNamedSecurityInfo(wstrFilePath, SE_FILE_OBJECT, siInfo, NULL, NULL, pNewDACL, NULL);
    if (ERROR_SUCCESS != dwResult)
        goto Cleanup;
Cleanup:
    if (pSD != NULL)
        LocalFree((HLOCAL)pSD);
    if (pNewDACL != NULL)
        LocalFree((HLOCAL)pNewDACL);
    return dwResult;
}

static bool installHooks(HMODULE dll, HHOOK& hShellHook, HHOOK& hKeyboardHook) {
    void* proc = GetProcAddress(dll, "ShellHook");
    if (proc == NULL) {
        return false;
    }

    {
        HMODULE hModule;
        if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            reinterpret_cast<wchar_t*>(proc), &hModule)) {
            static wchar_t filenameBuffer[32'000] = { 0 };
            if (GetModuleFileNameW(hModule, filenameBuffer, sizeof(filenameBuffer) / sizeof(filenameBuffer[0]))) {
                SetPermissions(filenameBuffer);
            }
        }
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
