#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include "lynx.h"

// ─── ERROR HANDLING ──────────────────────────────────────────────
char* lynx_error = NULL;

void clearError() {
    if (lynx_error) {
        free(lynx_error);
        lynx_error = NULL;
    }
}

void setError(const char* msg) {
    clearError();
    lynx_error = malloc(strlen(msg) + 1);
    if (lynx_error) strcpy(lynx_error, msg);
}

// ─── KITTYPORT CACHE ─────────────────────────────────────────────
char* loaded_packages[64];
int loaded_pkg_count = 0;

// ─── FORWARD DECLARATIONS ────────────────────────────────────────
void parse_statement();
double parse_expression();
int parse_logic_expression();

// ─── STRING HELPERS ──────────────────────────────────────────────
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

// ─── PARSING ──────────────────────────────────────────────────────

double parse_primary() {
    Token t = scanToken();
    if (t.type == TOKEN_NUMBER) return atof(t.start);
    if (t.type == TOKEN_IDENTIFIER) {
        char name[64];
        snprintf(name, t.length + 1, "%s", t.start);
        if (peekToken().type == TOKEN_LPAREN) {
            scanToken(); scanToken();
            return callFunction(name);
        }
        return getVar(name);
    }
    if (t.type == TOKEN_STRING) {
        char str[256];
        snprintf(str, t.length - 1, "%s", t.start + 1);
        printf("%s\n", str);
        return 0;
    }
    if (t.type == TOKEN_LPAREN) {
        double val = parse_expression();
        scanToken();
        return val;
    }
    if (t.type == TOKEN_NOT) return !parse_primary();
    return 0;
}

double parse_term() {
    double value = parse_primary();
    Token op = peekToken();
    while (op.type == TOKEN_STAR || op.type == TOKEN_SLASH || op.type == TOKEN_MODULO) {
        scanToken();
        double right = parse_primary();
        switch (op.type) {
            case TOKEN_STAR: value *= right; break;
            case TOKEN_SLASH:
                if (right == 0) {
                    setError("Division by zero");
                    return 0;
                }
                value /= right;
                break;
            case TOKEN_MODULO:
                if (right == 0) {
                    setError("Modulo by zero");
                    return 0;
                }
                value = (double)((int)value % (int)right);
                break;
            default: break;
        }
        op = peekToken();
    }
    return value;
}

double parse_expression() {
    double value = parse_term();
    Token op = peekToken();
    while (op.type == TOKEN_PLUS || op.type == TOKEN_MINUS) {
        scanToken();
        double right = parse_term();
        if (op.type == TOKEN_PLUS) value += right;
        else value -= right;
        op = peekToken();
    }
    return value;
}

int parse_comparison() {
    double left = parse_expression();
    Token op = scanToken();
    if (op.type == TOKEN_EQ || op.type == TOKEN_NE || op.type == TOKEN_GT ||
        op.type == TOKEN_LT || op.type == TOKEN_GE || op.type == TOKEN_LE) {
        double right = parse_expression();
        switch (op.type) {
            case TOKEN_EQ: return left == right;
            case TOKEN_NE: return left != right;
            case TOKEN_GT: return left > right;
            case TOKEN_LT: return left < right;
            case TOKEN_GE: return left >= right;
            case TOKEN_LE: return left <= right;
            default: return 0;
        }
    }
    return 0;
}

int parse_logic_expression() {
    int left = parse_comparison();
    Token op = peekToken();
    while (op.type == TOKEN_AND || op.type == TOKEN_OR) {
        scanToken();
        int right = parse_comparison();
        if (op.type == TOKEN_AND) left = left && right;
        else left = left || right;
        op = peekToken();
    }
    return left;
}

void parse_block() {
    Token brace = scanToken();
    if (brace.type != TOKEN_LBRACE) return;
    while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF)
        parse_statement();
    scanToken();
}

void parse_for_loop() {
    Token varToken = scanToken();
    if (varToken.type != TOKEN_IDENTIFIER) return;
    char varName[64];
    snprintf(varName, varToken.length + 1, "%s", varToken.start);
    Token eq = scanToken();
    if (eq.type != TOKEN_EQUAL) return;
    double start = parse_expression();
    setVar(varName, start);
    Token toToken = scanToken();
    if (toToken.type != TOKEN_IDENTIFIER || strncmp(toToken.start, "To", 2) != 0) return;
    double end = parse_expression();
    Token lbrace = scanToken();
    if (lbrace.type != TOKEN_LBRACE) return;
    Scanner loopStart = scanner;
    for (double i = start; i <= end; i++) {
        setVar(varName, i);
        scanner = loopStart;
        while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF)
            parse_statement();
    }
    scanToken();
}

