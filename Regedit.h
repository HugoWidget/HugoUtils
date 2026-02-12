#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <utility>

class ScopedHKey {
public:
    explicit ScopedHKey(HKEY hKey = nullptr) : m_hKey(hKey) {}
    ~ScopedHKey();
    ScopedHKey(const ScopedHKey&) = delete;
    ScopedHKey& operator=(const ScopedHKey&) = delete;
    ScopedHKey(ScopedHKey&& other) noexcept : m_hKey(other.m_hKey) {
        other.m_hKey = nullptr;
    }
    ScopedHKey& operator=(ScopedHKey&& other) noexcept;
    HKEY get() const { return m_hKey; }
    void reset(HKEY hKey = nullptr) ;
private:
    HKEY m_hKey = nullptr;
};

class GenericRegistry {
public:
    GenericRegistry() = delete;
    ~GenericRegistry() = delete;

    // 通用写入字符串类型注册表值
    static bool WriteStringValue(HKEY rootKey, const std::wstring& regPath,
        const std::wstring& valueName, const std::wstring& valueContent);

    // 通用读取字符串类型注册表值
    static bool ReadStringValue(HKEY rootKey, const std::wstring& regPath,
        const std::wstring& valueName, std::wstring& outValue);

    // 通用创建注册表项并写入值（仅当项不存在时执行）
    static bool CreateKeyAndWriteValue(HKEY rootKey, const std::wstring& regPath,
        const std::wstring& valueName, const std::wstring& valueContent);

    // 通用删除注册表值
    static bool DeleteValue(HKEY rootKey, const std::wstring& regPath,
        const std::wstring& valueName);
};