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

bool Config::LoadFromString(const std::string& json_str) {
    enum class ParseState {
        Start,
        InObject,
        ExpectKey,
        InKey,
        ExpectColon,
        ExpectValue,
        InStringValue,
        InNumberValue,
        InBoolValue,
        ExpectCommaOrEnd
    };

    ParseState state = ParseState::Start;
    std::string current_key;
    std::string current_value;
    bool in_escape = false;

    for (size_t i = 0; i < json_str.size(); ++i) {
        char c = json_str[i];

        switch (state) {
            case ParseState::Start:
                if (c == '{') state = ParseState::InObject;
                break;
            case ParseState::InObject:
                if (c == '"') {
                    state = ParseState::InKey;
                    current_key.clear();
                } else if (c == '}') {
                    return true;
                } else if (!std::isspace(static_cast<unsigned char>(c))) {
                    return false;
                }
                break;
            case ParseState::InKey:
                if (in_escape) {
                    current_key += c;
                    in_escape = false;
                } else if (c == '\\') {
                    in_escape = true;
                } else if (c == '"') {
                    state = ParseState::ExpectColon;
                    in_escape = false;
                } else {
                    current_key += c;
                }
                break;
            case ParseState::ExpectColon:
                if (c == ':') state = ParseState::ExpectValue;
                else if (!std::isspace(static_cast<unsigned char>(c))) return false;
                break;
            case ParseState::ExpectValue:
                if (c == '"') {
                    state = ParseState::InStringValue;
                    current_value.clear();
                    in_escape = false;
                } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
                    state = ParseState::InNumberValue;
                    current_value = c;
                } else if (c == 't' || c == 'f') {
                    state = ParseState::InBoolValue;
                    current_value = c;
                } else if (!std::isspace(static_cast<unsigned char>(c))) {
                    return false;
                }
                break;
            case ParseState::InStringValue:
                if (in_escape) {
                    current_value += c;
                    in_escape = false;
                } else if (c == '\\') {
                    in_escape = true;
                } else if (c == '"') {
                    values_[current_key] = current_value;
                    state = ParseState::ExpectCommaOrEnd;
                } else {
                    current_value += c;
                }
                break;
            case ParseState::InNumberValue:
                if (std::isdigit(static_cast<unsigned char>(c)) || c == '.' ||
                    c == 'e' || c == 'E' || c == '+' || c == '-') {
                    current_value += c;
                } else {
                    if (current_value.find('.') != std::string::npos ||
                        current_value.find('e') != std::string::npos ||
                        current_value.find('E') != std::string::npos) {
                        try {
                            values_[current_key] = std::stod(current_value);
                        } catch (...) {
                            values_[current_key] = 0.0;
                        }
                    } else {
                        try {
                            values_[current_key] = std::stoi(current_value);
                        } catch (...) {
                            try {
                                values_[current_key] = std::stod(current_value);
                            } catch (...) {
                                values_[current_key] = 0;
                            }
                        }
                    }
                    i--;
                    state = ParseState::ExpectCommaOrEnd;
                }
                break;
            case ParseState::InBoolValue:
                if (std::isalpha(static_cast<unsigned char>(c))) {
                    current_value += c;
                } else {
                    values_[current_key] = (current_value == "true");
                    i--;
                    state = ParseState::ExpectCommaOrEnd;
                }
                break;
            case ParseState::ExpectCommaOrEnd:
                if (c == ',') {
                    state = ParseState::InObject;
                } else if (c == '}') {
                    return true;
                } else if (!std::isspace(static_cast<unsigned char>(c))) {
                    return false;
                }
                break;
        }
    }

    return state == ParseState::ExpectCommaOrEnd || state == ParseState::InObject;
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
