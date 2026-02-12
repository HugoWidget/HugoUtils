#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <utility>
#include "Logger.h"
#include "regedit.h"
using namespace WinUtils;
static Logger logger(L"Regedit");

ScopedHKey::~ScopedHKey() {
    if (m_hKey != nullptr) {
        RegCloseKey(m_hKey);
        m_hKey = nullptr;
    }
};

ScopedHKey& ScopedHKey::operator=(ScopedHKey&& other) noexcept {
        if (this != &other) {
            reset();
            m_hKey = other.m_hKey;
            other.m_hKey = nullptr;
        }
        return *this;
    };

    void ScopedHKey::reset(HKEY hKey) {
        if (m_hKey != nullptr) RegCloseKey(m_hKey);
        m_hKey = hKey;
    };


    bool GenericRegistry::WriteStringValue(HKEY rootKey, const std::wstring& regPath,
        const std::wstring& valueName, const std::wstring& valueContent) {
        DWORD dwDisposition = 0;
        HKEY tempKey = nullptr;

        LONG lResult = RegCreateKeyExW(
            rootKey, regPath.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
            KEY_WRITE, nullptr, &tempKey, &dwDisposition
        );
        if (lResult != ERROR_SUCCESS) {
            std::cerr << "创建/打开注册表项失败，错误码：" << lResult << std::endl;
            return false;
        }

        ScopedHKey hKey(tempKey);

        lResult = RegSetValueExW(
            hKey.get(), valueName.c_str(), 0, REG_SZ,
            (const BYTE*)valueContent.c_str(),
            (DWORD)((valueContent.length() + 1) * sizeof(wchar_t))
        );
        if (lResult != ERROR_SUCCESS) {
            std::cerr << "写入注册表值失败，错误码：" << lResult << std::endl;
            return false;
        }

        std::cout << "注册表值写入成功！" << std::endl;
        return true;
    };


    bool GenericRegistry::ReadStringValue(HKEY rootKey, const std::wstring& regPath,
        const std::wstring& valueName, std::wstring& outValue) {
        HKEY tempKey = nullptr;
        LONG lResult = RegOpenKeyExW(
            rootKey, regPath.c_str(), 0, KEY_READ, &tempKey
        );
        if (lResult != ERROR_SUCCESS) {
            std::cerr << "打开注册表项失败，错误码：" << lResult << std::endl;
            return false;
        }

        ScopedHKey hKey(tempKey);

        DWORD dwType = 0;
        DWORD dwDataSize = 0;
        lResult = RegQueryValueExW(
            hKey.get(), valueName.c_str(), nullptr, &dwType, nullptr, &dwDataSize
        );
        if (lResult != ERROR_SUCCESS && lResult != ERROR_MORE_DATA) {
            std::cerr << "查询值长度失败，错误码：" << lResult << std::endl;
            return false;
        }

        if (dwType != REG_SZ) {
            std::cerr << "注册表值类型错误，非字符串类型！" << std::endl;
            return false;
        }

        outValue.resize(dwDataSize / sizeof(wchar_t));
        lResult = RegQueryValueExW(
            hKey.get(), valueName.c_str(), nullptr, &dwType,
            (BYTE*)outValue.data(), &dwDataSize
        );
        if (lResult != ERROR_SUCCESS) {
            std::cerr << "读取注册表值失败，错误码：" << lResult << std::endl;
            return false;
        }

        outValue = outValue.substr(0, outValue.find(L'\0'));
        std::wcout << L"注册表读取成功，值为：" << outValue << std::endl;
        return true;
    };

    // 通用创建注册表项并写入值（仅当项不存在时执行）
 bool GenericRegistry::CreateKeyAndWriteValue(HKEY rootKey, const std::wstring& regPath,
        const std::wstring& valueName, const std::wstring& valueContent) {
        HKEY tempKey = nullptr;
        LONG lResult = RegOpenKeyExW(
            rootKey, regPath.c_str(), 0, KEY_READ, &tempKey
        );
        if (lResult == ERROR_SUCCESS) {
            ScopedHKey hCheckKey(tempKey);
            std::cerr << "注册表项已存在，跳过创建操作！" << std::endl;
            return false;
        }
        else if (lResult != ERROR_FILE_NOT_FOUND) {
            std::cerr << "检查注册表项失败，错误码：" << lResult << std::endl;
            return false;
        }

        DWORD dwDisposition = 0;
        lResult = RegCreateKeyExW(
            rootKey, regPath.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
            KEY_WRITE, nullptr, &tempKey, &dwDisposition
        );
        if (lResult != ERROR_SUCCESS) {
            std::cerr << "创建注册表项失败，错误码：" << lResult << std::endl;
            return false;
        }

        ScopedHKey hNewKey(tempKey);
        if (dwDisposition != REG_CREATED_NEW_KEY) {
            std::cerr << "注册表项未新建（已存在），终止操作！" << std::endl;
            return false;
        }

        lResult = RegSetValueExW(
            hNewKey.get(), valueName.c_str(), 0, REG_SZ,
            (const BYTE*)valueContent.c_str(),
            (DWORD)((valueContent.length() + 1) * sizeof(wchar_t))
        );
        if (lResult != ERROR_SUCCESS) {
            std::cerr << "写入注册表值失败，错误码：" << lResult << std::endl;
            return false;
        }

        std::wcout << L"注册表项创建成功！写入值：" << valueContent << std::endl;
        return true;
    };

    // 通用删除注册表值
 bool GenericRegistry::DeleteValue(HKEY rootKey, const std::wstring& regPath,
     const std::wstring& valueName) {
     HKEY tempKey = nullptr;
     LONG lResult = RegOpenKeyExW(
         rootKey, regPath.c_str(), 0, KEY_SET_VALUE, &tempKey
     );
     if (lResult != ERROR_SUCCESS) {
         if (lResult == ERROR_FILE_NOT_FOUND) {
             std::cerr << "注册表项不存在，无需删除！" << std::endl;
         }
         else {
             std::cerr << "打开注册表项失败，错误码：" << lResult << std::endl;
         }
         return false;
     }

     ScopedHKey hKey(tempKey);

     lResult = RegDeleteValueW(hKey.get(), valueName.c_str());
     if (lResult != ERROR_SUCCESS) {
         if (lResult == ERROR_FILE_NOT_FOUND) {
             std::cerr << "注册表值不存在，无需删除！" << std::endl;
         }
         else {
             std::cerr << "删除注册表值失败，错误码：" << lResult << std::endl;
         }
         return false;
     }

     std::wcout << L"成功删除注册表值：" << valueName << std::endl;
     return true;
 };
