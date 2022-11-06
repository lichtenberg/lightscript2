#include <stdarg.h>

#include <assert.h>

#include "lstokens.h"

#include "tokenstream.hpp"
#include "lightscript.h"

LSToken::LSToken()
{
    type = YYEMPTY;
    lineno = 0;
    fpval = 0;
    strval = "";
}

LSToken::LSToken(lstoktype_t tt, char *fname, int lno, lstoken_t *tok)
{
    LSToken();

    type = tt;
    filename = fname;
    lineno = lno;

    switch (tt) {
        case tFLOAT:
            fpval = tok->f;
            break;
        case tIDENT:
        case tSTRING:
            if (tok->str) strval = tok->str;
            break;
        default:
            break;
    }
                       
}


LSToken::~LSToken()
{

}

/*  *********************************************************************
    *  List of valid tokens
    ********************************************************************* */

typedef struct tokenmap_s {
    lstoktype_t tt;
    const char *str;
} tokenmap_t;

static tokenmap_t tokenNames[] = {
    {YYEOF, "end-of-file"},
    {LBRACKET,"["},
    {RBRACKET,"]"},
    {LBRACE,"{"},
    {RBRACE,"}"},
    {LPAREN,"("},
    {RPAREN,")"},
    {COMMA,","},
    {SEMICOLON,";"},
    {tFLOAT,"floating-point-number"},
    {tIDENT,"identifier"},
    {tSTRING,"string"},
    {tMUSIC,"music"},
    {tFROM,"from"},
    {tTO,"to"},
    {tAT,"at"},
    {tDO,"do"},
    {tON,"on"},
    {tCOUNT,"count"},
    {tIDLE,"idle"},
    {tSPEED,"speed"},
    {tCASCADE,"cascade"},
    {tBRIGHTNESS,"brightness"},
    {tAS,"as"},
    {tMACRO,"macro"},
    {tPALETTE,"palette"},
    {tREVERSE,"reverse"},
    {tCOLOR,"color"},
    {tOPTION,"option"},
    {tDEFMACRO,"defmacro"},
    {tDEFSTRIP,"defstrip"},
    {tDEFANIM,"defanim"},
    {tCOMMENT,"comment"},
    {tPHYSICAL,"physical"},
    {tVIRTUAL,"virtual"},
    {tVSTRIP,"vstrip"},
    {tPSTRIP,"pstrip"},
    {tCHANNEL,"channel"},
    {tTYPE,"type"},
    {tSTART,"start"},
    {tSUBSTRIP,"substrip"},
    {YYEMPTY, NULL}
};


static const char *tokenName(lstoktype_t tt)
{
    tokenmap_t *tm;

    tm = tokenNames;
    while (tm->str) {
        if (tm->tt == tt) {
            return tm->str;
        }
        tm++;
    }

    return "unknown";
}

/*  *********************************************************************
    *  Token Streams
    ********************************************************************* */


LSTokenStream::LSTokenStream()
{

}

LSTokenStream::~LSTokenStream()
{

}


void LSTokenStream::add(LSToken& tok)
{
    tokens.push_back(tok);
}

lstoktype_t LSTokenStream::advance(void)
{
    lstoktype_t tt = YYEOF;

    if (!tokens.empty()) {
        tt = current();
        tokens.erase(tokens.begin());
    }
    return tt;
}

void LSTokenStream::error(const char *str, ...)
{
    va_list ap;

    printf("[%s:Line %d] ",currentFile(),currentLine());
    va_start(ap,str);
    vprintf(str,ap);
    va_end(ap);
    printf("\n");
    throw -1;
}

const char *LSTokenStream::tokenStr(lstoktype_t tt)
{
    return tokenName(tt);
}

const char *LSTokenStream::setStr(lstoktype_t set[])
{
    int i = 0;
    int cnt = 0;

    // See how many are in the set
    while (set[i] != ENDOFLIST) i++;
    cnt = i;

    // Build our string
    errorStr = "";
    if (cnt == 0) return errorStr.c_str();
    
    for (i = 0; i < cnt-1; i++) {
        errorStr += tokenName(set[i]);
        errorStr += ", ";
    }
    errorStr += tokenName(set[i]);

    return errorStr.c_str();
}


void LSTokenStream::match(lstoktype_t tt)
{
    if (current() == tt) {
        advance();
    } else {
        error("Expected '%s' but found '%s'",tokenName(tt),tokenName(current()));
    }
}

//
// Conveneince matchers for things whose values we want to know
//
std::string LSTokenStream::matchIdent(void)
{
    std::string ret;

    if (current() == tIDENT) {
        ret = tokens.front().getString();
        advance();
        return ret;
    } else {
        error("Expected identifier, but found '%s'",tokenName(current()));
    }

    ret = "";
    return ret;
}

std::string LSTokenStream::matchString(void)
{
    std::string ret;

    if (current() == tSTRING) {
        ret = tokens.front().getString();
        advance();
        return ret;
    } else {
        error("Expected string, but found '%s'",tokenName(current()));
    }

    ret = "";
    return ret;
}

int LSTokenStream::matchInt(void)
{
    int ret;
    
    if (current() == tFLOAT) {
        ret = (int) tokens.front().getFloat();
        advance();
        return ret;
    } else {
        error("Expected number but found '%s'",tokenName(current()));
    }
    return 0;
}

double LSTokenStream::matchFloat(void)
{
    double ret;
    
    if (current() == tFLOAT) {
        ret = tokens.front().getFloat();
        advance();
        return ret;
    } else {
        error("Expected floating-point-value but found '%s'",tokenName(current()));
    }
    return 0.0;
}

bool LSTokenStream::predict(lstoktype_t set[])
{
    lstoktype_t cur = current();
    int i = 0;

    while (set[i] != ENDOFLIST) {
        if (cur == set[i]) {
            return true;
        }
        i++;
    }
    return false;
}

bool LSTokenStream::get(LSToken& tok)
{
    // If the stream is empty just return EOF
    if (tokens.empty()) {
        return false;
    }

    // Pick off the first element and return its type.
    tok = tokens.front();
    return true;
}


lstoktype_t LSTokenStream::current(void)
{
    // If the stream is empty just return EOF
    if (tokens.empty()) {
        return YYEOF;
    }

    // Pick off the first element and return its type.
    return tokens.front().getType();
}

int LSTokenStream::currentLine(void)
{
    // If the stream is empty just return EOF
    if (tokens.empty()) {
        return 0;
    }

    // Pick off the first element and return its type.
    return tokens.front().getLine();
}

char *LSTokenStream::currentFile(void)
{
    // If the stream is empty just return EOF
    if (tokens.empty()) {
        return 0;
    }

    // Pick off the first element and return its type.
    return tokens.front().getFileName();
}
