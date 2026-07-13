// platform.h
#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define strdup _strdup
    #define strtok_r strtok_s
    #define mkdir(path, mode) _mkdir(path)
    #define PATH_SEP '\\'
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <dirent.h>
    #define PATH_SEP '/'
#endif

#endif