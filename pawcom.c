#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lynx.h"

#ifdef _WIN32
#include <direct.h>
#endif

// ─── EXTERNAL DECLARATIONS ────────────────────────────────────
extern Scanner scanner;
extern char* lynx_error;
extern char* loaded_packages[64];
extern int loaded_pkg_count;

extern double parse_expression();
extern void parse_block();
extern void setVar(const char* name, double value);
extern void setVarString(const char* name, const char* value);
extern double getVar(const char* name);
extern char* getVarString(const char* name);
extern void pounce(const char* name);
extern void hunt();
extern void load_lib(const char* lib_name);
extern void runFile(const char* path, int argc, char** argv);
extern void setError(const char* msg);
extern void clearError();

// ─── STRING HELPERS ────────────────────────────────────────────
static char* str_trim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static int str_contains(const char* haystack, const char* needle) {
    return strstr(haystack, needle) != NULL;
}

static char* str_replace(const char* src, const char* old, const char* new) {
    static char buffer[4096];
    char* p = buffer;
    const char* q = src;
    size_t old_len = strlen(old);
    size_t new_len = strlen(new);
    while (*q) {
        if (strncmp(q, old, old_len) == 0) {
            strcpy(p, new);
            p += new_len;
            q += old_len;
        } else {
            *p++ = *q++;
        }
    }
    *p = '\0';
    return buffer;
}

// ─── PARSE ARRAY ──────────────────────────────────────────────
static void parse_array() {
    Token nameToken = scanToken();
    if (nameToken.type != TOKEN_IDENTIFIER) return;
    char varName[64];
    snprintf(varName, nameToken.length + 1, "%s", nameToken.start);

    Token op = scanToken();
    if (op.type != TOKEN_EQUAL) return;

    Token bracket = scanToken();
    if (bracket.type != TOKEN_LBRACKET) {
        setError("Expected '[' for array");
        return;
    }

    int count = 0;
    double values[256];
    while (peekToken().type != TOKEN_RBRACKET && peekToken().type != TOKEN_EOF) {
        double val = parse_expression();
        values[count++] = val;
        if (peekToken().type == TOKEN_COMMA) scanToken();
    }

    if (peekToken().type == TOKEN_RBRACKET) scanToken();

    // Create array variable
    // TODO: Actually store array in memory.c
    printf("Array %s created with %d elements\n", varName, count);
}

// ─── PARSE TRY / CATCH ────────────────────────────────────────
static void parse_try_catch() {
    char* saved_error = lynx_error ? strdup(lynx_error) : NULL;
    clearError();

    Token brace = scanToken();
    if (brace.type != TOKEN_LBRACE) {
        setError("Expected '{' after Try");
        return;
    }

    while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF) {
        parse_statement();
        if (lynx_error) break;
    }
    if (peekToken().type == TOKEN_RBRACE) scanToken();

    if (!lynx_error) {
        Token next = peekToken();
        if (next.type == TOKEN_CATCH) {
            scanToken();
            brace = scanToken();
            if (brace.type == TOKEN_LBRACE) {
                int depth = 1;
                while (depth > 0 && peekToken().type != TOKEN_EOF) {
                    Token t = scanToken();
                    if (t.type == TOKEN_LBRACE) depth++;
                    if (t.type == TOKEN_RBRACE) depth--;
                }
            }
        }
        return;
    }

    Token next = peekToken();
    if (next.type != TOKEN_CATCH) {
        return;
    }

    scanToken();
    brace = scanToken();
    if (brace.type != TOKEN_LBRACE) {
        setError("Expected '{' after Catch");
        return;
    }

    while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF) {
        parse_statement();
    }
    if (peekToken().type == TOKEN_RBRACE) scanToken();
    clearError();
}

