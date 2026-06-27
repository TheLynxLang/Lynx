#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "lynx.h"

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAtEnd() { return *scanner.current == '\0'; }
static char advance() { return *scanner.current++; }
static char peek() { return *scanner.current; }
static char peekNext() { return scanner.current[1]; }

static Token makeToken(LynxTokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static LynxTokenType checkKeyword() {
    int len = (int)(scanner.current - scanner.start);
    const char* s = scanner.start;

    if (len == 3 && strncmp(s, "Set", 3) == 0) return TOKEN_SET;
    if (len == 4 && strncmp(s, "Roar", 4) == 0) return TOKEN_ROAR;
    if (len == 4 && strncmp(s, "Hunt", 4) == 0) return TOKEN_HUNT;
    if (len == 4 && strncmp(s, "Help", 4) == 0) return TOKEN_HELP;
    if (len == 10 && strncmp(s, "Stalk_Pack", 10) == 0) return TOKEN_STALK_PACK;
    if (len == 6 && strncmp(s, "Pounce", 6) == 0) return TOKEN_POUNCE;
    if (len == 2 && strncmp(s, "If", 2) == 0) return TOKEN_IF;
    if (len == 4 && strncmp(s, "Else", 4) == 0) return TOKEN_ELSE;
    if (len == 7 && strncmp(s, "LoadLib", 7) == 0) return TOKEN_LOAD_LIB;
    if (len == 4 && strncmp(s, "Func", 4) == 0) return TOKEN_FUNC;
    if (len == 6 && strncmp(s, "Return", 6) == 0) return TOKEN_RETURN;
    if (len == 3 && strncmp(s, "For", 3) == 0) return TOKEN_FOR;
    if (len == 5 && strncmp(s, "While", 5) == 0) return TOKEN_WHILE;
    if (len == 5 && strncmp(s, "Break", 5) == 0) return TOKEN_BREAK;
    if (len == 8 && strncmp(s, "Continue", 8) == 0) return TOKEN_CONTINUE;
    if (len == 3 && strncmp(s, "And", 3) == 0) return TOKEN_AND;
    if (len == 2 && strncmp(s, "Or", 2) == 0) return TOKEN_OR;
    if (len == 3 && strncmp(s, "Not", 3) == 0) return TOKEN_NOT;
    if (len == 3 && strncmp(s, "Run", 3) == 0) return TOKEN_RUN;

    // File I/O
    if (len == 15 && strncmp(s, "KittyWriteFile", 15) == 0) return TOKEN_KITTY_WRITE_FILE;
    if (len == 14 && strncmp(s, "KittyReadFile", 14) == 0) return TOKEN_KITTY_READ_FILE;
    if (len == 3 && strncmp(s, "Paw", 3) == 0) return TOKEN_PAW;
    if (len == 16 && strncmp(s, "KittyFileExists", 16) == 0) return TOKEN_KITTY_FILE_EXISTS;
    if (len == 15 && strncmp(s, "KittyListFiles", 15) == 0) return TOKEN_KITTY_LIST_FILES;
    if (len == 16 && strncmp(s, "KittyRemoveFile", 16) == 0) return TOKEN_KITTY_REMOVE_FILE;
    if (len == 14 && strncmp(s, "KittyReadDir", 14) == 0) return TOKEN_KITTY_READ_DIR;

    // Package manager
    if (len == 9 && strncmp(s, "KittyPort", 9) == 0) return TOKEN_KITTY_PORT;

    // String functions
    if (len == 11 && strncmp(s, "StringSplit", 11) == 0) return TOKEN_STRING_SPLIT;
    if (len == 14 && strncmp(s, "StringContains", 14) == 0) return TOKEN_STRING_CONTAINS;
    if (len == 13 && strncmp(s, "StringReplace", 13) == 0) return TOKEN_STRING_REPLACE;
    if (len == 4 && strncmp(s, "Trim", 4) == 0) return TOKEN_TRIM;
    if (len == 3 && strncmp(s, "Len", 3) == 0) return TOKEN_LEN;

    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (isalnum(peek()) || peek() == '_') advance();
    return makeToken(checkKeyword());
}

static Token number() {
    while (isdigit(peek())) advance();
    if (peek() == '.' && isdigit(peekNext())) {
        advance();
        while (isdigit(peek())) advance();
    }
    return makeToken(TOKEN_NUMBER);
}

Token peekToken() {
    Scanner checkpoint = scanner;
    Token token = scanToken();
    scanner = checkpoint;
    return token;
}

Token scanToken() {
    while (isspace(peek())) {
        if (advance() == '\n') scanner.line++;
    }

    if (peek() == '#') {
        while (peek() != '\n' && !isAtEnd()) advance();
        if (isAtEnd()) return makeToken(TOKEN_EOF);
        return scanToken();
    }

    if (peek() == '/' && peekNext() == '/') {
        advance(); advance();
        while (peek() != '\n' && !isAtEnd()) advance();
        if (isAtEnd()) return makeToken(TOKEN_EOF);
        return scanToken();
    }

    scanner.start = scanner.current;
    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();

    if (isalpha(c) || c == '_') return identifier();
    if (isdigit(c)) return number();

    switch (c) {
        case '+':
            if (peek() == '+') { advance(); return makeToken(TOKEN_INCREMENT); }
            return makeToken(TOKEN_PLUS);
        case '-':
            if (peek() == '-') { advance(); return makeToken(TOKEN_DECREMENT); }
            return makeToken(TOKEN_MINUS);
        case '*': return makeToken(TOKEN_STAR);
        case '/': return makeToken(TOKEN_SLASH);
        case '%': return makeToken(TOKEN_MODULO);
        case '{': return makeToken(TOKEN_LBRACE);
        case '}': return makeToken(TOKEN_RBRACE);
        case '(': return makeToken(TOKEN_LPAREN);
        case ')': return makeToken(TOKEN_RPAREN);
        case '[': return makeToken(TOKEN_LBRACKET);
        case ']': return makeToken(TOKEN_RBRACKET);
        case ',': return makeToken(TOKEN_COMMA);
        case ':': return makeToken(TOKEN_COLON);
        case '=':
            if (peek() == '=') { advance(); return makeToken(TOKEN_EQ); }
            return makeToken(TOKEN_EQUAL);
        case '!':
            if (peek() == '=') { advance(); return makeToken(TOKEN_NE); }
            return makeToken(TOKEN_NOT);
        case '>':
            if (peek() == '=') { advance(); return makeToken(TOKEN_GE); }
            return makeToken(TOKEN_GT);
        case '<':
            if (peek() == '=') { advance(); return makeToken(TOKEN_LE); }
            return makeToken(TOKEN_LT);
        case '"': {
            while (peek() != '"' && !isAtEnd()) {
                if (peek() == '\n') scanner.line++;
                advance();
            }
            if (isAtEnd()) return makeToken(TOKEN_ERROR);
            advance();
            return makeToken(TOKEN_STRING);
        }
    }

    return makeToken(TOKEN_ERROR);
}