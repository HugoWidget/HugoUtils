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

#include <iostream>

#include "WinUtils/ini.h"
namespace WinUtils
{
	using namespace INIParser;

	void INIStringUtil::trim(string_t& str)
	{
		str.erase(str.find_last_not_of(whitespaceDelimiters) + 1);
		str.erase(0, str.find_first_not_of(whitespaceDelimiters));
	}

	void INIStringUtil::toLower(string_t& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), [](const char_t c) {
			return static_cast<char>(std::tolower(c));
			});
	}

	void INIStringUtil::replace(string_t& str, string_t const& a, string_t const& b)
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


	PDataType INIParser::parseLine(string_t line, T_ParseValues& parseData)
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

	class INIReader::INIReaderPrivate {
	public:
		ifstream_t fileReadStream;
		T_LineDataPtr lineData;
	};

	INIReader::INIReader(std::filesystem::path const& filename, bool keepLineData)
		: pImpl(new INIReaderPrivate)
	{
		pImpl->fileReadStream.open(filename, std::ios::in | std::ios::binary);
		if (keepLineData)
		{
			pImpl->lineData = std::make_shared<T_LineData>();
		}
	}

	INIReader::~INIReader()
	{
		delete pImpl;
	}

	INIReader::T_LineData INIReader::readFile()
	{
		pImpl->fileReadStream.seekg(0, std::ios::end);
		const std::size_t fileSize = static_cast<std::size_t>(pImpl->fileReadStream.tellg());
		pImpl->fileReadStream.seekg(0, std::ios::beg);
		if (fileSize >= 3) {
			const char_t header[3] = {
				static_cast<char_t>(pImpl->fileReadStream.get()),
				static_cast<char_t>(pImpl->fileReadStream.get()),
				static_cast<char_t>(pImpl->fileReadStream.get())
			};
			isBOM = (
				header[0] == static_cast<char_t>(0xEF) &&
				header[1] == static_cast<char_t>(0xBB) &&
				header[2] == static_cast<char_t>(0xBF)
				);
		}
		else {
			isBOM = false;
		}
		string_t fileContents;
		fileContents.resize(fileSize);
		pImpl->fileReadStream.seekg(isBOM ? 3 : 0, std::ios::beg);
		pImpl->fileReadStream.read(fileContents.data(), fileSize);
		pImpl->fileReadStream.close();
		T_LineData output;
		if (fileSize == 0)
		{
			return output;
		}
		string_t buffer;
		buffer.reserve(50);
		for (std::size_t i = 0; i < fileSize; ++i)
		{
			const char_t& c = fileContents[i];
			if (c == '\n')
			{
				output.emplace_back(buffer);
				buffer.clear();
				continue;
			}
			if (c != '\0' && c != '\r')
			{
				buffer += c;
			}
		}
		output.emplace_back(buffer);
		return output;
	}

	bool INIReader::operator>>(INIStructure& data)
	{
		if (!pImpl->fileReadStream.is_open())
		{
			return false;
		}
		const T_LineData fileLines = readFile();
		string_t section;
		bool inSection = false;
		INIParser::T_ParseValues parseData;
		for (auto const& line : fileLines)
		{
			auto parseResult = INIParser::parseLine(line, parseData);
			if (parseResult == INIParser::PDataType::PDATA_SECTION)
			{
				inSection = true;
				data[section = parseData.first];
			}
			else if (inSection && parseResult == INIParser::PDataType::PDATA_KEYVALUE)
			{
				auto const& key = parseData.first;
				auto const& value = parseData.second;
				data[section][key] = value;
			}
			if (pImpl->lineData && parseResult != INIParser::PDataType::PDATA_UNKNOWN)
			{
				if (parseResult == INIParser::PDataType::PDATA_KEYVALUE && !inSection)
				{
					continue;
				}
				pImpl->lineData->emplace_back(line);
			}
		}
		return true;
	}

	INIReader::T_LineDataPtr INIReader::getLines()
	{
		return pImpl->lineData;
	}

	class INIGenerator::INIGeneratorPrivate {
	public:
		ofstream_t fileWriteStream;
	};
	INIGenerator::INIGenerator(std::filesystem::path const& filename)
		: pImpl(new INIGeneratorPrivate)
	{
		pImpl->fileWriteStream.open(filename, std::ios::out | std::ios::binary);
	}

	INIGenerator::~INIGenerator()
	{
		delete pImpl;
	}

	bool INIGenerator::operator<<(INIStructure const& data)
	{
		if (!pImpl->fileWriteStream.is_open())
		{
			return false;
		}
		if (data.size() == 0U)
		{
			return true;
		}
		auto it = data.begin();
		for (;;)
		{
			auto const& section = it->first;
			auto const& collection = it->second;
			pImpl->fileWriteStream
				<< TS("[")
				<< section
				<< TS("]");
			if (collection.size() != 0U)
			{
				pImpl->fileWriteStream << INIStringUtil::endl;
				auto it2 = collection.begin();
				for (;;)
				{
					auto key = it2->first;
					INIStringUtil::replace(key, TS("="), TS("\\="));
					auto value = it2->second;
					INIStringUtil::trim(value);
					pImpl->fileWriteStream
						<< key
						<< ((prettyPrint) ? TS(" = ") : TS("="))
						<< value;
					if (++it2 == collection.end())
					{
						break;
					}
					pImpl->fileWriteStream << INIStringUtil::endl;
				}
			}
			if (++it == data.end())
			{
				break;
			}
			pImpl->fileWriteStream << INIStringUtil::endl;
			if (prettyPrint)
			{
				pImpl->fileWriteStream << INIStringUtil::endl;
			}
		}
		return true;
	}

	class INIWriter::INIWriterPrivate {
	public:
		std::filesystem::path filename;
	};
	INIWriter::INIWriter(std::filesystem::path filename)
		: pImpl(new INIWriterPrivate)
	{
		pImpl->filename = std::move(filename);
	}

	INIWriter::~INIWriter()
	{
		delete pImpl;
	}

	INIWriter::T_LineData INIWriter::getLazyOutput(T_LineDataPtr const& lineData, INIStructure& data, INIStructure& original) const
	{
		T_LineData output;
		INIParser::T_ParseValues parseData;
		string_t sectionCurrent;
		bool parsingSection = false;
		bool continueToNextSection = false;
		bool discardNextEmpty = false;
		bool writeNewKeys = false;
		std::size_t lastKeyLine = 0;
		for (auto line = lineData->begin(); line != lineData->end(); ++line)
		{
			if (!writeNewKeys)
			{
				auto parseResult = INIParser::parseLine(*line, parseData);
				if (parseResult == INIParser::PDataType::PDATA_SECTION)
				{
					if (parsingSection)
					{
						writeNewKeys = true;
						parsingSection = false;
						--line;
						continue;
					}
					sectionCurrent = parseData.first;
					if (data.has(sectionCurrent))
					{
						parsingSection = true;
						continueToNextSection = false;
						discardNextEmpty = false;
						output.emplace_back(*line);
						lastKeyLine = output.size();
					}
					else
					{
						continueToNextSection = true;
						discardNextEmpty = true;
						continue;
					}
				}
				else if (parseResult == INIParser::PDataType::PDATA_KEYVALUE)
				{
					if (continueToNextSection)
					{
						continue;
					}
					if (data.has(sectionCurrent))
					{
						auto& collection = data[sectionCurrent];
						auto const& key = parseData.first;
						auto const& value = parseData.second;
						if (collection.has(key))
						{
							auto outputValue = collection[key];
							if (value == outputValue)
							{
								output.emplace_back(*line);
							}
							else
							{
								INIStringUtil::trim(outputValue);
								auto lineNorm = *line;
								INIStringUtil::replace(lineNorm, TS("\\="), TS("  "));
								auto equalsAt = lineNorm.find_first_of('=');
								auto valueAt = lineNorm.find_first_not_of(
									INIStringUtil::whitespaceDelimiters,
									equalsAt + 1
								);
								string_t outputLine = line->substr(0, valueAt);
								if (prettyPrint && equalsAt + 1 == valueAt)
								{
									outputLine += TS(" ");
								}
								outputLine += outputValue;
								output.emplace_back(outputLine);
							}
							lastKeyLine = output.size();
						}
					}
				}
				else
				{
					if (discardNextEmpty && line->empty())
					{
						discardNextEmpty = false;
					}
					else if (parseResult != INIParser::PDataType::PDATA_UNKNOWN)
					{
						output.emplace_back(*line);
					}
				}
			}
			if (writeNewKeys || std::next(line) == lineData->end())
			{
				T_LineData linesToAdd;
				if (data.has(sectionCurrent) && original.has(sectionCurrent))
				{
					auto const& collection = data[sectionCurrent];
					auto const& collectionOriginal = original[sectionCurrent];
					for (auto const& it : collection)
					{
						auto key = it.first;
						if (collectionOriginal.has(key))
						{
							continue;
						}
						auto value = it.second;
						INIStringUtil::replace(key, TS("="), TS("\\="));
						INIStringUtil::trim(value);
						linesToAdd.emplace_back(
							key + ((prettyPrint) ? TS(" = ") : TS("=")) + value
						);
					}
				}
				if (!linesToAdd.empty())
				{
					output.insert(
						output.begin() + lastKeyLine,
						linesToAdd.begin(),
						linesToAdd.end()
					);
				}
				if (writeNewKeys)
				{
					writeNewKeys = false;
					--line;
				}
			}
		}
		for (auto const& it : data)
		{
			auto const& section = it.first;
			if (original.has(section))
			{
				continue;
			}
			if (prettyPrint && !output.empty() && !output.back().empty())
			{
				output.emplace_back();
			}
			output.emplace_back(TS("[") + section + TS("]"));
			auto const& collection = it.second;
			for (auto const& it2 : collection)
			{
				auto key = it2.first;
				auto value = it2.second;
				INIStringUtil::replace(key, TS("="), TS("\\="));
				INIStringUtil::trim(value);
				output.emplace_back(
					key + ((prettyPrint) ? TS(" = ") : TS("=")) + value
				);
			}
		}
		return output;
	}

	bool INIWriter::operator<<(INIStructure& data)
	{
		if (!std::filesystem::exists(pImpl->filename))
		{
			INIGenerator generator(pImpl->filename);
			generator.prettyPrint = prettyPrint;
			return generator << data;
		}
		INIStructure originalData;
		T_LineDataPtr lineData;
		bool readSuccess = false;
		bool fileIsBOM = false;
		{
			INIReader reader(pImpl->filename, true);
			readSuccess = reader >> originalData;
			if (readSuccess)
			{
				lineData = reader.getLines();
				fileIsBOM = reader.isBOM;
			}
		}
		if (!readSuccess)
		{
			return false;
		}
		T_LineData output = getLazyOutput(lineData, data, originalData);
		ofstream_t fileWriteStream(pImpl->filename, std::ios::out | std::ios::binary);
		if (fileWriteStream.is_open())
		{
			if (fileIsBOM) {
				const char_t utf8_BOM[3] = {
					static_cast<char_t>(0xEF),
					static_cast<char_t>(0xBB),
					static_cast<char_t>(0xBF)
				};
				fileWriteStream.write(utf8_BOM, 3);
			}
			if (!output.empty())
			{
				auto line = output.begin();
				for (;;)
				{
					fileWriteStream << *line;
					if (++line == output.end())
					{
						break;
					}
					fileWriteStream << INIStringUtil::endl;
				}
			}
			return true;
		}
		return false;
	}

	class INIFile::INIFilePrivate {
	public:
		std::filesystem::path filename;
	};
	INIFile::INIFile(std::filesystem::path filename)
		: pImpl(new INIFilePrivate)
	{
		pImpl->filename = std::move(filename);
	}

	INIFile::~INIFile()
	{
		delete pImpl;
	}

	bool INIFile::read(INIStructure& data) const
	{
		if (data.size() != 0U)
		{
			data.clear();
		}
		if (pImpl->filename.empty())
		{
			return false;
		}
		INIReader reader(pImpl->filename);
		return reader >> data;
	}

	bool INIFile::generate(INIStructure const& data, bool pretty) const
	{
		if (pImpl->filename.empty())
		{
			return false;
		}
		INIGenerator generator(pImpl->filename);
		generator.prettyPrint = pretty;
		return generator << data;
	}

	bool INIFile::write(INIStructure& data, bool pretty) const
	{
		if (pImpl->filename.empty())
		{
			return false;
		}
		INIWriter writer(pImpl->filename);
		writer.prettyPrint = pretty;
		return writer << data;
	}
}

