#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lynx.h"

#define MAX_VARS 100
#define MAX_FUNCS 50

// No VariableImpl — use Variable from lynx.h
Variable den[MAX_VARS];
int varCount = 0;

typedef struct {
    char name[64];
    char params[10][64];
    int paramCount;
    char* body;
} FunctionDef;

FunctionDef functions[MAX_FUNCS];
int funcCount = 0;

void setVar(const char* name, double val) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) {
            den[i].value.numValue = val;
            den[i].type = VAR_NUMBER;
            if (den[i].value.strValue) free(den[i].value.strValue);
            den[i].value.strValue = NULL;
            return;
        }
    }
    
    if (varCount < MAX_VARS) {
        strcpy(den[varCount].name, name);
        den[varCount].value.numValue = val;
        den[varCount].type = VAR_NUMBER;
        den[varCount].value.strValue = NULL;
        varCount++;
    }
}

void setVarString(const char* name, const char* value) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) {
            if (den[i].value.strValue) free(den[i].value.strValue);
            den[i].value.strValue = malloc(strlen(value) + 1);
            strcpy(den[i].value.strValue, value);
            den[i].type = VAR_STRING;
            return;
        }
    }
    
    if (varCount < MAX_VARS) {
        strcpy(den[varCount].name, name);
        den[varCount].value.strValue = malloc(strlen(value) + 1);
        strcpy(den[varCount].value.strValue, value);
        den[varCount].type = VAR_STRING;
        den[varCount].value.numValue = 0;
        varCount++;
    }
}

double getVar(const char* name) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) {
            if (den[i].type == VAR_NUMBER) {
                return den[i].value.numValue;
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
                return den[i].value.strValue;
            }
            return "";
        }
    }
    return "";
}

void pounce(const char* name) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) {
            if (den[i].value.strValue) free(den[i].value.strValue);
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
            printf("   %-12s : %.5f (number)\n", den[i].name, den[i].value.numValue);
        } else if (den[i].type == VAR_STRING) {
            printf("   %-12s : \"%s\" (string)\n", den[i].name, den[i].value.strValue);
        }
    }
    printf("\n");
}

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
            
            Variable savedDen[MAX_VARS];
            int savedVarCount = varCount;
            for (int j = 0; j < varCount; j++) {
                savedDen[j] = den[j];
            }
            
            initScanner(functions[i].body);
            while (peekToken().type != TOKEN_EOF) {
                parse_statement();
            }
            
            for (int j = 0; j < varCount; j++) {
                if (den[j].value.strValue) free(den[j].value.strValue);
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
        if (den[i].value.strValue) free(den[i].value.strValue);
    }
    
    for (int i = 0; i < funcCount; i++) {
        if (functions[i].body) free(functions[i].body);
    }
}