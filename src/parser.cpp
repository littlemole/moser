//#include "pch.h"
#include "parser.h"
#include "compiler.h"

extern Compiler* currentCompiler;

void Parser::parsePrecedence(Precedence precedence) 
{
    advance();
    ParseFn prefixRule = getRule(previous.type)->prefix;
    if (prefixRule == NULL) 
    {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= Precedence::PREC_ASSIGNMENT;
    (currentCompiler->*prefixRule)(canAssign);

    while (precedence <= getRule(current.type)->precedence) 
    {
        advance();
        ParseFn infixRule = getRule(previous.type)->infix;
        (currentCompiler->*infixRule)(canAssign);
    }    
    if (canAssign && match(TokenType::EQUAL)) 
    {
        error("Invalid assignment target.");
    }
}

bool Parser::match(TokenType type) 
{
    if (!check(type)) return false;
    advance();
    return true;
}

bool Parser::check(TokenType type) 
{
    return current.type == type;
}

void Parser::consume(TokenType type, const char* message) 
{
    if (current.type == type) 
    {
        advance();
        return;
    }

    errorAtCurrent(message);
}

void Parser::advance() 
{
    beforePrevious = previous;
    previous = current;

    for (;;) 
    {
        current = scanner->scanToken();
        if (current.type != TokenType::ERROR) break;

        errorAtCurrent(current.start);
    }
}

void Parser::errorAtCurrent(const char* message) 
{
    errorAt(&current, message);
}

void Parser::error(const char* message) 
{
    errorAt(&previous, message);
}

void Parser::errorAt(Token* token, const char* message) 
{
    if (panicMode) return;
    panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TokenType::EOF_TOKEN) 
    {
        fprintf(stderr, " at end");
    } 
    else if (token->type == TokenType::ERROR) 
    {
        // Nothing.
    } 
    else 
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    hadError = true;
}

void Parser::synchronize() 
{
    panicMode = false;

    while (current.type != TokenType::EOF_TOKEN) 
    {
        if (previous.type == TokenType::SEMICOLON) return;
        switch (current.type) 
        {
            case TokenType::CLASS:
            case TokenType::FUN:
            case TokenType::VAR:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::PRINT:
            case TokenType::RETURN:
            return;

            default:
            ; // Do nothing.
        }

        advance();
    }
}

ParseRule* Parser::getRule(TokenType type) 
{
    uint8_t index = (uint8_t)type;
    return &rules[index];
}

ParseRule Parser::rules[((uint8_t)TokenType::EOF_TOKEN)+1];

int Parser::rule_initializer = init_rules();

