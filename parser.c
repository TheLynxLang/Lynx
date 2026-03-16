#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lynx.h"

// Forward declarations
void parse_statement();
double parse_expression();
int parse_logic_expression();

double parse_primary() {
    Token t = scanToken();
    
    if (t.type == TOKEN_NUMBER) {
        return atof(t.start);
    } 
    else if (t.type == TOKEN_IDENTIFIER) {
        char name[64];
        snprintf(name, t.length + 1, "%s", t.start);
        
        // Check for function call
        if (peekToken().type == TOKEN_LPAREN) {
            scanToken(); // consume (
            // TODO: Parse function arguments
            scanToken(); // consume )
            return callFunction(name);
        }
        
        return getVar(name);
    }
    else if (t.type == TOKEN_STRING) {
        char str[256];
        snprintf(str, t.length - 1, "%s", t.start + 1);
        printf("%s\n", str);
        return 0;
    }
    else if (t.type == TOKEN_LPAREN) {
        double val = parse_expression();
        scanToken(); // consume )
        return val;
    }
    else if (t.type == TOKEN_NOT) {
        return !parse_primary();
    }
    
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
                    printf("🐾 Can't divide by zero!\n");
                    return 0;
                }
                value /= right;
                break;
            case TOKEN_MODULO:
                if (right == 0) {
                    printf("🐾 Can't modulo by zero!\n");
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
        
        switch (op.type) {
            case TOKEN_PLUS: value += right; break;
            case TOKEN_MINUS: value -= right; break;
            default: break;
        }
        
        op = peekToken();
    }
    
    return value;
}

int parse_comparison() {
    double left = parse_expression();
    
    Token op = scanToken();
    if (op.type == TOKEN_EQ || op.type == TOKEN_NE || 
        op.type == TOKEN_GT || op.type == TOKEN_LT ||
        op.type == TOKEN_GE || op.type == TOKEN_LE) {
        
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
        
        switch (op.type) {
            case TOKEN_AND: left = left && right; break;
            case TOKEN_OR: left = left || right; break;
            default: break;
        }
        
        op = peekToken();
    }
    
    return left;
}

void parse_block() {
    Token brace = scanToken();
    if (brace.type != TOKEN_LBRACE) return;
    
    while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF) {
        parse_statement();
    }
    scanToken(); // consume }
}

void parse_for_loop() {
    // For i = 0 To 10 { ... }
    Token varToken = scanToken();
    if (varToken.type != TOKEN_IDENTIFIER) return;
    
    char varName[64];
    snprintf(varName, varToken.length + 1, "%s", varToken.start);
    
    Token eq = scanToken();
    if (eq.type != TOKEN_EQUAL) return;
    
    double start = parse_expression();
    setVar(varName, start);
    
    // Expect "To"
    Token toToken = scanToken();
    if (toToken.type != TOKEN_IDENTIFIER || strncmp(toToken.start, "To", 2) != 0) return;
    
    double end = parse_expression();
    
    Token lbrace = scanToken();
    if (lbrace.type != TOKEN_LBRACE) return;
    
    // Save loop body position
    Scanner loopStart = scanner;
    
    // Execute loop
    for (double i = start; i <= end; i++) {
        setVar(varName, i);
        scanner = loopStart;
        
        while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF) {
            parse_statement();
        }
    }
    
    scanToken(); // consume }
}

void parse_while_loop() {
    // While x > 5 { ... }
    int condition = parse_logic_expression();
    
    Token lbrace = scanToken();
    if (lbrace.type != TOKEN_LBRACE) return;
    
    Scanner loopStart = scanner;
    
    while (condition) {
        scanner = loopStart;
        
        while (peekToken().type != TOKEN_RBRACE && peekToken().type != TOKEN_EOF) {
            parse_statement();
        }
        
        condition = parse_logic_expression();
        scanToken(); // consume }
        lbrace = scanToken();
        if (lbrace.type != TOKEN_LBRACE) break;
        loopStart = scanner;
    }
}

void parse_function_def() {
    // Func myFunc(param1, param2) { ... }
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
        
        if (peekToken().type == TOKEN_COMMA) {
            scanToken();
        }
    }
    
    scanToken(); // consume )
    
    // Save function body
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

    if (t.type == TOKEN_HUNT) {
        hunt();
        return;
    }

    if (t.type == TOKEN_ROAR) {
        Token val = scanToken();
        if (val.type == TOKEN_STRING) {
            for(int i = 1; i < val.length - 1; i++) printf("%c", val.start[i]);
            printf("\n");
        } else if (val.type == TOKEN_IDENTIFIER) {
            char name[64];
            snprintf(name, val.length + 1, "%s", val.start);
            printf("%.5f\n", getVar(name));
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
            double finalVal = parse_expression();
            setVar(varName, finalVal);
        }
        else if (op.type == TOKEN_INCREMENT) {
            setVar(varName, getVar(varName) + 1);
        }
        else if (op.type == TOKEN_DECREMENT) {
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
        }
        return;
    }

    if (t.type == TOKEN_POUNCE) {
        Token nameToken = scanToken();
        if (nameToken.type == TOKEN_IDENTIFIER) {
            char varName[64];
            snprintf(varName, nameToken.length + 1, "%s", nameToken.start);
            pounce(varName);
        }
        return;
    }

    if (t.type == TOKEN_LOAD_LIB) {
        Token libToken = scanToken();
        if (libToken.type == TOKEN_STRING) {
            char libName[64];
            snprintf(libName, libToken.length - 1, "%s", libToken.start + 1);
            load_lib(libName);
        }
        return;
    }

    if (t.type == TOKEN_IF) {
        int condition = parse_logic_expression();
        
        if (condition) {
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
    
    if (t.type == TOKEN_FOR) {
        parse_for_loop();
        return;
    }
    
    if (t.type == TOKEN_WHILE) {
        parse_while_loop();
        return;
    }
    
    if (t.type == TOKEN_FUNC) {
        parse_function_def();
        return;
    }

    if (t.type == TOKEN_HELP || t.type == TOKEN_EOF) return;
}
