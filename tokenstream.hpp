
#pragma once

#include <string>
#include <vector>
#include "lstokens.h"


/*  *********************************************************************
    *  Token class : manages the stuff that comes out of Lex.
    ********************************************************************* */

class LSToken {

public:
    LSToken();
    LSToken(lstoktype_t tt, int lineno, lstoken_t *tok);
    ~LSToken();


private:
    lstoktype_t type;
    double fpval;
    int intval;
    std::string strval;
    int lineno;

public:
    inline lstoktype_t getType(void) { return type; }
    inline double getFloat(void) { return fpval; }
    inline std::string getString(void) { return strval; }
    inline int getInt(void) { return intval; }
    inline int getLine(void) { return lineno; }

};

/*  *********************************************************************
    *  Token Stream class : handles the set of tokens in our input file
    ********************************************************************* */

class LSTokenStream {
public:
    LSTokenStream();
    ~LSTokenStream();

private:
    std::string errorStr;

public:
    void add(LSToken& tok);
    lstoktype_t advance(void);
    void match(lstoktype_t tt);
    std::string matchIdent();
    std::string matchString();
    int matchInt();
    double matchFloat();
    bool predict(lstoktype_t set[]);
    lstoktype_t current(void);
    int currentLine(void);
    void error(const char *, ...);
    bool get(LSToken& tok);
    const char *tokenStr(lstoktype_t tt);
    const char *setStr(lstoktype_t set[]);

private:
    std::vector<LSToken> tokens;
    
};
