#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <urlmon.h>
#include <errno.h>
#include "lynx.h"

#pragma comment(lib, "urlmon.lib")

#define LYNX_VERSION "v1.4.0"

extern Scanner scanner;
extern char* lynx_error;
extern LynxError lynx_error_state;

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
    printf("  --version          - Show version\n");
    printf("  --update           - Self-update\n");
    printf("  help               - Show this menu\n\n");
}

void runFile(const char* path, int argc, char** argv) {
    char cleanPath[LYNX_MAX_PATH];
    if (path[0] == '"') {
        int len = (int)strlen(path) - 2;
        strncpy(cleanPath, path + 1, len);
        cleanPath[len] = '\0';
    } else {
        strcpy(cleanPath, path);
    }

    FILE* file = NULL;
    char fullPath[LYNX_MAX_PATH] = {0};
    
    // 1. Current working directory
    file = fopen(cleanPath, "rb");
    
    // 2. Lynx installation directory (where lynx.exe lives)
    if (!file) {
        char exePath[LYNX_MAX_PATH];
        GetModuleFileNameA(NULL, exePath, LYNX_MAX_PATH);
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash) {
            *lastSlash = '\0';
            snprintf(fullPath, LYNX_MAX_PATH, "%s\\%s", exePath, cleanPath);
            file = fopen(fullPath, "rb");
        }
    }
    
    // 3. %APPDATA%\LynxLang\std\
    if (!file) {
        char stdPath[LYNX_MAX_PATH];
        snprintf(stdPath, LYNX_MAX_PATH, "%s\\LynxLang\\std\\%s", getenv("APPDATA"), cleanPath);
        file = fopen(stdPath, "rb");
    }

    if (!file) {
        setErrorF("File '%s' not found", cleanPath);
        fprintf(stderr, "🐾 %s\n", lynx_error);
        clearError();
        return;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    char* buf = (char*)malloc(size + 1);
    if (buf) {
        fread(buf, 1, size, file);
        buf[size] = '\0';
        fclose(file);

        Scanner previousScanner = scanner;
        initScanner(buf);
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
    }
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    
    lynx_error_state.message = NULL;
    lynx_error_state.line = 0;
    lynx_error_state.col = 0;
    lynx_error = NULL;

    if (argc >= 2) {
        if (_stricmp(argv[1], "help") == 0 || _stricmp(argv[1], "--help") == 0) {
            show_help();
        } else if (_stricmp(argv[1], "--version") == 0) {
            printf("Lynx Engine %s\n", LYNX_VERSION);
        } else if (_stricmp(argv[1], "--update") == 0) {
            printf("🔄 Preparing update...\n");
            char tempInstaller[LYNX_MAX_PATH];
            sprintf(tempInstaller, "%s\\LynxInstaller.exe", getenv("TEMP"));
            const char* url = "https://github.com/justdev-chris/Lynx/releases/latest/download/LynxInstaller.exe";
            if (S_OK == URLDownloadToFileA(NULL, url, tempInstaller, 0, NULL)) {
                ShellExecuteA(NULL, "open", tempInstaller, NULL, NULL, SW_SHOWNORMAL);
                exit(0);
            } else {
                setErrorF("Update failed: Could not download installer");
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
            }
        } else if (_stricmp(argv[1], "init") == 0) {
            runFile("scripts/init.lnx", 0, NULL);
            return 0;
        } else if (_stricmp(argv[1], "add") == 0) {
            if (argc >= 3) {
                setVarString("__pkg", argv[2]);
                runFile("scripts/add.lnx", 0, NULL);
            } else {
                setErrorF("Usage: lynx add <package>");
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
            }
            return 0;
        } else if (_stricmp(argv[1], "install") == 0) {
            runFile("scripts/install.lnx", 0, NULL);
            return 0;
        } else if (_stricmp(argv[1], "remove") == 0) {
            if (argc >= 3) {
                setVarString("__pkg", argv[2]);
                runFile("scripts/remove.lnx", 0, NULL);
            } else {
                setErrorF("Usage: lynx remove <package>");
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
            }
            return 0;
        } else if (_stricmp(argv[1], "search") == 0) {
            if (argc >= 3) {
                setVarString("__term", argv[2]);
                runFile("scripts/search.lnx", 0, NULL);
            } else {
                setErrorF("Usage: lynx search <term>");
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
            }
            return 0;
        } else if (_stricmp(argv[1], "update") == 0) {
            runFile("scripts/update.lnx", 0, NULL);
            return 0;
        } else if (_stricmp(argv[1], "publish") == 0) {
            runFile("scripts/publish.lnx", 0, NULL);
            return 0;
        } else if (_stricmp(argv[1], "build") == 0) {
            runFile("src/main.lnx", 0, NULL);
            return 0;
        } else if (_stricmp(argv[1], "fmt") == 0) {
            if (argc >= 3) {
                format_file(argv[2]);
            } else {
                setErrorF("Usage: lynx fmt <file.lnx>");
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
            }
            return 0;
        } else if (_stricmp(argv[1], "check") == 0) {
            if (argc >= 3) {
                check_file(argv[2]);
            } else {
                setErrorF("Usage: lynx check <file.lnx>");
                fprintf(stderr, "🐾 %s\n", lynx_error);
                clearError();
            }
            return 0;
        } else {
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
            return 0;
        }

        unload_all_libs();
        cleanup_all();
        return 0;
    }

    char line[1024];
    printf("Lynx Engine %s | Type 'Help' for info\n", LYNX_VERSION);
    while (1) {
        printf("lynx > ");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        if (_stricmp(line, "help") == 0) {
            show_help();
        } else if (_stricmp(line, "exit") == 0) {
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