int Parser::init_rules()
{
  rules[(uint8_t)TokenType::LEFT_PAREN]    = {&Compiler::grouping, &Compiler::call,   Precedence::PREC_CALL};
  rules[(uint8_t)TokenType::RIGHT_PAREN]   = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::LEFT_BRACE]    = {&Compiler::mapInit,     &Compiler::mapAccess,   Precedence::PREC_CALL}; 
  rules[(uint8_t)TokenType::RIGHT_BRACE]   = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::LEFT_BRACKET]  = {&Compiler::arrayInit,     &Compiler::arrayAccess,   Precedence::PREC_CALL}; 
  rules[(uint8_t)TokenType::RIGHT_BRACKET] = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::DOLLAR]        = {&Compiler::meta,     NULL,   Precedence::PREC_CALL};
  rules[(uint8_t)TokenType::AT]            = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::COMMA]         = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::DOT]           = {NULL,     &Compiler::dot,   Precedence::PREC_CALL};
  rules[(uint8_t)TokenType::MINUS]         = {&Compiler::unary,    &Compiler::binary, Precedence::PREC_TERM};
  rules[(uint8_t)TokenType::MINUS_MINUS]   = {&Compiler::unary,    &Compiler::binary, Precedence::PREC_TERM};
  rules[(uint8_t)TokenType::PLUS]          = {NULL,     &Compiler::binary, Precedence::PREC_TERM};
  rules[(uint8_t)TokenType::PLUS_PLUS]     = {&Compiler::unary,     &Compiler::binary, Precedence::PREC_FACTOR};
  rules[(uint8_t)TokenType::SEMICOLON]     = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::SLASH]         = {NULL,     &Compiler::binary, Precedence::PREC_FACTOR};
  rules[(uint8_t)TokenType::MODULO]        = {NULL,     &Compiler::binary, Precedence::PREC_FACTOR};
  rules[(uint8_t)TokenType::STAR]          = {NULL,     &Compiler::binary, Precedence::PREC_FACTOR};
  rules[(uint8_t)TokenType::BANG]          = {&Compiler::unary,     NULL,  Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::BANG_EQUAL]    = {NULL,     &Compiler::binary, Precedence::PREC_EQUALITY};
  rules[(uint8_t)TokenType::EQUAL]         = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::PLUS_EQUAL]    = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::MINUS_EQUAL]   = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::EQUAL_EQUAL]   = {NULL,     &Compiler::binary,   Precedence::PREC_EQUALITY};
  rules[(uint8_t)TokenType::GREATER]       = {NULL,     &Compiler::binary,   Precedence::PREC_COMPARISON};
  rules[(uint8_t)TokenType::GREATER_EQUAL] = {NULL,     &Compiler::binary,   Precedence::PREC_COMPARISON};
  rules[(uint8_t)TokenType::LESS]          = {NULL,     &Compiler::binary,   Precedence::PREC_COMPARISON};
  rules[(uint8_t)TokenType::LESS_EQUAL]    = {NULL,     &Compiler::binary,   Precedence::PREC_COMPARISON};
  rules[(uint8_t)TokenType::SHIFT_RIGHT]   = {NULL,     &Compiler::binary,   Precedence::PREC_TERM};
  rules[(uint8_t)TokenType::SHIFT_LEFT]    = {NULL,     &Compiler::binary,   Precedence::PREC_TERM};
  rules[(uint8_t)TokenType::IDENTIFIER]    = {&Compiler::variable,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::STRING]        = {&Compiler::string,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::NUMBER]        = {&Compiler::number,   NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::INTEGER]       = {&Compiler::number,   NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::AND]           = {NULL,     &Compiler::and_,   Precedence::PREC_AND};
  rules[(uint8_t)TokenType::BIN_AND]       = {&Compiler::address,     &Compiler::binary,   Precedence::PREC_BIN_AND};
  rules[(uint8_t)TokenType::CLASS]         = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::ELSE]          = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::FALSE]         = {&Compiler::literal,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::FOR]           = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::FUN]           = {&Compiler::funExpression,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::IF]            = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::ISA]            = {NULL,     &Compiler::binary,   Precedence::PREC_CALL};
  rules[(uint8_t)TokenType::NIL]           = {&Compiler::literal,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::OR]            = {NULL,     &Compiler::or_,   Precedence::PREC_OR};
  rules[(uint8_t)TokenType::BIN_OR]        = {NULL,     &Compiler::binary,   Precedence::PREC_BIN_OR};
  rules[(uint8_t)TokenType::TILDE]         = {&Compiler::unary,     NULL,   Precedence::PREC_UNARY};
  rules[(uint8_t)TokenType::PRINT]         = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::RETURN]        = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::SUPER]         = {&Compiler::super_,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::THIS]          = {&Compiler::this_,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::TRUE]          = {&Compiler::literal,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::VAR]           = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::WHILE]         = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::BREAK]         = {&Compiler::break_,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::CONTINUE]      = {&Compiler::continue_,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::ERROR]         = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::TRY]           = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::CATCH]         = {NULL,     NULL,   Precedence::PREC_NONE};
  rules[(uint8_t)TokenType::EOF_TOKEN]     = {NULL,     NULL,   Precedence::PREC_NONE};

  return 0;
};


