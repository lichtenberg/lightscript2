#pragma once

#include "tokenstream.hpp"
#include "lightscript.h"


class LSParser {
public:
    LSParser();
    LSParser(LSTokenStream *stream, LSScript_t *script);
    ~LSParser();

private:
    LSTokenStream *tokenStream;
    LSScript_t *script;

public:
    int parse();
    
private:
    void parseTopLevel();
    LSCommand_t *parseScriptCmd();
    idlist_t *parseIDList();
    idlist_t *parseIDSingle();
    idlist_t *parseArgList();
    void parseOption(LSCommand_t *cmd);
    void parseOptionList(LSCommand_t *cmd);
    void parseMacroBody(idlist_t * &idl, cmdlist_t * &cmdl);
};
