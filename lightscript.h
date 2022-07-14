/*  *********************************************************************
    *  LightScript - A script processor for LED animations
    *  
    *  Data structures                          File: lightscript.h
    *  
    *  Our common include file, structures, constants, enums etc.
    *  
    *  Author:  Mitch Lichtenberg
    ********************************************************************* */


#pragma once

/*
 * lightscript stuff
 */

typedef enum {
    LSC_UNKNOWN = 0,
    LSC_CASCADE = 1,
    LSC_DO = 2,
    LSC_MACRO = 3,
    LSC_COMMENT = 4
} lsctype_t;

#define COLORFLG  0x1000000
typedef std::vector<std::string> idlist_t;
typedef std::vector<double> vallist_t;

typedef struct LSCommand_s {
    // Command type:
    lsctype_t lsc_type;

    // Where the command is in the file
    int lsc_line;

    // Time index (could be relative) of where the command starts.
    double lsc_from;
    double lsc_to;
    int lsc_count;

    // Macro name (if it's a macro call)
    std::string lsc_macro;
    vallist_t *lsc_macroArgs;

    std::string lsc_comment;
    // Animation
    std::string lsc_animation;
    // Strip list
    idlist_t *lsc_strips;


    // Options
    double opt_delay;
    int opt_speed;
    int opt_count;
    int opt_option;
//    int opt_palette;
//    std::string opt_paletteIdent;
    int opt_brightness;
    int opt_color;
    std::string opt_colorIdent;
    bool opt_reverse;
    
} LSCommand_t;
typedef std::vector<LSCommand_t *> cmdlist_t;
typedef std::vector<int> stripvec_t;


typedef struct LSMacro_s {
    std::string name;
    idlist_t *args;
    cmdlist_t *commands;
} LSMacro_t;

class LSSymTab;
class LSMacroTab;
class LSStripListTab;


typedef struct LSScript_s {
    // Global stuff about the script
    std::string lss_idleanimation;
    idlist_t *lss_idlestrips;
    std::string lss_music;

    // Set of script commands
    cmdlist_t lss_commands;

    // Start/Stop cues
    double lss_startcue;
    double lss_endcue;

    // Symbol tables
    LSSymTab *symbolTable;
    LSSymTab *stripTable;
    LSSymTab *animTable;
    LSSymTab *colorTable;
    LSStripListTab *stripListTable;
    LSMacroTab *macroTable;
} LSScript_t;
