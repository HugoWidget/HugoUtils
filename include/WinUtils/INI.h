/*
 * The MIT License (MIT)
 * Copyright (c) 2018 Danijel Durakovic
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
#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
#include <unordered_map>
#include <vector>
#include <memory>
#include <fstream>
#include <cctype>
#include <filesystem>

#include "WinUtilsDef.h"
namespace WinUtils
{

	namespace INIStringUtil
	{
#ifdef _WIN32
		inline const char_t* const endl = TS("\r\n");
#else
		inline const char_t* const endl = TS("\n");
#endif
		inline const char_t* const whitespaceDelimiters = TS(" \t\n\r\f\v");
		WUAPI void trim(string_t& str);
		WUAPI void toLower(string_t& str);
		WUAPI void replace(string_t& str, string_t const& a, string_t const& b);
	}

	template<typename T>
	class WUAPI INIMap
	{
	private:
		using T_DataIndexMap = std::unordered_map<string_t, std::size_t>;
		using T_DataItem = std::pair<string_t, T>;
		using T_DataContainer = std::vector<T_DataItem>;
		using T_MultiArgs = typename std::vector<std::pair<string_t, T>>;

		class INIMapPrivate;
		INIMapPrivate* pImpl = nullptr;
		std::size_t setEmpty(string_t& key);
	public:
		using const_iterator = typename T_DataContainer::const_iterator;

		INIMap();
		~INIMap();
		INIMap(INIMap const& other);

		T& operator[](string_t key);
		[[nodiscard]] T get(string_t key) const;
		[[nodiscard]] bool has(string_t key) const;
		void set(string_t key, T obj);
		void set(T_MultiArgs const& multiArgs);
		bool remove(string_t key);
		void clear();
		[[nodiscard]] std::size_t size() const;
		[[nodiscard]] const_iterator begin() const;
		[[nodiscard]] const_iterator end() const;

		template<typename U>
		[[nodiscard]] U get_as(string_t key, U default_value) const
		{
			T val = get(key);
			if (val.empty()) return default_value;
			istringstream_t iss(val);
			U result;
			iss >> result;
			if (iss.fail()) return default_value;
			return result;
		}
	};

	using INIStructure = INIMap<INIMap<string_t>>;
	namespace INIParser
	{
		using T_ParseValues = std::pair<string_t, string_t>;
		enum class PDataType : char
		{
			PDATA_NONE,
			PDATA_COMMENT,
			PDATA_SECTION,
			PDATA_KEYVALUE,
			PDATA_UNKNOWN
		};
		WUAPI PDataType parseLine(string_t line, T_ParseValues& parseData);
	}

	class WUAPI INIReader
	{
	public:
		using T_LineData = std::vector<string_t>;
		using T_LineDataPtr = std::shared_ptr<T_LineData>;

		bool isBOM = false;

		INIReader(std::filesystem::path const& filename, bool keepLineData = false);
		~INIReader();

		bool operator>>(INIStructure& data);
		T_LineDataPtr getLines();

	private:
		class INIReaderPrivate;
		INIReaderPrivate* pImpl = nullptr;
		T_LineData readFile();
	};

	class WUAPI INIGenerator
	{
	public:
		bool prettyPrint = false;

		INIGenerator(std::filesystem::path const& filename);
		~INIGenerator();

		bool operator<<(INIStructure const& data);

	private:
		class INIGeneratorPrivate;
		INIGeneratorPrivate* pImpl = nullptr;
	};

	class WUAPI INIWriter
	{
	public:
		bool prettyPrint = false;

		INIWriter(std::filesystem::path filename);
		~INIWriter();
		
		bool operator<<(INIStructure& data);

	private:
		using T_LineData = std::vector<string_t>;
		using T_LineDataPtr = std::shared_ptr<T_LineData>;
		class INIWriterPrivate;
		INIWriterPrivate* pImpl = nullptr;
		T_LineData getLazyOutput(T_LineDataPtr const& lineData, INIStructure& data, INIStructure& original) const;
	};

	class WUAPI INIFile
	{
	public:
		INIFile(std::filesystem::path filename);
		~INIFile();

		bool read(INIStructure& data) const;
		[[nodiscard]] bool generate(INIStructure const& data, bool pretty = false) const;
		bool write(INIStructure& data, bool pretty = false) const;

	private:
		class INIFilePrivate;
		INIFilePrivate* pImpl = nullptr;
	};
}


namespace WinUtils
{
	template<typename T>
	class INIMap<T>::INIMapPrivate
	{
	public:
		T_DataIndexMap dataIndexMap;
		T_DataContainer data;
	};

	template<typename T>
	INIMap<T>::INIMap() : pImpl(new INIMapPrivate) {}

	template<typename T>
	INIMap<T>::~INIMap() {
		delete pImpl;
	};

	template<typename T>
	std::size_t INIMap<T>::setEmpty(string_t& key)
	{
		const std::size_t index = pImpl->data.size();
		pImpl->dataIndexMap[key] = index;
		pImpl->data.emplace_back(key, T());
		return index;
	}

	template<typename T>
	INIMap<T>::INIMap(INIMap const& other) : pImpl(new INIMapPrivate)
	{
		pImpl->dataIndexMap = other.pImpl->dataIndexMap;
		pImpl->data = other.pImpl->data;
	}

	template<typename T>
	T& INIMap<T>::operator[](string_t key)
	{
		INIStringUtil::trim(key);
#if WU_INI_CASE_SENSITIVE
		INIStringUtil::toLower(key);
#endif
		auto it = pImpl->dataIndexMap.find(key);
		const bool hasIt = (it != pImpl->dataIndexMap.end());
		const std::size_t index = (hasIt) ? it->second : setEmpty(key);
		return pImpl->data[index].second;
	}

	template<typename T>
	T INIMap<T>::get(string_t key) const
	{
		INIStringUtil::trim(key);
#if WU_INI_CASE_SENSITIVE
		INIStringUtil::toLower(key);
#endif
		auto it = pImpl->dataIndexMap.find(key);
		if (it == pImpl->dataIndexMap.end())
		{
			return T();
		}
		return T(pImpl->data[it->second].second);
	}

	template<typename T>
	bool INIMap<T>::has(string_t key) const
	{
		INIStringUtil::trim(key);
#if WU_INI_CASE_SENSITIVE
		INIStringUtil::toLower(key);
#endif
		return (pImpl->dataIndexMap.count(key) == 1);
	}

	template<typename T>
	void INIMap<T>::set(string_t key, T obj)
	{
		INIStringUtil::trim(key);
#if WU_INI_CASE_SENSITIVE
		INIStringUtil::toLower(key);
#endif
		auto it = pImpl->dataIndexMap.find(key);
		if (it != pImpl->dataIndexMap.end())
		{
			pImpl->data[it->second].second = obj;
		}
		else
		{
			pImpl->dataIndexMap[key] = pImpl->data.size();
			pImpl->data.emplace_back(key, obj);
		}
	}

	template<typename T>
	void INIMap<T>::set(T_MultiArgs const& multiArgs)
	{
		for (auto const& it : multiArgs)
		{
			auto const& key = it.first;
			auto const& obj = it.second;
			set(key, obj);
		}
	}

	template<typename T>
	bool INIMap<T>::remove(string_t key)
	{
		INIStringUtil::trim(key);
#if WU_INI_CASE_SENSITIVE
		INIStringUtil::toLower(key);
#endif
		auto it = pImpl->dataIndexMap.find(key);
		if (it != pImpl->dataIndexMap.end())
		{
			std::size_t index = it->second;
			pImpl->data.erase(pImpl->data.begin() + index);
			pImpl->dataIndexMap.erase(it);
			for (auto& it2 : pImpl->dataIndexMap)
			{
				auto& vi = it2.second;
				if (vi > index)
				{
					vi--;
				}
			}
			return true;
		}
		return false;
	}

	template<typename T>
	void INIMap<T>::clear()
	{
		pImpl->data.clear();
		pImpl->dataIndexMap.clear();
	}

	template<typename T>
	std::size_t INIMap<T>::size() const
	{
		return pImpl->data.size();
	}

	template<typename T>
	typename INIMap<T>::const_iterator INIMap<T>::begin() const
	{
		return pImpl->data.begin();
	}

	template<typename T>
	typename INIMap<T>::const_iterator INIMap<T>::end() const
	{
		return pImpl->data.end();
	}
}