// ─── MAIN DISPATCH ────────────────────────────────────────────
void pawcom_parse_statement(Token t) {
    if (t.type == TOKEN_HUNT) { hunt(); return; }

    if (t.type == TOKEN_ROAR) {
        Token val = scanToken();
        if (val.type == TOKEN_STRING) {
            for (int i = 1; i < val.length - 1; i++) printf("%c", val.start[i]);
            printf("\n");
        } else if (val.type == TOKEN_IDENTIFIER) {
            char name[64];
            snprintf(name, val.length + 1, "%s", val.start);
            char* s = getVarString(name);
            if (s && strlen(s) > 0) printf("%s\n", s);
            else {
                double num = getVar(name);
                if (lynx_error) {
                    printf("%s\n", lynx_error);
                    clearError();
                } else {
                    printf("%.5f\n", num);
                }
            }
        } else if (val.type == TOKEN_NUMBER) {
            printf("%.5f\n", atof(val.start));
        }
        return;
    }

    if (t.type == TOKEN_SET) {
        Token nameToken = scanToken();
        if (nameToken.type != TOKEN_IDENTIFIER) return;
        char varName[64];
        snprintf(varName, nameToken.length + 1, "%s", nameToken.start);

        Token op = scanToken();
        if (op.type == TOKEN_EQUAL) {
            if (peekToken().type == TOKEN_LBRACKET) {
                parse_array();
                return;
            }
            if (peekToken().type == TOKEN_STRING) {
                Token str = scanToken();
                char s[256];
                snprintf(s, str.length - 1, "%s", str.start + 1);
                setVarString(varName, s);
            } else {
                double val = parse_expression();
                if (lynx_error) {
                    printf("%s\n", lynx_error);
                    clearError();
                    return;
                }
                setVar(varName, val);
            }
        } else if (op.type == TOKEN_INCREMENT) {
            setVar(varName, getVar(varName) + 1);
        } else if (op.type == TOKEN_DECREMENT) {
            setVar(varName, getVar(varName) - 1);
        }
        return;
    }

    if (t.type == TOKEN_STALK_PACK) {
        Token pathToken = scanToken();
        if (pathToken.type == TOKEN_STRING) {
            char path[256];
            snprintf(path, pathToken.length - 1, "%s", pathToken.start + 1);
            runFile(path, 0, NULL);
            if (lynx_error) {
                printf("%s\n", lynx_error);
                clearError();
            }
        }
        return;
    }

    if (t.type == TOKEN_POUNCE) {
        Token nameToken = scanToken();
        if (nameToken.type == TOKEN_IDENTIFIER) {
            char name[64];
            snprintf(name, nameToken.length + 1, "%s", nameToken.start);
            pounce(name);
            if (lynx_error) {
                printf("%s\n", lynx_error);
                clearError();
            }
        }
        return;
    }

    if (t.type == TOKEN_LOAD_LIB) {
        Token libToken = scanToken();
        if (libToken.type == TOKEN_STRING) {
            char lib[64];
            snprintf(lib, libToken.length - 1, "%s", libToken.start + 1);
            load_lib(lib);
        }
        return;
    }

    if (t.type == TOKEN_KITTY_PORT) {
        Token nameToken = scanToken();
        if (nameToken.type == TOKEN_STRING) {
            char name[64];
            snprintf(name, sizeof(name), "%s", nameToken.start + 1);
            name[nameToken.length - 2] = '\0';

            char lnxPath[256];
            snprintf(lnxPath, sizeof(lnxPath), "libs/%s/main.lnx", name);

            int alreadyLoaded = 0;
            for (int i = 0; i < loaded_pkg_count; i++) {
                if (strcmp(loaded_packages[i], name) == 0) { alreadyLoaded = 1; break; }
            }

            FILE* f = fopen(lnxPath, "r");
            if (f) {
                fclose(f);
                if (alreadyLoaded) return;

                extern Variable den[];
                extern int varCount;
                Variable savedDen[100];
                int savedCount = varCount;
                for (int i = 0; i < varCount; i++) savedDen[i] = den[i];

                runFile(lnxPath, 0, NULL);

                for (int i = 0; i < varCount; i++) {
                    if (den[i].value.strValue) free(den[i].value.strValue);
                }
                for (int i = 0; i < savedCount; i++) den[i] = savedDen[i];
                varCount = savedCount;

                loaded_packages[loaded_pkg_count++] = strdup(name);
                return;
            }

            char dllPath[256];
            snprintf(dllPath, sizeof(dllPath), "lib/%s.dll", name);
            f = fopen(dllPath, "r");
            if (f) {
                fclose(f);
                load_lib(name);
                return;
            }

            setError("KittyPort: package not found");
            printf("%s\n", lynx_error);
            clearError();
        }
        return;
    }

    // ─── FILE I/O ──────────────────────────────────────────────
    if (t.type == TOKEN_KITTY_WRITE_FILE) {
        Token path = scanToken();
        Token content = scanToken();
        if (path.type == TOKEN_STRING && content.type == TOKEN_STRING) {
            char p[256], c[4096];
            snprintf(p, path.length - 1, "%s", path.start + 1);
            snprintf(c, content.length - 1, "%s", content.start + 1);
            FILE* f = fopen(p, "w");
            if (f) { fwrite(c, 1, strlen(c), f); fclose(f); }
            else { setError("Could not write file"); printf("%s\n", lynx_error); clearError(); }
        }
        return;
    }

    if (t.type == TOKEN_KITTY_READ_FILE) {
        Token path = scanToken();
        if (path.type == TOKEN_STRING) {
            char p[256];
            snprintf(p, path.length - 1, "%s", path.start + 1);
            FILE* f = fopen(p, "r");
            if (f) {
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                rewind(f);
                char* buf = malloc(size + 1);
                fread(buf, 1, size, f);
                buf[size] = '\0';
                fclose(f);
                setVarString("__file_content", buf);
                printf("%s\n", buf);
                free(buf);
            } else {
                setError("File not found");
                printf("%s\n", lynx_error);
                clearError();
            }
        }
        return;
    }

    if (t.type == TOKEN_PAW) {
        Token path = scanToken();
        if (path.type == TOKEN_STRING) {
            char p[256];
            snprintf(p, path.length - 1, "%s", path.start + 1);
            #ifdef _WIN32
                if (_mkdir(p) != 0) {
                    setError("Could not create directory");
                    printf("%s\n", lynx_error);
                    clearError();
                }
            #else
                if (mkdir(p, 0777) != 0) {
                    setError("Could not create directory");
                    printf("%s\n", lynx_error);
                    clearError();
                }
            #endif
        }
        return;
    }

    if (t.type == TOKEN_KITTY_FILE_EXISTS) {
        Token path = scanToken();
        if (path.type == TOKEN_STRING) {
            char p[256];
            snprintf(p, path.length - 1, "%s", path.start + 1);
            FILE* f = fopen(p, "r");
            setVar("__result", f ? 1.0 : 0.0);
            if (f) fclose(f);
        }
        return;
    }

    if (t.type == TOKEN_KITTY_REMOVE_FILE) {
        Token path = scanToken();
        if (path.type == TOKEN_STRING) {
            char p[256];
            snprintf(p, path.length - 1, "%s", path.start + 1);
            if (remove(p) != 0) {
                setError("Could not remove file");
                printf("%s\n", lynx_error);
                clearError();
            }
        }
        return;
    }

    if (t.type == TOKEN_RUN) {
        Token cmd = scanToken();
        if (cmd.type == TOKEN_STRING) {
            char c[512];
            snprintf(c, cmd.length - 1, "%s", cmd.start + 1);
            int result = system(c);
            if (result != 0) {
                setError("Command failed");
                printf("%s\n", lynx_error);
                clearError();
            }
        }
        return;
    }

    if (t.type == TOKEN_TRY) {
        parse_try_catch();
        return;
    }

    if (t.type == TOKEN_ARGV) {
        Token idxToken = scanToken();
        if (idxToken.type == TOKEN_LBRACKET) {
            double idx = parse_expression();
            if (peekToken().type == TOKEN_RBRACKET) scanToken();
            // TODO: Return argv[idx] as a variable
            printf("argv[%.0f]\n", idx);
        }
        return;
    }

    if (t.type == TOKEN_GET_ERROR) {
        if (lynx_error) {
            printf("%s\n", lynx_error);
            clearError();
        } else {
            printf("OK\n");
        }
        return;
    }

    // ─── STRING FUNCTIONS ──────────────────────────────────────
    if (t.type == TOKEN_STRING_SPLIT) {
        Token strTok = scanToken();
        Token delimTok = scanToken();
        if (strTok.type == TOKEN_STRING && delimTok.type == TOKEN_STRING) {
            char str[4096];
            char delim[256];
            snprintf(str, strTok.length - 1, "%s", strTok.start + 1);
            snprintf(delim, delimTok.length - 1, "%s", delimTok.start + 1);
            char* token = strtok(str, delim);
            int idx = 0;
            while (token != NULL) {
                printf("[%d] %s\n", idx++, token);
                token = strtok(NULL, delim);
            }
            setVar("__result", (double)idx);
        }
        return;
    }

    if (t.type == TOKEN_STRING_CONTAINS) {
        Token hayTok = scanToken();
        Token needleTok = scanToken();
        if (hayTok.type == TOKEN_STRING && needleTok.type == TOKEN_STRING) {
            char hay[4096];
            char needle[256];
            snprintf(hay, hayTok.length - 1, "%s", hayTok.start + 1);
            snprintf(needle, needleTok.length - 1, "%s", needleTok.start + 1);
            setVar("__result", str_contains(hay, needle) ? 1.0 : 0.0);
        }
        return;
    }

    if (t.type == TOKEN_STRING_REPLACE) {
        Token srcTok = scanToken();
        Token oldTok = scanToken();
        Token newTok = scanToken();
        if (srcTok.type == TOKEN_STRING && oldTok.type == TOKEN_STRING && newTok.type == TOKEN_STRING) {
            char src[4096], old[256], new[256];
            snprintf(src, srcTok.length - 1, "%s", srcTok.start + 1);
            snprintf(old, oldTok.length - 1, "%s", oldTok.start + 1);
            snprintf(new, newTok.length - 1, "%s", newTok.start + 1);
            char* result = str_replace(src, old, new);
            setVarString("__result", result);
        }
        return;
    }

    if (t.type == TOKEN_TRIM) {
        Token strTok = scanToken();
        if (strTok.type == TOKEN_STRING) {
            char str[4096];
            snprintf(str, strTok.length - 1, "%s", strTok.start + 1);
            char* trimmed = str_trim(str);
            setVarString("__result", trimmed);
        }
        return;
    }

    if (t.type == TOKEN_LEN) {
        Token strTok = scanToken();
        if (strTok.type == TOKEN_STRING) {
            char str[4096];
            snprintf(str, strTok.length - 1, "%s", strTok.start + 1);
            setVar("__result", (double)strlen(str));
        }
        return;
    }

    // ─── UNKNOWN ────────────────────────────────────────────────
    if (t.type != TOKEN_HELP && t.type != TOKEN_EOF) {
        setError("Unknown statement");
        printf("%s\n", lynx_error);
        clearError();
    }
}