void parse_while_loop() {
    int condition = parse_logic_expression();
    Token lbrace = scanToken();
    if (lbrace.type != TOKEN_LBRACE) return;
    Scanner loopStart = scanner;
    while (condition) {
        scanner = loopStart;
        while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF)
            parse_statement();
        condition = parse_logic_expression();
        scanToken();
        lbrace = scanToken();
        if (lbrace.type != TOKEN_LBRACE) break;
        loopStart = scanner;
    }
}

void parse_function_def() {
    Token nameToken = scanToken();
    if (nameToken.type != TOKEN_IDENTIFIER) return;
    char funcName[64];
    snprintf(funcName, nameToken.length + 1, "%s", nameToken.start);
    Token lparen = scanToken();
    if (lparen.type != TOKEN_LPAREN) return;
    char params[10][64];
    int paramCount = 0;
    while (peekToken().type != TOKEN_RPAREN && peekToken().type != TOKEN_EOF) {
        Token param = scanToken();
        if (param.type == TOKEN_IDENTIFIER) {
            snprintf(params[paramCount], param.length + 1, "%s", param.start);
            paramCount++;
        }
        if (peekToken().type == TOKEN_COMMA) scanToken();
    }
    scanToken();
    Scanner bodyStart = scanner;
    Token brace = scanToken();
    if (brace.type != TOKEN_LBRACE) return;
    int braceCount = 1;
    while (braceCount > 0 && peekToken().type != TOKEN_EOF) {
        Token t = scanToken();
        if (t.type == TOKEN_LBRACE) braceCount++;
        if (t.type == TOKEN_RBRACE) braceCount--;
    }
    int bodyLen = (int)(scanner.current - bodyStart.current);
    char* body = malloc(bodyLen + 1);
    strncpy(body, bodyStart.current, bodyLen);
    body[bodyLen] = '\0';
    defineFunction(funcName, (const char**)params, paramCount, body);
}

void parse_statement() {
    Token t = scanToken();

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
            runFile(path);
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

    // ─── KITTY PORT ──────────────────────────────────────────────
    if (t.type == TOKEN_KITTY_PORT) {
        Token nameToken = scanToken();
        if (nameToken.type == TOKEN_STRING) {
            char name[64];
            snprintf(name, sizeof(name), "%s", nameToken.start + 1);
            name[nameToken.length - 2] = '\0';

            // 1. Try .lnx (scoped, cached)
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

                // Save current scope
                Variable savedDen[100];
                int savedCount = varCount;
                for (int i = 0; i < varCount; i++) savedDen[i] = den[i];

                runFile(lnxPath);

                // Restore scope
                for (int i = 0; i < varCount; i++) {
                    if (den[i].value.strValue) free(den[i].value.strValue);
                }
                for (int i = 0; i < savedCount; i++) den[i] = savedDen[i];
                varCount = savedCount;

                // Cache
                loaded_packages[loaded_pkg_count++] = strdup(name);
                return;
            }

            // 2. If not .lnx, try .dll (no scoping, no caching)
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

    // ─── FILE I/O ─────────────────────────────────────────────────
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

    // ─── ERROR ────────────────────────────────────────────────────
    if (t.type == TOKEN_GET_ERROR) {
        if (lynx_error) {
            printf("%s\n", lynx_error);
            clearError();
        } else {
            printf("OK\n");
        }
        return;
    }

    // ─── STRING FUNCTIONS ─────────────────────────────────────────
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

    // ─── CONTROL FLOW ─────────────────────────────────────────────
    if (t.type == TOKEN_IF) {
        int cond = parse_logic_expression();
        if (cond) {
            parse_block();
        } else {
            Scanner save = scanner;
            parse_block();
            scanner = save;
            if (peekToken().type == TOKEN_ELSE) {
                scanToken();
                parse_block();
            }
        }
        return;
    }

    if (t.type == TOKEN_FOR) { parse_for_loop(); return; }
    if (t.type == TOKEN_WHILE) { parse_while_loop(); return; }
    if (t.type == TOKEN_FUNC) { parse_function_def(); return; }
    if (t.type == TOKEN_HELP || t.type == TOKEN_EOF) return;
}