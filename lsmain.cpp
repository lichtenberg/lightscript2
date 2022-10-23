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
#include <signal.h>

#include "tokenstream.hpp"
#include "lightscript.h"
#include "symtab.hpp"
#include "parser.hpp"
#include "schedule.hpp"

#include "playback.h"

#define VERSION "2.2"

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

LSTokenStream tokenStream;
static LSScript_t *script = NULL;
static LSSchedule *schedule;

static char *findpicolight(void)
{
    DIR *dir;
    struct dirent *dp;
    char *devicenames[10];
    int picked;
    int devcnt = 0;
    int i;
    char devname[256];

    dir = opendir("/dev");              // We're going to look through /dev for files
    while ((dp = readdir(dir)) != NULL) {
        if ((devcnt < 10) && (strstr(dp->d_name,"cu.usbmodem"))) {
            devicenames[devcnt++] = strdup(dp->d_name);
        }
    }
    closedir(dir);


    if (devcnt == 0) {
        printf("No Picolight boards appear to be connected, is the power on?\n");
    } else if (devcnt == 1) {
        picked = 0;
        printf("[Found only one device, trying /dev/%s]\n",devicenames[0]);
    } else {
        printf("More than one possible device found.  Please choose one.\n");
        for (i = 0; i < devcnt; i++) {
            printf("  %d: /dev/%s\n",i,devicenames[i]);
        }
        for (;;) {
            printf("Choose the one that is connected to your Picolight:  ");
            fgets(devname,sizeof(devname),stdin);
            picked = atoi(devname);
            if ((picked < 0) || (picked >= devcnt)) continue;
            break;
        }
    }


    if (devcnt > 0) {
        sprintf(devname,"/dev/%s",devicenames[picked]);
    }

    for (i = 0; i < devcnt; i++) {
        free(devicenames[i]);
    }

    if (devcnt == 0) {
        return NULL;
    }
    return strdup(devname);

}

static bool tokenize_file(char *filename)
{
    yyin = fopen(filename,"r");
    lstoktype_t t;

    if (!yyin) {
        fprintf(stderr,"Could not open %s : %s\n",filename,strerror(errno));
        return false;
    }

    // Save file name for error messages.
    inpfilename = filename;
    yylineno = 1;

    // Call the lexer and read all the tokens into the token stream.
    while ((t = (lstoktype_t) yylex())) {
        LSToken tok = LSToken(t, yylineno, &yylval);
        tokenStream.add(tok);
    }

    fclose(yyin);

    return true;
}

static void script_showpstrips(LSScript_t *script)
{
    int idx;
    char chName;
    int chNum;
    
    printf("Physical strip table:\n");
    for (idx = 0; idx < MAXPSTRIPS; idx++) {
        PStrip_t *strip = &script->physicalStrips[idx];
        if (strip->name == "") continue;

        chName = "AB"[(PSTRIP_CHAN(strip->info) >> 3) & 1];
        chNum = (PSTRIP_CHAN(strip->info) & 0x7) + 1;
        printf("  %-20.20s  channel=%c%d type=%u count=%u\n",
               strip->name.c_str(),
               chName,chNum,
               PSTRIP_TYPE(strip->info),
               PSTRIP_COUNT(strip->info));
    }
}

static const char *stripChars = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static void script_showvstrips(LSScript_t *script)
{
    int idx;
    int ss;        
    VStrip_t *vstrip;

    printf("\nVirtual strip table (%u entries):\n",script->virtualStripCount);

    for (idx = 0; idx < script->virtualStripCount; idx++) {
        vstrip = &script->virtualStrips[idx];
        printf("  %-20.20s '%c'  ",vstrip->name.c_str(), stripChars[idx]);
        for (ss = 0; ss < vstrip->substripCount; ss++) {
            uint32_t chan = SUBSTRIP_CHAN(vstrip->substrips[ss]);
            uint32_t start = SUBSTRIP_START(vstrip->substrips[ss]);
            uint32_t count = SUBSTRIP_COUNT(vstrip->substrips[ss]);
            uint32_t reverse = SUBSTRIP_DIRECTION(vstrip->substrips[ss]);
            printf("[%c%u (%u,%u) %c]  ",
                   "AB"[(chan >> 3) & 1], chan & 7,
                   start, count,
                   reverse ? 'R' : 'F');
        }
        printf("\n");
    }
}


static void script_stats(LSScript_t *script)
{
    const char *music = script->lss_music.c_str() ? script->lss_music.c_str() : "not_set";
    const char *idleanim = script->lss_idleanimation.c_str() ? script->lss_idleanimation.c_str() : "not_set";
    
    printf("Number of commands:  %ld\n",script->lss_commands.size());
    printf("Music file:          %s\n",music);
    printf("Idle animation:      %s\n",idleanim);
    printf("Symbol table size:   %d\n",script->symbolTable->size());
    printf("Color table size:    %d\n",script->colorTable->size());
    printf("Anim table size:     %d\n",script->animTable->size());
    printf("StripList tab size:  %d\n",script->stripListTable->size());
    printf("Macro list size:     %d\n",script->macroTable->size());
    printf("\n");

    script_showpstrips(script);
    script_showvstrips(script);

    printf("\n\n\n");
}


