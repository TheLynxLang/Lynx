// platform.h
#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>
    #include <direct.h>
    #define strdup _strdup
    #define strtok_r strtok_s
    #define mkdir(path, mode) _mkdir(path)
    #define PATH_SEP '\\'
    #define PLATFORM_WINDOWS 1
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

    // ----- Windows API → POSIX mappings -----
    #define LoadLibrary(path) dlopen(path, RTLD_LAZY)
    #define GetProcAddress(handle, name) dlsym(handle, name)
    #define FreeLibrary(handle) dlclose(handle)

    // ----- Stub out Windows-only stuff -----
    #define ShellExecute(a,b,c,d,e,f) 0
#endif

#endif