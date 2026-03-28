/*
 * The MIT License (MIT)
 * Copyright(c) 2017-2025 by Giovanni Dicanio
 * Modifications Copyright (c) 2026 howdy213
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#pragma once
#ifndef WINREG_H
#define WINREG_H

#include "WinUtils/WinPch.h"

#include <Windows.h>
#include <crtdbg.h>

#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>
#include <span>
#include <expected>

#include "WinUtilsDef.h"

namespace WinUtils
{
	//------------------------------------------------------------------------------
	// A class template that stores a value of type T (e.g. DWORD, string_t)
	// on success, or a RegResult on error.
	//
	// Used as the return value of some Registry RegKey::TryGetXxxValue() methods
	// as an alternative to exception-throwing methods.
	//------------------------------------------------------------------------------
	template<typename T> using RegExpected = std::expected<T, RegResult>;

	//------------------------------------------------------------------------------
	//
	// Safe, efficient and convenient C++ wrapper around HKEY registry key handles.
	//
	// This class is movable but not copyable.
	//
	// This class is designed to be very *efficient* and low-overhead, for example:
	// non-throwing operations are carefully marked as noexcept, so the C++ compiler
	// can emit optimized code.
	//
	// Moreover, this class just wraps a raw HKEY handle, without any
	// shared-ownership overhead like in std::shared_ptr; you can think of this
	// class kind of like a std::unique_ptr for HKEYs.
	//
	// The class is also swappable (defines a custom non-member swap);
	// relational operators are properly overloaded as well.
	//
	//------------------------------------------------------------------------------
	class WUAPI RegKey
	{
	public:

		//
		// Construction/Destruction
		//

		// Initialize as an empty key handle
		RegKey() noexcept = default;

		// Take ownership of the input key handle
		explicit RegKey(HKEY hKey) noexcept;

		// Open the given registry key if it exists, else create a new key.
		// Uses default KEY_READ|KEY_WRITE|KEY_WOW64_64KEY access.
		// For finer grained control, call the Create() method overloads.
		// Throw RegException on failure.
		RegKey(HKEY hKeyParent, const string_t& subKey);

		// Open the given registry key if it exists, else create a new key.
		// Allow the caller to specify the desired access to the key
		// (e.g. KEY_READ|KEY_WOW64_64KEY for read-only access).
		// For finer grained control, call the Create() method overloads.
		// Throw RegException on failure.
		RegKey(HKEY hKeyParent, const string_t& subKey, REGSAM desiredAccess);


		// Take ownership of the input key handle.
		// The input key handle wrapper is reset to an empty state.
		RegKey(RegKey&& other) noexcept;

		// Move-assign from the input key handle.
		// Properly check against self-move-assign (which is safe and does nothing).
		RegKey& operator=(RegKey&& other) noexcept;

		// Ban copy
		RegKey(const RegKey&) = delete;
		RegKey& operator=(const RegKey&) = delete;

		// Safely close the wrapped key handle (if any)
		~RegKey() noexcept;


		//
		// Properties
		//

		// Access the wrapped raw HKEY handle
		[[nodiscard]] HKEY Get() const noexcept;

		// Is the wrapped HKEY handle valid?
		[[nodiscard]] bool IsValid() const noexcept;

		// Same as IsValid(), but allow a short "if (regKey)" syntax
		[[nodiscard]] explicit operator bool() const noexcept;

		// Is the wrapped handle a predefined handle (e.g.HKEY_CURRENT_USER) ?
		[[nodiscard]] bool IsPredefined() const noexcept;


		//
		// Operations
		//

		// Close current HKEY handle.
		// If there's no valid handle, do nothing.
		// This method doesn't close predefined HKEY handles (e.g. HKEY_CURRENT_USER).
		void Close() noexcept;

		// Transfer ownership of current HKEY to the caller.
		// Note that the caller is responsible for closing the key handle!
		[[nodiscard]] HKEY Detach() noexcept;

		// Take ownership of the input HKEY handle.
		// Safely close any previously open handle.
		// Input key handle can be nullptr.
		void Attach(HKEY hKey) noexcept;

		// Non-throwing swap;
		// Note: There's also a non-member swap overload
		void SwapWith(RegKey& other) noexcept;


		//
		// Wrappers around Windows Registry APIs.
		// See the official MSDN documentation for these APIs for detailed explanations
		// of the wrapper method parameters.
		//

		//
		// NOTE on the KEY_WOW64_64KEY flag
		// ================================
		//
		// By default, a 32-bit application running on 64-bit Windows accesses the 32-bit registry view
		// and a 64-bit application accesses the 64-bit registry view.
		// Using this KEY_WOW64_64KEY flag, both 32-bit or 64-bit applications access the 64-bit
		// registry view.
		//
		// MSDN documentation:
		// https://docs.microsoft.com/en-us/windows/win32/winprog64/accessing-an-alternate-registry-view
		//
		// If you want to use the default Windows API behavior, don't OR (|) the KEY_WOW64_64KEY flag
		// when specifying the desired access (e.g. just pass KEY_READ | KEY_WRITE as the desired
		// access parameter).
		//

		// Wrapper around RegCreateKeyEx, that allows you to specify desired access
		void Create(
			HKEY hKeyParent,
			const string_t& subKey,
			REGSAM desiredAccess = KEY_READ | KEY_WRITE | KEY_WOW64_64KEY
		);

		// Wrapper around RegCreateKeyEx
		void Create(
			HKEY hKeyParent,
			const string_t& subKey,
			REGSAM desiredAccess,
			DWORD options,
			SECURITY_ATTRIBUTES* securityAttributes,
			DWORD* disposition
		);

		// Wrapper around RegOpenKeyEx
		void Open(
			HKEY hKeyParent,
			const string_t& subKey,
			REGSAM desiredAccess = KEY_READ | KEY_WRITE | KEY_WOW64_64KEY
		);

		// Wrapper around RegCreateKeyEx, that allows you to specify desired access
		[[nodiscard]] RegResult TryCreate(
			HKEY hKeyParent,
			const string_t& subKey,
			REGSAM desiredAccess = KEY_READ | KEY_WRITE | KEY_WOW64_64KEY
		) noexcept;

		// Wrapper around RegCreateKeyEx
		[[nodiscard]] RegResult TryCreate(
			HKEY hKeyParent,
			const string_t& subKey,
			REGSAM desiredAccess,
			DWORD options,
			SECURITY_ATTRIBUTES* securityAttributes,
			DWORD* disposition
		) noexcept;

		// Wrapper around RegOpenKeyEx
		[[nodiscard]] RegResult TryOpen(
			HKEY hKeyParent,
			const string_t& subKey,
			REGSAM desiredAccess = KEY_READ | KEY_WRITE | KEY_WOW64_64KEY
		) noexcept;


		//
		// Registry Value Setters
		//

		void SetDwordValue(const string_t& valueName, DWORD data);
		void SetQwordValue(const string_t& valueName, const ULONGLONG& data);
		void SetStringValue(const string_t& valueName, const string_t& data);
		void SetExpandStringValue(const string_t& valueName, const string_t& data);
		void SetMultiStringValue(const string_t& valueName, const std::vector<string_t>& data);
		void SetBinaryValue(const string_t& valueName, std::span<const BYTE> data);


		//
		// Registry Value Setters Returning RegResult
		// (instead of throwing RegException on error)
		//

		[[nodiscard]] RegResult TrySetDwordValue(const string_t& valueName, DWORD data) noexcept;

		[[nodiscard]] RegResult TrySetQwordValue(const string_t& valueName,
			const ULONGLONG& data) noexcept;

		[[nodiscard]] RegResult TrySetStringValue(const string_t& valueName,
			const string_t& data);
		// Note: The TrySetStringValue method CANNOT be marked noexcept,
		// because internally the method *may* throw an exception if the input string is too big
		// (size_t value overflowing a DWORD).

		[[nodiscard]] RegResult TrySetExpandStringValue(const string_t& valueName,
			const string_t& data);
		// Note: The TrySetExpandStringValue method CANNOT be marked noexcept,
		// because internally the method *may* throw an exception if the input string is too big
		// (size_t value overflowing a DWORD).

		[[nodiscard]] RegResult TrySetMultiStringValue(const string_t& valueName,
			const std::vector<string_t>& data);
		// Note: The TrySetMultiStringValue method CANNOT be marked noexcept,
		// because internally the method *dynamically allocates memory* for creating the multi-string
		// that will be stored in the Registry.

		[[nodiscard]] RegResult TrySetBinaryValue(const string_t& valueName,
			std::span<const BYTE> data);
		// Note: This overload of the TrySetBinaryValue method CANNOT be marked noexcept,
		// because internally the method *may* throw an exception if the input span is too large
		// (span::size size_t value overflowing a DWORD).


		//
		// Registry Value Getters
		//

		[[nodiscard]] DWORD GetDwordValue(const string_t& valueName) const;
		[[nodiscard]] ULONGLONG GetQwordValue(const string_t& valueName) const;
		[[nodiscard]] string_t GetStringValue(const string_t& valueName) const;

		enum class ExpandStringOption
		{
			DontExpand,
			Expand
		};

		[[nodiscard]] string_t GetExpandStringValue(
			const string_t& valueName,
			ExpandStringOption expandOption = ExpandStringOption::DontExpand
		) const;

		[[nodiscard]] std::vector<string_t> GetMultiStringValue(const string_t& valueName) const;
		[[nodiscard]] std::vector<BYTE> GetBinaryValue(const string_t& valueName) const;

		// GetRawValue is very similar to GetBinaryValue, except that here in the "raw" version
		// I do *not* use the RRF_RT_REG_BINARY flag with the ::RegGetValueW API for type checking.
		// In this way, the type is *not* restricted to REG_BINARY, and this GetRawValue method
		// can be invoked to retrieve binary data for unsupported or less documented types like
		// REG_RESOURCE_LIST, REG_FULL_RESOURCE_DESCRIPTOR, REG_LINK, etc.
		//
		// This addition came from an issue opened on this code on GitHub:
		//
		// No way to get the value of a REG_RESOURCE_LIST key and some others
		// https://github.com/GiovanniDicanio/WinReg/issues/77
		//
		[[nodiscard]] std::vector<BYTE> GetRawValue(const string_t& valueName) const;


		//
		// Registry Value Getters Returning RegExpected<T>
		// (instead of throwing RegException on error)
		//

		[[nodiscard]] RegExpected<DWORD> TryGetDwordValue(const string_t& valueName) const;
		[[nodiscard]] RegExpected<ULONGLONG> TryGetQwordValue(const string_t& valueName) const;
		[[nodiscard]] RegExpected<string_t> TryGetStringValue(const string_t& valueName) const;

		[[nodiscard]] RegExpected<string_t> TryGetExpandStringValue(
			const string_t& valueName,
			ExpandStringOption expandOption = ExpandStringOption::DontExpand
		) const;

		[[nodiscard]] RegExpected<std::vector<string_t>>
			TryGetMultiStringValue(const string_t& valueName) const;

		[[nodiscard]] RegExpected<std::vector<BYTE>>
			TryGetBinaryValue(const string_t& valueName) const;

		// Similar to TryGetBinaryValue, but without type restriction to REG_BINARY.
		// Can be used to retrieve raw binary data for other types like REG_RESOURCE_LIST, etc.
		// Please read the comment to the GetRawValue method to learn more.
		[[nodiscard]] RegExpected<std::vector<BYTE>>
			TryGetRawValue(const string_t& valueName) const;


		//
		// Query Operations
		//

		// Information about a registry key (retrieved by QueryInfoKey)
		struct InfoKey
		{
			DWORD    NumberOfSubKeys;
			DWORD    NumberOfValues;
			FILETIME LastWriteTime;

			// Clear the structure fields
			InfoKey() noexcept
				: NumberOfSubKeys{ 0 }
				, NumberOfValues{ 0 }
			{
				LastWriteTime.dwHighDateTime = LastWriteTime.dwLowDateTime = 0;
			}

			InfoKey(DWORD numberOfSubKeys, DWORD numberOfValues, FILETIME lastWriteTime) noexcept
				: NumberOfSubKeys{ numberOfSubKeys }
				, NumberOfValues{ numberOfValues }
				, LastWriteTime{ lastWriteTime }
			{
			}
		};

		// Retrieve information about the registry key
		[[nodiscard]] InfoKey QueryInfoKey() const;

		// Return the DWORD type ID for the input registry value
		[[nodiscard]] DWORD QueryValueType(const string_t& valueName) const;


		enum class KeyReflection
		{
			ReflectionEnabled,
			ReflectionDisabled
		};

		// Determines whether reflection has been disabled or enabled for the specified key
		[[nodiscard]] KeyReflection QueryReflectionKey() const;

		// Enumerate the subkeys of the registry key, using RegEnumKeyEx
		[[nodiscard]] std::vector<string_t> EnumSubKeys() const;

		// Enumerate the values under the registry key, using RegEnumValue.
		// Returns a vector of pairs: In each pair, the wstring is the value name,
		// the DWORD is the value type.
		[[nodiscard]] std::vector<std::pair<string_t, DWORD>> EnumValues() const;

		// Check if the current key contains the specified value
		[[nodiscard]] bool ContainsValue(const string_t& valueName) const;

		// Check if the current key contains the specified sub-key
		[[nodiscard]] bool ContainsSubKey(const string_t& subKey) const;


		//
		// Query Operations Returning RegExpected
		// (instead of throwing RegException on error)
		//

		// Retrieve information about the registry key
		[[nodiscard]] RegExpected<InfoKey> TryQueryInfoKey() const;

		// Return the DWORD type ID for the input registry value
		[[nodiscard]] RegExpected<DWORD> TryQueryValueType(const string_t& valueName) const;


		// Determines whether reflection has been disabled or enabled for the specified key
		[[nodiscard]] RegExpected<KeyReflection> TryQueryReflectionKey() const;

		// Enumerate the subkeys of the registry key, using RegEnumKeyEx
		[[nodiscard]] RegExpected<std::vector<string_t>> TryEnumSubKeys() const;

		// Enumerate the values under the registry key, using RegEnumValue.
		// Returns a vector of pairs: In each pair, the wstring is the value name,
		// the DWORD is the value type.
		[[nodiscard]] RegExpected<std::vector<std::pair<string_t, DWORD>>> TryEnumValues() const;

		// Check if the current key contains the specified value
		[[nodiscard]] RegExpected<bool> TryContainsValue(const string_t& valueName) const;

		// Check if the current key contains the specified sub-key
		[[nodiscard]] RegExpected<bool> TryContainsSubKey(const string_t& subKey) const;


		//
		// Misc Registry API Wrappers
		//

		void DeleteValue(const string_t& valueName);
		void DeleteKey(const string_t& subKey, REGSAM desiredAccess);
		void DeleteTree(const string_t& subKey);
		void CopyTree(const string_t& sourceSubKey, const RegKey& destKey);
		void FlushKey();
		void LoadKey(const string_t& subKey, const string_t& filename);
		void SaveKey(const string_t& filename, SECURITY_ATTRIBUTES* securityAttributes) const;
		void EnableReflectionKey();
		void DisableReflectionKey();
		void ConnectRegistry(const string_t& machineName, HKEY hKeyPredefined);


		//
		// Misc Registry API Wrappers Returning RegResult Status
		// (instead of throwing RegException on error)
		//

		[[nodiscard]] RegResult TryDeleteValue(const string_t& valueName) noexcept;
		[[nodiscard]] RegResult TryDeleteKey(const string_t& subKey, REGSAM desiredAccess) noexcept;
		[[nodiscard]] RegResult TryDeleteTree(const string_t& subKey) noexcept;

		[[nodiscard]] RegResult TryCopyTree(const string_t& sourceSubKey,
			const RegKey& destKey) noexcept;

		[[nodiscard]] RegResult TryFlushKey() noexcept;

		[[nodiscard]] RegResult TryLoadKey(const string_t& subKey,
			const string_t& filename) noexcept;

		[[nodiscard]] RegResult TrySaveKey(const string_t& filename,
			SECURITY_ATTRIBUTES* securityAttributes) const noexcept;

		[[nodiscard]] RegResult TryEnableReflectionKey() noexcept;
		[[nodiscard]] RegResult TryDisableReflectionKey() noexcept;

		[[nodiscard]] RegResult TryConnectRegistry(const string_t& machineName,
			HKEY hKeyPredefined) noexcept;


		// Return a string representation of Windows registry types
		[[nodiscard]] static string_t RegTypeToString(DWORD regType);

		// Use defaulted spaceship operator to automatically generate all six relational
		// operators (==, !=, <, <=, >, >=). Because the class only contains a single HKEY member,
		friend auto operator<=>(const RegKey&, const RegKey&) = default;

	private:
		// The wrapped registry key handle
		HKEY m_hKey{ nullptr };
	};


	//------------------------------------------------------------------------------
	// An exception representing an error with the registry operations
	//------------------------------------------------------------------------------
	class WUAPI RegException
		: public std::system_error
	{
	public:
		RegException(LSTATUS errorCode, const char* message);
		RegException(LSTATUS errorCode, const std::string& message);
	};


	//------------------------------------------------------------------------------
	// A tiny wrapper around LSTATUS return codes used by the Windows Registry API.
	//------------------------------------------------------------------------------
	class WUAPI RegResult
	{
	public:

		// Initialize to success code (ERROR_SUCCESS)
		RegResult() noexcept = default;

		// Initialize with specific Windows Registry API LSTATUS return code
		explicit RegResult(LSTATUS result) noexcept;

		// Is the wrapped code a success code?
		[[nodiscard]] bool IsOk() const noexcept;

		// Is the wrapped error code a failure code?
		[[nodiscard]] bool Failed() const noexcept;

		// Is the wrapped code a success code?
		[[nodiscard]] explicit operator bool() const noexcept;

		// Get the wrapped Win32 code
		[[nodiscard]] LSTATUS Code() const noexcept;

		// Return the system error message associated to the current error code
		[[nodiscard]] string_t ErrorMessage() const;

		// Return the system error message associated to the current error code,
		// using the given input language identifier
		[[nodiscard]] string_t ErrorMessage(DWORD languageId) const;

	private:
		// Error code returned by Windows Registry C API;
		// default initialized to success code.
		LSTATUS m_result{ ERROR_SUCCESS };
	};

}
#endif