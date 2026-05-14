#include <gis_ai/gis_ai.h>

int main() {
    const int major = GisAi_GetVersionMajor();
    const int minor = GisAi_GetVersionMinor();
    const int patch = GisAi_GetVersionPatch();
    const char* version = GisAi_GetVersionString();
    return (major < 0 || minor < 0 || patch < 0 || version == nullptr) ? 1 : 0;
}
