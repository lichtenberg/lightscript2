
#include "lightscript.h"
#include "lstokens.h"

#include "tokenstream.hpp"

LSToken::LSToken()
{
    type = YYEMPTY;
    lineno = 0;
    fpval = 0;
    intval = 0;
    strval = "";
}

LSToken::LSToken(lstoktype_t tt, int lno, lstoken_t *tok)
{
    LSToken();

    type = tt;
    lineno = lno;

    switch (tt) {
        case tFLOAT:
            fpval = tok->f;
            break;
        case tWHOLE:
            intval = tok->w;
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

void LSTokenStream::advance(void)
{
    if (!tokens.empty()) {
        tokens.erase(tokens.begin());
    }
}

void LSTokenStream::match(lstoktype_t tt)
{
    if (current() == tt) {
        advance();
    } else {
        printf("[%d] Syntax error!\n", currentLine());
        // XXX put real error codes/etc here.
        throw -1;
    }
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
