#include "core/config.h"
#include "core/platform.h"
#include "core/exception.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <mutex>

namespace gis_ai {

static std::mutex g_config_mutex;

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

    try {
        nlohmann::json j = nlohmann::json::parse(file);
        std::lock_guard<std::mutex> lock(g_config_mutex);
        for (auto it = j.begin(); it != j.end(); ++it) {
            const auto& key = it.key();
            const auto& val = it.value();
            if (val.is_string()) {
                values_[key] = val.get<std::string>();
            } else if (val.is_boolean()) {
                values_[key] = val.get<bool>();
            } else if (val.is_number_integer()) {
                values_[key] = val.get<int>();
            } else if (val.is_number_float()) {
                values_[key] = val.get<double>();
            }
        }
        return true;
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

bool Config::LoadFromString(const std::string& json_str) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_str);
        std::lock_guard<std::mutex> lock(g_config_mutex);
        for (auto it = j.begin(); it != j.end(); ++it) {
            const auto& key = it.key();
            const auto& val = it.value();
            if (val.is_string()) {
                values_[key] = val.get<std::string>();
            } else if (val.is_boolean()) {
                values_[key] = val.get<bool>();
            } else if (val.is_number_integer()) {
                values_[key] = val.get<int>();
            } else if (val.is_number_float()) {
                values_[key] = val.get<double>();
            }
        }
        return true;
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

void Config::SaveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw GisAiConfigException("Cannot open config file for writing: " + path);
    }

    nlohmann::json j;
    {
        std::lock_guard<std::mutex> lock(g_config_mutex);
        for (const auto& [key, value] : values_) {
            std::visit([&j, &key](const auto& v) {
                j[key] = v;
            }, value);
        }
    }

    file << j.dump(2) << "\n";
}

std::optional<Config::Value> Config::Get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(g_config_mutex);
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
    std::lock_guard<std::mutex> lock(g_config_mutex);
    values_[key] = value;
}

bool Config::Has(const std::string& key) const {
    std::lock_guard<std::mutex> lock(g_config_mutex);
    return values_.find(key) != values_.end();
}

void Config::Remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(g_config_mutex);
    values_.erase(key);
}

void Config::Clear() {
    std::lock_guard<std::mutex> lock(g_config_mutex);
    values_.clear();
}

} // namespace gis_ai
