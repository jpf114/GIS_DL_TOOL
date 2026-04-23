#include "core/config.h"
#include "core/platform.h"
#include "core/exception.h"
#include <fstream>
#include <sstream>

namespace gis_ai {

Config& Config::Instance() {
    static Config instance;
    return instance;
}

bool Config::LoadFromFile(const std::string& path) {
    if (!Platform::FileExists(path)) {
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return LoadFromString(ss.str());
}

bool Config::LoadFromString(const std::string& /*json_str*/) {
    // Minimal JSON parsing - a production implementation would use
    // nlohmann/json or similar. For now, accept that the config
    // is primarily set via Set() calls.
    return true;
}

void Config::SaveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw GisAiConfigException("Cannot open config file for writing: " + path);
    }

    file << "{\n";
    bool first = true;
    for (const auto& [key, value] : values_) {
        if (!first) file << ",\n";
        first = false;
        file << "  \"" << key << "\": ";
        std::visit([&file](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                file << "\"" << v << "\"";
            } else if constexpr (std::is_same_v<T, bool>) {
                file << (v ? "true" : "false");
            } else {
                file << v;
            }
        }, value);
    }
    file << "\n}\n";
}

std::optional<Config::Value> Config::Get(const std::string& key) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string Config::GetString(const std::string& key, const std::string& default_val) const {
    auto val = Get(key);
    if (val && std::holds_alternative<std::string>(*val)) {
        return std::get<std::string>(*val);
    }
    return default_val;
}

int Config::GetInt(const std::string& key, int default_val) const {
    auto val = Get(key);
    if (val && std::holds_alternative<int>(*val)) {
        return std::get<int>(*val);
    }
    return default_val;
}

double Config::GetDouble(const std::string& key, double default_val) const {
    auto val = Get(key);
    if (val && std::holds_alternative<double>(*val)) {
        return std::get<double>(*val);
    }
    return default_val;
}

bool Config::GetBool(const std::string& key, bool default_val) const {
    auto val = Get(key);
    if (val && std::holds_alternative<bool>(*val)) {
        return std::get<bool>(*val);
    }
    return default_val;
}

void Config::Set(const std::string& key, const Value& value) {
    values_[key] = value;
}

bool Config::Has(const std::string& key) const {
    return values_.find(key) != values_.end();
}

void Config::Remove(const std::string& key) {
    values_.erase(key);
}

void Config::Clear() {
    values_.clear();
}

} // namespace gis_ai
