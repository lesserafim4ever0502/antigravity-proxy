// Starts Antigravity CLI with antigravity-proxy injected.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

#include "../injection/ProcessInjector.hpp"

static std::wstring QuoteArg(const std::wstring& arg) {
    if (arg.empty()) return L"\"\"";
    bool needsQuotes = false;
    for (wchar_t c : arg) {
        if (c == L' ' || c == L'\t' || c == L'"') {
            needsQuotes = true;
            break;
        }
    }
    if (!needsQuotes) return arg;

    std::wstring result = L"\"";
    size_t backslashes = 0;
    for (wchar_t c : arg) {
        if (c == L'\\') {
            backslashes++;
            continue;
        }
        if (c == L'"') {
            result.append(backslashes * 2 + 1, L'\\');
            result.push_back(c);
            backslashes = 0;
            continue;
        }
        result.append(backslashes, L'\\');
        backslashes = 0;
        result.push_back(c);
    }
    result.append(backslashes * 2, L'\\');
    result.push_back(L'"');
    return result;
}

static std::wstring GetDirectoryOf(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return L".";
    return path.substr(0, slash);
}

static std::wstring GetModulePath() {
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD len = GetModuleFileNameW(NULL, buffer.data(), (DWORD)buffer.size());
    if (len == 0) return L"";
    while (len >= buffer.size()) {
        buffer.resize(buffer.size() * 2);
        len = GetModuleFileNameW(NULL, buffer.data(), (DWORD)buffer.size());
        if (len == 0) return L"";
    }
    buffer.resize(len);
    return buffer;
}

static bool FileExists(const std::wstring& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static void PrintUsage() {
    std::wcerr << L"Usage: agy-proxy-launcher.exe [--target path\\to\\agy.exe] [--dll path\\to\\version.dll] [--cwd path] [--] [agy args...]\n";
}

int wmain(int argc, wchar_t** argv) {
    const std::wstring launcherPath = GetModulePath();
    const std::wstring launcherDir = GetDirectoryOf(launcherPath);
    std::wstring targetPath = launcherDir + L"\\agy.exe";
    std::wstring dllPath = launcherDir + L"\\version.dll";
    std::wstring workingDir;
    std::vector<std::wstring> passthroughArgs;

    for (int i = 1; i < argc; i++) {
        const std::wstring arg = argv[i];
        if (arg == L"--help" || arg == L"-h") {
            PrintUsage();
            return 0;
        }
        if (arg == L"--target") {
            if (++i >= argc) {
                std::wcerr << L"Missing value for --target\n";
                return 2;
            }
            targetPath = argv[i];
            continue;
        }
        if (arg == L"--dll") {
            if (++i >= argc) {
                std::wcerr << L"Missing value for --dll\n";
                return 2;
            }
            dllPath = argv[i];
            continue;
        }
        if (arg == L"--cwd") {
            if (++i >= argc) {
                std::wcerr << L"Missing value for --cwd\n";
                return 2;
            }
            workingDir = argv[i];
            continue;
        }
        if (arg == L"--") {
            for (++i; i < argc; i++) passthroughArgs.push_back(argv[i]);
            break;
        }
        passthroughArgs.push_back(arg);
    }

    if (!FileExists(targetPath)) {
        std::wcerr << L"agy target not found: " << targetPath << L"\n";
        return 2;
    }
    if (!FileExists(dllPath)) {
        std::wcerr << L"proxy DLL not found: " << dllPath << L"\n";
        return 2;
    }

    std::wstring commandLine = QuoteArg(targetPath);
    for (const auto& arg : passthroughArgs) {
        commandLine += L" ";
        commandLine += QuoteArg(arg);
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');
    if (workingDir.empty()) {
        std::vector<wchar_t> cwd(MAX_PATH, L'\0');
        DWORD len = GetCurrentDirectoryW((DWORD)cwd.size(), cwd.data());
        if (len > 0) {
            if (len >= cwd.size()) {
                cwd.resize(len + 1);
                len = GetCurrentDirectoryW((DWORD)cwd.size(), cwd.data());
            }
            if (len > 0 && len < cwd.size()) {
                workingDir.assign(cwd.data(), len);
            }
        }
    }
    const wchar_t* childWorkingDir = workingDir.empty() ? NULL : workingDir.c_str();

    if (!CreateProcessW(
            targetPath.c_str(),
            mutableCommandLine.data(),
            NULL,
            NULL,
            TRUE,
            CREATE_SUSPENDED,
            NULL,
            childWorkingDir,
            &si,
            &pi)) {
        std::wcerr << L"CreateProcessW failed, err=" << GetLastError() << L"\n";
        return 1;
    }

    std::string failureReason;
    const bool injected = Injection::ProcessInjector::InjectDll(pi.hProcess, dllPath, &failureReason);
    if (!injected) {
        std::cerr << "DLL injection failed: " << failureReason << "\n";
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 1;
    }

    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        exitCode = 1;
    }
    CloseHandle(pi.hProcess);
    return (int)exitCode;
}
