#ifndef LYNX_H
#define LYNX_H

#include <stdbool.h>
#include <stdarg.h>
#include <setjmp.h>

#define LYNX_MAX_PATH 4096
#define MAX_VARS 1000
#define MAX_FUNCS 200
#define MAX_PACKAGES 64
#define MAX_STRING 16384
#define MAX_RECURSION 100

typedef enum {
    TOKEN_SET, TOKEN_ROAR, TOKEN_HUNT, TOKEN_STALK_PACK, TOKEN_HELP,
    TOKEN_POUNCE, TOKEN_IF, TOKEN_ELSE, TOKEN_LOAD_LIB,
    TOKEN_FUNC, TOKEN_RETURN, TOKEN_FOR, TOKEN_WHILE, TOKEN_BREAK, TOKEN_CONTINUE,
    TOKEN_AND, TOKEN_OR, TOKEN_NOT,
    TOKEN_LBRACKET, TOKEN_RBRACKET,
    TOKEN_TRY, TOKEN_CATCH,
    TOKEN_ARGV,
    TOKEN_KITTY_WRITE_FILE,
    TOKEN_KITTY_READ_FILE,
    TOKEN_PAW,
    TOKEN_KITTY_FILE_EXISTS,
    TOKEN_KITTY_LIST_FILES,
    TOKEN_KITTY_REMOVE_FILE,
    TOKEN_KITTY_READ_DIR,
    TOKEN_KITTY_PORT,
    TOKEN_EXPORT,
    TOKEN_RUN,
    TOKEN_GETENV,
    TOKEN_GET_ERROR,
    TOKEN_STRING_SPLIT,
    TOKEN_STRING_CONTAINS,
    TOKEN_STRING_REPLACE,
    TOKEN_TRIM,
    TOKEN_LEN,
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
    int col;
} Token;

typedef struct {
    const char* start;
    const char* current;
    int line;
    int col;
} Scanner;

typedef enum {
    VAR_NUMBER,
    VAR_STRING,
    VAR_ARRAY
} VarType;

typedef struct Variable {
    char name[64];
    VarType type;
    union {
        double numValue;
        char* strValue;
        struct Variable** array;
    } value;
    int array_length;
    int array_capacity;
} Variable;

typedef struct {
    char name[64];
    char params[10][64];
    int paramCount;
    char* body;
} Function;

typedef struct {
    char* message;
    int line;
    int col;
} LynxError;

typedef struct {
    jmp_buf env;
    int is_trying;
    int caught;
    char* error_message;
    int error_line;
    int error_col;
} TryState;

extern Scanner scanner;
extern char* lynx_error;
extern char* loaded_packages[64];
extern int loaded_pkg_count;
extern Variable den[];
extern int varCount;
extern LynxError lynx_error_state;
extern TryState try_state;

void initScanner(const char* source);
Token scanToken();
Token peekToken();
void parse_statement();
void parse_block();
double parse_expression();
int parse_logic_expression();
void runFile(const char* path, int argc, char** argv);
void setVar(const char* name, double value);
void setVarString(const char* name, const char* value);
double getVar(const char* name);
char* getVarString(const char* name);
void setArrayElement(const char* name, int index, double value);
double getArrayElement(const char* name, int index);
int getArrayLength(const char* name);
void setArrayStringElement(const char* name, int index, const char* value);
char* getArrayStringElement(const char* name, int index);
void pounce(const char* name);
void hunt();
void defineFunction(const char* name, const char** params, int paramCount, const char* body);
int callFunction(const char* name);
void load_lib(const char* lib_name);
void unload_all_libs();
void clearError();
void setError(const char* msg, int line, int col);
void setErrorF(const char* format, ...);
char* getError();
const char* tokenTypeToString(LynxTokenType type);
char* getTokenText(Token t);
void format_file(const char* path);
void check_file(const char* path);
void cleanup_all();
void parse_try_catch();
int pawcom_parse_statement(Token t);

#endif