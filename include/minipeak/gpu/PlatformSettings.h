#pragma once

#include <map>
#include <variant>
#include <string>

typedef std::map<std::string, std::variant<float, int, std::string>> platform_settings_t;
class PlatformSettings {
    std::map<std::string, std::variant<float, int, std::string>> settings;

public:
    PlatformSettings() {}
    PlatformSettings(std::map<std::string, std::variant<float, int, std::string>> settings) : settings(settings) {}

    template <typename T>
    T operator()(const std::string& key, T def = {}) const {
        auto it = settings.find(key);
        if(it != settings.end())
            return std::get<T>(it->second);
        return def;
    }

    bool operator()(const std::string& key, bool def = false) const {
        return (bool)this->operator()<int>(key, def);
    }
};

typedef std::map<std::string, std::map<std::string, std::variant<float, int, std::string>>> all_platform_settings_t;
class PlatformSettingsFactory {
    std::map<std::string, std::map<std::string, std::variant<float, int, std::string>>> platform_settings;
public:
    PlatformSettingsFactory(const std::map<std::string, std::map<std::string, std::variant<float, int, std::string>>>&);
    PlatformSettings operator()(const std::string& platform);
};
