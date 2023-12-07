#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <sstream>
#include <fstream>
#include <functional>

namespace jcd::utils 
{
    class DataFile 
    {
    public:
        inline DataFile() = default;

        inline void SetString(const std::string &str, size_t index = 0) 
        {
            if (index >= m_content.size()) {
                m_content.resize(index + 1);
            }

            m_content[index] = str;
        }

        inline const std::string GetString(size_t index = 0) const 
        {
            if (index < m_content.size()) {
                return m_content[index];
            }
            else {
                return "";
            }
        }

        inline double GetReal(size_t index = 0) const 
        {
            std::atof(GetString(index).c_str());
        }

        inline void SetReal(double d, size_t index = 0)
        {
            SetString(std::to_string(d), index);
        }

        inline int32_t GetInt(size_t index = 0) const 
        {
            std::atoi(GetString(index).c_str());
        }

        inline void SetInt(int32_t n, size_t index = 0)
        {
            SetString(std::to_string(n), index);
        }

        inline size_t GetValueCount() const
        {
            return m_content.size();
        }

        inline bool HasProperty(const std::string &name) const
        {
            return m_mapObjects.count(name) > 0;
        }

        inline DataFile & GetProperty(const std::string &name) 
        {
            size_t x = name.find_first_of('.');
            if (x != std::string::npos) {
                std::string property = name.substr(0, x);
                if (HasProperty(property)) {
                    return operator[](property).GetProperty(name.substr(x+1, name.size()));
                }
                else {
                    return operatro[](name);
                }
            }
            else {
                return operator[](name);
            }
        }

        inline DataFile & GetIndexedProperty(const std::string &name, const size_t index) 
        {
            return GetProperty(name + "[" + std::to_string(index) + "]");
        }

        inline DataFile & operator[](const std::string &name)
        {
            if (m_mapObjects.count(name) == 0) {
                m_mapObjects[name] = m_vecObjects.size();
                m_vecObjects.push_back({name, DataFile()});
            }

            return m_vecObjects[m_mapObjects[name]].second;
        }

        inline static bool Write(
            const DataFile &obj, 
            const std::string &fileName, 
            const std::string &indent = "\t", 
            const char sep = ',')
        {
            size_t indentLevel = 0;
            std::string contextSep = std::string(1, sep) + " ";

            std::function<void(const DataFile &, std::ofstream &)> writeFunc = [&](const DataFile &obj, std::ofstream &file) {
                auto getIndent = [&](const std::string &str, size_t level) {
                    std::string out;
                    for (size_t n = 0; n < level; ++n) {
                        out += str;
                    }

                    return out;
                };

                for (const auto &ele : obj.m_vecObjects) {
                    const std::string &propertyName = ele.first;
                    const DataFile &datafile = ele.second;

                    // property doesn't have sub objects
                    if (datafile.m_vecObjects.empty()) {
                        // print the key
                        file << getIndent(indent, indentLevel) << propertyName << (datafile.m_isComment ? "" : " = ");

                        // print the value(s)
                        size_t count = datafile.GetValueCount();
                        size_t numItemLeft = count;
                        for (size_t i = 0; i < count; ++i) {
                            std::string valStr = datafile.GetString(i);

                            size_t x = valStr.find_first_of(sep);
                            if (x != std::string::npos) {
                                file << "\"" << valStr << "\"";
                            }
                            else {
                                file << valStr;
                            }

                            file << ((numItemLeft > 1) ? contextSep : "");
                            --numItemLeft;
                        }

                        file << "\n";
                    }
                    // property does have sub objects
                    else {
                        file << "\n" << getIndent(indent, indentLevel) << propertyName << "\n";

                        file << getIndent(indent, indentLevel) << "{\n";
                        ++indentLevel;
                        writeFunc(datafile, file);
                        file << getIndent(indent, indentLevel) << "}\n\n";
                    }
                }

                if (indentLevel > 0) {
                    --indentLevel;
                }
            };

            std::ofstream file(fileName);
            if (!file.is_open()) {
                return false;
            }

            writeFunc(obj, file);
            return true;
        }

        inline static bool Read(
            DataFile &obj, 
            const std::string &filename, 
            const char sep = ',')
        {
            std::ifstream file(filename);
            if (!file.is_open()) {
                return false;
            }

            std::string propName = "";
            std::string propValue = "";

            std::stack<std::reference_wrapper<DataFile>> stackPath;
            stackPath.push(obj);

            while (!file.eof()) {
                std::string line;
                std::getline(file, line);

                auto trim = [](std::string &s) {
                    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
                    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
                };

                trim(line);

                if (!line.empty()) {
                    if (line[0] == '#') {
                        DataFile comment;
                        comment.m_isComment = true;
                        stackPath.top().get().m_vecObjects.push_back({line, comment});
                    }
                    else {
                        size_t x = line.find_first_of('=');
                        if (x != std::string::npos) {
                            propName = line.substr(0, x);
                            trim(propName);

                            propValue = line.substr(x + 1, line.size());
                            trim(propValue);

                            bool inQuotes = false;
                            std::string token;
                            size_t tokenCount = 0;
                            for (auto c : propValue) {
                                if (c == '\"') {
                                    inQuotes = !inQuotes;
                                }
                                else {
                                    if (inQuotes) {
                                        token.append(1, c);
                                    }
                                    else {
                                        if (c == sep) {
                                            trim(token);
                                            stackPath.top().get()[propName].SetString(token, tokenCount);
                                            token.clear();
                                            ++tokenCount;
                                        }
                                        else {
                                            token.append(1, c);
                                        }
                                    }
                                }
                            }

                            if (!token.empty()) {
                                trim(token);
                                stackPath.top().get()[propName].SetString(token, tokenCount);
                            }
                        }
                        else {
                            if (line[0] == '{') {
                                stackPath.push_back(stackPath.top().get()[propName]);
                            }
                            else {
                                if (line[0] == '}') {
                                    stackPath.pop();
                                }
                                else {
                                    propName = line;
                                }
                            }
                        }
                    }
                }
            }

            file.close();
            return true;
        }

    private:
        std::vector<std::string> m_content;

        std::vector<std::pair<std::string, DataFile>> m_vecObjects;
        std::unordered_map<std::string, size_t> m_mapObjects;

        bool m_isComment = false;
    };
}
