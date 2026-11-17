#include "core/Config.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace dl
{
namespace
{
std::string Trim(const std::string& s)
{
    std::size_t a = 0;
    std::size_t b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a])) != 0)
    {
        ++a;
    }
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])) != 0)
    {
        --b;
    }
    return s.substr(a, b - a);
}

std::string Lower(std::string s)
{
    for (char& c : s)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

/// "1600   # pixels" -> "1600". A '#' only starts a comment when whitespace comes before it (so "a#b" stays as it is).
std::string StripInlineComment(const std::string& s)
{
    for (std::size_t i = 1; i < s.size(); ++i)
    {
        if (s[i] == '#' && std::isspace(static_cast<unsigned char>(s[i - 1])) != 0)
        {
            return s.substr(0, i);
        }
    }
    return s;
}

std::string MakeKey(const std::string& section, const std::string& key)
{
    return Lower(section) + "." + Lower(key);
}
} // namespace

Config::ParseResult Config::ParseString(const std::string& text)
{
    ParseResult result;
    m_values.clear();
    std::istringstream in(text);
    std::string line;
    std::string section;
    int lineNo = 0;
    while (std::getline(in, line))
    {
        ++lineNo;
        const std::string t = Trim(line);
        if (t.empty() || t[0] == '#' || t[0] == ';')
        {
            continue;
        }
        if (t[0] == '[')
        {
            if (t.back() != ']')
            {
                result.errors.push_back("line " + std::to_string(lineNo) + ": section is missing ']'");
                continue;
            }
            section = Trim(t.substr(1, t.size() - 2));
            continue;
        }
        const std::size_t eq = t.find('=');
        if (eq == std::string::npos)
        {
            result.errors.push_back("line " + std::to_string(lineNo) + ": expected 'key = value'");
            continue;
        }
        const std::string key = Trim(t.substr(0, eq));
        if (key.empty())
        {
            result.errors.push_back("line " + std::to_string(lineNo) + ": empty key");
            continue;
        }
        m_values[MakeKey(section, key)] = Trim(StripInlineComment(t.substr(eq + 1)));
        ++result.entries;
    }
    return result;
}

Config::ParseResult Config::LoadFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        ParseResult result;
        result.errors.push_back("cannot open " + path);
        return result;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string text = buffer.str();
    if (text.size() >= 3 && text.compare(0, 3, "\xEF\xBB\xBF") == 0)
    {
        text.erase(0, 3); // WHY: Windows editors like to write a BOM
    }
    return ParseString(text);
}

bool Config::Has(const std::string& section, const std::string& key) const
{
    return m_values.count(MakeKey(section, key)) != 0;
}

std::string Config::GetString(const std::string& section, const std::string& key, const std::string& fallback) const
{
    const auto it = m_values.find(MakeKey(section, key));
    return it != m_values.end() ? it->second : fallback;
}

int Config::GetInt(const std::string& section, const std::string& key, int fallback) const
{
    const auto it = m_values.find(MakeKey(section, key));
    if (it == m_values.end() || it->second.empty())
    {
        return fallback;
    }
    char* end = nullptr;
    const long v = std::strtol(it->second.c_str(), &end, 10);
    return (*end == '\0') ? static_cast<int>(v) : fallback;
}

float Config::GetFloat(const std::string& section, const std::string& key, float fallback) const
{
    const auto it = m_values.find(MakeKey(section, key));
    if (it == m_values.end() || it->second.empty())
    {
        return fallback;
    }
    char* end = nullptr;
    const double v = std::strtod(it->second.c_str(), &end);
    return (*end == '\0') ? static_cast<float>(v) : fallback;
}

bool Config::GetBool(const std::string& section, const std::string& key, bool fallback) const
{
    const auto it = m_values.find(MakeKey(section, key));
    if (it == m_values.end())
    {
        return fallback;
    }
    const std::string v = Lower(it->second);
    if (v == "1" || v == "true" || v == "yes" || v == "on")
    {
        return true;
    }
    if (v == "0" || v == "false" || v == "no" || v == "off")
    {
        return false;
    }
    return fallback;
}
} // namespace dl
