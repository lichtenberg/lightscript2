%{
#include "lstokens.h"

    static lstoken_t tok;
    static void printtoken(lstoken_t *tok);
    static inline void emit_kw(int kw) {
        tok.lst_type = kw; tok.lst_float = 0; tok.lst_whole = 0; tok.lst_str = NULL;
        printtoken(&tok);
    }
    static inline void emit_id(char *str) {
        tok.lst_type = tIDENT; tok.lst_float = 0; tok.lst_whole = 0; tok.lst_str = strdup(str);
        printtoken(&tok);
    }
    static inline void emit_string(char *str) {
        char *x;
        str++;
        if ((x = strchr(str,'"'))) *x = '\0';
        tok.lst_type = tSTRING; tok.lst_float = 0; tok.lst_whole = 0; tok.lst_str = strdup(str);
        printtoken(&tok);
    }
    static inline void emit_whole(char *str) {
        tok.lst_type = tWHOLE; tok.lst_float = 0; tok.lst_whole = atoi(str); tok.lst_str = NULL;
        printtoken(&tok);
    }
    static inline void emit_double(char *str) {
        tok.lst_type = tFLOAT; tok.lst_float = atof(str); tok.lst_whole = 0; tok.lst_str = NULL;
        printtoken(&tok);
    }
    static inline void emit_timefloat(char *str) {
        double minutes = 0;
        double seconds = 0;
        char *colon = strchr(str,':');
        if (colon) {
            *colon++ = '\0';
            if (*str != ':') minutes = atof(str);
            seconds = atof(colon);
        } else {
            seconds = atof(str);
        }
        seconds = minutes*60.0 + seconds;
        tok.lst_type = tFLOAT; tok.lst_float = atof(str); tok.lst_whole = 0; tok.lst_str = NULL;
        printtoken(&tok);
    }
%}

digit     [0-9]
letter    [A-Za-z]

%%

"music"         emit_kw(tMUSIC);
"from"          emit_kw(tFROM);
"to"            emit_kw(tTO);
"at"            emit_kw(tAT);
"do"            emit_kw(tDO);
"count"         emit_kw(tCOUNT);
"idle"          emit_kw(tIDLE);
"wait"          emit_kw(tWAIT);
"speed"         emit_kw(tSPEED);
"cascade"       emit_kw(tCASCADE);
"brightness"    emit_kw(tBRIGHTNESS);
"reverse"       emit_kw(tREVERSE);

{letter}({digit}|{letter}|_)*      emit_id(yytext);
{digit}+\.{digit}*                 emit_double(yytext);
{digit}+\:{digit}+\.{digit}+       emit_timefloat(yytext);
{digit}+                           emit_whole(yytext);
\".*\"                             emit_string(yytext);
\/\/.*$                            ;
\n                                 emit_kw(tNEWLINE);
\,                                 emit_kw(tCOMMA);

%%
int yywrap(void)
{
    return 1;
}

static char *toknames[] = {"tMUSIC","tFROM","tTO","tDO","tAT","tCOUNT","tIDLE","tWAIT","tSPEED","tCOMMA","tCASCADE",
                           "tBRIGHTNESS","tREVERSE",
    "tIDENT","tFLOAT","tWHOLE","tSTRING","tNEWLINE"};

static void printtoken(lstoken_t *tok)
{
    printf("%-10.10s ",toknames[tok->lst_type]);
    
    switch(tok->lst_type) {
        case tSTRING:
        case tIDENT:
             printf("'%s'",tok->lst_str);
             break;
        case tWHOLE:
             printf("%d",tok->lst_whole);
             break;
        case tFLOAT:
             printf("%6.3f",tok->lst_float);
             break;
    }

    printf("\n");
}

int main(int argc,char *argv[])
{
    yyin = fopen(argv[1],"r");
    yylex();
    fclose(yyin);
    return 0;
}



