#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>
    #include <direct.h>
    #include <urlmon.h>
    #define strdup _strdup
    #define strtok_r strtok_s
    #define mkdir(path, mode) _mkdir(path)
    #define PATH_SEP '\\'
    #define PLATFORM_WINDOWS 1
    #pragma comment(lib, "urlmon.lib")
#else
    // Linux / POSIX
    #include <unistd.h>
    #include <sys/stat.h>
    #include <dirent.h>
    #include <dlfcn.h>
    #include <errno.h>
    #include <string.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <stdint.h>
    #define PATH_SEP '/'
    #define PLATFORM_WINDOWS 0

    // ----- Windows types → POSIX equivalents -----
    #define HINSTANCE void*
    #define HMODULE void*
    #define FARPROC void*

    // ----- LoadLibrary macros (only if not already defined) -----
    #ifndef LoadLibrary
    #define LoadLibrary(path) dlopen(path, RTLD_LAZY)
    #define GetProcAddress(handle, name) dlsym(handle, name)
    #define FreeLibrary(handle) dlclose(handle)
    #endif

    // ----- Stub for Windows-only functions -----
    #define URLDownloadToFileA(a,b,c,d,e) 0
    #define S_OK 0
#endif

#endif