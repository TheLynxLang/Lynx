double parse_primary() {
    Token t = scanToken();
    if (t.type == TOKEN_NUMBER) return atof(t.start);
    
    // ─── LEN() ──────────────────────────────────────────────────
    if (t.type == TOKEN_LEN) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("Len expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token arg = scanToken();
        if (arg.type == TOKEN_IDENTIFIER) {
            char name[64];
            snprintf(name, arg.length + 1, "%s", arg.start);
            char* str = getVarString(name);
            double result = (double)strlen(str);
            setVar("__result", result);
            Token rparen = scanToken();
            if (rparen.type != TOKEN_RPAREN) {
                setErrorF("Len expects ')'", rparen.line, rparen.col);
                return 0;
            }
            return result;
        } else if (arg.type == TOKEN_STRING) {
            char str[4096];
            snprintf(str, arg.length - 1, "%s", arg.start + 1);
            double result = (double)strlen(str);
            setVar("__result", result);
            Token rparen = scanToken();
            if (rparen.type != TOKEN_RPAREN) {
                setErrorF("Len expects ')'", rparen.line, rparen.col);
                return 0;
            }
            return result;
        } else {
            setErrorF("Len expects a string or variable name", arg.line, arg.col);
            return 0;
        }
    }
    
    // ─── getenv() ──────────────────────────────────────────────
    if (t.type == TOKEN_GETENV) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("getenv expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token arg = scanToken();
        if (arg.type != TOKEN_STRING) {
            setErrorF("getenv expects a string", arg.line, arg.col);
            return 0;
        }
        char name[256];
        snprintf(name, arg.length - 1, "%s", arg.start + 1);
        const char* value = getenv(name);
        Token rparen = scanToken();
        if (rparen.type != TOKEN_RPAREN) {
            setErrorF("getenv expects ')'", rparen.line, rparen.col);
            return 0;
        }
        if (value) {
            setVarString("__result", value);
            setVar("__result", (double)strlen(value));
            return (double)strlen(value);
        } else {
            setVarString("__result", "");
            setVar("__result", 0);
            return 0;
        }
    }
    
    // ─── KittyCheckIfStringContains() ──────────────────────────
    if (t.type == TOKEN_STRING_CONTAINS) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("KittyCheckIfStringContains expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token hayTok = scanToken();
        Token comma = scanToken();
        Token needleTok = scanToken();
        if (hayTok.type != TOKEN_STRING && hayTok.type != TOKEN_IDENTIFIER) {
            setErrorF("KittyCheckIfStringContains expects string or variable", hayTok.line, hayTok.col);
            return 0;
        }
        if (needleTok.type != TOKEN_STRING && needleTok.type != TOKEN_IDENTIFIER) {
            setErrorF("KittyCheckIfStringContains expects string or variable", needleTok.line, needleTok.col);
            return 0;
        }
        char hay[4096], needle[4096];
        if (hayTok.type == TOKEN_STRING) {
            snprintf(hay, hayTok.length - 1, "%s", hayTok.start + 1);
        } else {
            char name[64];
            snprintf(name, hayTok.length + 1, "%s", hayTok.start);
            char* val = getVarString(name);
            snprintf(hay, sizeof(hay), "%s", val);
        }
        if (needleTok.type == TOKEN_STRING) {
            snprintf(needle, needleTok.length - 1, "%s", needleTok.start + 1);
        } else {
            char name[64];
            snprintf(name, needleTok.length + 1, "%s", needleTok.start);
            char* val = getVarString(name);
            snprintf(needle, sizeof(needle), "%s", val);
        }
        Token rparen = scanToken();
        if (rparen.type != TOKEN_RPAREN) {
            setErrorF("KittyCheckIfStringContains expects ')'", rparen.line, rparen.col);
            return 0;
        }
        int result = strstr(hay, needle) != NULL;
        setVar("__result", result ? 1.0 : 0.0);
        return result ? 1.0 : 0.0;
    }
    
    // ─── KittySplitString() ─────────────────────────────────────
    if (t.type == TOKEN_STRING_SPLIT) {
        Token lparen = scanToken();
        if (lparen.type != TOKEN_LPAREN) {
            setErrorF("KittySplitString expects '('", lparen.line, lparen.col);
            return 0;
        }
        Token strTok = scanToken();
        Token comma = scanToken();
        Token delimTok = scanToken();
        // ... handle similar to above ...
        // (I'll assume you can fill in the rest)
    }
    
    // ─── KittyReplaceString() ──────────────────────────────────
    if (t.type == TOKEN_STRING_REPLACE) {
        // ... similar
    }
    
    // ─── Trim() ─────────────────────────────────────────────────
    if (t.type == TOKEN_TRIM) {
        // ... similar
    }
    
    // ─── KittyFileExists() ──────────────────────────────────────
    if (t.type == TOKEN_KITTY_FILE_EXISTS) {
        // ... similar
    }
    
    // ─── IDENTIFIER (variable or user function) ─────────────────
    if (t.type == TOKEN_IDENTIFIER) {
        char name[64];
        snprintf(name, t.length + 1, "%s", t.start);
        if (peekToken().type == TOKEN_LPAREN) {
            scanToken(); 
            while (peekToken().type != TOKEN_RPAREN && peekToken().type != TOKEN_EOF) {
                // skip args for now
                parse_expression();
                if (peekToken().type == TOKEN_COMMA) scanToken();
            }
            if (peekToken().type == TOKEN_RPAREN) scanToken();
            return callFunction(name);
        }
        return getVar(name);
    }
    
    if (t.type == TOKEN_STRING) {
        char str[256];
        snprintf(str, t.length - 1, "%s", t.start + 1);
        printf("%s\n", str);
        return 0;
    }
    
    if (t.type == TOKEN_LPAREN) {
        double val = parse_expression();
        if (peekToken().type == TOKEN_RPAREN) {
            scanToken();
            return val;
        } else {
            Token next = peekToken();
            char* text = getTokenText(next);
            setErrorF("Expected ')' after expression, got '%s' (type: %s)", 
                      text, tokenTypeToString(next.type));
            return 0;
        }
    }
    
    if (t.type == TOKEN_NOT) {
        Token next = peekToken();
        if (next.type == TOKEN_EOF) {
            setErrorF("Expected expression after 'Not'");
            return 0;
        }
        return !parse_primary();
    }
    
    char* text = getTokenText(t);
    setErrorF("Expected number, identifier, string, or '(' got '%s' (type: %s)", 
              text, tokenTypeToString(t.type));
    return 0;
}