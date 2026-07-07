#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lynx.h"

#define MAX_VARS 100
#define MAX_FUNCS 50

// ─── VARIABLE STORAGE ────────────────────────────────────────────
Variable den[MAX_VARS];
int varCount = 0;

// ─── FUNCTION STORAGE ────────────────────────────────────────────
typedef struct {
    char name[64];
    char params[10][64];
    int paramCount;
    char* body;
} FunctionDef;

FunctionDef functions[MAX_FUNCS];
int funcCount = 0;

// ─── ERROR STATE ────────────────────────────────────────────────
LynxError lynx_error_state = {0};

void clearError() {
    if (lynx_error_state.message) {
        free(lynx_error_state.message);
        lynx_error_state.message = NULL;
    }
    lynx_error_state.line = 0;
    lynx_error_state.col = 0;
}

void setError(const char* msg) {
    clearError();
    lynx_error_state.message = malloc(strlen(msg) + 1);
    if (lynx_error_state.message) strcpy(lynx_error_state.message, msg);
    // line/col would be set by parser
}

char* getError() {
    return lynx_error_state.message;
}

// ─── VARIABLE MANAGEMENT ────────────────────────────────────────
static Variable* findVar(const char* name) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) return &den[i];
    }
    return NULL;
}

void setVar(const char* name, double val) {
    Variable* v = findVar(name);
    if (v) {
        // Clear old array data if it was an array
        if (v->type == VAR_ARRAY) {
            for (int i = 0; i < v->array_length; i++) {
                if (v->value.array[i]) free(v->value.array[i]);
            }
            free(v->value.array);
        }
        if (v->type == VAR_STRING && v->value.strValue) {
            free(v->value.strValue);
        }
        v->type = VAR_NUMBER;
        v->value.numValue = val;
        v->array_length = 0;
        v->array_capacity = 0;
        return;
    }
    
    if (varCount < MAX_VARS) {
        strcpy(den[varCount].name, name);
        den[varCount].type = VAR_NUMBER;
        den[varCount].value.numValue = val;
        den[varCount].value.strValue = NULL;
        den[varCount].value.array = NULL;
        den[varCount].array_length = 0;
        den[varCount].array_capacity = 0;
        varCount++;
    }
}

void setVarString(const char* name, const char* value) {
    Variable* v = findVar(name);
    if (v) {
        if (v->type == VAR_ARRAY) {
            for (int i = 0; i < v->array_length; i++) {
                if (v->value.array[i]) free(v->value.array[i]);
            }
            free(v->value.array);
        }
        if (v->value.strValue) free(v->value.strValue);
        v->type = VAR_STRING;
        v->value.strValue = malloc(strlen(value) + 1);
        if (v->value.strValue) strcpy(v->value.strValue, value);
        v->array_length = 0;
        v->array_capacity = 0;
        return;
    }
    
    if (varCount < MAX_VARS) {
        strcpy(den[varCount].name, name);
        den[varCount].type = VAR_STRING;
        den[varCount].value.strValue = malloc(strlen(value) + 1);
        if (den[varCount].value.strValue) strcpy(den[varCount].value.strValue, value);
        den[varCount].value.array = NULL;
        den[varCount].array_length = 0;
        den[varCount].array_capacity = 0;
        varCount++;
    }
}

double getVar(const char* name) {
    Variable* v = findVar(name);
    if (!v) {
        setError("Variable not found");
        return 0;
    }
    if (v->type == VAR_NUMBER) return v->value.numValue;
    if (v->type == VAR_STRING) return 0;
    if (v->type == VAR_ARRAY) {
        setError("Cannot get array as number");
        return 0;
    }
    return 0;
}

char* getVarString(const char* name) {
    Variable* v = findVar(name);
    if (!v) {
        setError("Variable not found");
        return "";
    }
    if (v->type == VAR_STRING) return v->value.strValue;
    if (v->type == VAR_ARRAY) {
        setError("Cannot get array as string");
        return "";
    }
    return "";
}

// ─── ARRAY FUNCTIONS ────────────────────────────────────────────
void setArrayElement(const char* name, int index, double value) {
    Variable* v = findVar(name);
    if (!v) {
        setError("Array variable not found");
        return;
    }
    
    if (v->type != VAR_ARRAY) {
        setError("Variable is not an array");
        return;
    }
    
    if (index < 0) {
        setError("Array index out of bounds (negative)");
        return;
    }
    
    // Expand array if needed
    if (index >= v->array_capacity) {
        int new_cap = index + 10;
        v->value.array = realloc(v->value.array, new_cap * sizeof(Variable*));
        for (int i = v->array_capacity; i < new_cap; i++) {
            v->value.array[i] = NULL;
        }
        v->array_capacity = new_cap;
    }
    
    if (v->value.array[index] == NULL) {
        v->value.array[index] = calloc(1, sizeof(Variable));
        v->array_length++;
    }
    
    v->value.array[index]->type = VAR_NUMBER;
    v->value.array[index]->value.numValue = value;
}

double getArrayElement(const char* name, int index) {
    Variable* v = findVar(name);
    if (!v) {
        setError("Array variable not found");
        return 0;
    }
    
    if (v->type != VAR_ARRAY) {
        setError("Variable is not an array");
        return 0;
    }
    
    if (index < 0 || index >= v->array_capacity || v->value.array[index] == NULL) {
        setError("Array index out of bounds");
        return 0;
    }
    
    return v->value.array[index]->value.numValue;
}

