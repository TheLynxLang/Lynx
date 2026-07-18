#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <setjmp.h>
#include "lynx.h"
#include "platform.h"

// ─── GLOBALS ──────────────────────────────────────────────────
extern char* lynx_error;
extern LynxError lynx_error_state;
extern TryState try_state;

char* loaded_packages[64];
int loaded_pkg_count = 0;

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

// ─── ERROR HANDLING ──────────────────────────────────────────────
void clearError() {
    if (lynx_error) {
        free(lynx_error);
        lynx_error = NULL;
    }
    if (lynx_error_state.message) {
        free(lynx_error_state.message);
        lynx_error_state.message = NULL;
    }
    lynx_error_state.line = 0;
    lynx_error_state.col = 0;
}

void setError(const char* msg, int line, int col) {
    clearError();
    lynx_error = malloc(256);
    lynx_error_state.message = malloc(256);
    if (lynx_error) {
        snprintf(lynx_error, 256, "[Line %d, Col %d] %s", line, col, msg);
        if (lynx_error_state.message) {
            strcpy(lynx_error_state.message, lynx_error);
        }
        lynx_error_state.line = line;
        lynx_error_state.col = col;
    }
    
    if (try_state.is_trying) {
        try_state.error_message = lynx_error_state.message;
        try_state.error_line = line;
        try_state.error_col = col;
        longjmp(try_state.env, 1);
    }
}

void setErrorF(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    int line = 1, col = 1;
    if (scanner.current != NULL) {
        Token t = peekToken();
        line = t.line;
        col = t.col;
    }
    setError(buffer, line, col);
}

// ─── PARSING ──────────────────────────────────────────────────────

