/*
 * Copyright 2025-2026 howdy213, JYardX
 *
 * This file is part of HugoUtils.
 *
 * HugoUtils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HugoUtils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with HugoUtils. If not, see <https://www.gnu.org/licenses/>.
 */
#include "HugoUtils/HugoUtilsDef.h"
#ifndef HU_DISABLE_FREEZE

#include <fstream>
#include <format>
#include <filesystem>

#include "HugoUtils/HugoFreeze/HFreezeFileBackend.h"
#include "WinUtils/Logger.h"

using namespace WinUtils;
using namespace std;

static Logger logger(L"HFreezeFileBackend");

HFreezeFileBackend::HFreezeFileBackend() noexcept
    : m_configPath(FRZ_CONFIG_PATH) {
}

void HFreezeFileBackend::setConfigPath(const std::wstring& path) noexcept {
    m_configPath = path;
}

const std::wstring& HFreezeFileBackend::getConfigPath() const noexcept {
    return m_configPath;
}

bool HFreezeFileBackend::getConfig(HConfigFile& out) const noexcept {
    filesystem::path p(m_configPath);
    ifstream configFile(p, ios::binary | ios::in);
    if (!configFile.is_open()) {
        DWORD err = GetLastError();
        logger.DLog(LogLevel::Error, format(L"Open config fail, err: {}", err));
        return false;
    }

    unsigned char buffer[FRZ_CONFIG_SIZE] = { 0 };
    configFile.read(reinterpret_cast<char*>(buffer), FRZ_CONFIG_SIZE);
    if (!configFile || configFile.gcount() != static_cast<streamsize>(FRZ_CONFIG_SIZE)) {
        logger.DLog(LogLevel::Error, L"Invalid config size");
        configFile.close();
        return false;
    }
    configFile.close();

    out = HConfigFile(buffer);
    return true;
}

bool HFreezeFileBackend::setConfig(const HConfigFile& cfg) const noexcept {
    unsigned char buffer[FRZ_CONFIG_SIZE] = { 0 };
    cfg.toBuffer(buffer);

    HANDLE hConfigFile = CreateFileW(
        m_configPath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        0,
        nullptr);

    if (hConfigFile == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        logger.DLog(LogLevel::Error,
            format(L"Open config file fail, err: {}", err));
        return false;
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(hConfigFile, buffer, static_cast<DWORD>(FRZ_CONFIG_SIZE),
        &bytesWritten, nullptr)) {
        DWORD err = GetLastError();
        logger.DLog(LogLevel::Error,
            format(L"Write config file fail, err: {}", err));
        CloseHandle(hConfigFile);
        return false;
    }

    CloseHandle(hConfigFile);
    return true;
}

bool HFreezeFileBackend::readBlob(unsigned char* data, size_t size) const noexcept {
    if (!data || size < FRZ_CONFIG_SIZE) {
        return false;
    }

    HConfigFile cfg;
    if (!getConfig(cfg)) {
        return false;
    }
    cfg.toBuffer(data);
    return true;
}

bool HFreezeFileBackend::writeBlob(const unsigned char* data, size_t size) const noexcept {
    if (!data || size < FRZ_CONFIG_SIZE) {
        return false;
    }

    return setConfig(HConfigFile(data));
}

#endif // !HU_DISABLE_FREEZE
