/*  *********************************************************************
    *  LightScript - A script processor for LED animations
    *  
    *  Main Program                             File: lsmain.c
    *  
    *  Top-level functions for the lightscript parser.
    *  
    *  Author:  Mitch Lichtenberg
    ********************************************************************* */


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <dirent.h>
#include "lightscript.h"

extern "C" {
#include "lstokens.h"
    void yyparse(void);
    extern int yylineno;
    extern FILE *yyin;
    void yyerror(char *str,...);
    lstoken_t yylval;
    extern int yylex();
};

char *inpfilename = (char *) "input";
int debug = 0;
int script_error = 0;



static void parse_file(char *filename)
{
    yyin = fopen(filename,"r");
    int t;

    if (!yyin) {
        fprintf(stderr,"Could not open %s : %s\n",filename,strerror(errno));
        return ;
    }

    // Save file name for error messages.
    inpfilename = filename;

    while ((t = yylex())) {
        printf("[%d]: ",yylineno);
        printf("%d ",t);
        if (t == tIDENT) printf("%s ",yylval.str);
        printf("\n");
    }

    fclose(yyin);

}


int main(int argc,char *argv[])
{
    if (argc > 1) {
        parse_file(argv[1]);
    } else {
        printf("need to supply a script name\n");
    }
    
    return 0;
}

