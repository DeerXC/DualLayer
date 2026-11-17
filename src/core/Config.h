// Config.h - tiny INI reader for tunable values (data-driven: change a number without recompiling; F5 reloads it in the game).
// Syntax:   [section]   key = value   lines starting with # or ; are comments; a " #" after a value starts a comment too.
// Access:   Get*(section, key, default) never fails: a missing or malformed value gives the default.
// Owns:     its key/value map.   Threading: main thread only.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace dl
{
class Config
{
public:
    struct ParseResult
    {
        int entries = 0;                  ///< number of key = value pairs read
        std::vector<std::string> errors;  ///< "line N: reason" (parsing continues after an error)
    };

    /// Replaces all values. Pure: no file access.
    ParseResult ParseString(const std::string& text);

    /// Reads a file. A file that cannot be opened is reported in `errors` and leaves the values unchanged.
    ParseResult LoadFile(const std::string& path);

    bool Has(const std::string& section, const std::string& key) const;
    std::string GetString(const std::string& section, const std::string& key, const std::string& fallback) const;
    int GetInt(const std::string& section, const std::string& key, int fallback) const;
    float GetFloat(const std::string& section, const std::string& key, float fallback) const; // UNITS: as written in the file
    bool GetBool(const std::string& section, const std::string& key, bool fallback) const;

private:
    std::map<std::string, std::string> m_values; // key = "section.key"
};
} // namespace dl
