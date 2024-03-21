#include "common.h"
#include "scanner.h"

void Scanner::initScanner(const char* src)
{
    start = src;
    current = src;
    line = 1;
}

static bool isDigit(char c) 
{    
    return c >= '0' && c <= '9';
}

static bool isAlpha(char c) 
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static bool isHex(char c) 
{
    if(isDigit(c)) return true;

    return (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

Token Scanner::scanToken() 
{
    skipWhitespace();
    start = current;

    if (isAtEnd()) return makeToken(TokenType::EOF_TOKEN);

    char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c) /*|| (c == '-' && isDigit(*current)) */) return number();
    switch (c) 
    {
        case '$': return makeToken(TokenType::DOLLAR);
        case '@': return makeToken(TokenType::AT);
        case '(': return makeToken(TokenType::LEFT_PAREN);
        case ')': return makeToken(TokenType::RIGHT_PAREN);
        case '{': return makeToken(TokenType::LEFT_BRACE);
        case '}': return makeToken(TokenType::RIGHT_BRACE);
        case '[': return makeToken(TokenType::LEFT_BRACKET);
        case ']': return makeToken(TokenType::RIGHT_BRACKET);
        case ';': return makeToken(TokenType::SEMICOLON);
        case ',': return makeToken(TokenType::COMMA);
        case '.': 
        {
            if( match('.') )
            {
                if(!match('.'))
                {
                    return makeToken(TokenType::ERROR);
                }
                return makeToken(TokenType::ELIPSIS);
            }
            return makeToken(TokenType::DOT);
        }
        case '~': return makeToken(TokenType::TILDE);
        case '%': return makeToken(TokenType::MODULO);
        case ':': return makeToken(TokenType::COLON);
        case '-': return makeToken(
            match('-') ? TokenType::MINUS_MINUS :
            match('=') ? TokenType::MINUS_EQUAL : 
            TokenType::MINUS
        );
        case '+': return makeToken(
            match('+') ? TokenType::PLUS_PLUS : 
            match('=') ? TokenType::PLUS_EQUAL : 
            TokenType::PLUS
        );
        case '/': return makeToken(TokenType::SLASH);
        case '*': return makeToken(TokenType::STAR);
        case '&': return makeToken(match('&') ? TokenType::AND : TokenType::BIN_AND);
        case '|': return makeToken(match('|') ? TokenType::OR : TokenType::BIN_OR);

        case '!':
            return makeToken(
                match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
        case '=':
            return makeToken(
                match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
        case '<':
            return makeToken(
                match('=') ? TokenType::LESS_EQUAL :
                match('<') ? TokenType::SHIFT_LEFT : TokenType::LESS);
        case '>':
            return makeToken(
                match('=') ? TokenType::GREATER_EQUAL :
                match('>') ? TokenType::SHIFT_RIGHT : TokenType::GREATER);    
        case '"': return string();            
    }


    return errorToken("Unexpected character.");
}

Token Scanner::identifier() 
{
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}


Token Scanner::hexNum() 
{
    advance();

    while (isHex(peek())) advance();

    return makeToken(TokenType::INTEGER);
}

Token Scanner::number() 
{
    if(peek()=='x' || peek() =='X')
    {
        return hexNum();
    }
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) 
    {
        // Consume the ".".
        advance();

        while (isDigit(peek())) advance();
        
        return makeToken(TokenType::NUMBER);
    }

    return makeToken(TokenType::INTEGER);
}

Token Scanner::string() 
{
    while (peek() != '"' && !isAtEnd()) 
    {
        if (peek() == '\n') 
        {
            line++;  
        } 
        if(peek() == '\\' && peekNext() == '\\')
        {
            advance();
            advance();
        }
        else if(peek() == '\\' && peekNext() == '"')
        {
            advance();
            advance();
        }
        else
        {
            advance();
        }
    }

    if (isAtEnd()) return errorToken("Unterminated string.");

    // The closing quote.
    advance();
    return makeToken(TokenType::STRING);
}

TokenType Scanner::checkKeyword(int begin, int length, const char* rest, TokenType type) 
{
    if (current - start == begin + length &&
        memcmp( (void*)(start + begin), rest, length) == 0) 
    {
        return type;
    }

    return TokenType::IDENTIFIER;
}

TokenType Scanner::identifierType() 
{
    switch (start[0]) 
    {
        case 'a': return checkKeyword(1, 2, "nd", TokenType::AND);
        case 'b': return checkKeyword(1, 4, "reak", TokenType::BREAK);
        case 'c': 
        {
            if (current - start > 1) 
            {
                switch(start[1])
                {
                    case 'l' : return checkKeyword(2, 3, "ass", TokenType::CLASS);
                    case 'o' : return checkKeyword(2, 6, "ntinue", TokenType::CONTINUE);
                    case 'a' : 
                    {
                        switch(start[2])
                        {
                            case 't' : return checkKeyword(3, 2, "ch", TokenType::CATCH);
                            case 's' : return checkKeyword(3, 1, "e", TokenType::CASE);
                            break;
                        }
                    }
                }
            }
            break;
        }
        case 'd': 
        {
            if (current - start > 1) 
            {
                switch(start[1])
                {
                    case 'o' : return checkKeyword(2, 0, "",  TokenType::DO);
                    case 'e' : return checkKeyword(2, 4, "lete", TokenType::DELETE);          
                }  
            }
            break;
        }
        case 'e': 
            if (current - start > 1) 
            {
                switch(start[1])
                {
                    case 'l' : return checkKeyword(2, 2, "se", TokenType::ELSE);
                    case 'x' : return checkKeyword(2, 4, "tern", TokenType::EXTERN);
                }
            }        
            break;
        return checkKeyword(1, 3, "lse", TokenType::ELSE);
        case 'f':
            if (current - start > 1) 
            {
                switch (start[1]) 
                {
                    case 'a': return checkKeyword(2, 3, "lse", TokenType::FALSE);
                    case 'i': return checkKeyword(2, 5, "nally", TokenType::FINALLY);
                    case 'o': return checkKeyword(2, 1, "r", TokenType::FOR);
                    case 'u': return checkKeyword(2, 1, "n", TokenType::FUN);
                }
            }
            break;        
        case 'i': 
        {
            switch(start[1])
            {
                case 'f' : return checkKeyword(2,0, "X", TokenType::IF);
                case 's' : return checkKeyword(2,1,"a", TokenType::ISA);
            }
            break;
        }
        case 'n': return checkKeyword(1, 2, "il", TokenType::NIL);
        case 'o': return checkKeyword(1, 1, "r", TokenType::OR);
        case 'p': return checkKeyword(1, 4, "rint", TokenType::PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TokenType::RETURN);
        case 's': 
        {
            switch(start[1])
            {
                case 't' : return checkKeyword(2, 4, "atic", TokenType::STATIC);
                case 'u' : return checkKeyword(2, 3, "per", TokenType::SUPER);
                case 'w' : return checkKeyword(2, 4, "itch", TokenType::SWITCH);
            }
            break;
        }
        case 't':
            if (current - start > 1) 
            {
                switch (start[1]) 
                {
                    case 'h': 
                    {
                        switch(start[2]) 
                        {
                            case 'i' : return checkKeyword(3, 1, "s", TokenType::THIS);
                            case 'r' : return checkKeyword(3, 2, "ow", TokenType::THROW);
                        }
                        break;
                    }
                    case 'r': 
                    {
                        switch(start[2])
                        {
                            case 'y' : return checkKeyword(3, 0, "", TokenType::TRY); //return TokenType::TRY;
                            case 'u' : return checkKeyword(3, 1, "e", TokenType::TRUE);
                        }
                        break;
                    }
                }
            }
            break;        
        case 'v': return checkKeyword(1, 2, "ar", TokenType::VAR);
        case 'w': return checkKeyword(1, 4, "hile", TokenType::WHILE);
    }
    return TokenType::IDENTIFIER;
}

bool Scanner::match(char expected) 
{
    if (isAtEnd()) return false;
    if (*current != expected) return false;
    current++;
    return true;
}

char Scanner::peek() 
{
    return *current;
}

char Scanner::peekNext() 
{
    if (isAtEnd()) return '\0';
    return current[1];
}

char Scanner::advance() 
{
    current++;
    return current[-1];
}

bool Scanner::isAtEnd() 
{
    return *current == 0;
}

Token Scanner::makeToken(TokenType type) 
{
    Token token;
    token.type = type;
    token.start = start;
    token.length = (int)(current - start);
    token.line = line;
    return token;
}

Token Scanner::errorToken(const char* message) 
{
    Token token;
    token.type = TokenType::ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = line;
    return token;
}

void Scanner::skipWhitespace() 
{
    for (;;) 
    {
        char c = peek();
        switch (c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                line++;
                advance();
                break;
            case '#':
            {
                // A # comment goes until the end of the line.
                while (peek() != '\n' && !isAtEnd()) advance();
                break;
            }
            case '/':
                if (peekNext() == '/') 
                {
                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !isAtEnd()) advance();
                }
                else if(peekNext() == '*')
                {
                    // A multiline comment goes until the end marker */
                    while ( !(peek() == '*' && peekNext() == '/') && !isAtEnd())
                    {
                        if(peek() == '\n') line++;
                        advance();
                    }
                    advance();
                    advance();
                } 
                else 
                {
                    return;
                }
                break;
            default:
            return;
        }
    }
}