double parse_primary() {
    Token t = scanToken();
    if (t.type == TOKEN_NUMBER) return atof(t.start);
    
    // ─── LEN() ──────────────────────────────────────────────────
    if (t.type == TOKEN_LEN) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("Len expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token arg = scanToken();
        if (arg.type == TOKEN_IDENTIFIER) {
            char name[64];
            snprintf(name, arg.length + 1, "%s", arg.start);
            char* str = getVarString(name);
            double result = (double)strlen(str);
            setVar("__result", result);
            Token rparen = scanToken();
            if (rparen.type != TOKEN_RPAREN) {
                setErrorF("Len expects ')'", rparen.line, rparen.col);
                return 0;
            }
            return result;
        } else if (arg.type == TOKEN_STRING) {
            char str[4096];
            snprintf(str, arg.length - 1, "%s", arg.start + 1);
            double result = (double)strlen(str);
            setVar("__result", result);
            Token rparen = scanToken();
            if (rparen.type != TOKEN_RPAREN) {
                setErrorF("Len expects ')'", rparen.line, rparen.col);
                return 0;
            }
            return result;
        } else {
            setErrorF("Len expects a string or variable name", arg.line, arg.col);
            return 0;
        }
    }
    
    // ─── getenv() ──────────────────────────────────────────────
    if (t.type == TOKEN_GETENV) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("getenv expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token arg = scanToken();
        if (arg.type != TOKEN_STRING) {
            setErrorF("getenv expects a string", arg.line, arg.col);
            return 0;
        }
        char name[256];
        snprintf(name, arg.length - 1, "%s", arg.start + 1);
        const char* value = getenv(name);
        Token rparen = scanToken();
        if (rparen.type != TOKEN_RPAREN) {
            setErrorF("getenv expects ')'", rparen.line, rparen.col);
            return 0;
        }
        if (value) {
            setVarString("__result", value);
            setVar("__result", (double)strlen(value));
            return (double)strlen(value);
        } else {
            setVarString("__result", "");
            setVar("__result", 0);
            return 0;
        }
    }
    
    // ─── KittyCheckIfStringContains() ──────────────────────────
    if (t.type == TOKEN_STRING_CONTAINS) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("KittyCheckIfStringContains expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token hayTok = scanToken();
        Token comma = scanToken();
        Token needleTok = scanToken();
        if (hayTok.type != TOKEN_STRING && hayTok.type != TOKEN_IDENTIFIER) {
            setErrorF("KittyCheckIfStringContains expects string or variable", hayTok.line, hayTok.col);
            return 0;
        }
        if (needleTok.type != TOKEN_STRING && needleTok.type != TOKEN_IDENTIFIER) {
            setErrorF("KittyCheckIfStringContains expects string or variable", needleTok.line, needleTok.col);
            return 0;
        }
        char hay[4096], needle[4096];
        if (hayTok.type == TOKEN_STRING) {
            snprintf(hay, hayTok.length - 1, "%s", hayTok.start + 1);
        } else {
            char name[64];
            snprintf(name, hayTok.length + 1, "%s", hayTok.start);
            char* val = getVarString(name);
            snprintf(hay, sizeof(hay), "%s", val);
        }
        if (needleTok.type == TOKEN_STRING) {
            snprintf(needle, needleTok.length - 1, "%s", needleTok.start + 1);
        } else {
            char name[64];
            snprintf(name, needleTok.length + 1, "%s", needleTok.start);
            char* val = getVarString(name);
            snprintf(needle, sizeof(needle), "%s", val);
        }
        Token rparen = scanToken();
        if (rparen.type != TOKEN_RPAREN) {
            setErrorF("KittyCheckIfStringContains expects ')'", rparen.line, rparen.col);
            return 0;
        }
        int result = str_contains(hay, needle);
        setVar("__result", result ? 1.0 : 0.0);
        return result ? 1.0 : 0.0;
    }
    
    // ─── KittySplitString() ─────────────────────────────────────
    if (t.type == TOKEN_STRING_SPLIT) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("KittySplitString expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token strTok = scanToken();
        Token comma = scanToken();
        Token delimTok = scanToken();
        if (strTok.type != TOKEN_STRING && strTok.type != TOKEN_IDENTIFIER) {
            setErrorF("KittySplitString expects string or variable", strTok.line, strTok.col);
            return 0;
        }
        if (delimTok.type != TOKEN_STRING && delimTok.type != TOKEN_IDENTIFIER) {
            setErrorF("KittySplitString expects string or variable", delimTok.line, delimTok.col);
            return 0;
        }
        char str[4096], delim[256];
        if (strTok.type == TOKEN_STRING) {
            snprintf(str, strTok.length - 1, "%s", strTok.start + 1);
        } else {
            char name[64];
            snprintf(name, strTok.length + 1, "%s", strTok.start);
            char* val = getVarString(name);
            snprintf(str, sizeof(str), "%s", val);
        }
        if (delimTok.type == TOKEN_STRING) {
            snprintf(delim, delimTok.length - 1, "%s", delimTok.start + 1);
        } else {
            char name[64];
            snprintf(name, delimTok.length + 1, "%s", delimTok.start);
            char* val = getVarString(name);
            snprintf(delim, sizeof(delim), "%s", val);
        }
        Token rparen = scanToken();
        if (rparen.type != TOKEN_RPAREN) {
            setErrorF("KittySplitString expects ')'", rparen.line, rparen.col);
            return 0;
        }
        int count = 0;
        char** result = split_string(str, delim, &count);
        for (int i = 0; i < count; i++) {
            setArrayStringElement("__result", i, result[i]);
            free(result[i]);
        }
        free(result);
        setVar("__result_count", (double)count);
        return (double)count;
    }
    
    // ─── KittyReplaceString() ──────────────────────────────────
    if (t.type == TOKEN_STRING_REPLACE) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("KittyReplaceString expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token srcTok = scanToken();
        Token comma1 = scanToken();
        Token oldTok = scanToken();
        Token comma2 = scanToken();
        Token newTok = scanToken();
        if (srcTok.type != TOKEN_STRING && srcTok.type != TOKEN_IDENTIFIER) {
            setErrorF("KittyReplaceString expects string or variable", srcTok.line, srcTok.col);
            return 0;
        }
        if (oldTok.type != TOKEN_STRING && oldTok.type != TOKEN_IDENTIFIER) {
            setErrorF("KittyReplaceString expects string or variable", oldTok.line, oldTok.col);
            return 0;
        }
        if (newTok.type != TOKEN_STRING && newTok.type != TOKEN_IDENTIFIER) {
            setErrorF("KittyReplaceString expects string or variable", newTok.line, newTok.col);
            return 0;
        }
        char src[4096], old[256], new[256];
        if (srcTok.type == TOKEN_STRING) {
            snprintf(src, srcTok.length - 1, "%s", srcTok.start + 1);
        } else {
            char name[64];
            snprintf(name, srcTok.length + 1, "%s", srcTok.start);
            char* val = getVarString(name);
            snprintf(src, sizeof(src), "%s", val);
        }
        if (oldTok.type == TOKEN_STRING) {
            snprintf(old, oldTok.length - 1, "%s", oldTok.start + 1);
        } else {
            char name[64];
            snprintf(name, oldTok.length + 1, "%s", oldTok.start);
            char* val = getVarString(name);
            snprintf(old, sizeof(old), "%s", val);
        }
        if (newTok.type == TOKEN_STRING) {
            snprintf(new, newTok.length - 1, "%s", newTok.start + 1);
        } else {
            char name[64];
            snprintf(name, newTok.length + 1, "%s", newTok.start);
            char* val = getVarString(name);
            snprintf(new, sizeof(new), "%s", val);
        }
        Token rparen = scanToken();
        if (rparen.type != TOKEN_RPAREN) {
            setErrorF("KittyReplaceString expects ')'", rparen.line, rparen.col);
            return 0;
        }
        char* result = str_replace(src, old, new);
        setVarString("__result", result);
        return (double)strlen(result);
    }
    
    // ─── Trim() ─────────────────────────────────────────────────
    if (t.type == TOKEN_TRIM) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("Trim expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token strTok = scanToken();
        if (strTok.type != TOKEN_STRING && strTok.type != TOKEN_IDENTIFIER) {
            setErrorF("Trim expects string or variable", strTok.line, strTok.col);
            return 0;
        }
        char str[4096];
        if (strTok.type == TOKEN_STRING) {
            snprintf(str, strTok.length - 1, "%s", strTok.start + 1);
        } else {
            char name[64];
            snprintf(name, strTok.length + 1, "%s", strTok.start);
            char* val = getVarString(name);
            snprintf(str, sizeof(str), "%s", val);
        }
        Token rparen = scanToken();
        if (rparen.type != TOKEN_RPAREN) {
            setErrorF("Trim expects ')'", rparen.line, rparen.col);
            return 0;
        }
        char* trimmed = str_trim_copy(str);
        setVarString("__result", trimmed);
        free(trimmed);
        return (double)strlen(trimmed);
    }
    
    // ─── KittyFileExists() ──────────────────────────────────────
    if (t.type == TOKEN_KITTY_FILE_EXISTS) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("KittyFileExists expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token pathTok = scanToken();
        if (pathTok.type != TOKEN_STRING && pathTok.type != TOKEN_IDENTIFIER) {
            setErrorF("KittyFileExists expects string or variable", pathTok.line, pathTok.col);
            return 0;
        }
        char path[4096];
        if (pathTok.type == TOKEN_STRING) {
            snprintf(path, pathTok.length - 1, "%s", pathTok.start + 1);
        } else {
            char name[64];
            snprintf(name, pathTok.length + 1, "%s", pathTok.start);
            char* val = getVarString(name);
            snprintf(path, sizeof(path), "%s", val);
        }
        Token rparen = scanToken();
        if (rparen.type != TOKEN_RPAREN) {
            setErrorF("KittyFileExists expects ')'", rparen.line, rparen.col);
            return 0;
        }
        FILE* f = fopen(path, "r");
        int exists = f ? 1 : 0;
        if (f) fclose(f);
        setVar("__result", exists ? 1.0 : 0.0);
        return exists ? 1.0 : 0.0;
    }
    
    // ─── IDENTIFIER ─────────────────────────────────────────────
    if (t.type == TOKEN_IDENTIFIER) {
        char name[64];
        snprintf(name, t.length + 1, "%s", t.start);
        if (peekToken().type == TOKEN_LPAREN) {
            scanToken(); 
            int argCount = 0;
            double args[10];
            while (peekToken().type != TOKEN_RPAREN && peekToken().type != TOKEN_EOF) {
                if (argCount > 0 && peekToken().type == TOKEN_COMMA) scanToken();
                args[argCount++] = parse_expression();
            }
            if (peekToken().type == TOKEN_RPAREN) scanToken();
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
        if (peekToken().type == TOKEN_RPAREN) {
            scanToken();
            return val;
        } else {
            Token next = peekToken();
            char* text = getTokenText(next);
            setErrorF("Expected ')' after expression, got '%s' (type: %s)", 
                      text, tokenTypeToString(next.type));
            return 0;
        }
    }
    
    if (t.type == TOKEN_NOT) {
        Token next = peekToken();
        if (next.type == TOKEN_EOF) {
            setErrorF("Expected expression after 'Not'");
            return 0;
        }
        return !parse_primary();
    }
    
    char* text = getTokenText(t);
    setErrorF("Expected number, identifier, string, or '(' got '%s' (type: %s)", 
              text, tokenTypeToString(t.type));
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
                    setErrorF("Division by zero at line %d", op.line);
                    return 0;
                }
                value /= right;
                break;
            case TOKEN_MODULO:
                if (right == 0) {
                    setErrorF("Modulo by zero at line %d", op.line);
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
    char* text = getTokenText(op);
    setErrorF("Expected comparison operator (==, !=, >, <, >=, <=), got '%s' (type: %s)", 
              text, tokenTypeToString(op.type));
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
    if (brace.type != TOKEN_LBRACE) {
        char* text = getTokenText(brace);
        setErrorF("Expected '{' at start of block, got '%s' (type: %s)", 
                  text, tokenTypeToString(brace.type));
        return;
    }
    while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF) {
        parse_statement();
        if (lynx_error) break;
    }
    if (peekToken().type == TOKEN_RBRACE) {
        scanToken();
    } else {
        setErrorF("Expected '}' at end of block, got EOF");
    }
}

