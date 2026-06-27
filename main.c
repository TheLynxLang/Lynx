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

// Compiler stub — will be replaced with real PE writer + x86_64 emitter
void compile_to_exe(const char* input, const char* output, int standalone) {
    printf("🐾 Compiling %s -> %s (standalone: %d)\n", input, output, standalone);
    printf("⚠️  Full PE + x86_64 emitter coming soon. For now, copying stub.\n");

    // Read input file
    FILE* f = fopen(input, "rb");
    if (!f) {
        printf("❌ Input file not found: %s\n", input);
        return;
    }
    fclose(f);

    // Write minimal .exe stub (placeholder)
    // In real implementation, this will contain PE header + x86_64 machine code
    unsigned char stub[] = {
        0x4D, 0x5A, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD,
        0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x54, 0x68,
        0x69, 0x73, 0x20, 0x70, 0x72, 0x6F, 0x67, 0x72,
        0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F,
        0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E,
        0x20, 0x69, 0x6E, 0x20, 0x44, 0x4F, 0x53, 0x20,
        0x6D, 0x6F, 0x64, 0x65, 0x2E, 0x0D, 0x0D, 0x0A,
        0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    FILE* out = fopen(output, "wb");
    if (out) {
        fwrite(stub, 1, sizeof(stub), out);
        fclose(out);
        printf("✅ Written stub: %s\n", output);
        printf("⚠️  Replace with real PE + x86_64 emitter later.\n");
    } else {
        printf("❌ Failed to write: %s\n", output);
    }
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
                // Pass package name to add.lnx via __pkg variable
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