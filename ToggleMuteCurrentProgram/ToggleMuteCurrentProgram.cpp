#include <windows.h>
#include <iostream>
#include <psapi.h>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "Audio.h"

DWORD GetFocusedProcessId();
std::wstring GetProcessName(DWORD processId);
std::vector<std::wstring> LoadTargetProcessNames(const std::string& filePath);
bool IsTargetProcess(const std::wstring& processName, const std::vector<std::wstring>& targets);
std::string GetConfigPath();

int main()
{
    //Continue with these suggestions: https://chatgpt.com/c/6a319d3b-b698-83ed-9758-1f17f91605a5
    //ShowWindow(GetConsoleWindow(), SW_NORMAL);

    bool down = false;
    int toggleKey = VK_SUBTRACT;
    std::vector<std::wstring> targetProcessNames = LoadTargetProcessNames(GetConfigPath());

    if (targetProcessNames.size() == 0) {
        std::cout << "Add an executable name of the process you want to toggle mute to config.txt file!" << std::endl;
        return 0;
    }

    while (true) {
        SHORT state = GetAsyncKeyState(toggleKey);
        bool isDown = (state & 0x8000) != 0;

        if (isDown && !down) {
            DWORD focusedProcessID = GetFocusedProcessId();
            std::wstring processName = GetProcessName(focusedProcessID);

            std::wcout << processName << " (" << focusedProcessID << ")" << std::endl;

            if (IsTargetProcess(processName, targetProcessNames)) {
                toggleMute(focusedProcessID);
            }

            down = true;
        }
        else if (!isDown) {
            down = false;
        }
        Sleep(20); //Sleep for 20 milliseconds
    }

    return 0;
}

DWORD GetFocusedProcessId() {
    DWORD processId;
    GetWindowThreadProcessId(GetForegroundWindow(), &processId); //also can use GetActiveWindow() in place of GetForegroundWindow(). (active vs in focus)
    return processId;
}

std::wstring GetProcessName(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);

    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;

    if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
        CloseHandle(hProcess);

        std::wstring fullPath(path);
        size_t pos = fullPath.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            return fullPath.substr(pos + 1);
        }

        return fullPath;
    }

    CloseHandle(hProcess);
    return L"<unknown>";
}

std::vector<std::wstring> LoadTargetProcessNames(const std::string& filePath) {
    std::ifstream file(filePath);

    std::vector<std::wstring> names;

    if (!file.is_open()) {
        return names;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        // Remove Windows CR if file uses CRLF
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        names.push_back(
            std::wstring(line.begin(), line.end())
        );
    }

    return names;
}

bool IsTargetProcess(const std::wstring& processName, const std::vector<std::wstring>& targets) {
    return std::find(targets.begin(), targets.end(), processName) != targets.end();
}

std::string GetExeDirectory() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);

    std::string path(buffer);

    size_t lastSlash = path.find_last_of("\\/");
    return path.substr(0, lastSlash);
}

std::string GetConfigPath() {
    return GetExeDirectory() + "\\config.txt";
}