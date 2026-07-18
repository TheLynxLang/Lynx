#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lynx.h"
#include "platform.h"

// ─── GLOBAL DEFINITIONS ──────────────────────────────────────
char* lynx_error = NULL;
LynxError lynx_error_state = {0};

// ─── CONSTANTS ────────────────────────────────────────────────
#define MAX_VARS 1000
#define MAX_FUNCS 200
#define MAX_RECURSION 100

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

// ─── RECURSION GUARD ────────────────────────────────────────────
static int recursionDepth = 0;

// ─── ERROR STATE (storage only) ─────────────────────────────────
char* getError() {
    if (lynx_error_state.message) {
        return strdup(lynx_error_state.message);
    }
    return strdup("OK");
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
        if (v->type == VAR_ARRAY) {
            for (int i = 0; i < v->array_capacity; i++) {
                if (v->value.array[i]) {
                    if (v->value.array[i]->type == VAR_STRING && v->value.array[i]->value.strValue) {
                        free(v->value.array[i]->value.strValue);
                        v->value.array[i]->value.strValue = NULL;
                    }
                    free(v->value.array[i]);
                    v->value.array[i] = NULL;
                }
            }
            free(v->value.array);
            v->value.array = NULL;
        }
        if (v->type == VAR_STRING && v->value.strValue) {
            free(v->value.strValue);
            v->value.strValue = NULL;
        }
        v->type = VAR_NUMBER;
        v->value.numValue = val;
        v->array_length = 0;
        v->array_capacity = 0;
        v->value.array = NULL;
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
        // Clean up existing data
        if (v->type == VAR_ARRAY) {
            for (int i = 0; i < v->array_capacity; i++) {
                if (v->value.array[i]) {
                    if (v->value.array[i]->type == VAR_STRING && v->value.array[i]->value.strValue) {
                        free(v->value.array[i]->value.strValue);
                        v->value.array[i]->value.strValue = NULL;
                    }
                    free(v->value.array[i]);
                    v->value.array[i] = NULL;
                }
            }
            free(v->value.array);
            v->value.array = NULL;
        }
        if (v->value.strValue) {
            free(v->value.strValue);
            v->value.strValue = NULL;
        }
        v->type = VAR_STRING;
        v->value.strValue = malloc(strlen(value) + 1);
        if (v->value.strValue) {
            strcpy(v->value.strValue, value);
        }
        v->array_length = 0;
        v->array_capacity = 0;
        v->value.array = NULL;
        return;
    }
    
    // Create new variable
    if (varCount < MAX_VARS) {
        strcpy(den[varCount].name, name);
        den[varCount].type = VAR_STRING;
        den[varCount].value.strValue = malloc(strlen(value) + 1);
        if (den[varCount].value.strValue) {
            strcpy(den[varCount].value.strValue, value);
        }
        den[varCount].value.array = NULL;
        den[varCount].array_length = 0;
        den[varCount].array_capacity = 0;
        varCount++;
        printf("🐾 DEBUG: Created string var '%s' = '%s' (varCount=%d)\n", name, value, varCount);
    } else {
        printf("🐾 ERROR: varCount (%d) >= MAX_VARS (%d)\n", varCount, MAX_VARS);
    }
}

double getVar(const char* name) {
    Variable* v = findVar(name);
    if (!v) {
        return 0;
    }
    if (v->type == VAR_NUMBER) return v->value.numValue;
    if (v->type == VAR_STRING) return 0;
    return 0;
}

char* getVarString(const char* name) {
    Variable* v = findVar(name);
    if (!v) {
        printf("🐾 DEBUG: getVarString - '%s' not found\n", name);
        return "";
    }
    if (v->type == VAR_STRING) {
        printf("🐾 DEBUG: getVarString - '%s' = '%s'\n", name, v->value.strValue);
        return v->value.strValue;
    }
    printf("🐾 DEBUG: getVarString - '%s' exists but type=%d\n", name, v->type);
    return "";
}

// ─── ARRAY FUNCTIONS ────────────────────────────────────────────
void setArrayElement(const char* name, int index, double value) {
    Variable* v = findVar(name);
    if (!v) {
        return;
    }
    
    if (v->type != VAR_ARRAY) {
        if (v->type == VAR_NUMBER && v->value.numValue == 0 && !v->value.strValue) {
            v->type = VAR_ARRAY;
            v->value.array = NULL;
            v->array_length = 0;
            v->array_capacity = 0;
        } else {
            return;
        }
    }
    
    if (index < 0) return;
    
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
    if (!v) return 0;
    
    if (v->type != VAR_ARRAY) return 0;
    
    if (index < 0 || index >= v->array_capacity || v->value.array[index] == NULL) {
        return 0;
    }
    
    return v->value.array[index]->value.numValue;
}

int getArrayLength(const char* name) {
    Variable* v = findVar(name);
    if (!v) return 0;
    
    if (v->type != VAR_ARRAY) return 0;
    
    return v->array_length;
}

