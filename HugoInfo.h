#pragma once
#include <string>
#include <optional>
#include <filesystem>
#include <Windows.h>
#include "WinUtils.h"
class HugoInfo {
public:
    // 获取 Hugo 版本号（提取 SeewoService_xxx 中的 xxx 部分）
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