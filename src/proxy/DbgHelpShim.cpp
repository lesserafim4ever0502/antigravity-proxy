#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

namespace {

DWORD WINAPI LoadVersionDll(LPVOID) {
    LoadLibraryW(L"version.dll");
    return 0;
}

HMODULE LoadSystemDbgHelp() {
    static HMODULE module = []() -> HMODULE {
        wchar_t systemDir[MAX_PATH] = {};
        const UINT len = GetSystemDirectoryW(systemDir, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return nullptr;
        }

        wchar_t path[MAX_PATH] = {};
        lstrcpynW(path, systemDir, MAX_PATH);
        lstrcatW(path, L"\\dbghelp.dll");
        return LoadLibraryW(path);
    }();
    return module;
}

using SymFromAddrFn = BOOL(WINAPI*)(HANDLE, DWORD64, DWORD64*, void*);

}  // namespace

extern "C" __declspec(dllexport) BOOL WINAPI SymFromAddr(
    HANDLE process,
    DWORD64 address,
    DWORD64* displacement,
    void* symbol) {
    const HMODULE dbghelp = LoadSystemDbgHelp();
    if (!dbghelp) {
        return FALSE;
    }

    const auto fn = reinterpret_cast<SymFromAddrFn>(GetProcAddress(dbghelp, "SymFromAddr"));
    if (!fn) {
        return FALSE;
    }
    return fn(process, address, displacement, symbol);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        const HANDLE thread = CreateThread(nullptr, 0, LoadVersionDll, nullptr, 0, nullptr);
        if (thread) {
            CloseHandle(thread);
        }
    }
    return TRUE;
}
