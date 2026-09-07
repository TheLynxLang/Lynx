#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "platform.h"
#include <errno.h>
#include "lynx.h"

// Case-insensitive string compare for cross-platform
#ifdef _WIN32
#define STRICMP _stricmp
#else
#define STRICMP strcasecmp
#endif

#define LYNX_VERSION "v1.4.0"

extern Scanner scanner;
extern char* lynx_error;
extern LynxError lynx_error_state;

// ─── GLOBAL TRY/CATCH STATE ──────────────────────────────────
TryState try_state = {0};

// ─── DEBUG FLAG ──────────────────────────────────────────────────
int lynx_debug_mode = 1;  // 1 = on (default), 0 = off

// ─── DEBUG MACRO ─────────────────────────────────────────────────
#define DEBUG_PRINT(fmt, ...) \
    do { if (lynx_debug_mode) { printf("🐾 DEBUG " fmt, ##__VA_ARGS__); } } while(0)

// ─── CONFIG PATH ─────────────────────────────────────────────────
#define CONFIG_PATH ".lynx/config"

// ─── DIRECTORY HELPER ──────────────────────────────────────────
static void create_dir(const char* path) {
    #ifdef _WIN32
    _mkdir(path);
    #else
    mkdir(path, 0777);
    #endif
}

// ─── RECURSIVE MKDIR ──────────────────────────────────────────
static void mkdir_p(const char* path) {
    if (!path || strlen(path) == 0) return;
    
    char tmp[LYNX_MAX_PATH];
    char* p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    // Remove trailing slash
    if (len > 0 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\')) {
        tmp[len - 1] = '\0';
    }
    
    #ifdef _WIN32
    // Handle drive letters (C:)
    if (len >= 2 && tmp[1] == ':') {
        p = tmp + 2;
    } else {
        p = tmp + 1;
    }
    #else
    p = tmp + 1;
    #endif
    
    for (; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            #ifdef _WIN32
            _mkdir(tmp);
            #else
            mkdir(tmp, 0777);
            #endif
            *p = '/';
        }
    }
    
    #ifdef _WIN32
    _mkdir(tmp);
    #else
    mkdir(tmp, 0777);
    #endif
}

// ─── CONFIG LOADING ─────────────────────────────────────────────
static void load_config() {
    char config_path[LYNX_MAX_PATH];
    char* home = NULL;
    
    #ifdef _WIN32
    home = getenv("USERPROFILE");
    if (!home) home = getenv("HOME");
    #else
    home = getenv("HOME");
    #endif
    
    if (home) {
        snprintf(config_path, sizeof(config_path), "%s/%s", home, CONFIG_PATH);
    } else {
        snprintf(config_path, sizeof(config_path), "%s", CONFIG_PATH);
    }
    
    FILE* f = fopen(config_path, "r");
    if (!f) {
        // No config file - use default (debug ON)
        lynx_debug_mode = 1;
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Parse "debug = on" or "debug = off"
        if (strstr(line, "debug") != NULL) {
            char* equals = strchr(line, '=');
            if (equals) {
                char* value = equals + 1;
                while (isspace(*value)) value++;
                if (strcmp(value, "on") == 0 || strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                    lynx_debug_mode = 1;
                } else if (strcmp(value, "off") == 0 || strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                    lynx_debug_mode = 0;
                }
            }
        }
    }
    
    fclose(f);
}

// ─── SAVE CONFIG ─────────────────────────────────────────────────
static void save_config() {
    char config_path[LYNX_MAX_PATH];
    char* home = NULL;
    
    #ifdef _WIN32
    home = getenv("USERPROFILE");
    if (!home) home = getenv("HOME");
    #else
    home = getenv("HOME");
    #endif
    
    if (home) {
        snprintf(config_path, sizeof(config_path), "%s/%s", home, CONFIG_PATH);
    } else {
        snprintf(config_path, sizeof(config_path), "%s", CONFIG_PATH);
    }
    
    // Create .lynx directory if it doesn't exist
    char dir_path[LYNX_MAX_PATH];
    #ifdef _WIN32
    snprintf(dir_path, sizeof(dir_path), "%s\\.lynx", home ? home : ".");
    #else
    snprintf(dir_path, sizeof(dir_path), "%s/.lynx", home ? home : ".");
    #endif
    mkdir_p(dir_path);
    
    FILE* f = fopen(config_path, "w");
    if (f) {
        fprintf(f, "# Lynx configuration\n");
        fprintf(f, "debug = %s\n", lynx_debug_mode ? "on" : "off");
        fclose(f);
    }
}

void show_help() {
    printf("\n🐾 LYNX %s COMMANDS:\n", LYNX_VERSION);
    printf("\n  init               - Create new Lynx project\n");
    printf("  add <pkg>          - Add dependency to lynx.toml\n");
    printf("  install            - Install all dependencies\n");
    printf("  remove <pkg>       - Remove a package\n");
    printf("  search <term>      - Search registry for packages\n");
    printf("  update             - Update all packages to latest\n");
    printf("  publish            - Pack project for registry\n");
    printf("  build              - Run src/main.lnx\n");
    printf("  run <file.lnx>     - Run script\n");
    printf("  fmt <file.lnx>     - Auto-format Lynx file\n");
    printf("  check <file.lnx>   - Check syntax without executing\n");
    printf("  debug              - Toggle debug messages\n");
    printf("  --version          - Show version\n");
    printf("  --update           - Self-update\n");
    printf("  help               - Show this menu\n\n");
}

void runFile(const char* path, int argc, char** argv) {
    (void)argc;
    (void)argv;

    DEBUG_PRINT("runFile: ENTER (path='%s')\n", path);

    char cleanPath[LYNX_MAX_PATH];
    if (path[0] == '"') {
        int len = (int)strlen(path) - 2;
        if (len < 0) len = 0;
        strncpy(cleanPath, path + 1, len);
        cleanPath[len] = '\0';
    } else {
        strncpy(cleanPath, path, LYNX_MAX_PATH - 1);
        cleanPath[LYNX_MAX_PATH - 1] = '\0';
    }

    FILE* file = NULL;
    char fullPath[LYNX_MAX_PATH] = {0};
    
    file = fopen(cleanPath, "rb");
    
    if (!file) {
        char exePath[LYNX_MAX_PATH];
        #ifdef _WIN32
        GetModuleFileNameA(NULL, exePath, LYNX_MAX_PATH);
        #else
        ssize_t len = readlink("/proc/self/exe", exePath, LYNX_MAX_PATH - 1);
        if (len != -1) exePath[len] = '\0';
        else exePath[0] = '\0';
        #endif
        char* lastSlash = strrchr(exePath, PATH_SEP);
        if (lastSlash) {
            *lastSlash = '\0';
            snprintf(fullPath, LYNX_MAX_PATH, "%s%c%s", exePath, PATH_SEP, cleanPath);
            file = fopen(fullPath, "rb");
        }
    }
    
    if (!file) {
        char stdPath[LYNX_MAX_PATH];
        const char* appdata = getenv("APPDATA");
        if (!appdata) appdata = getenv("HOME");
        if (appdata) {
            snprintf(stdPath, LYNX_MAX_PATH, "%s%cLynxLang%cstd%c%s", appdata, PATH_SEP, PATH_SEP, PATH_SEP, cleanPath);
            file = fopen(stdPath, "rb");
        }
    }

    if (!file) {
        fprintf(stderr, "🐾 File '%s' not found\n", cleanPath);
        DEBUG_PRINT("runFile: EXIT (file not found)\n");
        return;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    char* buf = (char*)malloc(size + 1);
    if (buf) {
        size_t read = fread(buf, 1, size, file);
        buf[read] = '\0';
        fclose(file);

        if (read >= 3 &&
            (unsigned char)buf[0] == 0xEF &&
            (unsigned char)buf[1] == 0xBB &&
            (unsigned char)buf[2] == 0xBF) {
            memmove(buf, buf + 3, read - 2);
            buf[read - 3] = '\0';
        }

        DEBUG_PRINT("runFile: About to initScanner\n");
        Scanner previousScanner = scanner;
        initScanner(buf);
        DEBUG_PRINT("runFile: Scanner initialized, starting parse loop\n");
        
        while (peekToken().type != TOKEN_EOF) {
            parse_statement();
            if (lynx_error) {
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
                break;
            }
        }
        scanner = previousScanner;

        free(buf);
    } else {
        fclose(file);
    }

    DEBUG_PRINT("runFile: EXIT\n");
}

// ─── PACKAGE MANAGER FUNCTIONS ──────────────────────────────────

static char* read_file_content(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* buf = malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t bytes = fread(buf, 1, size, f);
    buf[bytes] = '\0';
    fclose(f);
    return buf;
}

static int write_file_content(const char* path, const char* content) {
    FILE* f = fopen(path, "w");
    if (!f) return 0;
    fwrite(content, 1, strlen(content), f);
    fclose(f);
    return 1;
}

static int pkg_exists(const char* content, const char* pkg) {
    char needle[256];
    snprintf(needle, sizeof(needle), "%s = ", pkg);
    return strstr(content, needle) != NULL;
}

// ─── RESOLVE LATEST VERSION ─────────────────────────────────────
static void resolve_latest_version(const char* pkg, char* out_version, size_t out_size) {
    out_version[0] = '\0';
    
    // Download packages.json
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -k -L -o packages.json https://raw.githubusercontent.com/justdev-chris/lynx-registry/main/packages.json --ssl-no-revoke");
    system(cmd);
    
    char* json = read_file_content("packages.json");
    if (!json) {
        remove("packages.json");
        return;
    }
    
    // Find package
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", pkg);
    char* pkg_pos = strstr(json, search);
    if (pkg_pos) {
        char* latest_pos = strstr(pkg_pos, "\"latest\":");
        if (latest_pos) {
            sscanf(latest_pos, "\"latest\": \"%[^\"]\"", out_version);
        }
    }
    
    free(json);
    remove("packages.json");
}

static void pkg_add(const char* pkg) {
    // Resolve latest version
    char latest_version[256] = {0};
    resolve_latest_version(pkg, latest_version, sizeof(latest_version));
    
    if (strlen(latest_version) == 0) {
        printf("❌ Could not find package '%s' in registry\n", pkg);
        return;
    }
    
    char* content = read_file_content("lynx.toml");
    if (!content) {
        // Create default lynx.toml
        content = strdup("[package]\nname = \"my_project\"\nversion = \"0.1.0\"\nauthors = [\"Anonymous\"]\ndescription = \"A Lynx project\"\n\n[dependencies]\n");
    }
    
    if (pkg_exists(content, pkg)) {
        printf("⚠️ %s already exists in dependencies\n", pkg);
        free(content);
        return;
    }
    
    char newLine[256];
    snprintf(newLine, sizeof(newLine), "%s = \"%s\"\n", pkg, latest_version);
    char* newContent = malloc(strlen(content) + strlen(newLine) + 1);
    strcpy(newContent, content);
    strcat(newContent, newLine);
    
    if (write_file_content("lynx.toml", newContent)) {
        printf("✅ Added %s (%s)\n", pkg, latest_version);
    } else {
        printf("❌ Failed to write lynx.toml\n");
    }
    
    free(content);
    free(newContent);
}

static void pkg_remove(const char* pkg) {
    char* content = read_file_content("lynx.toml");
    if (!content) {
        printf("⚠️ lynx.toml not found\n");
        return;
    }
    
    char needle[256];
    snprintf(needle, sizeof(needle), "%s = ", pkg);
    char* pos = strstr(content, needle);
    if (!pos) {
        printf("⚠️ %s not found in dependencies\n", pkg);
        free(content);
        return;
    }
    
    // Find end of line
    char* end = strchr(pos, '\n');
    if (!end) end = pos + strlen(pos);
    
    // Create new content without this line
    int beforeLen = pos - content;
    int afterLen = strlen(end);
    char* newContent = malloc(beforeLen + afterLen + 1);
    strncpy(newContent, content, beforeLen);
    strcpy(newContent + beforeLen, end);
    
    if (write_file_content("lynx.toml", newContent)) {
        printf("✅ Removed %s\n", pkg);
    } else {
        printf("❌ Failed to write lynx.toml\n");
    }
    
    free(content);
    free(newContent);
}

static void pkg_install() {
    char* content = read_file_content("lynx.toml");
    if (!content) {
        printf("⚠️ lynx.toml not found\n");
        return;
    }
    
    // Download packages.json once for version resolution
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -k -L -o packages.json https://raw.githubusercontent.com/justdev-chris/lynx-registry/main/packages.json --ssl-no-revoke");
    system(cmd);
    char* registry = read_file_content("packages.json");
    if (!registry) {
        printf("⚠️ Could not fetch package registry\n");
        free(content);
        return;
    }
    
    // Find [dependencies] section
    char* deps = strstr(content, "[dependencies]");
    if (!deps) {
        printf("⚠️ No [dependencies] section found\n");
        free(content);
        free(registry);
        remove("packages.json");
        return;
    }
    
    // Create libs directory
    create_dir("libs");
    
    // Parse each line in dependencies
    char* line = strtok(deps + 14, "\n");
    int installed = 0;
    while (line) {
        if (strlen(line) > 0 && line[0] != '[' && line[0] != '#') {
            char pkg[256] = {0};
            char version[256] = {0};
            if (sscanf(line, "%[^= ] = \"%[^\"]\"", pkg, version) == 2) {
                char actual_version[256] = {0};
                
                // If version is "latest", resolve from registry
                if (strcmp(version, "latest") == 0) {
                    char search[256];
                    snprintf(search, sizeof(search), "\"%s\":", pkg);
                    char* pkg_pos = strstr(registry, search);
                    if (pkg_pos) {
                        char* latest_pos = strstr(pkg_pos, "\"latest\":");
                        if (latest_pos) {
                            sscanf(latest_pos, "\"latest\": \"%[^\"]\"", actual_version);
                        }
                    }
                    if (strlen(actual_version) == 0) {
                        printf("❌ Could not find latest version for %s\n", pkg);
                        continue;
                    }
                    strcpy(version, actual_version);
                }
                
                printf("📦 Installing %s (%s)...\n", pkg, version);
                
                // Create package directory
                char pkgDir[LYNX_MAX_PATH];
                snprintf(pkgDir, sizeof(pkgDir), "libs/%s", pkg);
                create_dir(pkgDir);
                
                // Download package
                char url[512];
                char dest[512];
                snprintf(url, sizeof(url), "https://raw.githubusercontent.com/justdev-chris/lynx-registry/main/packages/%s/%s/package.tar.gz", pkg, version);
                snprintf(dest, sizeof(dest), "libs/%s.tar.gz", pkg);
                snprintf(cmd, sizeof(cmd), "curl -k -L -o %s %s --ssl-no-revoke", dest, url);
                
                int result = system(cmd);
                if (result == 0) {
                    // Check if file was downloaded successfully
                    FILE* check = fopen(dest, "rb");
                    if (check) {
                        fseek(check, 0, SEEK_END);
                        long size = ftell(check);
                        fclose(check);
                        if (size < 100) {
                            printf("❌ Downloaded file too small (%ld bytes). URL may be invalid.\n", size);
                            remove(dest);
                            continue;
                        }
                    } else {
                        printf("❌ Failed to download %s\n", pkg);
                        continue;
                    }
                    
                    // Extract - cross-platform cleanup
                    snprintf(cmd, sizeof(cmd), "tar -xzf %s -C libs/%s/", dest, pkg);
                    system(cmd);
                    remove(dest);  // C function, works everywhere
                    
                    printf("✅ Installed %s\n", pkg);
                    installed++;
                } else {
                    printf("❌ Failed to install %s\n", pkg);
                }
            }
        }
        line = strtok(NULL, "\n");
    }
    
    free(content);
    free(registry);
    remove("packages.json");
    
    if (installed > 0) {
        printf("✅ Installed %d package(s)\n", installed);
    } else {
        printf("✅ No packages to install\n");
    }
}

static void pkg_search(const char* term) {
    // Download packages.json from registry
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -k -L -o packages.json https://raw.githubusercontent.com/justdev-chris/lynx-registry/main/packages.json --ssl-no-revoke");
    int result = system(cmd);
    
    if (result != 0) {
        printf("❌ Failed to download package registry\n");
        return;
    }
    
    char* content = read_file_content("packages.json");
    if (!content) {
        printf("❌ Failed to read packages.json\n");
        return;
    }
    
    // Find packages section
    char* packages = strstr(content, "\"packages\"");
    if (packages) {
        // Find each package name
        char* p = packages;
        int found = 0;
        printf("📦 Found packages matching '%s':\n", term);
        while ((p = strstr(p, "\"")) != NULL) {
            p++;
            char* end = strstr(p, "\"");
            if (end) {
                int len = end - p;
                char pkg_name[256];
                strncpy(pkg_name, p, len);
                pkg_name[len] = '\0';
                
                // Skip common JSON keys
                if (strcmp(pkg_name, "packages") != 0 && 
                    strcmp(pkg_name, "description") != 0 && 
                    strcmp(pkg_name, "versions") != 0 && 
                    strcmp(pkg_name, "latest") != 0 &&
                    strcmp(pkg_name, "author") != 0 && 
                    strcmp(pkg_name, "badge") != 0) {
                    if (strstr(pkg_name, term)) {
                        printf("  📦 %s\n", pkg_name);
                        found++;
                    }
                }
                p = end;
            }
        }
        if (found == 0) {
            printf("❌ No results found for '%s'\n", term);
        } else {
            printf("✅ Found %d package(s)\n", found);
        }
    } else {
        printf("❌ No packages found in registry\n");
    }
    
    free(content);
    remove("packages.json");
}

static void pkg_update() {
    printf("🔄 Updating packages...\n");
    
    // Read lynx.toml and update each dependency to latest
    char* content = read_file_content("lynx.toml");
    if (!content) {
        printf("⚠️ lynx.toml not found\n");
        return;
    }
    
    // Download packages.json
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -k -L -o packages.json https://raw.githubusercontent.com/justdev-chris/lynx-registry/main/packages.json --ssl-no-revoke");
    system(cmd);
    char* registry = read_file_content("packages.json");
    if (!registry) {
        printf("⚠️ Could not fetch package registry\n");
        free(content);
        return;
    }
    
    // Find [dependencies] section
    char* deps = strstr(content, "[dependencies]");
    if (!deps) {
        printf("⚠️ No [dependencies] section found\n");
        free(content);
        free(registry);
        remove("packages.json");
        return;
    }
    
    // Parse each line in dependencies and update versions
    char* line = strtok(deps + 14, "\n");
    int updated = 0;
    char new_content[8192] = {0};
    
    // Copy everything before [dependencies]
    int deps_pos = deps - content;
    strncpy(new_content, content, deps_pos + 14); // include "[dependencies]\n"
    
    while (line) {
        if (strlen(line) > 0 && line[0] != '[' && line[0] != '#') {
            char pkg[256] = {0};
            char current_version[256] = {0};
            if (sscanf(line, "%[^= ] = \"%[^\"]\"", pkg, current_version) == 2) {
                // Find latest version
                char latest_version[256] = {0};
                char search[256];
                snprintf(search, sizeof(search), "\"%s\":", pkg);
                char* pkg_pos = strstr(registry, search);
                if (pkg_pos) {
                    char* latest_pos = strstr(pkg_pos, "\"latest\":");
                    if (latest_pos) {
                        sscanf(latest_pos, "\"latest\": \"%[^\"]\"", latest_version);
                    }
                }
                
                if (strlen(latest_version) > 0 && strcmp(current_version, latest_version) != 0) {
                    printf("📦 Updating %s (%s → %s)\n", pkg, current_version, latest_version);
                    snprintf(line, strlen(line) + 256, "%s = \"%s\"", pkg, latest_version);
                    updated++;
                }
                strcat(new_content, line);
                strcat(new_content, "\n");
            }
        } else {
            // Preserve comments and empty lines
            strcat(new_content, line);
            strcat(new_content, "\n");
        }
        line = strtok(NULL, "\n");
    }
    
    free(content);
    free(registry);
    remove("packages.json");
    
    if (write_file_content("lynx.toml", new_content)) {
        if (updated > 0) {
            printf("🔄 Reinstalling updated packages...\n");
            pkg_install();
            printf("✅ Updated %d package(s)\n", updated);
        } else {
            printf("✅ All packages up to date\n");
        }
    } else {
        printf("❌ Failed to update lynx.toml\n");
    }
}

static void pkg_publish() {
    printf("📦 Packing project for registry...\n");
    
    // 1. Check if lynx.toml exists
    char* content = read_file_content("lynx.toml");
    if (!content) {
        printf("❌ lynx.toml not found. Run 'lynx init' first.\n");
        return;
    }
    
    // 2. Parse package name and version from lynx.toml
    char pkg_name[256] = {0};
    char pkg_version[256] = {0};
    char* name_line = strstr(content, "name = ");
    if (name_line) {
        sscanf(name_line, "name = \"%[^\"]\"", pkg_name);
    }
    char* version_line = strstr(content, "version = ");
    if (version_line) {
        sscanf(version_line, "version = \"%[^\"]\"", pkg_version);
    }
    
    if (strlen(pkg_name) == 0 || strlen(pkg_version) == 0) {
        printf("❌ Could not find name and version in lynx.toml\n");
        free(content);
        return;
    }
    
    printf("📦 Packaging %s v%s\n", pkg_name, pkg_version);
    
    // 3. Create temp directory for packaging
    char temp_dir[LYNX_MAX_PATH];
    #ifdef _WIN32
    const char* temp_env = getenv("TEMP");
    if (!temp_env) temp_env = ".";
    snprintf(temp_dir, sizeof(temp_dir), "%s\\lynx_pkg_%s", temp_env, pkg_name);
    #else
    snprintf(temp_dir, sizeof(temp_dir), "/tmp/lynx_pkg_%s", pkg_name);
    #endif
    
    // Remove existing temp dir if it exists
    #ifdef _WIN32
    char cmd[LYNX_MAX_PATH];
    snprintf(cmd, sizeof(cmd), "rmdir /s /q %s 2>nul", temp_dir);
    system(cmd);
    create_dir(temp_dir);
    #else
    char cmd[LYNX_MAX_PATH];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", temp_dir);
    system(cmd);
    create_dir(temp_dir);
    #endif
    
    // 4. Copy files to temp directory
    #ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "xcopy /E /I src %s\\src >nul", temp_dir);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "copy lynx.toml %s\\ >nul", temp_dir);
    system(cmd);
    #else
    snprintf(cmd, sizeof(cmd), "cp -r src %s/", temp_dir);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "cp lynx.toml %s/", temp_dir);
    system(cmd);
    #endif
    
    // 5. Create package.tar.gz
    char tarball[LYNX_MAX_PATH];
    snprintf(tarball, sizeof(tarball), "%s.tar.gz", pkg_name);
    
    // Remove existing tarball
    remove(tarball);
    
    #ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "tar -czf %s -C %s . 2>nul", tarball, temp_dir);
    #else
    snprintf(cmd, sizeof(cmd), "tar -czf %s -C %s .", tarball, temp_dir);
    #endif
    int result = system(cmd);
    
    if (result != 0) {
        printf("❌ Failed to create tarball. Make sure 'tar' is installed.\n");
        #ifdef _WIN32
        snprintf(cmd, sizeof(cmd), "rmdir /s /q %s", temp_dir);
        #else
        snprintf(cmd, sizeof(cmd), "rm -rf %s", temp_dir);
        #endif
        system(cmd);
        free(content);
        return;
    }
    
    // 6. Clean up temp directory
    #ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "rmdir /s /q %s", temp_dir);
    #else
    snprintf(cmd, sizeof(cmd), "rm -rf %s", temp_dir);
    #endif
    system(cmd);
    
    // 7. Output result
    printf("✅ Package created: %s\n", tarball);
    printf("\n📤 To publish:\n");
    printf("   1. Fork https://github.com/justdev-chris/lynx-registry\n");
    printf("   2. Create directory: packages/%s/%s/\n", pkg_name, pkg_version);
    printf("   3. Upload %s to that directory\n", tarball);
    printf("   4. Update packages.json with your package info\n");
    printf("   5. Submit a Pull Request\n");
    
    free(content);
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetConsoleOutputCP(65001);
    #endif
    
    // Load config FIRST
    load_config();
    
    lynx_error_state.message = NULL;
    lynx_error_state.line = 0;
    lynx_error_state.col = 0;
    lynx_error = NULL;
    
    try_state.is_trying = 0;
    try_state.caught = 0;
    try_state.error_message = NULL;
    try_state.error_line = 0;
    try_state.error_col = 0;

    if (argc >= 2) {
        if (STRICMP(argv[1], "help") == 0 || STRICMP(argv[1], "--help") == 0) {
            show_help();
            return 0;
        } else if (STRICMP(argv[1], "--version") == 0) {
            printf("Lynx Engine %s\n", LYNX_VERSION);
            return 0;
        }
        // ─── SELF-UPDATE WITH VERSION CHECK ──────────────────────
        else if (STRICMP(argv[1], "--update") == 0) {
            #ifdef _WIN32
            printf("🔍 Checking for updates...\n");
            
            // Get latest version from GitHub API
            system("curl -k -L -o latest.json https://api.github.com/repos/TheLynxLang/Lynx/releases/latest --ssl-no-revoke");
            char* json = read_file_content("latest.json");
            
            if (json) {
                char latest_version[64] = {0};
                char* tag = strstr(json, "\"tag_name\"");
                if (tag) {
                    sscanf(tag, "\"tag_name\": \"%[^\"]\"", latest_version);
                    // Remove 'v' prefix if present
                    char* v = latest_version;
                    if (v[0] == 'v') v++;
                    
                    // Compare versions (simple string compare, works for x.y.z)
                    char current[64];
                    strcpy(current, LYNX_VERSION);
                    
                    if (strcmp(v, current) > 0) {
                        printf("🔄 New version %s available! Current: %s\n", v, current);
                        printf("📥 Downloading update...\n");
                        
                        char tempInstaller[LYNX_MAX_PATH];
                        sprintf(tempInstaller, "%s\\LynxInstaller.exe", getenv("TEMP"));
                        const char* url = "https://github.com/TheLynxLang/Lynx/releases/latest/download/LynxInstaller.exe";
                        if (S_OK == URLDownloadToFileA(NULL, url, tempInstaller, 0, NULL)) {
                            ShellExecuteA(NULL, "open", tempInstaller, NULL, NULL, SW_SHOWNORMAL);
                            exit(0);
                        } else {
                            setErrorF("Update failed: Could not download installer");
                            fprintf(stderr, "🐾 %s\n", lynx_error);
                            clearError();
                        }
                    } else {
                        printf("✅ Lynx is already up to date (v%s)\n", current);
                    }
                } else {
                    printf("⚠️ Could not parse version information\n");
                }
                free(json);
                remove("latest.json");
            } else {
                printf("❌ Failed to check for updates\n");
            }
            return 0;
            #else
            // Linux version
            printf("🔍 Checking for updates...\n");
            system("curl -k -L -o latest.json https://api.github.com/repos/TheLynxLang/Lynx/releases/latest");
            char* json = read_file_content("latest.json");
            if (json) {
                char latest_version[64] = {0};
                char* tag = strstr(json, "\"tag_name\"");
                if (tag) {
                    sscanf(tag, "\"tag_name\": \"%[^\"]\"", latest_version);
                    char* v = latest_version;
                    if (v[0] == 'v') v++;
                    printf("📦 Latest version: %s\n", v);
                    printf("   Download from: https://github.com/TheLynxLang/Lynx/releases\n");
                }
                free(json);
                remove("latest.json");
            } else {
                printf("❌ Failed to check for updates\n");
            }
            return 0;
            #endif
        }
        
        // ─── DEBUG TOGGLE ──────────────────────────────────────
        else if (STRICMP(argv[1], "debug") == 0) {
            lynx_debug_mode = !lynx_debug_mode;
            save_config();  // Save to file
            printf("🐾 Debug mode: %s\n", lynx_debug_mode ? "ON" : "OFF");
            return 0;
        }
        
        // ─── PACKAGE MANAGER COMMANDS (C implementation) ──────
        else if (STRICMP(argv[1], "add") == 0) {
            if (argc >= 3) {
                pkg_add(argv[2]);
            } else {
                printf("Usage: lynx add <package>\n");
            }
            return 0;
        }
        else if (STRICMP(argv[1], "remove") == 0) {
            if (argc >= 3) {
                pkg_remove(argv[2]);
            } else {
                printf("Usage: lynx remove <package>\n");
            }
            return 0;
        }
        else if (STRICMP(argv[1], "install") == 0) {
            pkg_install();
            return 0;
        }
        else if (STRICMP(argv[1], "search") == 0) {
            if (argc >= 3) {
                pkg_search(argv[2]);
            } else {
                printf("Usage: lynx search <term>\n");
            }
            return 0;
        }
        else if (STRICMP(argv[1], "update") == 0) {
            pkg_update();
            return 0;
        }
        else if (STRICMP(argv[1], "publish") == 0) {
            pkg_publish();
            return 0;
        }
        
        // ─── SCRIPT COMMANDS ─────────────────────────────────
        else if (STRICMP(argv[1], "init") == 0) {
            clearError();

            DEBUG_PRINT("MAIN: argc = %d\n", argc);
            DEBUG_PRINT("MAIN: argv[2] = %s\n", argc >= 3 ? argv[2] : "(null)");
            DEBUG_PRINT("MAIN: argv[3] = %s\n", argc >= 4 ? argv[3] : "(null)");

            if (argc >= 3) {
                setVarString("__project_name", argv[2]);
                DEBUG_PRINT("MAIN: Called setVarString(__project_name, %s)\n", argv[2]);
            } else {
                setVarString("__project_name", "my_project");
                DEBUG_PRINT("MAIN: Called setVarString(__project_name, my_project)\n");
            }
            if (argc >= 4) {
                setVarString("__author", argv[3]);
                DEBUG_PRINT("MAIN: Called setVarString(__author, %s)\n", argv[3]);
            } else {
                setVarString("__author", "Anonymous");
                DEBUG_PRINT("MAIN: Called setVarString(__author, Anonymous)\n");
            }
            
            DEBUG_PRINT("MAIN: After setVarString, __project_name = '%s'\n", getVarString("__project_name"));
            DEBUG_PRINT("MAIN: After setVarString, __author = '%s'\n", getVarString("__author"));
            DEBUG_PRINT("MAIN: varCount = %d\n", varCount);
            
            DEBUG_PRINT("MAIN: All variables:\n");
            for (int i = 0; i < varCount; i++) {
                DEBUG_PRINT("  %d: %s (type=%d)\n", i, den[i].name, den[i].type);
                if (den[i].type == VAR_STRING) {
                    DEBUG_PRINT("      strValue = '%s'\n", den[i].value.strValue ? den[i].value.strValue : "(null)");
                }
            }
            
            DEBUG_PRINT("MAIN: BEFORE runFile, __project_name = '%s'\n", getVarString("__project_name"));
            runFile("scripts/init.lnx", 0, NULL);
            DEBUG_PRINT("MAIN: AFTER runFile, __project_name = '%s'\n", getVarString("__project_name"));
            
            unload_all_libs();
            cleanup_all();
            return 0;
        }
        else if (STRICMP(argv[1], "build") == 0) {
            runFile("src/main.lnx", 0, NULL);
            unload_all_libs();
            cleanup_all();
            return 0;
        }
        else if (STRICMP(argv[1], "fmt") == 0) {
            if (argc >= 3) {
                format_file(argv[2]);
            } else {
                setErrorF("Usage: lynx fmt <file.lnx>");
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
            }
            unload_all_libs();
            cleanup_all();
            return 0;
        }
        else if (STRICMP(argv[1], "check") == 0) {
            if (argc >= 3) {
                check_file(argv[2]);
            } else {
                setErrorF("Usage: lynx check <file.lnx>");
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
            }
            unload_all_libs();
            cleanup_all();
            return 0;
        }
        else if (STRICMP(argv[1], "run") == 0) {
            if (argc >= 3) {
                runFile(argv[2], 0, NULL);
            } else {
                setErrorF("Usage: lynx run <file.lnx>");
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
            }
            unload_all_libs();
            cleanup_all();
            return 0;
        }
        else {
            // Try to run as script
            char scriptPath[256];
            snprintf(scriptPath, sizeof(scriptPath), "scripts/%s.lnx", argv[1]);
            FILE* f = fopen(scriptPath, "r");
            if (f) {
                fclose(f);
                runFile(scriptPath, 0, NULL);
            } else {
                FILE* test = fopen(argv[1], "r");
                if (test) {
                    fclose(test);
                    runFile(argv[1], 0, NULL);
                } else {
                    setErrorF("Unknown command: %s", argv[1]);
                    fprintf(stderr, "🐾 %s\n", lynx_error);
                    fprintf(stderr, "   Run 'lynx help' for available commands\n");
                    clearError();
                }
            }
            unload_all_libs();
            cleanup_all();
            return 0;
        }
    }

    // ─── REPL MODE ──────────────────────────────────────────────
    char line[1024];
    printf("Lynx Engine %s | Type 'Help' for info\n", LYNX_VERSION);
    while (1) {
        printf("lynx > ");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        if (STRICMP(line, "help") == 0) {
            show_help();
        } else if (STRICMP(line, "exit") == 0) {
            break;
        } else if (strstr(line, ".lnx") != NULL) {
            runFile(line, 0, NULL);
        } else {
            initScanner(line);
            parse_statement();
            if (lynx_error) {
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
            }
        }
    }

    unload_all_libs();
    cleanup_all();
    printf("🐾 Goodbye!\n");
    return 0;
}
