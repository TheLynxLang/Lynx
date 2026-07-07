#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <ctype.h>
#include "lynx.h"


// GLOBALS
char* lynx_error = NULL;
char* loaded_packages[64];
int loaded_pkg_count = 0;

// ─── FORWARD DECLARATIONS ──────────────────────────────────────
void parse_statement();
double parse_expression();
int parse_logic_expression();

// ─── EXTERNAL ERROR HANDLING ──────────────────────────────────
extern char* lynx_error;
extern void setError(const char* msg);
extern void clearError();

// ─── KITTYPORT CACHE ──────────────────────────────────────────
extern char* loaded_packages[64];
extern int loaded_pkg_count;

// ─── PARSING ──────────────────────────────────────────────────

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

// ─── PARSE STATEMENT ──────────────────────────────────────────
void parse_statement() {
    Token t = scanToken();

    // ─── CONTROL FLOW ──────────────────────────────────────────────
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

    // ─── BUILT-IN COMMANDS (handled in pawcom.c) ────────────────
    // All built-in commands (Set, Roar, Hunt, Pounce, KittyPort,
    // file I/O, Try/Catch, Argv, string functions, etc.) are
    // implemented in pawcom.c via pawcom_parse_statement().

    extern void pawcom_parse_statement(Token t);
    pawcom_parse_statement(t);

    if (t.type == TOKEN_HELP || t.type == TOKEN_EOF) return;
}