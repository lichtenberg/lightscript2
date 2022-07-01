
#pragma once

typedef enum {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    tFLOAT = 258,                  /* tFLOAT  */
    tWHOLE = 259,                  /* tWHOLE  */
    tIDENT = 260,                  /* tIDENT  */
    tSTRING = 261,                 /* tSTRING  */
    tMUSIC = 262,                  /* tMUSIC  */
    tFROM = 263,                   /* tFROM  */
    tTO = 264,                     /* tTO  */
    tAT = 265,                     /* tAT  */
    tDO = 266,                     /* tDO  */
    tON = 267,                     /* tON  */
    tCOUNT = 268,                  /* tCOUNT  */
    tIDLE = 269,                   /* tIDLE  */
    tSPEED = 270,                  /* tSPEED  */
    tCASCADE = 271,                /* tCASCADE  */
    tDELAY = 272,                  /* tDELAY  */
    tBRIGHTNESS = 273,             /* tBRIGHTNESS  */
    tDEFINE = 274,                 /* tDEFINE  */
    tAS = 275,                     /* tAS  */
    tMACRO = 276,                  /* tMACRO  */
    tPALETTE = 277,                /* tPALETTE  */
    tREVERSE = 278,                /* tREVERSE  */
    tCOLOR = 279,                  /* tCOLOR  */
    tOPTION = 280                  /* tOPTION  */
} lstoktype_t;

#define CHARTOKEN(x) ((lstoktype_t) (x))


typedef struct lstoken_s {
    double f;
    int w;
    char *str;
} lstoken_t;

