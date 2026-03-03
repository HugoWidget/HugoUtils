#ifndef INI_H
#define INI_H

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
#define INI_CASE_SENSITIVE
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
		inline void trim(string_t& str)
		{
			str.erase(str.find_last_not_of(whitespaceDelimiters) + 1);
			str.erase(0, str.find_first_not_of(whitespaceDelimiters));
		}
#ifndef INI_CASE_SENSITIVE
		inline void toLower(string_t& str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](const char_t c) {
				return static_cast<char>(std::tolower(c));
				});
		}
#endif
		inline void replace(string_t& str, string_t const& a, string_t const& b)
		{
			if (!a.empty())
			{
				std::size_t pos = 0;
				while ((pos = str.find(a, pos)) != string_t::npos)
				{
					str.replace(pos, a.size(), b);
					pos += b.size();
				}
			}
		}
	}

	template<typename T>
	class INIMap
	{
	private:
		using T_DataIndexMap = std::unordered_map<string_t, std::size_t>;
		using T_DataItem = std::pair<string_t, T>;
		using T_DataContainer = std::vector<T_DataItem>;
		using T_MultiArgs = typename std::vector<std::pair<string_t, T>>;

		T_DataIndexMap dataIndexMap;
		T_DataContainer data;

		std::size_t setEmpty(string_t& key);

	public:
		using const_iterator = typename T_DataContainer::const_iterator;

		INIMap() = default;
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

		inline PDataType parseLine(string_t line, T_ParseValues& parseData)
		{
			parseData.first.clear();
			parseData.second.clear();
			INIStringUtil::trim(line);
			if (line.empty())
			{
				return PDataType::PDATA_NONE;
			}
			const char_t firstCharacter = line[0];
			if (firstCharacter == TS(';'))
			{
				return PDataType::PDATA_COMMENT;
			}
			if (firstCharacter == TS('['))
			{
				auto commentAt = line.find_first_of(TS(';'));
				if (commentAt != string_t::npos)
				{
					line = line.substr(0, commentAt);
				}
				auto closingBracketAt = line.find_last_of(TS(']'));
				if (closingBracketAt != string_t::npos)
				{
					auto section = line.substr(1, closingBracketAt - 1);
					INIStringUtil::trim(section);
					parseData.first = section;
					return PDataType::PDATA_SECTION;
				}
			}
			auto lineNorm = line;
			INIStringUtil::replace(lineNorm, TS("\\="), TS("  "));
			auto equalsAt = lineNorm.find_first_of('=');
			if (equalsAt != string_t::npos)
			{
				auto key = line.substr(0, equalsAt);
				INIStringUtil::trim(key);
				INIStringUtil::replace(key, TS("\\="), TS("="));
				auto value = line.substr(equalsAt + 1);
				INIStringUtil::trim(value);
				parseData.first = key;
				parseData.second = value;
				return PDataType::PDATA_KEYVALUE;
			}
			return PDataType::PDATA_UNKNOWN;
		}
	}

	class INIWriter;

	class INIReader
	{
	public:
		using T_LineData = std::vector<string_t>;
		using T_LineDataPtr = std::shared_ptr<T_LineData>;

		bool isBOM = false;

		INIReader(std::filesystem::path const& filename, bool keepLineData = false);
		~INIReader() = default;

		bool operator>>(INIStructure& data);
		T_LineDataPtr getLines();

	private:
		ifstream_t fileReadStream;
		T_LineDataPtr lineData;

		T_LineData readFile();
	};

	class INIGenerator
	{
	public:
		bool prettyPrint = false;

		INIGenerator(std::filesystem::path const& filename);
		~INIGenerator() = default;

		bool operator<<(INIStructure const& data);

	private:
		ofstream_t fileWriteStream;
	};

	class INIWriter
	{
	public:
		bool prettyPrint = false;

		INIWriter(std::filesystem::path filename);
		~INIWriter() = default;

		bool operator<<(INIStructure& data);

	private:
		using T_LineData = std::vector<string_t>;
		using T_LineDataPtr = std::shared_ptr<T_LineData>;

		std::filesystem::path filename;

		T_LineData getLazyOutput(T_LineDataPtr const& lineData, INIStructure& data, INIStructure& original) const;
	};

	class INIFile
	{
	public:
		INIFile(std::filesystem::path filename);
		~INIFile() = default;

		bool read(INIStructure& data) const;
		[[nodiscard]] bool generate(INIStructure const& data, bool pretty = false) const;
		bool write(INIStructure& data, bool pretty = false) const;

	private:
		std::filesystem::path filename;
	};
}


namespace WinUtils
{
	template<typename T>
	std::size_t INIMap<T>::setEmpty(string_t& key)
	{
		const std::size_t index = data.size();
		dataIndexMap[key] = index;
		data.emplace_back(key, T());
		return index;
	}

	template<typename T>
	INIMap<T>::INIMap(INIMap const& other) : dataIndexMap(other.dataIndexMap), data(other.data)
	{
	}

	template<typename T>
	T& INIMap<T>::operator[](string_t key)
	{
		INIStringUtil::trim(key);
#ifndef INI_CASE_SENSITIVE
		INIStringUtil::toLower(key);
#endif
		auto it = dataIndexMap.find(key);
		const bool hasIt = (it != dataIndexMap.end());
		const std::size_t index = (hasIt) ? it->second : setEmpty(key);
		return data[index].second;
	}

	template<typename T>
	T INIMap<T>::get(string_t key) const
	{
		INIStringUtil::trim(key);
#ifndef INI_CASE_SENSITIVE
		INIStringUtil::toLower(key);
#endif
		auto it = dataIndexMap.find(key);
		if (it == dataIndexMap.end())
		{
			return T();
		}
		return T(data[it->second].second);
	}

	template<typename T>
	bool INIMap<T>::has(string_t key) const
	{
		INIStringUtil::trim(key);
#ifndef INI_CASE_SENSITIVE
		INIStringUtil::toLower(key);
#endif
		return (dataIndexMap.count(key) == 1);
	}

	template<typename T>
	void INIMap<T>::set(string_t key, T obj)
	{
		INIStringUtil::trim(key);
#ifndef INI_CASE_SENSITIVE
		INIStringUtil::toLower(key);
#endif
		auto it = dataIndexMap.find(key);
		if (it != dataIndexMap.end())
		{
			data[it->second].second = obj;
		}
		else
		{
			dataIndexMap[key] = data.size();
			data.emplace_back(key, obj);
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
#ifndef INI_CASE_SENSITIVE
		INIStringUtil::toLower(key);
#endif
		auto it = dataIndexMap.find(key);
		if (it != dataIndexMap.end())
		{
			std::size_t index = it->second;
			data.erase(data.begin() + index);
			dataIndexMap.erase(it);
			for (auto& it2 : dataIndexMap)
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
		data.clear();
		dataIndexMap.clear();
	}

	template<typename T>
	std::size_t INIMap<T>::size() const
	{
		return data.size();
	}

	template<typename T>
	typename INIMap<T>::const_iterator INIMap<T>::begin() const
	{
		return data.begin();
	}

	template<typename T>
	typename INIMap<T>::const_iterator INIMap<T>::end() const
	{
		return data.end();
	}
}

#endif // INI_H