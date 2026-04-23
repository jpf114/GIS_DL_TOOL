#ifndef GIS_AI_PLATFORM_H
#define GIS_AI_PLATFORM_H

#include <string>
#include "core/export.h"

namespace gis_ai {

#ifdef _WIN32
    #define GIS_AI_PLATFORM_WINDOWS 1
    #define GIS_AI_PATH_SEPARATOR '\\'
#else
    #define GIS_AI_PLATFORM_POSIX 1
    #define GIS_AI_PATH_SEPARATOR '/'
#endif

class GIS_AI_API Platform {
public:
    static std::string GetPathSeparator();
    static std::string JoinPath(const std::string& a, const std::string& b);
    static std::string GetExecutableDir();
    static std::string GetHomeDir();
    static bool FileExists(const std::string& path);
    static bool DirExists(const std::string& path);
    static bool CreateDir(const std::string& path);
    static unsigned int GetHardwareConcurrency();
};

} // namespace gis_ai

#endif // GIS_AI_PLATFORM_H