void setArrayStringElement(const char* name, int index, const char* value) {
    Variable* v = findVar(name);
    if (!v) return;
    
    if (v->type != VAR_ARRAY) {
        if (v->type == VAR_NUMBER && v->value.numValue == 0 && !v->value.strValue) {
            v->type = VAR_ARRAY;
            v->value.array = NULL;
            v->array_length = 0;
            v->array_capacity = 0;
        } else {
            return;
        }
    }
    
    if (index < 0) return;
    
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
    if (v->value.array[index]->value.strValue) {
        free(v->value.array[index]->value.strValue);
        v->value.array[index]->value.strValue = NULL;
    }
    v->value.array[index]->value.strValue = malloc(strlen(value) + 1);
    if (v->value.array[index]->value.strValue) strcpy(v->value.array[index]->value.strValue, value);
}

char* getArrayStringElement(const char* name, int index) {
    Variable* v = findVar(name);
    if (!v) return "";
    
    if (v->type != VAR_ARRAY) return "";
    
    if (index < 0 || index >= v->array_capacity || v->value.array[index] == NULL) {
        return "";
    }
    
    if (v->value.array[index]->type != VAR_STRING) {
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
                den[i].value.strValue = NULL;
            }
            if (den[i].type == VAR_ARRAY) {
                for (int j = 0; j < den[i].array_capacity; j++) {
                    if (den[i].value.array[j]) {
                        if (den[i].value.array[j]->type == VAR_STRING && 
                            den[i].value.array[j]->value.strValue) {
                            free(den[i].value.array[j]->value.strValue);
                            den[i].value.array[j]->value.strValue = NULL;
                        }
                        free(den[i].value.array[j]);
                        den[i].value.array[j] = NULL;
                    }
                }
                free(den[i].value.array);
                den[i].value.array = NULL;
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
    if (recursionDepth >= MAX_RECURSION) {
        setError("Recursion depth exceeded", 0, 0);
        return 0;
    }
    recursionDepth++;
    
    for (int i = 0; i < funcCount; i++) {
        if (strcmp(functions[i].name, name) == 0) {
            printf("🐾 Called function: %s\n", name);
            
            Variable* savedDen = malloc(varCount * sizeof(Variable));
            if (!savedDen) {
                setError("Out of memory", 0, 0);
                recursionDepth--;
                return 0;
            }
            
            int savedVarCount = varCount;
            for (int j = 0; j < varCount; j++) {
                savedDen[j] = den[j];
            }
            
            initScanner(functions[i].body);
            while (peekToken().type != TOKEN_EOF) {
                parse_statement();
                if (lynx_error) break;
            }
            
            for (int j = 0; j < varCount; j++) {
                if (den[j].type == VAR_STRING && den[j].value.strValue) {
                    free(den[j].value.strValue);
                    den[j].value.strValue = NULL;
                }
                if (den[j].type == VAR_ARRAY) {
                    for (int k = 0; k < den[j].array_capacity; k++) {
                        if (den[j].value.array[k]) {
                            if (den[j].value.array[k]->type == VAR_STRING && 
                                den[j].value.array[k]->value.strValue) {
                                free(den[j].value.array[k]->value.strValue);
                                den[j].value.array[k]->value.strValue = NULL;
                            }
                            free(den[j].value.array[k]);
                            den[j].value.array[k] = NULL;
                        }
                    }
                    free(den[j].value.array);
                    den[j].value.array = NULL;
                }
            }
            for (int j = 0; j < savedVarCount; j++) {
                den[j] = savedDen[j];
            }
            varCount = savedVarCount;
            free(savedDen);
            
            recursionDepth--;
            return 1;
        }
    }
    
    printf("🐾 Function '%s' not found\n", name);
    recursionDepth--;
    return 0;
}

// ─── CLEANUP ────────────────────────────────────────────────────
void cleanup_all() {
    for (int i = 0; i < varCount; i++) {
        if (den[i].type == VAR_STRING && den[i].value.strValue) {
            free(den[i].value.strValue);
            den[i].value.strValue = NULL;
        }
        if (den[i].type == VAR_ARRAY) {
            for (int j = 0; j < den[i].array_capacity; j++) {
                if (den[i].value.array[j]) {
                    if (den[i].value.array[j]->type == VAR_STRING && 
                        den[i].value.array[j]->value.strValue) {
                        free(den[i].value.array[j]->value.strValue);
                        den[i].value.array[j]->value.strValue = NULL;
                    }
                    free(den[i].value.array[j]);
                    den[i].value.array[j] = NULL;
                }
            }
            free(den[i].value.array);
            den[i].value.array = NULL;
        }
    }
    
    for (int i = 0; i < funcCount; i++) {
        if (functions[i].body) {
            free(functions[i].body);
            functions[i].body = NULL;
        }
    }
    
    if (lynx_error_state.message) {
        free(lynx_error_state.message);
        lynx_error_state.message = NULL;
    }
    
    if (lynx_error) {
        free(lynx_error);
        lynx_error = NULL;
    }
}