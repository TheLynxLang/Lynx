#ifndef LYNX_H
#define LYNX_H

#include <stdbool.h>

// Token Types
typedef enum {
    // Commands
    TOKEN_SET, TOKEN_ROAR, TOKEN_HUNT, TOKEN_STALK_PACK, TOKEN_HELP,
    TOKEN_POUNCE, TOKEN_IF, TOKEN_ELSE, TOKEN_LOAD_LIB,
    
    // New: Functions and Loops
    TOKEN_FUNC, TOKEN_RETURN, TOKEN_FOR, TOKEN_WHILE, TOKEN_BREAK, TOKEN_CONTINUE,
    
    // New: Logic and Array
    TOKEN_AND, TOKEN_OR, TOKEN_NOT, TOKEN_LBRACKET, TOKEN_RBRACKET,
    
    // Identifiers and Values
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    
    // Operators
    TOKEN_EQUAL, TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MODULO,
    TOKEN_INCREMENT, TOKEN_DECREMENT,
    TOKEN_EQ, TOKEN_NE, TOKEN_GT, TOKEN_LT, TOKEN_GE, TOKEN_LE,
    
    // Delimiters
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_COMMA, TOKEN_COLON,
    
    // End
    TOKEN_EOF, TOKEN_ERROR
} LynxTokenType;

// Token Structure
typedef struct {
    LynxTokenType type;
    const char* start;
    int length;
    int line;
} Token;

// Scanner State
typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

// Variable Type - now supports strings and numbers
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

// Function Definition
typedef struct {
    char name[64];
    char params[10][64];
    int paramCount;
    char* body;
} Function;

// Global Scanner
extern Scanner scanner;

// Scanner Functions
void initScanner(const char* source);
Token scanToken();
Token peekToken();

// Parser Functions
void parse_statement();
void parse_block();
double parse_expression();
int check_condition();
void runFile(const char* path);

// Variable Functions
void setVar(const char* name, double value);
void setVarString(const char* name, const char* value);
double getVar(const char* name);
char* getVarString(const char* name);
void pounce(const char* name);
void hunt();

// Function Functions
void defineFunction(const char* name, const char** params, int paramCount, const char* body);
int callFunction(const char* name);

// Library Loading
void load_lib(const char* lib_name);
void unload_all_libs();

// Cleanup
void cleanup_all();

#endif
