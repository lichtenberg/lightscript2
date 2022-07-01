
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

public:
    void add(LSToken& tok);
    void advance(void);
    void match(lstoktype_t tt);
    lstoktype_t current(void);
    int currentLine(void);
    bool get(LSToken& tok);

private:
    std::vector<LSToken> tokens;
    
};
