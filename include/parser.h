#ifndef moc_parser_h
#define moc_parser_h

#include "common.h"
#include "scanner.h"

/*
  the MOSER parser.
*/

class Compiler;
typedef void (Compiler::*ParseFn)(bool);

enum class Precedence : uint8_t 
{
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_BIN_OR,
  PREC_BIN_AND,
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
};

class ParseRule
{
public:
    ParseFn     prefix = nullptr;
    ParseFn     infix = nullptr;
    Precedence  precedence;
};

class Parser
{
public:

    Parser(Scanner* s ) : scanner(s) {}

    Scanner* scanner = nullptr;

    Token current;
    Token previous;
    Token beforePrevious;

    bool hadError = false;
    bool panicMode = false;

    void parsePrecedence(Precedence precedence) ;

    bool match(TokenType type);
    bool check(TokenType type);    
    void consume(TokenType type, const char* message);
    void advance();

    void errorAtCurrent(const char* message);
    void error(const char* message);
    void errorAt(Token* token, const char* message);

    void synchronize();

    ParseRule* getRule(TokenType type);

    static int rule_initializer;
    static int init_rules();

    static ParseRule rules[];
};




#endif
