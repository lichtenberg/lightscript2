
#include <vector>
#include <string>
#include "lightscript.h"
#include "parser.hpp"
#include "symtab.hpp"



LSParser::LSParser()
{

}

LSParser::~LSParser()
{

}

LSParser::LSParser(LSTokenStream *stream, LSScript_t *scr)
{
    LSParser();
    tokenStream = stream;
    script = scr;
}


int LSParser::parse(void)
{
    int ret = 0;

    try {
        parseTopLevel();
    } catch (int e) {
        ret = e;
    }

    return ret;
}


idlist_t *LSParser::parseIDList(void)
{
    idlist_t *idlist;

    idlist = new idlist_t;

    tokenStream->match(CHARTOKEN('['));

    while (tokenStream->current() != CHARTOKEN(']')) {
        idlist->push_back(tokenStream->matchIdent());
        if (tokenStream->current() == CHARTOKEN(',')) {
            tokenStream->advance();
            continue;
        } else {
            // Drop down to either match our bracket or die.
            break;
        }
    }

    tokenStream->match(CHARTOKEN(']'));

    return idlist;
}

idlist_t *LSParser::parseIDSingle(void)
{
    idlist_t *idlist;

    idlist = new idlist_t;

    idlist->push_back(tokenStream->matchIdent());

    return idlist;
}


idlist_t *LSParser::parseArgList(void)
{
    idlist_t *idlist;

    idlist = new idlist_t;
    
    tokenStream->match(CHARTOKEN('('));

    while (tokenStream->current() != CHARTOKEN(')')) {
        idlist->push_back(tokenStream->matchIdent());
        if (tokenStream->current() == CHARTOKEN(',')) {
            tokenStream->advance();
            continue;
        } else {
            // Drop down to either match our bracket or die.
            break;
        }
    }

    tokenStream->match(CHARTOKEN(')'));
    return idlist;
}

void LSParser::parseMacroBody(idlist_t * &idl, cmdlist_t * &cmdl)
{
    cmdlist_t *cmdlist;
    idlist_t *idlist = NULL;
    
    // If the macro has an arglist, parse it.
    if (tokenStream->current() == CHARTOKEN('(')) {
        idl = parseArgList();
    }
    
    tokenStream->match(CHARTOKEN('{'));

    cmdlist = new cmdlist_t;

    while (tokenStream->current() != CHARTOKEN('}')) {
        cmdlist->push_back(parseScriptCmd());
    }
    tokenStream->match(CHARTOKEN('}'));

    cmdl = cmdlist;
    idl = idlist;
}

void LSParser::parseOption(LSCommand_t *cmd)
{
    lstoktype_t terminals[] = {
        tON,
        tCASCADE,
        tDO,
        tMACRO,
        tBRIGHTNESS,
        tDELAY,
        tSPEED,
        tCOUNT,
        tOPTION,
        tPALETTE,
        tCOLOR,
        tREVERSE,
        ENDOFLIST};

    lstoktype_t tt;

    // Check our terminals
    if (tokenStream->predict(terminals) == false) {
        tokenStream->error("Expected option name (%s) but found '%s'",tokenStream->setStr(terminals), tokenStream->tokenStr(tokenStream->current()));
    }

    // Ok, consume the token but remember its type
    tt = tokenStream->advance();

    // Act on the terminal.
    switch (tt) {
        case tON:
            if (tokenStream->current() == tIDENT) {
                // Just a single identifier
                cmd->lsc_strips = parseIDSingle();
            } else {
                // list of identifiers.
                cmd->lsc_strips = parseIDList();
            }
            break;
        case tCASCADE:
            cmd->lsc_type = LSC_CASCADE;
            cmd->lsc_animation = tokenStream->matchIdent();
            break;
        case tDO:
            cmd->lsc_type = LSC_DO;
            cmd->lsc_animation = tokenStream->matchIdent();
            break;
        case tMACRO:
            cmd->lsc_type = LSC_MACRO;
            cmd->lsc_macro = tokenStream->matchIdent();
            if (tokenStream->current() == CHARTOKEN('(')) {
                // Parse arguments here
                cmd->lsc_macroArgs = parseArgList();
            }
            break;
        case tBRIGHTNESS:
            cmd->opt_brightness = tokenStream->matchInt();
            break;
        case tDELAY:
            cmd->opt_delay = tokenStream->matchFloat();
            break;
        case tSPEED:
            cmd->opt_speed = tokenStream->matchInt();
            break;
        case tCOUNT:
            cmd->opt_count = tokenStream->matchInt();
            break;
        case tOPTION:
            cmd->opt_option = tokenStream->matchInt();
            break;
        case tPALETTE:
            if (tokenStream->current() == tWHOLE) {
                cmd->opt_color = tokenStream->matchInt();
                cmd->opt_colorIdent = "";
            } else {
                cmd->opt_colorIdent = tokenStream->matchIdent();
            }
            break;
        case tCOLOR:
            if (tokenStream->current() == tWHOLE) {
                cmd->opt_color = tokenStream->matchInt() | COLORFLG;
                cmd->opt_colorIdent = "";
            } else {
                cmd->opt_colorIdent = tokenStream->matchIdent();
            }
            break;
        case tREVERSE:
            cmd->opt_reverse = true;
            break;
        default:
            break;
    }

}