void parse_for_loop() {
    Token varToken = scanToken();
    if (varToken.type != TOKEN_IDENTIFIER) {
        char* text = getTokenText(varToken);
        setErrorF("For loop expects variable name, got '%s' (type: %s)", 
                  text, tokenTypeToString(varToken.type));
        return;
    }
    char varName[64];
    snprintf(varName, varToken.length + 1, "%s", varToken.start);
    
    Token eq = scanToken();
    if (eq.type != TOKEN_EQUAL) {
        char* text = getTokenText(eq);
        setErrorF("For loop expects '=' after variable, got '%s' (type: %s)", 
                  text, tokenTypeToString(eq.type));
        return;
    }
    double start = parse_expression();
    setVar(varName, start);
    
    Token toToken = scanToken();
    if (toToken.type != TOKEN_IDENTIFIER || strncmp(toToken.start, "To", 2) != 0) {
        char* text = getTokenText(toToken);
        setErrorF("For loop expects 'To' after start value, got '%s' (type: %s)", 
                  text, tokenTypeToString(toToken.type));
        return;
    }
    double end = parse_expression();
    
    Token lbrace = scanToken();
    if (lbrace.type != TOKEN_LBRACE) {
        char* text = getTokenText(lbrace);
        setErrorF("For loop expects '{' after range, got '%s' (type: %s)", 
                  text, tokenTypeToString(lbrace.type));
        return;
    }
    
    Scanner loopStart = scanner;
    for (double i = start; i <= end; i++) {
        setVar(varName, i);
        scanner = loopStart;
        while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF) {
            parse_statement();
            if (lynx_error) break;
        }
        if (lynx_error) break;
    }
    scanToken();
}

