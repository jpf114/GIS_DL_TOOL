#include "core/platform.h"

#ifdef GIS_AI_PLATFORM_WINDOWS
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <pwd.h>
#endif

#include <filesystem>
#include <thread>

namespace gis_ai {

std::string Platform::GetPathSeparator() {
    return std::string(1, GIS_AI_PATH_SEPARATOR);
}

std::string Platform::JoinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    char sep = GIS_AI_PATH_SEPARATOR;
    if (a.back() == sep || a.back() == '/' || a.back() == '\\') {
        return a + b;
    }
    return a + sep + b;
}

std::string Platform::GetExecutableDir() {
#ifdef GIS_AI_PLATFORM_WINDOWS
    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        std::string exe_path(path, len);
        auto pos = exe_path.find_last_of("\\/");
        if (pos != std::string::npos) {
            return exe_path.substr(0, pos);
        }
    }
#else
    char path[4096];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len > 0) {
        path[len] = '\0';
        std::string exe_path(path);
        auto pos = exe_path.find_last_of('/');
        if (pos != std::string::npos) {
            return exe_path.substr(0, pos);
        }
    }
#endif
    try {
        return std::filesystem::current_path().string();
    } catch (...) {
        return ".";
    }
}

std::string Platform::GetHomeDir() {
#ifdef GIS_AI_PLATFORM_WINDOWS
    const char* home = getenv("USERPROFILE");
    if (home) return std::string(home);
    return ".";
#else
    const char* home = getenv("HOME");
    if (home) return std::string(home);
    struct passwd* pw = getpwuid(getuid());
    return pw ? std::string(pw->pw_dir) : ".";
#endif
}

bool Platform::FileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool Platform::DirExists(const std::string& path) {
    return std::filesystem::is_directory(path);
}

bool Platform::CreateDir(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (...) {
        return false;
    }
}

unsigned int Platform::GetHardwareConcurrency() {
    return std::thread::hardware_concurrency();
}

} // namespace gis_ai