static LSScript_t *do_parse(void)
{
    LSScript_t *script = new LSScript_t;

    memset(script, 0, sizeof(LSScript_t));
    
    LSParser *parser = new LSParser(&tokenStream, script);

    script->symbolTable = new LSSymTab("symbol");
    script->animTable = new LSSymTab("animations");
    script->colorTable = new LSSymTab("colors");
    script->stripListTable = new LSStripListTab;
    script->macroTable = new LSMacroTab;

    // Initialize the index values from the pstrip and vstrip tables,
    // not sure if we really need it.
    for (unsigned int idx = 0; idx < MAXPSTRIPS; idx++) {
            script->physicalStrips[idx].idx = idx;
        }
    for (unsigned int idx = 0; idx < MAXVSTRIPS; idx++) {
            script->virtualStrips[idx].idx = idx;
        }

    // Go parse the file.
    if (parser->parse() == 0) {
        printf("File parsed successfully\n");
        script_stats(script);
        return script;
    }

    return NULL;
}

static bool read_and_parse(char *configfilename, char *scriptfilename)
{
    if (configfilename) {
        if (!tokenize_file(configfilename)) {
            return false;
        }
    }

    if (scriptfilename) {
        if (!tokenize_file(scriptfilename)) {
            return false;
        }
    }

    // OK, both files were read, go parse them.
    script = do_parse();

    if (script == NULL) {
        printf("Errors found while parsing the script and config files\n");
        return false;
    }

    // Generate our schedule if we get this far.
    schedule = new LSSchedule(script);
    if (schedule->generate() == false) {
        printf("Errors found while generating the schedule\n");
        return false;
    }

    return true;
}



static double _parsetime(char *str) {
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
    return seconds;
}

static int parse_range(char *str, double *start, double *end)
{
    char *x;
    
    *end = 0;
    *start = 0;

    // See if it's just the start, or the start and end
    if ((x = strchr(str,'-'))) {
        *x++ = 0;
        *start = _parsetime(str);
        *end = _parsetime(x);
    } else {
        *start = _parsetime(str);
    }

    return  1;
    
}

static void usage(void)
{
    fprintf(stderr,"Usage: lightscript [-c configfile] [-v] [-p device] command script-file\n\n");
    fprintf(stderr,"    -c configfile       Specifies the name of a configuration file\n");
    fprintf(stderr,"    -p device           Specifies the name of the Arduino device\n");
    fprintf(stderr,"    -s time             Starting time for playback\n");
    fprintf(stderr,"    -v                  Print diagnostic output\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"  Commands:\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"      check     Check but do not play the script\n");
    fprintf(stderr,"      play      Check, then play the script\n");
    fprintf(stderr,"      mplay     Check, then play the script with background music\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"    script-file         Name of script file to process\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"    If not specified, and 'lightscript.cfg' exists, this file will be processed\n");
    fprintf(stderr,"    as a configuration file if '-c' is not specified\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"Example usage\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"    ./lightscript play test1.ls          Play test1.ls on the LED system\n");
    fprintf(stderr,"    ./lightscript check test1.ls         Check for syntax errors but do not play\n");
    fprintf(stderr,"\n");
    exit(1);

}

#define CMD_PLAY        1
#define CMD_CHECK       2
#define CMD_MPLAY       3

void inthandler(int x)
{
    play_interrupt();
}


int main(int argc,char *argv[])
{

    char *configfilename = (char *) "lightscript.cfg";
    char *scriptfilename = NULL;
    char *playdevice = NULL;
    char *command;
    char *picolight = NULL;
    int cmdnum = 0;
    double start_cue = 0;
    double end_cue = 0;
    int ch;

    printf("Lightscript version %s\n\n",VERSION);
    
    while ((ch = getopt(argc,argv,"c:vp:s:")) != -1) {
        switch (ch) {
            case 'c':
                configfilename = optarg;
                break;
            case 'v':
                debug = 1;
                break;
            case 'p':
                playdevice = optarg;
                break;
            case 's':
                parse_range(optarg,&start_cue,&end_cue);
                break;
        }
    }

    // Skip over options
    argc -= optind;
    argv += optind;

    if (argc < 2) {
        usage();
        exit(1);
    }

    command = argv[0];
    scriptfilename = argv[1];
    
    if (strcmp(command,"play") == 0) cmdnum = CMD_PLAY;
    else if (strcmp(command,"check") == 0) cmdnum = CMD_CHECK;
    else if (strcmp(command,"mplay") == 0) cmdnum = CMD_MPLAY;

    if (cmdnum == 0) {
        fprintf(stderr,"You must specify a command, 'play', 'mplay', or 'check' before the file name\n");
        fprintf(stderr,"\n");
        usage();
    }

    // Read and parse the file, bail if we can't do it.
    if (read_and_parse(configfilename, scriptfilename) == false) {
        exit(1);
    }

    // We have a valid script, currently held in the global 'script'
    script->lss_startcue = start_cue;
    script->lss_endcue = end_cue;

    if ((cmdnum == CMD_PLAY) || (cmdnum == CMD_MPLAY)) {
        // See if we were passed a device to play.
        if (playdevice != NULL) {
            picolight = playdevice;
        } else {
            picolight = findpicolight();
        }
        if (!picolight) {
            exit(1);
        }
    }

    // Print the schedule if we get this far.
    schedule->printSched();

    struct sigaction sigint_action;
    memset(&sigint_action,0,sizeof(sigint_action));
    sigint_action.sa_handler = inthandler;
    sigint_action.sa_flags = 0;
    sigfillset(&sigint_action.sa_mask);
    sigaction(SIGINT, &sigint_action, NULL);

    if ((cmdnum == CMD_MPLAY) || (cmdnum == CMD_PLAY)) {
        play_opendevice(picolight);
        play_init(script, schedule);
        play_initdevice(script);
    }

    switch (cmdnum) {
        case CMD_PLAY:
            play_script(0);
            break;
        case CMD_MPLAY:
            play_script(1);
            break;
        default:
            break;
    }
    

    return 0;
}

