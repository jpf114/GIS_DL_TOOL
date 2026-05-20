#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>
#include "param_card_widget.h"

namespace gis_ai::gui {

inline std::string getStringParam(const std::map<std::string, ParamValue>& params, const std::string& key) {
    auto it = params.find(key);
    if (it == params.end()) return {};
    if (auto* v = std::get_if<std::string>(&it->second)) return *v;
    return {};
}

inline int getIntParam(const std::map<std::string, ParamValue>& params, const std::string& key, int def = 0) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    if (auto* v = std::get_if<int>(&it->second)) return *v;
    return def;
}

inline double getDoubleParam(const std::map<std::string, ParamValue>& params, const std::string& key, double def = 0.0) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    if (auto* v = std::get_if<double>(&it->second)) return *v;
    if (auto* v = std::get_if<int>(&it->second)) return static_cast<double>(*v);
    return def;
}

inline bool getBoolParam(const std::map<std::string, ParamValue>& params, const std::string& key, bool def = false) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    if (auto* v = std::get_if<bool>(&it->second)) return *v;
    return def;
}

inline std::array<double, 4> getExtentParam(const std::map<std::string, ParamValue>& params, const std::string& key) {
    auto it = params.find(key);
    if (it == params.end()) return {0, 0, 0, 0};
    if (auto* v = std::get_if<std::array<double, 4>>(&it->second)) return *v;
    return {0, 0, 0, 0};
}

}
