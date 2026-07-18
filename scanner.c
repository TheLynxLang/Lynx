#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "lynx.h"
#include "platform.h"

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.col = 1;
    
    // Skip UTF-8 BOM (EF BB BF)
    if ((unsigned char)source[0] == 0xEF && 
        (unsigned char)source[1] == 0xBB && 
        (unsigned char)source[2] == 0xBF) {
        scanner.current += 3;
        scanner.start += 3;
    }
    
    // Skip UTF-16 LE BOM (FF FE)
    if ((unsigned char)source[0] == 0xFF && 
        (unsigned char)source[1] == 0xFE) {
        scanner.current += 2;
        scanner.start += 2;
    }
    
    // Skip UTF-16 BE BOM (FE FF)
    if ((unsigned char)source[0] == 0xFE && 
        (unsigned char)source[1] == 0xFF) {
        scanner.current += 2;
        scanner.start += 2;
    }
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}

static char advance() {
    scanner.col++;
    return *scanner.current++;
}

static char peek() {
    return *scanner.current;
}

static char peekNext() {
    return scanner.current[1];
}

static Token makeToken(LynxTokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    token.col = scanner.col - token.length;
    return token;
}

const char* tokenTypeToString(LynxTokenType type) {
    switch (type) {
        case TOKEN_SET: return "Set";
        case TOKEN_ROAR: return "Roar";
        case TOKEN_HUNT: return "Hunt";
        case TOKEN_STALK_PACK: return "Stalk_Pack";
        case TOKEN_POUNCE: return "Pounce";
        case TOKEN_IF: return "If";
        case TOKEN_ELSE: return "Else";
        case TOKEN_LOAD_LIB: return "LoadLib";
        case TOKEN_FUNC: return "Func";
        case TOKEN_RETURN: return "Return";
        case TOKEN_FOR: return "For";
        case TOKEN_WHILE: return "While";
        case TOKEN_BREAK: return "Break";
        case TOKEN_CONTINUE: return "Continue";
        case TOKEN_AND: return "And";
        case TOKEN_OR: return "Or";
        case TOKEN_NOT: return "Not";
        case TOKEN_RUN: return "Run";
        case TOKEN_TRY: return "Try";
        case TOKEN_CATCH: return "Catch";
        case TOKEN_ARGV: return "Argv";
        case TOKEN_EXPORT: return "Export";
        case TOKEN_KITTY_WRITE_FILE: return "KittyWriteFile";
        case TOKEN_KITTY_READ_FILE: return "KittyReadFile";
        case TOKEN_PAW: return "Paw";
        case TOKEN_KITTY_FILE_EXISTS: return "KittyFileExists";
        case TOKEN_KITTY_LIST_FILES: return "KittyListFiles";
        case TOKEN_KITTY_REMOVE_FILE: return "KittyRemoveFile";
        case TOKEN_KITTY_READ_DIR: return "KittyReadDir";
        case TOKEN_KITTY_PORT: return "KittyPort";
        case TOKEN_GET_ERROR: return "GetError";
        case TOKEN_STRING_SPLIT: return "KittySplitString";
        case TOKEN_STRING_CONTAINS: return "KittyCheckIfStringContains";
        case TOKEN_STRING_REPLACE: return "KittyReplaceString";
        case TOKEN_TRIM: return "Trim";
        case TOKEN_LEN: return "Len";
        case TOKEN_IDENTIFIER: return "identifier";
        case TOKEN_STRING: return "string literal";
        case TOKEN_NUMBER: return "number";
        case TOKEN_EQUAL: return "=";
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_MODULO: return "%";
        case TOKEN_INCREMENT: return "++";
        case TOKEN_DECREMENT: return "--";
        case TOKEN_EQ: return "==";
        case TOKEN_NE: return "!=";
        case TOKEN_GT: return ">";
        case TOKEN_LT: return "<";
        case TOKEN_GE: return ">=";
        case TOKEN_LE: return "<=";
        case TOKEN_LBRACE: return "{";
        case TOKEN_RBRACE: return "}";
        case TOKEN_LPAREN: return "(";
        case TOKEN_RPAREN: return ")";
        case TOKEN_LBRACKET: return "[";
        case TOKEN_RBRACKET: return "]";
        case TOKEN_COMMA: return ",";
        case TOKEN_COLON: return ":";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "error token";
        default: return "unknown";
    }
}

char* getTokenText(Token t) {
    static char buffer[256];
    if (t.length < 255) {
        snprintf(buffer, t.length + 1, "%s", t.start);
    } else {
        snprintf(buffer, sizeof(buffer), "%s...", t.start);
    }
    return buffer;
}

static LynxTokenType checkKeyword() {
    const char* s = scanner.start;
    int len = (int)(scanner.current - scanner.start);

    // ─── TRY/CATCH MUST BE CHECKED FIRST ──────────────────────
    if (strcmp(s, "Try") == 0) return TOKEN_TRY;
    if (strcmp(s, "Catch") == 0) return TOKEN_CATCH;

    // ─── OTHER KEYWORDS ──────────────────────────────────────
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
    if (strcmp(s, "Argv") == 0) return TOKEN_ARGV;
    if (strcmp(s, "Export") == 0) return TOKEN_EXPORT;

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

    // Error
    if (strcmp(s, "GetError") == 0) return TOKEN_GET_ERROR;

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

static void skip_whitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == '\n') {
            scanner.line++;
            scanner.col = 1;
            advance();
        } else if (c == '\r') {
            advance();
            if (peek() == '\n') {
                scanner.line++;
                scanner.col = 1;
                advance();
            }
        } else if (isspace(c)) {
            advance();
        } else {
            break;
        }
    }
}

Token scanToken() {
    skip_whitespace();

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
                if (peek() == '\n') {
                    scanner.line++;
                    scanner.col = 1;
                }
                advance();
            }
            if (isAtEnd()) {
                Token t = makeToken(TOKEN_ERROR);
                setErrorF("Unterminated string literal", t.line, t.col);
                return t;
            }
            advance();
            return makeToken(TOKEN_STRING);
        }
    }

    Token t = makeToken(TOKEN_ERROR);
    char msg[256];
    snprintf(msg, sizeof(msg), "Unexpected character '%c'", c);
    setErrorF(msg, t.line, t.col);
    return t;
}