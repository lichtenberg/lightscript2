
#include "lightscript.h"
#include "lstokens.h"

#include "tokenstream.hpp"

LSToken::LSToken()
{
    type = YYEMPTY;
    fpval = 0;
    intval = 0;
    strval = "";
}

LSToken::LSToken(lstoktype_t tt)
{
    LSToken();
    type = tt;
}

LSToken::LSToken(lstoktype_t tt, char *str)
{
    LSToken();
    type = tt;
    strval = str;
}

LSToken::LSToken(int whole)
{
    LSToken();
    type = tWHOLE;
    intval = whole;
}

LSToken::LSToken(double d)
{
    LSToken();
    type = tFLOAT;
    fpval = d;
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
        printf("Syntax error!\n");
        // throw xxx;
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
