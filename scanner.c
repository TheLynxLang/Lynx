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
    const char* s = scanner.start;

    if (strcmp(s, "Set") == 0) return TOKEN_SET;
    if (strcmp(s, "Roar") == 0) return TOKEN_ROAR;
    if (strcmp(s, "Hunt") == 0) return TOKEN_HUNT;
    if (strcmp(s, "Help") == 0) return TOKEN_HELP;
    if (strcmp(s, "Stalk_Pack") == 0) return TOKEN_STALK_PACK;
    if (strcmp(s, "Pounce") == 0) return TOKEN_POUNCE;
    if (strcmp(s, "If") == 0) return TOKEN_IF;
    if (strcmp(s, "Else") == 0) return TOKEN_ELSE;
    if (strcmp(s, "LoadLib") == 0) return TOKEN_LOAD_LIB;
    if (strcmp(s, "Func") == 0) return TOKEN_FUNC;
    if (strcmp(s, "Return") == 0) return TOKEN_RETURN;
    if (strcmp(s, "For") == 0) return TOKEN_FOR;
    if (strcmp(s, "While") == 0) return TOKEN_WHILE;
    if (strcmp(s, "Break") == 0) return TOKEN_BREAK;
    if (strcmp(s, "Continue") == 0) return TOKEN_CONTINUE;
    if (strcmp(s, "And") == 0) return TOKEN_AND;
    if (strcmp(s, "Or") == 0) return TOKEN_OR;
    if (strcmp(s, "Not") == 0) return TOKEN_NOT;
    if (strcmp(s, "Run") == 0) return TOKEN_RUN;

    // File I/O
    if (strcmp(s, "KittyWriteFile") == 0) return TOKEN_KITTY_WRITE_FILE;
    if (strcmp(s, "KittyReadFile") == 0) return TOKEN_KITTY_READ_FILE;
    if (strcmp(s, "Paw") == 0) return TOKEN_PAW;
    if (strcmp(s, "KittyFileExists") == 0) return TOKEN_KITTY_FILE_EXISTS;
    if (strcmp(s, "KittyListFiles") == 0) return TOKEN_KITTY_LIST_FILES;
    if (strcmp(s, "KittyRemoveFile") == 0) return TOKEN_KITTY_REMOVE_FILE;
    if (strcmp(s, "KittyReadDir") == 0) return TOKEN_KITTY_READ_DIR;

    // Package manager
    if (strcmp(s, "KittyPort") == 0) return TOKEN_KITTY_PORT;

    // String functions
    if (strcmp(s, "KittySplitString") == 0) return TOKEN_STRING_SPLIT;
    if (strcmp(s, "KittyCheckIfStringContains") == 0) return TOKEN_STRING_CONTAINS;
    if (strcmp(s, "KittyReplaceString") == 0) return TOKEN_STRING_REPLACE;
    if (strcmp(s, "Trim") == 0) return TOKEN_TRIM;
    if (strcmp(s, "Len") == 0) return TOKEN_LEN;

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