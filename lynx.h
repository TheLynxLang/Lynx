#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    // ─── WINDOWS ──────────────────────────────────────────────
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
    // ─── LINUX / POSIX ────────────────────────────────────────
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
    
    // ─── Windows types → POSIX equivalents ──────────────────
    #define HINSTANCE void*
    #define HMODULE void*
    #define FARPROC void*
    
    // ─── LoadLibrary macros ──────────────────────────────────
    #ifndef LoadLibrary
    #define LoadLibrary(path) dlopen(path, RTLD_LAZY)
    #define GetProcAddress(handle, name) dlsym(handle, name)
    #define FreeLibrary(handle) dlclose(handle)
    #endif
    
    // ─── Windows-only functions (stubs) ──────────────────────
    #define URLDownloadToFileA(a,b,c,d,e) 0
    #define S_OK 0
    #define GetModuleFileNameA(a,b,c) 0
    
    // ShellExecute stub - just returns 0
    static inline int ShellExecuteA(void* a, const char* b, const char* c,
                                    const char* d, const char* e, int f) {
        return 0;
    }
    
    // ─── Windows console stub ─────────────────────────────────
    static inline void SetConsoleOutputCP(int cp) {
        (void)cp;  // Do nothing on Linux
    }
    
#endif

#endif
