#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lynx.h"
#include "platform.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

typedef struct {
    char name[64];
    HINSTANCE handle;
} LoadedLib;

// ─── GLOBALS ──────────────────────────────────────────────────
LoadedLib loaded_libs[32];
int lib_count = 0;

RegisteredFunc registered_funcs[256];
int registered_count = 0;

// ─── HELPER: Remove extension ───────────────────────────────────
static void strip_extension(char* str) {
    if (!str) return;
    char* dot = strrchr(str, '.');
    if (dot) *dot = '\0';
}

// ─── HELPER: Check if file exists ───────────────────────────────
static int file_exists(const char* path) {
    FILE* f = fopen(path, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

// ─── REGISTER FUNCTION (called by DLL) ──────────────────────────
void lynx_register_func(const char* name, void* func) {
    if (!name || !func) return;
    if (registered_count >= 256) {
        printf("🐾 Error: Max registered functions (256) exceeded\n");
        return;
    }
    
    // Check if already registered
    for (int i = 0; i < registered_count; i++) {
        if (strcmp(registered_funcs[i].name, name) == 0) {
            registered_funcs[i].func = func;
            return;
        }
    }
    
    strncpy(registered_funcs[registered_count].name, name, 63);
    registered_funcs[registered_count].name[63] = '\0';
    registered_funcs[registered_count].func = func;
    registered_count++;
}

// ─── FIND REGISTERED FUNCTION ────────────────────────────────────
void* lynx_find_func(const char* name) {
    if (!name) return NULL;
    for (int i = 0; i < registered_count; i++) {
        if (strcmp(registered_funcs[i].name, name) == 0) {
            return registered_funcs[i].func;
        }
    }
    return NULL;
}

// ─── LIST REGISTERED FUNCTIONS ──────────────────────────────────
void lynx_list_funcs() {
    printf("🐾 Registered functions:\n");
    for (int i = 0; i < registered_count; i++) {
        printf("  %s\n", registered_funcs[i].name);
    }
    if (registered_count == 0) {
        printf("  (none)\n");
    }
}

// ─── LOAD LIBRARY ───────────────────────────────────────────────
void load_lib(const char* lib_name) {
    if (!lib_name || strlen(lib_name) == 0) {
        printf("🐾 Error: Invalid library name\n");
        return;
    }
    
    if (strlen(lib_name) > 63) {
        printf("🐾 Error: Library name too long (max 63 chars)\n");
        return;
    }
    
    // Check if already loaded
    for (int i = 0; i < lib_count; i++) {
        if (strcmp(loaded_libs[i].name, lib_name) == 0) {
            printf("🐾 Library %s already loaded\n", lib_name);
            return;
        }
    }
    
    if (lib_count >= 32) {
        printf("🐾 Error: Maximum libraries (%d) already loaded\n", 32);
        return;
    }
    
    char path[LYNX_MAX_PATH];
    char lib_copy[64];
    strncpy(lib_copy, lib_name, 63);
    lib_copy[63] = '\0';
    strip_extension(lib_copy);
    
    HINSTANCE handle = NULL;
    int found = 0;
    
    // ─── Try 1: As-is (full path) ──────────────────────────────
    if (strchr(lib_name, '/') != NULL || strchr(lib_name, '\\') != NULL) {
        #ifdef _WIN32
        snprintf(path, LYNX_MAX_PATH, "%s.dll", lib_name);
        #else
        snprintf(path, LYNX_MAX_PATH, "%s.so", lib_name);
        #endif
        if (file_exists(path)) {
            handle = LoadLibrary(path);
            if (handle) found = 1;
        }
    }
    
    // ─── Try 2: libs/<pkgname>.dll ─────────────────────────────
    if (!found) {
        #ifdef _WIN32
        snprintf(path, LYNX_MAX_PATH, ".\\libs\\%s.dll", lib_copy);
        #else
        snprintf(path, LYNX_MAX_PATH, "./libs/%s.so", lib_copy);
        #endif
        if (file_exists(path)) {
            handle = LoadLibrary(path);
            if (handle) found = 1;
        }
    }
    
    // ─── Try 3: libs/<pkgname>/<pkgname>.dll ──────────────────
    if (!found) {
        #ifdef _WIN32
        snprintf(path, LYNX_MAX_PATH, ".\\libs\\%s\\%s.dll", lib_copy, lib_copy);
        #else
        snprintf(path, LYNX_MAX_PATH, "./libs/%s/%s.so", lib_copy, lib_copy);
        #endif
        if (file_exists(path)) {
            handle = LoadLibrary(path);
            if (handle) found = 1;
        }
    }
    
    // ─── Try 4: libs/<pkgname>/main.dll ────────────────────────
    if (!found) {
        #ifdef _WIN32
        snprintf(path, LYNX_MAX_PATH, ".\\libs\\%s\\main.dll", lib_copy);
        #else
        snprintf(path, LYNX_MAX_PATH, "./libs/%s/main.so", lib_copy);
        #endif
        if (file_exists(path)) {
            handle = LoadLibrary(path);
            if (handle) found = 1;
        }
    }
    
    // ─── Try 5: libs/<pkgname>/bin/<pkgname>.dll ──────────────
    if (!found) {
        #ifdef _WIN32
        snprintf(path, LYNX_MAX_PATH, ".\\libs\\%s\\bin\\%s.dll", lib_copy, lib_copy);
        #else
        snprintf(path, LYNX_MAX_PATH, "./libs/%s/bin/%s.so", lib_copy, lib_copy);
        #endif
        if (file_exists(path)) {
            handle = LoadLibrary(path);
            if (handle) found = 1;
        }
    }
    
    // ─── Try 6: Current directory ──────────────────────────────
    if (!found) {
        #ifdef _WIN32
        snprintf(path, LYNX_MAX_PATH, ".\\%s.dll", lib_copy);
        #else
        snprintf(path, LYNX_MAX_PATH, "./%s.so", lib_copy);
        #endif
        if (file_exists(path)) {
            handle = LoadLibrary(path);
            if (handle) found = 1;
        }
    }
    
    // ─── Try 7: System path ────────────────────────────────────
    if (!found) {
        #ifdef _WIN32
        snprintf(path, LYNX_MAX_PATH, "%s.dll", lib_copy);
        #else
        snprintf(path, LYNX_MAX_PATH, "%s.so", lib_copy);
        #endif
        handle = LoadLibrary(path);
        if (handle) found = 1;
    }
    
    if (!found) {
        printf("🐾 Failed to load %s\n", lib_name);
        return;
    }
    
    // ─── DLL LOADED ─────────────────────────────────────────────
    strncpy(loaded_libs[lib_count].name, lib_name, 63);
    loaded_libs[lib_count].name[63] = '\0';
    loaded_libs[lib_count].handle = handle;
    lib_count++;
    
    printf("🐾 Loaded library: %s\n", lib_name);
    
    // ─── REGISTER ALL EXPORTED FUNCTIONS ───────────────────────
    // Look for lynx_init() in the DLL
    typedef void (*LynxInitFunc)(void (*)(const char*, void*));
    LynxInitFunc init_func = (LynxInitFunc)GetProcAddress(handle, "lynx_init");
    
    if (init_func) {
        printf("🐾 Initializing %s...\n", lib_name);
        init_func(lynx_register_func);
        printf("🐾 Registered %d functions from %s\n", registered_count, lib_name);
    } else {
        // Try to register common functions by name
        printf("🐾 No lynx_init() found in %s\n", lib_name);
        printf("   Trying to register common functions...\n");
        
        // List of common function names to try
        const char* common_funcs[] = {
            "prowl", "sniff", "hiss", "pad",
            "scratch", "claw", "bat", "tail",
            "whisker", "nap",
            NULL
        };
        
        int registered = 0;
        for (int i = 0; common_funcs[i] != NULL; i++) {
            void* func = GetProcAddress(handle, common_funcs[i]);
            if (func) {
                lynx_register_func(common_funcs[i], func);
                registered++;
            }
        }
        printf("🐾 Registered %d common functions\n", registered);
    }
    
    // Show what's registered
    lynx_list_funcs();
}

// ─── UNLOAD ALL LIBRARIES ──────────────────────────────────────
void unload_all_libs() {
    for (int i = 0; i < lib_count; i++) {
        if (loaded_libs[i].handle) {
            FreeLibrary(loaded_libs[i].handle);
            loaded_libs[i].handle = NULL;
        }
    }
    lib_count = 0;
    registered_count = 0;
}
