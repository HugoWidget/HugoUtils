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
#pragma once
 // #define HU_DISABLE_ALL
 // #define HU_DISABLE_ART
 // #define HU_DISABLE_GPL3
 // #define HU_DISABLE_INFO
 // #define HU_DISABLE_PASSWORD
 // #define HU_DISABLE_FREEZE
 // #define HU_DISABLE_FREEZE_API
 // #define HU_DISABLE_FREEZE_DRIVER
 // #define HU_DISABLE_MOUNT
 // #define HU_DISABLE_INSTALLER
#include "WinUtils/WinPch.h"

#ifdef HU_DISABLE_ALL
#define HU_DISABLE_ALL
#define HU_DISABLE_ART
#define HU_DISABLE_GPL3
#define HU_DISABLE_INFO
#define HU_DISABLE_PASSWORD
#define HU_DISABLE_FREEZE
#define HU_DISABLE_MOUNT
#define HU_DISABLE_INSTALLER
#endif

#ifdef HU_DISABLE_FREEZE
#define HU_DISABLE_FREEZE_API
#define HU_DISABLE_FREEZE_DRIVER
#endif

#if defined(_M_IX86)
#define HU_X86
#elif defined(_M_X64) || defined(_WIN64)
#define HU_X64
#else
#if defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 4
#define HU_X86
#elif defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 8
#define HU_X64
#else
#define HU_X64
#endif
#endif

#ifdef HU_X86
#define HU_DISABLE_INSTALLER
#define CPPHTTPLIB_HTTPLIB_H
#endif