#include "minipeak/gpu/PlatformSettings.h"
#include "vector"

PlatformSettingsFactory::PlatformSettingsFactory(const std::map<std::string, std::map<std::string, std::variant<float, int, std::string>>>& platform_settings) {
    std::vector<std::string> keys;
    auto default_it = platform_settings.find("");
    platform_settings_t defaults;
    if(default_it != platform_settings.end()) {
        defaults = default_it->second;
    }

    for(auto& platform : platform_settings) {
        auto settings = defaults;
        for(auto& kv : platform.second) {
            settings[kv.first] = kv.second;
        }
        this->platform_settings[platform.first] = settings;
    }
}

PlatformSettings PlatformSettingsFactory::operator()(const std::string& platform) {
    if(this->platform_settings.find(platform) != platform_settings.end()) {
        return PlatformSettings(platform_settings[platform]);
    }
    return PlatformSettings(platform_settings[""]);
}