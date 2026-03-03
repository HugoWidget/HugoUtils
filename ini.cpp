#include "ini.h"
#include <iostream>

namespace WinUtils
{
    INIReader::INIReader(std::filesystem::path const& filename, bool keepLineData)
        : fileReadStream(filename, std::ios::in | std::ios::binary)
    {
        if (keepLineData)
        {
            lineData = std::make_shared<T_LineData>();
        }
    }

    INIReader::T_LineData INIReader::readFile()
    {
        fileReadStream.seekg(0, std::ios::end);
        const std::size_t fileSize = static_cast<std::size_t>(fileReadStream.tellg());
        fileReadStream.seekg(0, std::ios::beg);
        if (fileSize >= 3) {
            const char_t header[3] = {
                static_cast<char_t>(fileReadStream.get()),
                static_cast<char_t>(fileReadStream.get()),
                static_cast<char_t>(fileReadStream.get())
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
        fileReadStream.seekg(isBOM ? 3 : 0, std::ios::beg);
        fileReadStream.read(fileContents.data(), fileSize);
        fileReadStream.close();
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
        if (!fileReadStream.is_open())
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
            if (lineData && parseResult != INIParser::PDataType::PDATA_UNKNOWN)
            {
                if (parseResult == INIParser::PDataType::PDATA_KEYVALUE && !inSection)
                {
                    continue;
                }
                lineData->emplace_back(line);
            }
        }
        return true;
    }

    INIReader::T_LineDataPtr INIReader::getLines()
    {
        return lineData;
    }

    // INIGenerator 成员函数实现
    INIGenerator::INIGenerator(std::filesystem::path const& filename)
        : fileWriteStream(filename, std::ios::out | std::ios::binary)
    {
    }

    bool INIGenerator::operator<<(INIStructure const& data)
    {
        if (!fileWriteStream.is_open())
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
            fileWriteStream
                << TS("[")
                << section
                << TS("]");
            if (collection.size() != 0U)
            {
                fileWriteStream << INIStringUtil::endl;
                auto it2 = collection.begin();
                for (;;)
                {
                    auto key = it2->first;
                    INIStringUtil::replace(key, TS("="), TS("\\="));
                    auto value = it2->second;
                    INIStringUtil::trim(value);
                    fileWriteStream
                        << key
                        << ((prettyPrint) ? TS(" = ") : TS("="))
                        << value;
                    if (++it2 == collection.end())
                    {
                        break;
                    }
                    fileWriteStream << INIStringUtil::endl;
                }
            }
            if (++it == data.end())
            {
                break;
            }
            fileWriteStream << INIStringUtil::endl;
            if (prettyPrint)
            {
                fileWriteStream << INIStringUtil::endl;
            }
        }
        return true;
    }

    INIWriter::INIWriter(std::filesystem::path filename)
        : filename(std::move(filename))
    {
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
        if (!std::filesystem::exists(filename))
        {
            INIGenerator generator(filename);
            generator.prettyPrint = prettyPrint;
            return generator << data;
        }
        INIStructure originalData;
        T_LineDataPtr lineData;
        bool readSuccess = false;
        bool fileIsBOM = false;
        {
            INIReader reader(filename, true);
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
        ofstream_t fileWriteStream(filename, std::ios::out | std::ios::binary);
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

    INIFile::INIFile(std::filesystem::path filename)
        : filename(std::move(filename))
    {
    }

    bool INIFile::read(INIStructure& data) const
    {
        if (data.size() != 0U)
        {
            data.clear();
        }
        if (filename.empty())
        {
            return false;
        }
        INIReader reader(filename);
        return reader >> data;
    }

    bool INIFile::generate(INIStructure const& data, bool pretty) const
    {
        if (filename.empty())
        {
            return false;
        }
        INIGenerator generator(filename);
        generator.prettyPrint = pretty;
        return generator << data;
    }

    bool INIFile::write(INIStructure& data, bool pretty) const
    {
        if (filename.empty())
        {
            return false;
        }
        INIWriter writer(filename);
        writer.prettyPrint = pretty;
        return writer << data;
    }
}