#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include "lynx.h"
#include "platform.h"

#ifdef _WIN32
#include <direct.h>
#endif

// ─── EXTERNAL DECLARATIONS ────────────────────────────────────
extern Scanner scanner;
extern char* lynx_error;
extern char* loaded_packages[64];
extern int loaded_pkg_count;
extern TryState try_state;

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
extern void setError(const char* msg, int line, int col);
extern void setErrorF(const char* format, ...);
extern void clearError();
extern const char* tokenTypeToString(LynxTokenType type);
extern char* getTokenText(Token t);

// ─── STRING HELPERS ────────────────────────────────────────────
static char* str_trim_copy(const char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return strdup("");
    const char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    size_t len = end - str + 1;
    char* result = malloc(len + 1);
    strncpy(result, str, len);
    result[len] = '\0';
    return result;
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

// ─── SPLIT STRING (Windows safe) ──────────────────────────────
static char** split_string(const char* str, const char* delim, int* count) {
    char* copy = strdup(str);
    char** result = malloc(256 * sizeof(char*));
    *count = 0;
    char* next_token = NULL;
    char* token = strtok_r(copy, delim, &next_token);
    while (token && *count < 256) {
        result[(*count)++] = strdup(token);
        token = strtok_r(NULL, delim, &next_token);
    }
    free(copy);
    return result;
}

// ─── PARSE ARRAY ──────────────────────────────────────────────
static void parse_array() {
    Token nameToken = scanToken();
    if (nameToken.type != TOKEN_IDENTIFIER) {
        char* text = getTokenText(nameToken);
        setErrorF("Array assignment expects variable name, got '%s' (type: %s)", 
                  text, tokenTypeToString(nameToken.type));
        return;
    }
    char varName[64];
    snprintf(varName, nameToken.length + 1, "%s", nameToken.start);

    Token op = scanToken();
    if (op.type != TOKEN_EQUAL) {
        char* text = getTokenText(op);
        setErrorF("Array assignment expects '=', got '%s' (type: %s)", 
                  text, tokenTypeToString(op.type));
        return;
    }

    Token bracket = scanToken();
    if (bracket.type != TOKEN_LBRACKET) {
        char* text = getTokenText(bracket);
        setErrorF("Array assignment expects '[', got '%s' (type: %s)", 
                  text, tokenTypeToString(bracket.type));
        return;
    }

    int count = 0;
    double values[256];
    while (peekToken().type != TOKEN_RBRACKET && peekToken().type != TOKEN_EOF) {
        double val = parse_expression();
        if (lynx_error) return;
        values[count++] = val;
        if (peekToken().type == TOKEN_COMMA) scanToken();
    }

    if (peekToken().type == TOKEN_RBRACKET) {
        scanToken();
    } else {
        setErrorF("Array assignment expects ']' to close array");
        return;
    }

    printf("Array %s created with %d elements\n", varName, count);
}

// ─── PARSE TRY / CATCH ────────────────────────────────────────
void parse_try_catch() {
    try_state.is_trying = 1;
    try_state.caught = 0;
    try_state.error_message = NULL;
    try_state.error_line = 0;
    try_state.error_col = 0;
    
    if (setjmp(try_state.env) == 0) {
        // Try block
        Token brace = scanToken();
        if (brace.type != TOKEN_LBRACE) {
            char* text = getTokenText(brace);
            setErrorF("Try expects '{', got '%s' (type: %s)", 
                      text, tokenTypeToString(brace.type));
            try_state.is_trying = 0;
            return;
        }
        
        while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF) {
            parse_statement();
            if (lynx_error && !try_state.is_trying) break;
        }
        if (peekToken().type == TOKEN_RBRACE) scanToken();
        
        // If no error occurred, skip the Catch block
        if (!lynx_error) {
            try_state.is_trying = 0;
            if (peekToken().type == TOKEN_CATCH) {
                scanToken();  // Consume Catch
                Token brace2 = scanToken();
                if (brace2.type == TOKEN_LBRACE) {
                    int depth = 1;
                    while (depth > 0 && peekToken().type != TOKEN_EOF) {
                        Token t = scanToken();
                        if (t.type == TOKEN_LBRACE) depth++;
                        if (t.type == TOKEN_RBRACE) depth--;
                    }
                }
            }
        } else {
            // Error occurred, but we'll handle it in the longjmp path
            try_state.is_trying = 0;
        }
    } else {
        // Catch block (jumped here from setError)
        try_state.is_trying = 0;
        try_state.caught = 1;
        clearError();
        
        Token next = peekToken();
        if (next.type != TOKEN_CATCH) {
            // No Catch block - re-throw the error
            if (try_state.error_message) {
                fprintf(stderr, "🐾 %s\n", try_state.error_message);
            }
            return;
        }
        scanToken();  // Consume Catch
        
        Token brace = scanToken();
        if (brace.type != TOKEN_LBRACE) {
            char* text = getTokenText(brace);
            setErrorF("Catch expects '{', got '%s' (type: %s)", 
                      text, tokenTypeToString(brace.type));
            return;
        }
        
        while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF) {
            parse_statement();
            if (lynx_error) break;
        }
        if (peekToken().type == TOKEN_RBRACE) scanToken();
        clearError();
    }
}

