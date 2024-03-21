#ifndef moc_scanner_h
#define moc_scanner_

#include <cstring>

/*
    the MOSER Scanner / Lexer
*/

enum class TokenType 
{
    // Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,
    COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR, AT,
    // One or two character tokens.
    PLUS_PLUS, MINUS_MINUS,
    PLUS_EQUAL, MINUS_EQUAL,
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL, TILDE,
    MODULO, COLON, DOLLAR,
    // Literals.
    IDENTIFIER, STRING, NUMBER, INTEGER,
    // Keywords.
    AND, CLASS, ELSE, FALSE,
    FOR, FUN, IF, NIL, OR,
    BIN_OR, BIN_AND, DELETE,
    PRINT, RETURN, SUPER, THIS,
    TRUE, VAR, DO, WHILE, BREAK, CONTINUE,
    TRY, CATCH, THROW, FINALLY,
    EXTERN, ELIPSIS, ISA,
	SHIFT_LEFT, SHIFT_RIGHT,
    SWITCH, CASE, STATIC,

    ERROR, EOF_TOKEN
} ;

struct Token 
{
    TokenType type;
    const char* start;
    int length;
    int line;

    bool equals(const Token& rhs) 
    {
      if (length != rhs.length) return false;
      return memcmp(start, rhs.start, length) == 0;
    }

    std::string str()
    {
        return std::string(start,length);
    }

};

class Scanner
{
public:
    void initScanner(const char* source);
    Token scanToken();

    const char* start = nullptr;
    const char* current = nullptr;
    int line = 0;    

private:

    Token identifier();
    Token number();
    Token hexNum();
    Token string();

    TokenType identifierType();
    TokenType checkKeyword(int start, int length, const char* rest, TokenType type);

    bool match(char expected);
    char peek();
    char peekNext();
    char advance();

    void skipWhitespace();
    bool isAtEnd();
    Token makeToken(TokenType type);
    Token errorToken(const char* message);

};

#endif