void parse_while_loop() {
    Scanner condStart = scanner;
    int condition = parse_logic_expression();
    if (lynx_error) return;
    
    Token lbrace = scanToken();
    if (lbrace.type != TOKEN_LBRACE) {
        char* text = getTokenText(lbrace);
        setErrorF("While loop expects '{' after condition, got '%s' (type: %s)", 
                  text, tokenTypeToString(lbrace.type));
        return;
    }
    
    Scanner bodyStart = scanner;
    int braceDepth = 1;
    while (braceDepth > 0 && peekToken().type != TOKEN_EOF) {
        Token t = scanToken();
        if (t.type == TOKEN_LBRACE) braceDepth++;
        if (t.type == TOKEN_RBRACE) braceDepth--;
    }
    Scanner bodyEnd = scanner;
    
    int bodyLen = (int)(bodyEnd.current - bodyStart.start);
    char* body = malloc(bodyLen + 1);
    strncpy(body, bodyStart.start, bodyLen);
    body[bodyLen] = '\0';
    
    while (condition && !lynx_error) {
        initScanner(body);
        while (peekToken().type != TOKEN_EOF) {
            parse_statement();
            if (lynx_error) break;
        }
        if (lynx_error) break;
        initScanner(condStart.start);
        condition = parse_logic_expression();
    }
    free(body);
}