void LSParser::parseOptionList(LSCommand_t *cmd)
{
    while (tokenStream->current() != CHARTOKEN(';')) {
        parseOption(cmd);
    }
}


LSCommand_t *LSParser::parseScriptCmd()
{
    lstoktype_t terminals[] = {
        tAT,
        tFROM,
        tMUSIC,
        tIDLE,
        tDEFINE,
        tDEFMACRO,
        tDEFANIM,
        tDEFSTRIP,
        tDEFCOLOR,
        tDEFPALETTE,
        ENDOFLIST};
    lstoktype_t tt;
    bool save = false;

    std::string id;
    int v;

    LSCommand_t *cmd;

    // Allocate a command.
    cmd = new LSCommand_t;

    memset(cmd,0,sizeof(LSCommand_t));

    cmd->opt_color = 0x40;              // Default to RGB palette unless overridden

    // Remmeber about where the script command was.
    cmd->lsc_line = tokenStream->currentLine();

    // Check our terminals
    if (tokenStream->predict(terminals) == false) {
        tokenStream->error("Expected script command");
    }

    // Ok, consume the token but remember its type
    tt = tokenStream->advance();

    // Act on the terminal.
    switch (tt) {
        case tAT:
            cmd->lsc_type = LSC_DO;
            cmd->lsc_from = tokenStream->matchFloat();
            cmd->lsc_to = cmd->lsc_from;
            parseOptionList(cmd);
            cmd->lsc_count = 1;
            save = true;
            break;
        case tFROM:
            cmd->lsc_type = LSC_DO;
            cmd->lsc_from = tokenStream->matchFloat();
            tokenStream->match(tTO);
            cmd->lsc_to = tokenStream->matchFloat();
            parseOptionList(cmd);
            cmd->lsc_count = cmd->opt_count;
            save = true;
            break;
        case tMUSIC:
            script->lss_music = tokenStream->matchString();
            break;
        case tIDLE:
            script->lss_idleanimation = tokenStream->matchIdent();
            parseOptionList(cmd);
            script->lss_idlestrips = cmd->lsc_strips;
            cmd->lsc_strips = NULL;
            break;
        case tDEFSTRIP:
            id = tokenStream->matchIdent();
            tokenStream->match(tAS);

            switch (tokenStream->current()) {
                case CHARTOKEN('['):
                    // strip list
                    script->stripListTable->addStripList(id,parseIDList());
                    break;
                default:
                    // Single strip number
                    v = tokenStream->matchInt();
                    script->stripTable->addSym(id,v);
            }
            break;
        case tDEFINE:
            id = tokenStream->matchIdent();
            tokenStream->match(tAS);
            v = tokenStream->matchInt();
            script->symbolTable->addSym(id,v);
            break;
        case tDEFANIM:
            id = tokenStream->matchIdent();
            tokenStream->match(tAS);
            v = tokenStream->matchInt();
            script->animTable->addSym(id,v);
            break;
        case tDEFCOLOR:
        case tDEFPALETTE:
            id = tokenStream->matchIdent();
            tokenStream->match(tAS);
            v = tokenStream->matchInt();
            if (tt == tDEFCOLOR) v |= COLORFLG;
            script->colorTable->addSym(id,v);
            break;
        case tDEFMACRO:
            id = tokenStream->matchIdent();
            idlist_t *idlist;
            cmdlist_t *cmdlist;
            parseMacroBody(idlist,cmdlist);
            script->macroTable->addMacro(id, idlist, cmdlist);
            break;

        default:
            tokenStream->error("Should not happen");
            break;
    }

    // Commands end in semicolons.
    tokenStream->match(CHARTOKEN(';'));

    if (!save) {
        delete cmd;
        cmd = NULL;
    }

    return cmd;
}


void LSParser::parseTopLevel()
{
    LSCommand_t *cmd;
    
    while (tokenStream->current() != YYEOF) {
        cmd = parseScriptCmd();
        if (cmd) {
            script->lss_commands.push_back(cmd);
        }
    }
}
