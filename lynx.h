#ifndef LYNX_H
#define LYNX_H

#include <stdbool.h>

typedef enum {
    TOKEN_SET, TOKEN_ROAR, TOKEN_HUNT, TOKEN_STALK_PACK, TOKEN_HELP,
    TOKEN_POUNCE, TOKEN_IF, TOKEN_ELSE, TOKEN_LOAD_LIB,
    TOKEN_FUNC, TOKEN_RETURN, TOKEN_FOR, TOKEN_WHILE, TOKEN_BREAK, TOKEN_CONTINUE,
    TOKEN_AND, TOKEN_OR, TOKEN_NOT, TOKEN_LBRACKET, TOKEN_RBRACKET,

    // File I/O
    TOKEN_KITTY_WRITE_FILE,
    TOKEN_KITTY_READ_FILE,
    TOKEN_PAW,
    TOKEN_KITTY_FILE_EXISTS,
    TOKEN_KITTY_LIST_FILES,
    TOKEN_KITTY_REMOVE_FILE,
    TOKEN_KITTY_READ_DIR,

    // System
    TOKEN_RUN,

    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    TOKEN_EQUAL, TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MODULO,
    TOKEN_INCREMENT, TOKEN_DECREMENT,
    TOKEN_EQ, TOKEN_NE, TOKEN_GT, TOKEN_LT, TOKEN_GE, TOKEN_LE,
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_COMMA, TOKEN_COLON,
    TOKEN_EOF, TOKEN_ERROR
} LynxTokenType;

typedef struct {
    LynxTokenType type;
    const char* start;
    int length;
    int line;
} Token;

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

typedef enum {
    VAR_NUMBER,
    VAR_STRING,
    VAR_ARRAY
} VarType;

typedef struct {
    char name[64];
    VarType type;
    union {
        double numValue;
        char* strValue;
    } value;
} Variable;

typedef struct {
    char name[64];
    char params[10][64];
    int paramCount;
    char* body;
} Function;

extern Scanner scanner;

void initScanner(const char* source);
Token scanToken();
Token peekToken();

void parse_statement();
void parse_block();
double parse_expression();
int check_condition();
void runFile(const char* path);

void setVar(const char* name, double value);
void setVarString(const char* name, const char* value);
double getVar(const char* name);
char* getVarString(const char* name);
void pounce(const char* name);
void hunt();

void defineFunction(const char* name, const char** params, int paramCount, const char* body);
int callFunction(const char* name);

void load_lib(const char* lib_name);
void unload_all_libs();

void compile_to_exe(const char* input, const char* output, int standalone);

void cleanup_all();

#endif