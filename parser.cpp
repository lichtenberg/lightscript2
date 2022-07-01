

void LSParser::parseIDList(void)
{
    tokenStream->match(CHARTOKEN('['));

    while (tokenStream->current() != CHARTOKEN(']')) {
        tokenStream->match(tIDENT);
    }

    tokenStream->match(CHARTOKEN(']'));
}
