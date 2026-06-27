#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <urlmon.h>
#include "lynx.h"

#pragma comment(lib, "urlmon.lib")

#define LYNX_VERSION "v1.4.0"

extern Scanner scanner;

void show_help() {
    printf("\n🐾 LYNX %s COMMANDS:\n", LYNX_VERSION);
    printf("\n  init               - Create new Lynx project\n");
    printf("  add <pkg>          - Add dependency\n");
    printf("  build              - Compile project to .exe (dynamic)\n");
    printf("  build --standalone - Compile project to standalone .exe\n");
    printf("  run <file.lnx>     - Run script\n");
    printf("  --compile-to-exe <in> <out> [--standalone]\n");
    printf("  --version          - Show version\n");
    printf("  --update           - Self-update\n");
    printf("  help               - Show this menu\n\n");
}

void runFile(const char* path) {
    char cleanPath[MAX_PATH];
    if (path[0] == '"') {
        int len = strlen(path) - 2;
        strncpy(cleanPath, path + 1, len);
        cleanPath[len] = '\0';
    } else {
        strcpy(cleanPath, path);
    }

    FILE* file = fopen(cleanPath, "rb");
    if (!file) {
        char stdPath[MAX_PATH];
        sprintf(stdPath, "%s\\LynxLang\\std\\%s", getenv("APPDATA"), cleanPath);
        file = fopen(stdPath, "rb");
    }

    if (!file) {
        fprintf(stderr, "🐾 Lynx Error: File '%s' not found.\n", cleanPath);
        return;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    char* buf = malloc(size + 1);
    if (buf) {
        fread(buf, 1, size, file);
        buf[size] = '\0';
        fclose(file);

        Scanner previousScanner = scanner;
        initScanner(buf);
        while (peekToken().type != TOKEN_EOF) {
            parse_statement();
        }
        scanner = previousScanner;
        free(buf);
    }
}

void compile_to_exe(const char* input, const char* output, int standalone) {
    printf("🐾 Compiling %s -> %s (standalone: %d)\n", input, output, standalone);

    FILE* f = fopen(input, "rb");
    if (!f) {
        printf("❌ Input file not found: %s\n", input);
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* src = malloc(size + 1);
    fread(src, 1, size, f);
    src[size] = '\0';
    fclose(f);

    extern uint8_t* compile_to_machine(const char* source, size_t* out_size);
    size_t code_size;
    uint8_t* code = compile_to_machine(src, &code_size);

    if (!code) {
        printf("❌ Compilation failed.\n");
        free(src);
        return;
    }

    extern void write_pe(const char* output, uint8_t* code, size_t code_size, int standalone);
    write_pe(output, code, code_size, standalone);

    free(code);
    free(src);
    printf("✅ Compiled: %s\n", output);
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);

    if (argc >= 2) {
        if (_stricmp(argv[1], "help") == 0 || _stricmp(argv[1], "--help") == 0) {
            show_help();
        }
        else if (_stricmp(argv[1], "--version") == 0) {
            printf("Lynx Engine %s\n", LYNX_VERSION);
        }
        else if (_stricmp(argv[1], "--update") == 0) {
            printf("🔄 Preparing update...\n");
            char tempInstaller[MAX_PATH];
            sprintf(tempInstaller, "%s\\LynxInstaller.exe", getenv("TEMP"));
            const char* url = "https://github.com/justdev-chris/Lynx/releases/latest/download/LynxInstaller.exe";
            if (S_OK == URLDownloadToFileA(NULL, url, tempInstaller, 0, NULL)) {
                ShellExecuteA(NULL, "open", tempInstaller, NULL, NULL, SW_SHOWNORMAL);
                exit(0);
            } else {
                printf("❌ Update failed.\n");
            }
        }
        else if (_stricmp(argv[1], "init") == 0) {
            runFile("scripts/init.lnx");
            return 0;
        }
        else if (_stricmp(argv[1], "build") == 0) {
            int standalone = 0;
            if (argc >= 3 && _stricmp(argv[2], "--standalone") == 0) {
                standalone = 1;
            }
            setVar("__flag", standalone ? 1.0 : 0.0);
            runFile("scripts/build.lnx");
            return 0;
        }
        else if (_stricmp(argv[1], "add") == 0) {
            if (argc >= 3) {
                setVarString("__pkg", argv[2]);
                runFile("scripts/add.lnx");
            } else {
                printf("🐾 Usage: lynx add <package>\n");
            }
            return 0;
        }
        else if (_stricmp(argv[1], "--compile-to-exe") == 0) {
            if (argc < 4) {
                printf("Usage: lynx --compile-to-exe <input.lnx> <output.exe> [--standalone]\n");
                return 1;
            }
            int standalone = 0;
            if (argc >= 5 && _stricmp(argv[4], "--standalone") == 0) {
                standalone = 1;
            }
            compile_to_exe(argv[2], argv[3], standalone);
            return 0;
        }
        else {
            runFile(argv[1]);
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
            runFile(line);
        } else {
            initScanner(line);
            parse_statement();
        }
    }

    unload_all_libs();
    cleanup_all();
    printf("🐾 Goodbye!\n");
    return 0;
}