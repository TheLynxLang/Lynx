#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lynx.h"

#define MAX_VARS 100
#define MAX_FUNCS 50

typedef struct {
    char name[64];
    double numValue;
    char* strValue;
    VarType type;
} VariableImpl;

VariableImpl den[MAX_VARS];
int varCount = 0;

typedef struct {
    char name[64];
    char params[10][64];
    int paramCount;
    char* body;
} FunctionDef;

FunctionDef functions[MAX_FUNCS];
int funcCount = 0;

// Variable Management
void setVar(const char* name, double val) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) {
            den[i].numValue = val;
            den[i].type = VAR_NUMBER;
            if (den[i].strValue) free(den[i].strValue);
            den[i].strValue = NULL;
            return;
        }
    }
    
    if (varCount < MAX_VARS) {
        strcpy(den[varCount].name, name);
        den[varCount].numValue = val;
        den[varCount].type = VAR_NUMBER;
        den[varCount].strValue = NULL;
        varCount++;
    }
}

void setVarString(const char* name, const char* value) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) {
            if (den[i].strValue) free(den[i].strValue);
            den[i].strValue = malloc(strlen(value) + 1);
            strcpy(den[i].strValue, value);
            den[i].type = VAR_STRING;
            return;
        }
    }
    
    if (varCount < MAX_VARS) {
        strcpy(den[varCount].name, name);
        den[varCount].strValue = malloc(strlen(value) + 1);
        strcpy(den[varCount].strValue, value);
        den[varCount].type = VAR_STRING;
        den[varCount].numValue = 0;
        varCount++;
    }
}

double getVar(const char* name) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) {
            if (den[i].type == VAR_NUMBER) {
                return den[i].numValue;
            }
            return 0;
        }
    }
    return 0;
}

char* getVarString(const char* name) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) {
            if (den[i].type == VAR_STRING) {
                return den[i].strValue;
            }
            return "";
        }
    }
    return "";
}

void pounce(const char* name) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) {
            if (den[i].strValue) free(den[i].strValue);
            for (int j = i; j < varCount - 1; j++) {
                den[j] = den[j + 1];
            }
            varCount--;
            printf("🐾 Pounced %s\n", name);
            return;
        }
    }
    printf("🐾 %s not found in den\n", name);
}

void hunt() {
    printf("\n🐾 DEN CONTENTS:\n");
    for (int i = 0; i < varCount; i++) {
        if (den[i].type == VAR_NUMBER) {
            printf("   %-12s : %.5f (number)\n", den[i].name, den[i].numValue);
        } else if (den[i].type == VAR_STRING) {
            printf("   %-12s : \"%s\" (string)\n", den[i].name, den[i].strValue);
        }
    }
    printf("\n");
}

// Function Management
void defineFunction(const char* name, const char** params, int paramCount, const char* body) {
    if (funcCount >= MAX_FUNCS) {
        printf("🐾 Too many functions defined!\n");
        return;
    }
    
    strcpy(functions[funcCount].name, name);
    functions[funcCount].paramCount = paramCount;
    functions[funcCount].body = malloc(strlen(body) + 1);
    strcpy(functions[funcCount].body, body);
    
    for (int i = 0; i < paramCount; i++) {
        strcpy(functions[funcCount].params[i], params[i]);
    }
    
    funcCount++;
    printf("🐾 Defined function: %s\n", name);
}

int callFunction(const char* name) {
    for (int i = 0; i < funcCount; i++) {
        if (strcmp(functions[i].name, name) == 0) {
            printf("🐾 Called function: %s\n", name);
            
            // Save current variables
            VariableImpl savedDen[MAX_VARS];
            int savedVarCount = varCount;
            for (int j = 0; j < varCount; j++) {
                savedDen[j] = den[j];
            }
            
            // Execute function body
            initScanner(functions[i].body);
            while (peekToken().type != TOKEN_EOF) {
                parse_statement();
            }
            
            // Restore variables
            for (int j = 0; j < varCount; j++) {
                if (den[j].strValue) free(den[j].strValue);
            }
            for (int j = 0; j < savedVarCount; j++) {
                den[j] = savedDen[j];
            }
            varCount = savedVarCount;
            
            return 1;
        }
    }
    
    printf("🐾 Function '%s' not found\n", name);
    return 0;
}

void cleanup_all() {
    for (int i = 0; i < varCount; i++) {
        if (den[i].strValue) free(den[i].strValue);
    }
    
    for (int i = 0; i < funcCount; i++) {
        if (functions[i].body) free(functions[i].body);
    }
}
