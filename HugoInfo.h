#pragma once
#include <Windows.h>

#include <string>
#include <optional>
#include <filesystem>

#include "WinUtils.h"
class HugoInfo {
public:
    std::optional<std::wstring> getHugoVersion();
    std::optional<std::wstring> getHugoFolder();
    std::optional<std::wstring> getHugoProtectDriverFolder();
    std::optional<std::wstring> getHugoProtectDriverPath();
private:
    static std::optional<std::filesystem::path> FindSpecificDir(
        const std::filesystem::path& baseDir,
        std::wstring_view prefix
    );
};