// ─── KITTY PORT WITH EXPORT SUPPORT ───────────────────────────
static void kitty_port(const char* name) {
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
        Variable* savedDen = malloc(varCount * sizeof(Variable));
        int savedCount = varCount;
        for (int i = 0; i < varCount; i++) savedDen[i] = den[i];

        runFile(lnxPath, 0, NULL);

        for (int i = 0; i < varCount; i++) {
            if (den[i].value.strValue) free(den[i].value.strValue);
        }
        for (int i = 0; i < savedCount; i++) den[i] = savedDen[i];
        varCount = savedCount;
        free(savedDen);

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

    setErrorF("KittyPort: Package '%s' not found in libs/ or lib/", name);
    printf("%s\n", lynx_error);
    clearError();
}

// ─── MAIN DISPATCH ────────────────────────────────────────────
int pawcom_parse_statement(Token t) {
    if (t.type == TOKEN_HUNT) { hunt(); return 1; }

    // ─── ROAR WITH STRING CONCATENATION ────────────────────────
    if (t.type == TOKEN_ROAR) {
        // Build the result string
        char result[4096] = "";
        int first = 1;
        
        while (1) {
            Token val = scanToken();
            
            // If we hit a newline or EOF without anything to print
            if (val.type == TOKEN_EOF || val.type == TOKEN_ERROR) {
                break;
            }
            
            if (val.type == TOKEN_STRING) {
                char str[4096];
                snprintf(str, val.length - 1, "%s", val.start + 1);
                strncat(result, str, sizeof(result) - strlen(result) - 1);
                first = 0;
            } else if (val.type == TOKEN_IDENTIFIER) {
                char name[64];
                snprintf(name, val.length + 1, "%s", val.start);
                char* s = getVarString(name);
                if (s && strlen(s) > 0) {
                    strncat(result, s, sizeof(result) - strlen(result) - 1);
                } else {
                    double num = getVar(name);
                    if (lynx_error) {
                        printf("%s\n", lynx_error);
                        clearError();
                        return 1;
                    }
                    char numStr[32];
                    snprintf(numStr, sizeof(numStr), "%.5f", num);
                    strncat(result, numStr, sizeof(result) - strlen(result) - 1);
                }
                first = 0;
            } else if (val.type == TOKEN_NUMBER) {
                char numStr[32];
                snprintf(numStr, sizeof(numStr), "%.5f", atof(val.start));
                strncat(result, numStr, sizeof(result) - strlen(result) - 1);
                first = 0;
            } else {
                // If we got something else and it's not an operator, error
                if (val.type != TOKEN_PLUS) {
                    char* text = getTokenText(val);
                    setErrorF("Roar expects string, identifier, or number, got '%s' (type: %s)",
                              text, tokenTypeToString(val.type));
                    printf("%s\n", lynx_error);
                    clearError();
                    return 1;
                }
                // It's a '+', check if there's more
                Token next = peekToken();
                if (next.type == TOKEN_EOF || next.type == TOKEN_ERROR) {
                    break;
                }
                continue; // Continue to next token
            }
            
            // Check if next token is '+'
            Token next = peekToken();
            if (next.type == TOKEN_PLUS) {
                scanToken(); // consume '+'
                // Check if there's anything after '+'
                Token after = peekToken();
                if (after.type == TOKEN_EOF || after.type == TOKEN_ERROR) {
                    setErrorF("Expected expression after '+' in Roar");
                    printf("%s\n", lynx_error);
                    clearError();
                    return 1;
                }
                continue;
            } else {
                break;
            }
        }
        
        // If result is empty, print nothing
        if (strlen(result) > 0) {
            printf("%s\n", result);
        }
        return 1;
    }

    if (t.type == TOKEN_SET) {
        Token nameToken = scanToken();
        if (nameToken.type != TOKEN_IDENTIFIER) {
            char* text = getTokenText(nameToken);
            setErrorF("Set expects a variable name, got '%s' (type: %s)",
                      text, tokenTypeToString(nameToken.type));
            printf("%s\n", lynx_error);
            clearError();
            return 1;
        }
        char varName[64];
        snprintf(varName, nameToken.length + 1, "%s", nameToken.start);

        Token op = scanToken();
        if (op.type == TOKEN_EQUAL) {
            if (peekToken().type == TOKEN_LBRACKET) {
                parse_array();
                if (lynx_error) {
                    printf("%s\n", lynx_error);
                    clearError();
                }
                return 1;
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
                    return 1;
                }
                setVar(varName, val);
            }
        } else if (op.type == TOKEN_INCREMENT) {
            setVar(varName, getVar(varName) + 1);
        } else if (op.type == TOKEN_DECREMENT) {
            setVar(varName, getVar(varName) - 1);
        } else {
            char* text = getTokenText(op);
            setErrorF("Set expected '=', '++', or '--', got '%s' (type: %s)",
                      text, tokenTypeToString(op.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
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
        } else {
            char* text = getTokenText(pathToken);
            setErrorF("Stalk_Pack expects a string path, got '%s' (type: %s)",
                      text, tokenTypeToString(pathToken.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
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
        } else {
            char* text = getTokenText(nameToken);
            setErrorF("Pounce expects a variable name, got '%s' (type: %s)",
                      text, tokenTypeToString(nameToken.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_LOAD_LIB) {
        Token libToken = scanToken();
        if (libToken.type == TOKEN_STRING) {
            char lib[64];
            snprintf(lib, libToken.length - 1, "%s", libToken.start + 1);
            load_lib(lib);
        } else {
            char* text = getTokenText(libToken);
            setErrorF("LoadLib expects a library name (string), got '%s' (type: %s)",
                      text, tokenTypeToString(libToken.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_KITTY_PORT) {
        Token nameToken = scanToken();
        if (nameToken.type == TOKEN_STRING) {
            char name[64];
            snprintf(name, sizeof(name), "%s", nameToken.start + 1);
            name[nameToken.length - 2] = '\0';
            kitty_port(name);
        } else {
            char* text = getTokenText(nameToken);
            setErrorF("KittyPort expects a package name (string), got '%s' (type: %s)",
                      text, tokenTypeToString(nameToken.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
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
            if (f) { 
                fwrite(c, 1, strlen(c), f); 
                fclose(f); 
            } else {
                setErrorF("KittyWriteFile: Could not open '%s' for writing", p);
                printf("%s\n", lynx_error);
                clearError();
            }
        } else {
            char* text1 = getTokenText(path);
            char* text2 = getTokenText(content);
            setErrorF("KittyWriteFile expects two strings (path, content), got '%s' (type: %s) and '%s' (type: %s)",
                      text1, tokenTypeToString(path.type), text2, tokenTypeToString(content.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
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
                setErrorF("KittyReadFile: File '%s' not found", p);
                printf("%s\n", lynx_error);
                clearError();
            }
        } else {
            char* text = getTokenText(path);
            setErrorF("KittyReadFile expects a string path, got '%s' (type: %s)",
                      text, tokenTypeToString(path.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_PAW) {
        Token path = scanToken();
        if (path.type == TOKEN_STRING) {
            char p[256];
            snprintf(p, path.length - 1, "%s", path.start + 1);
            #ifdef _WIN32
                if (_mkdir(p) != 0 && errno != EEXIST) {
                    setErrorF("Paw: Could not create directory '%s'", p);
                    printf("%s\n", lynx_error);
                    clearError();
                }
            #else
                if (mkdir(p, 0777) != 0 && errno != EEXIST) {
                    setErrorF("Paw: Could not create directory '%s'", p);
                    printf("%s\n", lynx_error);
                    clearError();
                }
            #endif
        } else {
            char* text = getTokenText(path);
            setErrorF("Paw expects a string path, got '%s' (type: %s)",
                      text, tokenTypeToString(path.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_KITTY_FILE_EXISTS) {
        Token path = scanToken();
        if (path.type == TOKEN_STRING) {
            char p[256];
            snprintf(p, path.length - 1, "%s", path.start + 1);
            FILE* f = fopen(p, "r");
            setVar("__result", f ? 1.0 : 0.0);
            if (f) fclose(f);
        } else {
            char* text = getTokenText(path);
            setErrorF("KittyFileExists expects a string path, got '%s' (type: %s)",
                      text, tokenTypeToString(path.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_KITTY_REMOVE_FILE) {
        Token path = scanToken();
        if (path.type == TOKEN_STRING) {
            char p[256];
            snprintf(p, path.length - 1, "%s", path.start + 1);
            if (remove(p) != 0) {
                setErrorF("KittyRemoveFile: Could not remove '%s'", p);
                printf("%s\n", lynx_error);
                clearError();
            }
        } else {
            char* text = getTokenText(path);
            setErrorF("KittyRemoveFile expects a string path, got '%s' (type: %s)",
                      text, tokenTypeToString(path.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_RUN) {
        Token cmd = scanToken();
        if (cmd.type == TOKEN_STRING) {
            char c[512];
            snprintf(c, cmd.length - 1, "%s", cmd.start + 1);
            int result = system(c);
            if (result != 0) {
                setErrorF("Run: Command failed with exit code %d", result);
                printf("%s\n", lynx_error);
                clearError();
            }
        } else {
            char* text = getTokenText(cmd);
            setErrorF("Run expects a string command, got '%s' (type: %s)",
                      text, tokenTypeToString(cmd.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_TRY) {
        parse_try_catch();
        return 1;
    }

    if (t.type == TOKEN_ARGV) {
        Token idxToken = scanToken();
        if (idxToken.type == TOKEN_LBRACKET) {
            double idx = parse_expression();
            if (peekToken().type == TOKEN_RBRACKET) {
                scanToken();
                printf("argv[%.0f]\n", idx);
            } else {
                Token next = peekToken();
                char* text = getTokenText(next);
                setErrorF("Argv expects ']' after index, got '%s' (type: %s)",
                          text, tokenTypeToString(next.type));
                printf("%s\n", lynx_error);
                clearError();
            }
        } else {
            char* text = getTokenText(idxToken);
            setErrorF("Argv expects '[', got '%s' (type: %s)",
                      text, tokenTypeToString(idxToken.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_GET_ERROR) {
        if (lynx_error) {
            printf("%s\n", lynx_error);
            clearError();
        } else {
            printf("OK\n");
        }
        return 1;
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
            
            int count = 0;
            char** result = split_string(str, delim, &count);
            
            for (int i = 0; i < count; i++) {
                setArrayStringElement("__result", i, result[i]);
                free(result[i]);
            }
            free(result);
            setVar("__result_count", (double)count);
        } else {
            char* text1 = getTokenText(strTok);
            char* text2 = getTokenText(delimTok);
            setErrorF("KittySplitString expects two strings (string, delimiter), got '%s' (type: %s) and '%s' (type: %s)",
                      text1, tokenTypeToString(strTok.type), text2, tokenTypeToString(delimTok.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
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
        } else {
            char* text1 = getTokenText(hayTok);
            char* text2 = getTokenText(needleTok);
            setErrorF("KittyCheckIfStringContains expects two strings (haystack, needle), got '%s' (type: %s) and '%s' (type: %s)",
                      text1, tokenTypeToString(hayTok.type), text2, tokenTypeToString(needleTok.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
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
        } else {
            char* text1 = getTokenText(srcTok);
            char* text2 = getTokenText(oldTok);
            char* text3 = getTokenText(newTok);
            setErrorF("KittyReplaceString expects three strings (source, old, new), got '%s' (type: %s), '%s' (type: %s), '%s' (type: %s)",
                      text1, tokenTypeToString(srcTok.type), text2, tokenTypeToString(oldTok.type), text3, tokenTypeToString(newTok.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_TRIM) {
        Token strTok = scanToken();
        if (strTok.type == TOKEN_STRING) {
            char str[4096];
            snprintf(str, strTok.length - 1, "%s", strTok.start + 1);
            char* trimmed = str_trim_copy(str);
            setVarString("__result", trimmed);
            free(trimmed);
        } else {
            char* text = getTokenText(strTok);
            setErrorF("Trim expects a string, got '%s' (type: %s)",
                      text, tokenTypeToString(strTok.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_LEN) {
        Token strTok = scanToken();
        if (strTok.type == TOKEN_STRING) {
            char str[4096];
            snprintf(str, strTok.length - 1, "%s", strTok.start + 1);
            setVar("__result", (double)strlen(str));
        } else if (strTok.type == TOKEN_IDENTIFIER) {
            char name[64];
            snprintf(name, strTok.length + 1, "%s", strTok.start);
            char* str = getVarString(name);
            setVar("__result", (double)strlen(str));
        } else {
            char* text = getTokenText(strTok);
            setErrorF("Len expects a string or variable name, got '%s' (type: %s)",
                      text, tokenTypeToString(strTok.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    if (t.type == TOKEN_EXPORT) {
        Token nameToken = scanToken();
        if (nameToken.type == TOKEN_IDENTIFIER) {
            char name[64];
            snprintf(name, nameToken.length + 1, "%s", nameToken.start);
            setVarString("__exports", getVarString(name));
            printf("🐾 Exported: %s\n", name);
        } else {
            char* text = getTokenText(nameToken);
            setErrorF("Export expects a variable name, got '%s' (type: %s)",
                      text, tokenTypeToString(nameToken.type));
            printf("%s\n", lynx_error);
            clearError();
        }
        return 1;
    }

    // ─── UNKNOWN ────────────────────────────────────────────────
    if (t.type != TOKEN_HELP && t.type != TOKEN_EOF) {
        char* text = getTokenText(t);
        setErrorF("Unknown command '%s' (type: %s) at line %d. See 'Help' for available commands",
                  text, tokenTypeToString(t.type), t.line);
        printf("%s\n", lynx_error);
        clearError();
    }
    return 0;
}