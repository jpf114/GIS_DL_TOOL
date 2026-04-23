#ifndef GIS_AI_CONFIG_H
#define GIS_AI_CONFIG_H

#include <string>
#include <unordered_map>
#include <variant>
#include <optional>

namespace gis_ai {

class Config {
public:
    using Value = std::variant<std::string, int, double, bool>;

    static Config& Instance();

    bool LoadFromFile(const std::string& path);
    bool LoadFromString(const std::string& json_str);
    void SaveToFile(const std::string& path) const;

    std::optional<Value> Get(const std::string& key) const;
    std::string GetString(const std::string& key, const std::string& default_val = "") const;
    int GetInt(const std::string& key, int default_val = 0) const;
    double GetDouble(const std::string& key, double default_val = 0.0) const;
    bool GetBool(const std::string& key, bool default_val = false) const;

    void Set(const std::string& key, const Value& value);
    bool Has(const std::string& key) const;
    void Remove(const std::string& key);
    void Clear();

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::unordered_map<std::string, Value> values_;
};

} // namespace gis_ai

#endif // GIS_AI_CONFIG_H