void parse_function_def() {
    Token nameToken = scanToken();
    if (nameToken.type != TOKEN_IDENTIFIER) {
        char* text = getTokenText(nameToken);
        setErrorF("Function definition expects function name, got '%s' (type: %s)", 
                  text, tokenTypeToString(nameToken.type));
        return;
    }
    char funcName[64];
    snprintf(funcName, nameToken.length + 1, "%s", nameToken.start);
    
    Token lparen = scanToken();
    if (lparen.type != TOKEN_LPAREN) {
        char* text = getTokenText(lparen);
        setErrorF("Function definition expects '(' after function name, got '%s' (type: %s)", 
                  text, tokenTypeToString(lparen.type));
        return;
    }
    
    char params[10][64];
    int paramCount = 0;
    while (peekToken().type != TOKEN_RPAREN && peekToken().type != TOKEN_EOF) {
        Token param = scanToken();
        if (param.type == TOKEN_IDENTIFIER) {
            snprintf(params[paramCount], param.length + 1, "%s", param.start);
            paramCount++;
        } else {
            char* text = getTokenText(param);
            setErrorF("Function parameter must be identifier, got '%s' (type: %s)", 
                      text, tokenTypeToString(param.type));
            return;
        }
        if (peekToken().type == TOKEN_COMMA) scanToken();
    }
    if (peekToken().type == TOKEN_RPAREN) {
        scanToken();
    } else {
        setErrorF("Function definition expects ')' after parameters");
        return;
    }
    
    Scanner bodyStart = scanner;
    Token brace = scanToken();
    if (brace.type != TOKEN_LBRACE) {
        char* text = getTokenText(brace);
        setErrorF("Function definition expects '{' after parameters, got '%s' (type: %s)", 
                  text, tokenTypeToString(brace.type));
        return;
    }
    
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

// ─── FORMAT / CHECK ──────────────────────────────────────────────
void format_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        setErrorF("Cannot open file '%s' for formatting", path);
        printf("%s\n", lynx_error);
        clearError();
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* src = malloc(size + 1);
    fread(src, 1, size, f);
    src[size] = '\0';
    fclose(f);

    char* result = malloc(size * 2 + 1);
    result[0] = '\0';
    int indent = 0;
    int line_start = 1;

    for (int i = 0; src[i]; i++) {
        char c = src[i];
        if (c == '\n') {
            strcat(result, "\n");
            line_start = 1;
        } else if (line_start) {
            for (int j = 0; j < indent * 2; j++) strcat(result, " ");
            line_start = 0;
            for (int j = i; src[j] && src[j] != '\n'; j++) {
                if (src[j] == '{') indent++;
                else if (src[j] == '}') indent--;
            }
            strcat(result, &src[i]);
            while (src[i] && src[i] != '\n') i++;
            if (src[i]) i--;
        }
    }
    if (strlen(result) > 0 && result[strlen(result)-1] != '\n')
        strcat(result, "\n");

    f = fopen(path, "w");
    if (f) {
        fwrite(result, 1, strlen(result), f);
        fclose(f);
        printf("🐾 Formatted %s\n", path);
    } else {
        setErrorF("Cannot write to file '%s'", path);
        printf("%s\n", lynx_error);
        clearError();
    }

    free(src);
    free(result);
}

void check_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        setErrorF("Cannot open file '%s' for checking", path);
        printf("%s\n", lynx_error);
        clearError();
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* src = malloc(size + 1);
    fread(src, 1, size, f);
    src[size] = '\0';
    fclose(f);

    char* old_error = lynx_error ? strdup(lynx_error) : NULL;
    clearError();

    initScanner(src);
    while (peekToken().type != TOKEN_EOF) {
        parse_statement();
        if (lynx_error) break;
    }

    if (lynx_error) {
        printf("🐾 Error: %s\n", lynx_error);
        clearError();
        free(src);
        return;
    } else {
        printf("✅ No errors found in %s\n", path);
    }

    if (old_error) {
        lynx_error = old_error;
    }

    free(src);
}

// ─── PARSE STATEMENT ──────────────────────────────────────────
void parse_statement() {
    Token t = scanToken();

    if (t.type == TOKEN_IF) {
        int cond = parse_logic_expression();
        if (lynx_error) return;
        
        if (cond) {
            parse_block();
        } else {
            Scanner save = scanner;
            parse_block();
            scanner = save;
            if (peekToken().type == TOKEN_ELSE) {
                scanToken();
                if (peekToken().type == TOKEN_IF) {
                    parse_statement();
                } else {
                    parse_block();
                }
            }
        }
        return;
    }

    if (t.type == TOKEN_FOR) { parse_for_loop(); return; }
    if (t.type == TOKEN_WHILE) { parse_while_loop(); return; }
    if (t.type == TOKEN_FUNC) { parse_function_def(); return; }
    
    if (t.type == TOKEN_TRY) {
        extern void parse_try_catch(void);
        parse_try_catch();
        return;
    }

    extern int pawcom_parse_statement(Token t);
    if (pawcom_parse_statement(t)) {
        return;
    }

    if (t.type == TOKEN_HELP || t.type == TOKEN_EOF) return;
    
    char* text = getTokenText(t);
    const char* typeName = tokenTypeToString(t.type);
    setErrorF("Unexpected '%s' (type: %s) at line %d. Expected a command like Roar, Set, If, Func, etc.",
              text, typeName, t.line);
    printf("%s\n", lynx_error);
    clearError();
}