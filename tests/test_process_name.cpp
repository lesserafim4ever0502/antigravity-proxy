#include <cassert>
#include <string>

#include "hooks/ProcessName.hpp"

int main() {
    using Hooks::GetCreateProcessTargetBaseNameA;
    using Hooks::IsLanguageServerProcessName;

    // lpApplicationName 明确给出路径时，不按空格截断文件名。
    assert(GetCreateProcessTargetBaseNameA(
               "C:\\Users\\test\\AppData\\Local\\Programs\\Antigravity\\Antigravity IDE.exe",
               nullptr) == "Antigravity IDE.exe");

    // 命令行带引号时，保留引号内完整 exe 路径，再提取文件名。
    assert(GetCreateProcessTargetBaseNameA(
               nullptr,
               "\"C:\\Users\\test\\AppData\\Local\\Programs\\Antigravity\\Antigravity IDE.exe\" --type=renderer") ==
           "Antigravity IDE.exe");

    // 命令行未加引号但包含 .exe 时，优先截到 .exe，避免参数污染进程名。
    assert(GetCreateProcessTargetBaseNameA(
               nullptr,
               "C:\\Users\\test\\AppData\\Local\\Programs\\Antigravity\\resources\\bin\\language_server.exe --stdio") ==
           "language_server.exe");

    // 普通无空格路径保持历史行为。
    assert(GetCreateProcessTargetBaseNameA(nullptr, "node.exe --inspect") == "node.exe");

    // 新旧 language server 命名都要触发窄范围 node 子进程兼容。
    assert(IsLanguageServerProcessName("language_server.exe"));
    assert(IsLanguageServerProcessName("language_server"));
    assert(IsLanguageServerProcessName("language_server_windows_x64.exe"));
    assert(!IsLanguageServerProcessName("Antigravity IDE.exe"));

    return 0;
}