int getArrayLength(const char* name) {
    Variable* v = findVar(name);
    if (!v) {
        setError("Array variable not found");
        return 0;
    }
    
    if (v->type != VAR_ARRAY) {
        setError("Variable is not an array");
        return 0;
    }
    
    return v->array_length;
}

void setArrayStringElement(const char* name, int index, const char* value) {
    Variable* v = findVar(name);
    if (!v) {
        setError("Array variable not found");
        return;
    }
    
    if (v->type != VAR_ARRAY) {
        setError("Variable is not an array");
        return;
    }
    
    if (index < 0) {
        setError("Array index out of bounds (negative)");
        return;
    }
    
    if (index >= v->array_capacity) {
        int new_cap = index + 10;
        v->value.array = realloc(v->value.array, new_cap * sizeof(Variable*));
        for (int i = v->array_capacity; i < new_cap; i++) {
            v->value.array[i] = NULL;
        }
        v->array_capacity = new_cap;
    }
    
    if (v->value.array[index] == NULL) {
        v->value.array[index] = calloc(1, sizeof(Variable));
        v->array_length++;
    }
    
    v->value.array[index]->type = VAR_STRING;
    if (v->value.array[index]->value.strValue) free(v->value.array[index]->value.strValue);
    v->value.array[index]->value.strValue = malloc(strlen(value) + 1);
    if (v->value.array[index]->value.strValue) strcpy(v->value.array[index]->value.strValue, value);
}

char* getArrayStringElement(const char* name, int index) {
    Variable* v = findVar(name);
    if (!v) {
        setError("Array variable not found");
        return "";
    }
    
    if (v->type != VAR_ARRAY) {
        setError("Variable is not an array");
        return "";
    }
    
    if (index < 0 || index >= v->array_capacity || v->value.array[index] == NULL) {
        setError("Array index out of bounds");
        return "";
    }
    
    if (v->value.array[index]->type != VAR_STRING) {
        setError("Array element is not a string");
        return "";
    }
    
    return v->value.array[index]->value.strValue;
}

// ─── DELETE VARIABLE ────────────────────────────────────────────
void pounce(const char* name) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(den[i].name, name) == 0) {
            if (den[i].type == VAR_STRING && den[i].value.strValue) {
                free(den[i].value.strValue);
            }
            if (den[i].type == VAR_ARRAY) {
                for (int j = 0; j < den[i].array_capacity; j++) {
                    if (den[i].value.array[j]) {
                        if (den[i].value.array[j]->type == VAR_STRING && den[i].value.array[j]->value.strValue) {
                            free(den[i].value.array[j]->value.strValue);
                        }
                        free(den[i].value.array[j]);
                    }
                }
                free(den[i].value.array);
            }
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

// ─── LIST VARIABLES ─────────────────────────────────────────────
void hunt() {
    printf("\n🐾 DEN CONTENTS:\n");
    for (int i = 0; i < varCount; i++) {
        if (den[i].type == VAR_NUMBER) {
            printf("   %-12s : %.5f (number)\n", den[i].name, den[i].value.numValue);
        } else if (den[i].type == VAR_STRING) {
            printf("   %-12s : \"%s\" (string)\n", den[i].name, den[i].value.strValue);
        } else if (den[i].type == VAR_ARRAY) {
            printf("   %-12s : [ ", den[i].name);
            int count = 0;
            for (int j = 0; j < den[i].array_capacity; j++) {
                if (den[i].value.array[j]) {
                    if (count > 0) printf(", ");
                    if (den[i].value.array[j]->type == VAR_NUMBER) {
                        printf("%.5f", den[i].value.array[j]->value.numValue);
                    } else if (den[i].value.array[j]->type == VAR_STRING) {
                        printf("\"%s\"", den[i].value.array[j]->value.strValue);
                    }
                    count++;
                }
            }
            printf(" ] (array, %d elements)\n", den[i].array_length);
        }
    }
    printf("\n");
}

// ─── FUNCTIONS ──────────────────────────────────────────────────
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
                if (den[j].type == VAR_STRING && den[j].value.strValue) {
                    free(den[j].value.strValue);
                }
                if (den[j].type == VAR_ARRAY) {
                    for (int k = 0; k < den[j].array_capacity; k++) {
                        if (den[j].value.array[k]) {
                            if (den[j].value.array[k]->type == VAR_STRING && den[j].value.array[k]->value.strValue) {
                                free(den[j].value.array[k]->value.strValue);
                            }
                            free(den[j].value.array[k]);
                        }
                    }
                    free(den[j].value.array);
                }
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

// ─── CLEANUP ────────────────────────────────────────────────────
void cleanup_all() {
    for (int i = 0; i < varCount; i++) {
        if (den[i].type == VAR_STRING && den[i].value.strValue) {
            free(den[i].value.strValue);
        }
        if (den[i].type == VAR_ARRAY) {
            for (int j = 0; j < den[i].array_capacity; j++) {
                if (den[i].value.array[j]) {
                    if (den[i].value.array[j]->type == VAR_STRING && den[i].value.array[j]->value.strValue) {
                        free(den[i].value.array[j]->value.strValue);
                    }
                    free(den[i].value.array[j]);
                }
            }
            free(den[i].value.array);
        }
    }
    
    for (int i = 0; i < funcCount; i++) {
        if (functions[i].body) free(functions[i].body);
    }
    
    clearError();
}