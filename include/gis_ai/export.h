#ifndef GIS_AI_EXPORT_H
#define GIS_AI_EXPORT_H

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef GIS_AI_SHARED_BUILD
        #define GIS_AI_API __declspec(dllexport)
    #elif defined(GIS_AI_STATIC_BUILD)
        #define GIS_AI_API
    #else
        #define GIS_AI_API __declspec(dllimport)
    #endif
#else
    #define GIS_AI_API __attribute__((visibility("default")))
#endif

#endif
