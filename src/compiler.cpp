//#include "pch.h"
#include "compiler.h"
#include "debug.h"
#include "foreign.h"


Compiler* currentCompiler = nullptr;
ClassCompiler* currentClass = nullptr;

Compiler::Compiler(VM& v) : vm(v), type(FunctionType::TYPE_SCRIPT)
{
    function = new ObjFunction(vm, nullptr,0,0,false);

    if (type != FunctionType::TYPE_FUNCTION) 
    {
        locals.push_back(
            Local { 
                { TokenType::STRING,"this", 4, 0 },
                0
            } 
        );
    } 
    else 
    {
        locals.push_back(
            Local { 
                { TokenType::STRING,"", 0, 0 },
                0
            } 
        );
    }

    currentCompiler = this;
}

Compiler::Compiler(VM& v, Compiler* parent,FunctionType ft, bool async)
    : vm(v), enclosing(parent), type(ft)
{
    function = new ObjFunction(vm,nullptr,0,0,async);

    filename = enclosing->filename;
    function->chunk.filename = filename;

    locals.push_back(
        Local { 
            { TokenType::STRING,"", 0, 0},
            0
        } 
    );

    currentCompiler = this;
}

Compiler::Compiler(VM& v, Compiler* parent,FunctionType ft, Scanner* s, Parser* p, bool async) 
    : vm(v), enclosing(parent), type(ft), scanner(s), parser(p)
{
    if (type != FunctionType::TYPE_SCRIPT) 
    {
        auto name = new ObjString(vm, parser->previous.str());
        function = new ObjFunction(vm, name,0,0,async);
    }
    else
    {
        function = new ObjFunction(vm, nullptr,0,0,false);
    }

    filename = enclosing->filename;
    function->chunk.filename = filename;

    if (type != FunctionType::TYPE_FUNCTION) 
    {
        locals.push_back(
            Local { 
                { TokenType::STRING,"this", 4, parser->scanner->line },
                0
            } 
        );
    } 
    else 
    {
        locals.push_back(
            Local { 
                { TokenType::STRING,"", 0, parser->scanner->line },
                0
            } 
        );
    }

    currentCompiler = this;
}

Compiler::~Compiler()
{
    currentCompiler = enclosing;
}

ObjFunction* Compiler::compile(const char* file, std::string source)
{
    filename = file;

    Scanner s;
    scanner = &s;

    Parser p(scanner);
    parser = &p;

    scanner->initScanner(source.c_str());
    parser->advance();

    metadata = NIL_VAL;

    while (!parser->match(TokenType::EOF_TOKEN)) 
    {
        declaration();
    }

    ObjFunction* fun = endCompiler();
    fun->chunk.filename = file;
    return parser->hadError ? NULL : fun;
}


// main parse: declaration, statement, expression

void Compiler::declaration() 
{
//    metadata = NIL_VAL;
    while(parser->match(TokenType::SEMICOLON)); // swallow

    if(parser->current.type == TokenType::EOF_TOKEN) return;

    if (parser->match(TokenType::AT)) 
    {
        metaDeclaration();
    }
    else
    if (parser->match(TokenType::CLASS)) 
    {
        classDeclaration();
    }
    else if (parser->match(TokenType::FUN)) 
    {
        funDeclaration();
    }  
    else if (parser->match(TokenType::VAR)) 
    {
        varDeclaration();
    } 
    else if (parser->match(TokenType::EXTERN)) 
    {
        externDeclaration();
    } 
    else if (parser->match(TokenType::ASYNC)) 
    {
        asyncDeclaration();
    } 
    else 
    {
        statement();
    }
    if (parser->panicMode) parser->synchronize();
}

void Compiler::metaDeclaration() 
{
    parser->consume(TokenType::IDENTIFIER,"@ annotation needs identifier");
    Token name = parser->previous;

    if(IS_NIL(metadata))
    {
        metadata = Value(new ObjMap(vm));
    }
    auto meta = ::as<ObjMap>(metadata);
    Value result = NIL_VAL;

    if(parser->match(TokenType::LEFT_PAREN))
    {
        if(parser->match(TokenType::IDENTIFIER) )
        {
            auto map = new ObjMap(vm);
            result = Value(map);
            meta->item(name.str(),result);

            do
            {
                std::string key = parser->previous.str();
                Value val = NIL_VAL;
                if(parser->match(TokenType::EQUAL) ||
                   parser->match(TokenType::COLON) )
                {
                    if(parser->match(TokenType::NUMBER))
                    {
                        double d = strtod(parser->previous.start, NULL);
                        val = Value(d);
                    }
                    else if(parser->match(TokenType::INTEGER))
                    {
                        ptrdiff_t i = strtoll(parser->previous.start, NULL,10);
                        val = Value(i);
                    }
                    else if(parser->match(TokenType::FALSE))
                    {
                        val = Value(false);
                    }
                    else if(parser->match(TokenType::TRUE))
                    {
                        val = Value(true);
                    }
                    else if(parser->match(TokenType::STRING))
                    {
                        std::string s = parser->previous.str();
                        s = s.substr(1,s.size()-2);

                        val = Value(new ObjString(vm,s));
                    }
                    else if(parser->match(TokenType::IDENTIFIER))
                    {
                        std::string s = parser->previous.str();
                        val = Value(new ObjString(vm,s));
                    }
                }
                map->item(key,val);
                if(parser->match(TokenType::COMMA)) {}
            }
            while(parser->match(TokenType::IDENTIFIER));
            parser->consume(TokenType::RIGHT_PAREN,"need left parent ) after annotation.");
            return;
        }

        auto array = new ObjArray(vm);

        do 
        {
            Value val = NIL_VAL;
            if(parser->match(TokenType::NUMBER))
            {
                double d = strtod(parser->previous.start, NULL);
                val = Value(d);
            }
            else if(parser->match(TokenType::INTEGER))
            {
                ptrdiff_t i = strtoll(parser->previous.start, NULL,10);
                val = Value(i);
            }
            else if(parser->match(TokenType::FALSE))
            {
                val = Value(false);
            }
            else if(parser->match(TokenType::TRUE))
            {
                val = Value(true);
            }
            else if(parser->match(TokenType::STRING))
            {
                std::string s = parser->previous.str();
                s = s.substr(1,s.size()-2);

                val = Value(new ObjString(vm,s));
            }
            else if(parser->match(TokenType::IDENTIFIER))
            {
                std::string s = parser->previous.str();
                val = Value(new ObjString(vm,s));
            }

            array->add(val);
        } while(parser->match(TokenType::COMMA));

        parser->consume(TokenType::RIGHT_PAREN,"need left parent ) after annotation.");

        if(array->size().to_int() == 1)
        {
            result = array->index(0);
        }
        else if(array->size().to_int() > 1)
        {
            result = Value(array);
        }
        else
        {
            result = NIL_VAL;
        }
    }
    meta->item(name.str(),result);
}

void Compiler::statement() 
{
    if (parser->match(TokenType::PRINT)) 
    {
        printStatement();
    } 
    else if (parser->match(TokenType::TRY)) 
    {
        tryStatement();
    }     
    else if (parser->match(TokenType::THROW)) 
    {
        throwStatement();
    }     
    else if (parser->match(TokenType::FOR)) 
    {
        forStatement();    
    }
    else if (parser->match(TokenType::IF)) 
    {
        ifStatement();
    }
    else if (parser->match(TokenType::RETURN)) 
    {
        returnStatement();
    }    
    else if (parser->match(TokenType::WHILE)) 
    {
        whileStatement();    
    }
    else if (parser->match(TokenType::DO)) 
    {
        doWhileStatement();    
    }
    else if (parser->match(TokenType::DELETE)) 
    {
        deleteStatement();    
    }
    else if (parser->match(TokenType::SWITCH)) 
    {
        switchStatement();    
    }
    else if (parser->match(TokenType::LEFT_BRACE)) 
    {
        beginScope();
        block();
        endScope();
    }    
    else 
    {
        expressionStatement();
    }
}

void Compiler::expression() 
{
    parser->parsePrecedence(Precedence::PREC_ASSIGNMENT);
}

// declarations

void Compiler::classDeclaration() 
{
    parser->consume(TokenType::IDENTIFIER, "Expect class name.");
    Token className = parser->previous;    
    cindex_t nameConstant = identifierConstant(&parser->previous);
    declareVariable(&parser->previous);

    emitBytes(OpCode::OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    cindex_t m = makeConstant(metadata);
    metadata = NIL_VAL;

    namedVariable(className, false);
    emitBytes(OpCode::OP_SET_META,m);


    ClassCompiler classCompiler;
    classCompiler.enclosing = currentClass;
    classCompiler.name = className;
    currentClass = &classCompiler;    

    // declarations

    if (parser->match(TokenType::COLON))
    {
        if (parser->match(TokenType::IDENTIFIER))
        {
            variable(false);
            if (className.equals(parser->previous))
            {
                parser->error("A class can't inherit from itself.");
            }
        }
        else
        {
            parser->consume(TokenType::LEFT_PAREN, "Expect (");
            grouping(false);
        }

        namedVariable(className, false);
        emitByte(OpCode::OP_INHERIT);
        classCompiler.hasSuperclass = true;

        beginScope();
        addLocal(Token{ TokenType::SUPER,"super",5,className.line });
        defineVariable(0);
    }

    namedVariable(className, false);
    parser->consume(TokenType::LEFT_BRACE, "Expect '{' before class body.");
    while ( !parser->check(TokenType::RIGHT_BRACE) && 
            !parser->check(TokenType::EOF_TOKEN)) 
    {
        while(parser->match(TokenType::AT))
        {
            metaDeclaration();
        }
		
		bool async = false;
		if(parser->match(TokenType::ASYNC)) 
		{
			async = true;
		}

        if( (parser->current.type == TokenType::IDENTIFIER) && (parser->current.str() == "get"))
        {
            parser->consume(TokenType::IDENTIFIER,"getter");
            method(OpCode::OP_GETTER,async);
        }
        else if((parser->current.type == TokenType::IDENTIFIER) && (parser->current.str() == "set"))
        {
            parser->consume(TokenType::IDENTIFIER,"setter");
            method(OpCode::OP_SETTER,async);
        }
        else if(parser->current.type == TokenType::STATIC )
        {
            parser->consume(TokenType::STATIC,"static");
            method(OpCode::OP_STATIC_METHOD,async);
        }
        else 
        {
            method(OpCode::OP_METHOD,async);
        }
    }    
    parser->consume(TokenType::RIGHT_BRACE, "Expect '}' after class body.");
    emitByte(OpCode::OP_POP);

    if (classCompiler.hasSuperclass) 
    {
        endScope();
    }
    currentClass = currentClass->enclosing;
}

void Compiler::asyncDeclaration() 
{
	parser->consume(TokenType::FUN, "Expect fun after async.");

    cindex_t global = parseVariable("Expect function name.");
    markInitialized();
    parseFunction(FunctionType::TYPE_FUNCTION, true);
    defineVariable(global);
}

void Compiler::funDeclaration() 
{
    cindex_t global = parseVariable("Expect function name.");
    markInitialized();
    parseFunction(FunctionType::TYPE_FUNCTION,false);
    defineVariable(global);
}

void Compiler::varDeclaration() 
{
    Token name = parser->current;
    cindex_t global = parseVariable("Expect variable name.");

    if (parser->match(TokenType::EQUAL)) 
    {
        expression();
        // add meta?
    } 
    else 
    {
        emitByte(OpCode::OP_NIL);
    }

    parser->consume(TokenType::SEMICOLON,
            "Expect ';' after variable declaration.");

    defineVariable(global);

    if(!IS_NIL(metadata))
    {
        auto r = resolve(name);
        emitBytes(r.getOp,r.arg);

        auto m = makeConstant(metadata);
        metadata = NIL_VAL;
        emitBytes(OpCode::OP_SET_META,m);
    }
}

void Compiler::externDeclaration()
{
    if( parser->match(TokenType::IDENTIFIER))
    {
        if(parser->previous.str() == "from")
        {
            parser->consume(TokenType::STRING,"need lib string after extern from.");
            
            std::string lib = parser->previous.str();
            lib = lib.substr(1,lib.size()-2);
            nativeDeclaration(lib);
            return;
        }
        if(parser->previous.str() == "callback")
        {
            callbackDeclaration();
            return;
        }
        if(parser->previous.str() == "struct")
        {
            structDeclaration();
            return;
        }
        parser->error("extern declaration failed.");
        return;
    }

    nativeDeclaration("");
}

void Compiler::nativeDeclaration(const std::string& lib)
{
    parser->consume(TokenType::LEFT_BRACE, "need left brace { after extern from 'lib'.");

    while(!parser->match(TokenType::RIGHT_BRACE))
    {
        parser->consume(TokenType::IDENTIFIER,"need return type identifier for extern.");
        std::string retType = parser->previous.str();

        parser->consume(TokenType::IDENTIFIER,"need fun name identifier for extern.");
        Token f = parser->previous;
        std::string fun = parser->previous.str();
        std::string as = fun;

        parser->consume(TokenType::LEFT_PAREN,"need left paren for extern declaration after fun name.");

        std::vector<std::string> args;
        int startVariadic = 0;
        while(!parser->match(TokenType::RIGHT_PAREN))
        {
            parser->consume(TokenType::IDENTIFIER,"need fun name identifier for extern.");
            std::string arg = parser->previous.str();
            args.push_back(arg);
            if(parser->match(TokenType::COMMA)) 
            {
                //swallow
            }           
            if(parser->match(TokenType::ELIPSIS)) 
            {
                startVariadic = (int)args.size() + 1;
                args.push_back(parser->previous.str());
                if(parser->match(TokenType::COMMA)) 
                {
                    //swallow
                }           
            }           
        }
        if(parser->match(TokenType::IDENTIFIER))
        {
            if (parser->previous.str() != "as")
            {
                parser->errorAtCurrent("native declaration must be followed by 'as' or ';'");
                return;
            }
            parser->consume(TokenType::IDENTIFIER,"as mus be followed by identifier.");
            as = parser->previous.str();
        }
        parser->consume(TokenType::SEMICOLON,"need semicolon adter extern declaration parameters.");

        Token t{ TokenType::IDENTIFIER, as.c_str(), (int) as.size(),f.line};
        cindex_t nameConstant = identifierConstant(&t);
        declareVariable(&t);

        if(startVariadic == 0)
        {
            ObjForeignFunction* foreign = new ObjForeignFunction(vm,lib,retType,fun,args);
            emitConstant(foreign);
        }
        else
        {
            ObjVariadicForeignFunction* foreign = new ObjVariadicForeignFunction(vm,lib,retType,fun,args);
            emitConstant(foreign);
        }
        defineVariable(nameConstant);
    }
}

void Compiler::callbackDeclaration()
{
    parser->consume(TokenType::IDENTIFIER,"need return type identifier for extern callback.");
    std::string retType = parser->previous.str();

    parser->consume(TokenType::IDENTIFIER,"need fun type name identifier for extern callback.");
    Token f = parser->previous;
    std::string fun = parser->previous.str();
    std::string as = fun;

    parser->consume(TokenType::LEFT_PAREN,"need left paren for extern callback declaration after fun name.");

    std::vector<std::string> args;
    while(!parser->match(TokenType::RIGHT_PAREN))
    {
        parser->consume(TokenType::IDENTIFIER,"need fun name identifier for extern callback.");
        std::string arg = parser->previous.str();
        args.push_back(arg);
        if(parser->match(TokenType::COMMA)) 
        {
            //swallow
        }           
    }
    parser->consume(TokenType::SEMICOLON,"need semicolon adter extern declaration parameters.");

    cindex_t nameConstant = identifierConstant(&f);
    declareVariable(&f);

    ObjCallbackType* cb = new ObjCallbackType(vm,f.str(),retType,args);
    emitConstant(cb);
    defineVariable(nameConstant);
}

void Compiler::structDeclaration()
{
    parser->consume(TokenType::IDENTIFIER,"need struct type identifier for extern struct.");
    std::string structName = parser->previous.str();
    Token s = parser->previous;

    parser->consume(TokenType::LEFT_BRACE,"need left brace for extern struct declaration after struct name.");

    std::vector<std::pair<std::string,std::string>> elements;
    while(!parser->match(TokenType::RIGHT_BRACE))
    {
        parser->consume(TokenType::IDENTIFIER,"need struct element type identifier for extern struct.");
        std::string typ = parser->previous.str();

        parser->consume(TokenType::IDENTIFIER,"need struct element name identifier for extern struct.");
        std::string name = parser->previous.str();
        if(parser->match(TokenType::LEFT_BRACKET))
        {
            parser->consume(TokenType::INTEGER,"strcut array member needs size.");
            std::string index = parser->previous.str();
            name += "[";
            name += index;
            name += "]";
            parser->consume(TokenType::RIGHT_BRACKET,"struct array needs closing right bracket.");            
        }
        elements.push_back( {name,typ} );

        if(parser->match(TokenType::COMMA)) 
        {
            //swallow
        }           
    }

    cindex_t nameConstant = identifierConstant(&s);
    declareVariable(&s);

    ObjStruct* strct = new ObjStruct( vm, s.str(), elements);
    emitConstant(strct);
    defineVariable(nameConstant);
}

// statements 

void Compiler::expressionStatement() 
{
    expression();
    parser->consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    emitByte(OpCode::OP_POP);
}

void Compiler::printStatement() 
{
    expression();
    parser->consume(TokenType::SEMICOLON, "Expect ';' after value.");
    emitByte(OpCode::OP_PRINT);
}

void Compiler::returnStatement() 
{
    if (type == FunctionType::TYPE_SCRIPT) 
    {
        parser->error("Can't return from top-level code.");
    }

    if (parser->match(TokenType::SEMICOLON)) 
    {
        if (type == FunctionType::TYPE_CONSTRUCTOR)
        {
            emitBytes(OpCode::OP_GET_LOCAL, (uint16_t)0);
        }
        else
        {
            emitByte(OpCode::OP_NIL);
        }
        emitByte(OpCode::OP_RETURN);
    } 
    else 
    {
        if (type == FunctionType::TYPE_CONSTRUCTOR) 
        {
            parser->error("Can't return a value from an initializer.");
        }
        expression();
        parser->consume(TokenType::SEMICOLON, "Expect ';' after return value.");
        emitByte(OpCode::OP_RETURN);
    }
}

void Compiler::tryStatement()
{
    function->exHandlers.push_back( (int) function->breakes.size());
    int beginTry = emitTry();
    statement();
    int jumpExit = emitJump(OpCode::OP_JUMP);

    if(parser->match(TokenType::CATCH))
    {
        beginScope();
        parser->consume(TokenType::LEFT_PAREN, "Expect '(' after catch.");

        if(parser->match(TokenType::VAR))
        {
        }

        cindex_t global = parseVariable("Expect variable name.");

        parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after catch var.");

        patchTry(beginTry);

        defineVariable(global);
        statement();
        endScope();
    }

    patchTryFinal(beginTry+2);

    int jump = (int) currentChunk()->code.size() - jumpExit -2;
    if (jump > UINT16_MAX) 
    {
        parser->error("Too much code to jump over.");
    }

    currentChunk()->code[jumpExit] = (jump >> 8) & 0xff;
    currentChunk()->code[jumpExit + 1] = jump & 0xff;

    emitEndTry();

    if(parser->match(TokenType::FINALLY))
    {
        statement();
    }

    emitByte(OpCode::OP_FINALLY);

    function->exHandlers.pop_back();

}

void Compiler::throwStatement()
{ 
    int n = function->exHandlers.empty() ? 0 : (int)function->loops.size() - function->exHandlers.back();

    for( int i = 0; i< n; i++)
    {
        emitByte(OpCode::OP_POP);
    }
    expression();
    emitByte(OpCode::OP_THROW);

    parser->consume(TokenType::SEMICOLON, "Expect ';' after throwstatement.");
}


void Compiler::ifStatement()
{
    parser->consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after condition."); 

    int thenJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    emitByte(OpCode::OP_POP);

    statement();

    int elseJump = emitJump(OpCode::OP_JUMP);

    patchJump(thenJump);    
    emitByte(OpCode::OP_POP);

    if (parser->match(TokenType::ELSE)) statement();

    patchJump(elseJump);
}

void Compiler::whileStatement() 
{
    function->breakes.push_back(std::vector<int>());
    function->loops.push_back(std::vector<int>());

    int loopStart = (int) currentChunk()->code.size();

    parser->consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    emitByte(OpCode::OP_POP);
    statement();

    emitLoop(loopStart);
    patchJump(exitJump);
    emitByte(OpCode::OP_POP);

    auto& br = function->breakes.back();
    while(!br.empty())
    {
        int pos = br.back();
        br.pop_back();
        patchJump(pos);
    }
    function->breakes.pop_back();

    auto& loop = function->loops.back();
    while(!loop.empty())
    {
        int pos = loop.back();
        loop.pop_back();
        patchLoop(pos,loopStart);
    }
    function->loops.pop_back();

}

void Compiler::doWhileStatement() 
{
    function->breakes.push_back(std::vector<int>());
    function->loops.push_back(std::vector<int>());

    int loopStart = (int) currentChunk()->code.size();
    // do {
    statement();
    // while condition

    auto& loop = function->loops.back();
    while(!loop.empty())
    {
        int pos = loop.back();
        loop.pop_back();
        patchLoop(pos,loopStart);
    }
    function->loops.pop_back();

    parser->consume(TokenType::WHILE, "Expect 'while' after 'do block'.");
    parser->consume(TokenType::LEFT_PAREN, "Expect '(' after 'while block'.");
    expression();
    parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after while condition.");
    parser->consume(TokenType::SEMICOLON, "Expect ';' after while condition.");

    int exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    emitByte(OpCode::OP_POP);

    emitLoop(loopStart);
    patchJump(exitJump);
    emitByte(OpCode::OP_POP);

    auto& br = function->breakes.back();
    while(!br.empty())
    {
        int pos = br.back();
        br.pop_back();
        patchJump(pos);
    }
    function->breakes.pop_back();
}

void Compiler::forOfStatement()
{
    function->breakes.push_back(std::vector<int>());
    function->loops.push_back(std::vector<int>());

    Token variable = parser->previous;

    parser->consume(TokenType::IDENTIFIER, "slurp of");

    // loop counter variable
    std::string loopvar = "__ci__" + variable.str();
    Token count(TokenType::IDENTIFIER, loopvar.c_str(), (int)loopvar.size(), variable.line);

    declareVariable(&count);
    cindex_t id = 0;
    if (scopeDepth == 0)
        id = identifierConstant(&count);

    defineVariable(id);

    // initialize with -1
    emitConstant(-1);

    auto rv = resolve(count);
    emitBytes(rv.setOp, rv.arg);

    expression(); // array variable

    parser->consume(TokenType::RIGHT_PAREN, "need ).");
    int loopStart = (int)currentChunk()->code.size();

    emitByte(OpCode::OP_DUP);
    emitBytes(rv.getOp, rv.arg);

    emitConstant(1);
    emitByte(OpCode::OP_ADD);

    emitBytes(rv.setOp, rv.arg);
    emitByte(OpCode::OP_POP);

    emitByte(OpCode::OP_DUP);

    std::string lengthStr("length");
    Token tokenLength{ TokenType::STRING, lengthStr.c_str(), (int)lengthStr.size(), variable.line };
    cindex_t length = identifierConstant(&tokenLength);

    emitBytes(OpCode::OP_GET_PROPERTY,length);

    emitBytes(rv.getOp, rv.arg);

    emitByte(OpCode::OP_GREATER);

    int exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    emitByte(OpCode::OP_POP);

    emitByte(OpCode::OP_DUP);

    emitBytes(rv.getOp, rv.arg);

    emitByte(OpCode::OP_ARRAY_GET);

    rv = resolve(variable);
    emitBytes(rv.setOp, rv.arg);
    emitByte(OpCode::OP_POP);

    statement();

    emitLoop(loopStart);
    patchJump(exitJump);

    auto& br = function->breakes.back();
    while (!br.empty())
    {
        int pos = br.back();
        br.pop_back();
        patchJump(pos);
    }
    function->breakes.pop_back();

    auto& loop = function->loops.back();
    while (!loop.empty())
    {
        int pos = loop.back();
        loop.pop_back();
        patchLoop(pos, loopStart);
    }
    function->loops.pop_back();
    endScope();
}

void Compiler::forInStatement()
{
    function->breakes.push_back(std::vector<int>());
    function->loops.push_back(std::vector<int>());

    Token variable = parser->previous;

    parser->consume(TokenType::IDENTIFIER, "slurp in");

    // loop counter variable
    std::string loopvar = "__ci__" + variable.str();
    Token count(TokenType::IDENTIFIER, loopvar.c_str(), (int)loopvar.size(), variable.line);

    declareVariable(&count);
    cindex_t id = 0;
    if (scopeDepth == 0)
        id = identifierConstant(&count);

    defineVariable(id);

    // initialize with -1
    emitConstant(-1);

    auto rv = resolve(count);
    emitBytes(rv.setOp, rv.arg);

    // keys array variable
    std::string keysvar = "__keys__" + variable.str();
    Token keys(TokenType::IDENTIFIER, keysvar.c_str(), (int)keysvar.size(), variable.line);

    declareVariable(&keys);
    cindex_t kid = 0;
    if (scopeDepth == 0)
        kid = identifierConstant(&keys);

    defineVariable(kid);


    expression(); // map variable
    emitByte(OpCode::OP_DUP);

    // get the keys array from map and put into keys variable
    std::string keysStr("keys");
    Token tokenKeys{ TokenType::STRING, keysStr.c_str(), (int)keysStr.size(), variable.line };
    cindex_t keys_index = identifierConstant(&tokenKeys);
    emitBytes(OpCode::OP_INVOKE, keys_index);
    emitByte(0);

    rv = resolve(keys);
    emitBytes(rv.setOp, rv.arg);

    parser->consume(TokenType::RIGHT_PAREN, "need ).");

    // LOOP
    int loopStart = (int)currentChunk()->code.size();

    emitByte(OpCode::OP_DUP);
    rv = resolve(count);
    emitBytes(rv.getOp, rv.arg);

    emitConstant(1);
    emitByte(OpCode::OP_ADD);

    rv = resolve(count);
    emitBytes(rv.setOp, rv.arg);
    emitByte(OpCode::OP_POP);

    emitByte(OpCode::OP_DUP);

    std::string lengthStr("length");
    Token tokenLength{ TokenType::STRING, lengthStr.c_str(), (int)lengthStr.size(), variable.line };
    cindex_t length = identifierConstant(&tokenLength);

    emitBytes(OpCode::OP_GET_PROPERTY, length);

    rv = resolve(count);
    emitBytes(rv.getOp, rv.arg);

    emitByte(OpCode::OP_GREATER);

    int exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    emitByte(OpCode::OP_POP);

    emitByte(OpCode::OP_DUP);

    rv = resolve(count);
    emitBytes(rv.getOp, rv.arg);

    emitByte(OpCode::OP_ARRAY_GET);

    rv = resolve(variable);
    emitBytes(rv.setOp, rv.arg);
    emitByte(OpCode::OP_POP);

    statement();

    emitLoop(loopStart);
    patchJump(exitJump);

    auto& br = function->breakes.back();
    while (!br.empty())
    {
        int pos = br.back();
        br.pop_back();
        patchJump(pos);
    }
    function->breakes.pop_back();

    auto& loop = function->loops.back();
    while (!loop.empty())
    {
        int pos = loop.back();
        loop.pop_back();
        patchLoop(pos, loopStart);
    }
    function->loops.pop_back();
    endScope();
}

void Compiler::forStatement() 
{
    function->breakes.push_back(std::vector<int>());
    function->loops.push_back(std::vector<int>());

    beginScope();
    parser->consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");

    if (parser->match(TokenType::SEMICOLON)) 
    {
        // No initializer.
    } 
    else if (parser->match(TokenType::VAR))
    {
        cindex_t vglobal = parseVariable("Expect variable name.");
        if (parser->match(TokenType::EQUAL))
        {
            expression();
        }
        else
        {
            emitByte(OpCode::OP_NIL);
        }

        defineVariable(vglobal);

        if (parser->check(TokenType::IDENTIFIER))
        {
            if (parser->current.str() == "of")
            {
                return forOfStatement();
            }
            if (parser->current.str() == "in")
            {
                return forInStatement();
            }
        }
        parser->consume(TokenType::SEMICOLON,
            "Expect ';' after variable declaration.");
    } 
    else 
    {
        expressionStatement();
    }

    int loopStart = (int) currentChunk()->code.size();
    int exitJump = -1;
    if (!parser->match(TokenType::SEMICOLON)) 
    {
        expression();
        parser->consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
        emitByte(OpCode::OP_POP); // Condition.
    }
      
    if (!parser->match(TokenType::RIGHT_PAREN)) 
    {
        int bodyJump = emitJump(OpCode::OP_JUMP);
        int incrementStart = (int) currentChunk()->code.size();
        expression();
        emitByte(OpCode::OP_POP);
        parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }
    statement();
    emitLoop(loopStart);

    if (exitJump != -1) 
    {
        patchJump(exitJump);
        emitByte(OpCode::OP_POP); // Condition.
    }

    auto& br = function->breakes.back();
    while(!br.empty())
    {
        int pos = br.back();
        br.pop_back();
        patchJump(pos);
    }
    function->breakes.pop_back();
    
    auto& loop = function->loops.back();
    while(!loop.empty())
    {
        int pos = loop.back();
        loop.pop_back();
        patchLoop(pos,loopStart);
    }
    function->loops.pop_back();

    endScope();
}

void Compiler::switchStatement() 
{
    parser->consume(TokenType::LEFT_PAREN, "need left parent for switch statement considition.");
    expression();
    parser->consume(TokenType::RIGHT_PAREN, "need right parent after switch statement condition.");
    parser->consume(TokenType::LEFT_BRACE, "need left brace after switch statement considition.");

    std::vector<int> exitJumps;
    while(!parser->match(TokenType::RIGHT_BRACE))
    {
        parser->consume(TokenType::CASE, "need case statement.");

        emitByte(OpCode::OP_DUP);

        if( parser->match(TokenType::INTEGER))
        {
            number(false);
        }
        else if( parser->match(TokenType::NUMBER))
        {
            number(false);
        }
        else if( parser->match(TokenType::STRING))
        {
            string(false);
        }
        else
        {
            parser->errorAtCurrent("case condition must be string or number");
        }

        parser->consume(TokenType::COLON, "need colon after case statement condition.");
        emitByte(OpCode::OP_EQUAL);
        int thenJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
        statement();
        emitByte(OpCode::OP_POP);
        int exitJump = emitJump(OpCode::OP_JUMP);
        exitJumps.push_back(exitJump);
        patchJump(thenJump);

        emitByte(OpCode::OP_POP);
    }

    for(auto& it : exitJumps)
    {
        patchJump(it);
    }
    emitByte(OpCode::OP_POP);
}

void Compiler::deleteStatement() 
{
    expression();

    parser->consume(TokenType::IDENTIFIER, "need from for delete statement.");
    Token var = parser->previous;

    if(var.str() != "from")
    {
        parser->errorAtCurrent("from needed after delete");
    }

    expression();
    emitByte(OpCode::OP_DELETE);
    parser->consume(TokenType::SEMICOLON,"need semicolon after delete statement.");
}

// parsing functions

void Compiler::block() 
{
    while (!parser->check(TokenType::RIGHT_BRACE) && !parser->check(TokenType::EOF_TOKEN)) 
    {
        declaration();
    }

    parser->consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
}

cindex_t Compiler::parseVariable(const char* errorMessage) 
{
    parser->consume(TokenType::IDENTIFIER, errorMessage);

    declareVariable(&parser->previous);
    if (scopeDepth > 0) return 0;

    cindex_t id = identifierConstant(&parser->previous);

    return id;
}

void Compiler::parseFunction(FunctionType typ, bool async) 
{
    Compiler compiler(vm,this,typ,scanner,parser,async);
    
    compiler.beginScope(); 

    parser->consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");

    if (!parser->check(TokenType::RIGHT_PAREN)) 
    {
        do 
        {
            compiler.function->arity_++;
            if (compiler.function->arity() > 255) 
            {
                parser->errorAtCurrent("Can't have more than 255 parameters.");
            }
            cindex_t constant = compiler.parseVariable("Expect parameter name.");
            compiler.defineVariable(constant);
        } while (parser->match(TokenType::COMMA));
    } 
        
    parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
    parser->consume(TokenType::LEFT_BRACE, "Expect '{' before function body.");
    compiler.block();

    ObjFunction* fun = compiler.endCompiler();
    if(!IS_NIL(metadata))
    {
        fun->metadata = metadata;
        metadata = NIL_VAL;
    }
    emitBytes(OpCode::OP_CLOSURE, makeConstant(fun));   

    for (int i = 0; i < fun->upvalueCount(); i++) 
    {
        emitByte((uint8_t)(compiler.upvalues[i].isLocal ? 1 : 0));
        emitBytes(compiler.upvalues[i].index);
    }

    if(!IS_NIL(fun->metadata))
    {
        auto map = as<ObjMap>(fun->metadata);
        if(map)
        {
            Value p = map->item("Decorator");
            if(!IS_NIL(p))
            {
                auto array = as<ObjArray>(p);
                if(array)
                {
                    for(int i = 0; i < array->size().to_int(); i++)
                    {
                        std::string name = array->index(i).to_string();
                        namedVariable(Token(TokenType::IDENTIFIER,name.c_str(), (int) name.size(),parser->current.line),false);
                        emitByte(OpCode::OP_DECORATOR);
                    }
                }
                else
                {
                    std::string name = p.to_string();
                    namedVariable(Token(TokenType::IDENTIFIER,name.c_str(), (int) name.size(),parser->current.line),false);
                    emitByte(OpCode::OP_DECORATOR);
                }
            }
        }
    }

}

void Compiler::method(OpCode op, bool async) 
{
    bool dtor = false;
    if(parser->match(TokenType::TILDE))
    {
        dtor = true;
    }
    parser->consume(TokenType::IDENTIFIER, "Expect method name.");

    std::string funName = parser->previous.str();

    cindex_t constant = 0;
    if(dtor)
    {
        std::string dtorname("~");
        dtorname += funName;
        Token token { TokenType::IDENTIFIER, dtorname.c_str(),(int)dtorname.size(),parser->previous.line };
        constant = identifierConstant(&token);
    }
    else
    {
        constant = identifierConstant(&parser->previous);
    }

    FunctionType typ = FunctionType::TYPE_METHOD;
    if( funName == std::string(currentClass->name.start,currentClass->name.length))
    {
        if(dtor)
        {
            typ = FunctionType::TYPE_DESTRUCTOR;
        }
        else
        {
            typ = FunctionType::TYPE_CONSTRUCTOR;
        }
    }
    else
    {
        if(dtor)
        {
            parser->errorAtCurrent("destructor must have class name.");
        }
    }
    parseFunction(typ,async);    
    emitBytes(op, constant);
}

cindex_t Compiler::argumentList() 
{
    uint8_t argCount = 0;
    if (!parser->check(TokenType::RIGHT_PAREN)) 
    {
        do 
        {
            expression();
            if (argCount == 255) 
            {
                parser->error("Can't have more than 255 arguments.");
            }            
            argCount++;
        } while (parser->match(TokenType::COMMA));
    }
    parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}


// expression parsers driven by pratt parser table

void Compiler::break_(bool /* canAssign */)
{
    if(function->breakes.empty())
    {
        parser->errorAtCurrent("break without loop.");
        return;
    }

    // check for exception handlers to unwind
    if(!function->exHandlers.empty())
    {
        int pos = (int)function->exHandlers.size()-1;
        if(pos>=0)
        {
            if(function->exHandlers[pos] >= (int) function->breakes.size())
            {
                parser->errorAtCurrent("cannot break out of try catch");
                return;                
            }
        }
    }

    int jmp = emitJump(OpCode::OP_JUMP);
    function->breakes.back().push_back(jmp);
}

void Compiler::continue_(bool /* canAssign */)
{
    if(function->loops.empty())
    {
        parser->errorAtCurrent("continue without loop.");
        return;
    }

    // unwind exception handlers, if any
    if(!function->exHandlers.empty())
    {
        int pos = (int) function->exHandlers.size()-1;
        if(pos>=0)
        {
            if(function->exHandlers[pos] >= (int) function->breakes.size())
            {
                parser->errorAtCurrent("cannot continue out of try catch");
                return;                
            }
        }
    }


    int jmp = emitJump(OpCode::OP_LOOP);
    function->loops.back().push_back(jmp);
}

void Compiler::variable(bool canAssign) 
{
    if( parser->current.type == TokenType::PLUS_PLUS ||
        parser->current.type == TokenType::MINUS_MINUS
    )
    {
        Token var = parser->previous;
        ResolvedVar rv = resolve(var);

        emitBytes(rv.getOp,(uint16_t)rv.arg); 
        emitBytes(rv.getOp,(uint16_t)rv.arg); // twice as we leave the first on the stack
        emitConstant(1);
        if(parser->current.type == TokenType::PLUS_PLUS)
        {
            emitByte(OpCode::OP_ADD);
        }
        else
        {
            emitByte(OpCode::OP_SUBTRACT);
        }
        emitBytes(rv.setOp,(uint16_t)rv.arg); 
        emitByte(OpCode::OP_POP);
        parser->advance();
        return;
    }

    namedVariable(parser->previous,canAssign);
}

void Compiler::dot(bool canAssign)
{
    Token before = parser->beforePrevious;
    std::string identifier = parser->current.str();
    if(identifier.starts_with("_"))
    {
        if(before.type != TokenType::THIS && before.type != TokenType::SUPER)
        {
            parser->errorAtCurrent("use of private _ method without this");
            return;
        }
    }
    parser->consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
    cindex_t name = identifierConstant(&parser->previous);

    if (canAssign && parser->match(TokenType::EQUAL)) 
    {
        expression();
        emitBytes(OpCode::OP_SET_PROPERTY, name);
    } 
    else if (parser->match(TokenType::LEFT_PAREN)) 
    {
        uint8_t argCount = (uint8_t) argumentList();
        emitBytes(OpCode::OP_INVOKE, name);
        emitByte(argCount);
    }
    else 
    {
        emitBytes(OpCode::OP_GET_PROPERTY, name);
    }
}

void Compiler::number(bool) 
{
    char c = *(parser->previous.start + 1);
    if( c == 'x' || c == 'X')
    {
        std::string s(parser->previous.start, parser->previous.length);
        size_t pos = s.find_first_not_of("0xX");
        if(pos!=std::string::npos)
        {
            s = s.substr(pos);
        }
        ptrdiff_t value = (int)strtoll(s.c_str(), NULL, 16);
        emitConstant(value);
    }
    else if(parser->previous.type == TokenType::INTEGER)
    {
        std::string s(parser->previous.start, parser->previous.length);
        ptrdiff_t value = strtoll(s.c_str(), NULL,10);
        emitConstant(value);
    }
    else
    {
        double value = strtod(parser->previous.start, NULL);
        emitConstant(value);
    }
}

void Compiler::literal(bool) 
{
    switch (parser->previous.type) 
    {
        case TokenType::FALSE: emitByte(OpCode::OP_FALSE); break;
        case TokenType::NIL: emitByte(OpCode::OP_NIL); break;
        case TokenType::TRUE: emitByte(OpCode::OP_TRUE); break;
        default: return; // Unreachable.
    }
}

void Compiler::string(bool) 
{
    std::string raw(parser->previous.start + 1, parser->previous.length - 2);
    std::string s = unescape(raw);
    emitConstant(new ObjString(vm, s.c_str(),s.size())	);
}


void Compiler::grouping(bool) 
{
    expression();
    parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
}

void Compiler::call(bool /* canAssign */)
{
    uint8_t argCount = (uint8_t) argumentList();
    emitBytes(OpCode::OP_CALL, argCount);
}

void Compiler::this_(bool /* canAssign */)
{
    
    if (currentClass == nullptr) 
    {
        parser->error("Can't use 'this' outside of a class.");
        return;
    } 
       
    variable(false);
}

void Compiler::address(bool /* canAssign */)
{
    Token var = parser->current;
    OpCode op;
    cindex_t arg = (cindex_t)resolveLocal(&var);
    if (arg != 65535) // -1 
    {
        op = OpCode::OP_GET_LOCAL_ADDR;
    } 
    else if ((arg = (cindex_t) resolveUpvalue(&var)) != 65535) // -1 
    {
        parser->errorAtCurrent("cannot take address of upvalue");
        return;
    }    
    else 
    {
        arg = identifierConstant(&var);            
        op = OpCode::OP_GET_GLOBAL_ADDR;
    }

    emitBytes(op,arg);
    parser->advance();
}

void Compiler::meta(bool /* canAssign */)
{
    parser->parsePrecedence(Precedence::PREC_PRIMARY);
    emitByte(OpCode::OP_GET_META);
}

void Compiler::super_(bool /* canAssign */)
{
    if (currentClass == NULL) 
    {
        parser->error("Can't use 'super' outside of a class.");
    } 
    else if (!currentClass->hasSuperclass) 
    {
        parser->error("Can't use 'super' in a class with no superclass.");
    }

    parser->consume(TokenType::DOT, "Expect '.' after 'super'.");
    parser->consume(TokenType::IDENTIFIER, "Expect superclass method name.");
    cindex_t name = identifierConstant(&parser->previous);

    namedVariable({ TokenType::THIS,  "this", 4, 0}, false);
    namedVariable({ TokenType::SUPER, "super", 5, 0}, false);

    emitBytes(OpCode::OP_GET_SUPER, name);    
}

void Compiler::arrayInit(bool /* canAssign */)
{
    int n = 0;
    while(!parser->match(TokenType::RIGHT_BRACKET))
    {
        expression();
        n++;
        parser->match(TokenType::COMMA);
    }
    emitBytes(OpCode::OP_ARRAY_INIT,(uint16_t)n);
}

void Compiler::mapInit(bool /* canAssign */)
{
    int n = 0;
    while(!parser->match(TokenType::RIGHT_BRACE))
    {
        expression();
        parser->consume(TokenType::COLON, "Expect : for map key-value pair.");
        expression();
        n++;
        parser->match(TokenType::COMMA);
    }
    emitBytes(OpCode::OP_MAP_INIT,(uint16_t)n);
}

void Compiler::arrayAccess(bool canAssign)
{
    expression();
    if(parser->match(TokenType::COLON))
    {
        if(parser->match(TokenType::RIGHT_BRACKET))
        {
            emitConstant(Value(-1));
            emitByte(OpCode::OP_ARRAY_SLICE);
            return;
        }
        expression();
        parser->consume(TokenType::RIGHT_BRACKET,"Expect closing ']'");
        emitByte(OpCode::OP_ARRAY_SLICE);
        return;
    }
    parser->consume(TokenType::RIGHT_BRACKET,"Expect closing ']'");
    if (canAssign && parser->match(TokenType::EQUAL)) 
    {
        expression();
        emitByte(OpCode::OP_ARRAY_SET);
    } 
    else 
    {
        emitByte(OpCode::OP_ARRAY_GET);
    }    
}

void Compiler::mapAccess(bool canAssign)
{
    expression();
    parser->consume(TokenType::RIGHT_BRACE,"Expect closing '}'");
    if (canAssign && parser->match(TokenType::EQUAL)) 
    {
        expression();
        emitByte(OpCode::OP_MAP_SET);
    } 
    else 
    {
        emitByte(OpCode::OP_MAP_GET);
    }    
}

void Compiler::funExpression(bool /* canAssign */)
{
    parseFunction(FunctionType::TYPE_FUNCTION,false);
}


void Compiler::and_(bool) 
{
    int endJump = emitJump(OpCode::OP_JUMP_IF_FALSE);

    emitByte(OpCode::OP_POP);
    parser->parsePrecedence(Precedence::PREC_AND);

    patchJump(endJump);
}

void Compiler::or_(bool) 
{
    int elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    int endJump = emitJump(OpCode::OP_JUMP);

    patchJump(elseJump);
    emitByte(OpCode::OP_POP);

    parser->parsePrecedence(Precedence::PREC_OR);
    patchJump(endJump);
}


void Compiler::unary(bool) 
{
    TokenType operatorType = parser->previous.type;

    switch (operatorType) 
    {
        case TokenType::PLUS_PLUS: 
        case TokenType::MINUS_MINUS: 
        {            
            Token var = parser->current;
            //unused: cindex_t global = 
            parseVariable("Expect variable name.");

            ResolvedVar rv = resolve(var);

            emitBytes(rv.getOp,(uint16_t)rv.arg); 
            emitConstant(Value(1));
            if(parser->current.type == TokenType::PLUS_PLUS)
            {
                emitByte(OpCode::OP_ADD);
            }
            else
            {
                emitByte(OpCode::OP_SUBTRACT);
            }
            emitBytes(rv.setOp,(uint16_t)rv.arg); 
            return;
            break;
        }
        default: break;
    }

    parser->parsePrecedence(Precedence::PREC_UNARY);

    switch (operatorType) 
    {
        case TokenType::BANG: emitByte(OpCode::OP_NOT); break;        
        case TokenType::MINUS: emitByte(OpCode::OP_NEGATE); break;
        case TokenType::TILDE: emitByte(OpCode::OP_BIN_NEGATE); break;
        case TokenType::CO_AWAIT: emitByte(OpCode::OP_CO_AWAIT); break;
        default: 
            return; // Unreachable.
    }
}

void Compiler::binary(bool) 
{
    TokenType operatorType = parser->previous.type;

    ParseRule* rule = parser->getRule(operatorType);

    parser->parsePrecedence((Precedence)( (uint8_t)(rule->precedence) + 1));

    switch (operatorType) 
        {
        case TokenType::BANG_EQUAL:    emitBytes(OpCode::OP_EQUAL, OpCode::OP_NOT); break;
        case TokenType::EQUAL_EQUAL:   emitByte(OpCode::OP_EQUAL); break;
        case TokenType::GREATER:       emitByte(OpCode::OP_GREATER); break;
        case TokenType::GREATER_EQUAL: emitBytes(OpCode::OP_LESS, OpCode::OP_NOT); break;
        case TokenType::LESS:          emitByte(OpCode::OP_LESS); break;
        case TokenType::LESS_EQUAL:    emitBytes(OpCode::OP_GREATER, OpCode::OP_NOT); break;        
        case TokenType::PLUS:          emitByte(OpCode::OP_ADD); break;
        case TokenType::MINUS:         emitByte(OpCode::OP_SUBTRACT); break;
        case TokenType::STAR:          emitByte(OpCode::OP_MULTIPLY); break;
        case TokenType::SLASH:         emitByte(OpCode::OP_DIVIDE); break;
        case TokenType::MODULO:        emitByte(OpCode::OP_MODULO); break;
        case TokenType::BIN_AND:       emitByte(OpCode::OP_BIN_AND); break;
        case TokenType::BIN_OR:        emitByte(OpCode::OP_BIN_OR); break;                
        case TokenType::SHIFT_LEFT:    emitByte(OpCode::OP_SHIFT_LEFT); break;                
        case TokenType::SHIFT_RIGHT:   emitByte(OpCode::OP_SHIFT_RIGHT); break;                
        case TokenType::ISA:           emitByte(OpCode::OP_ISA); break;                     
        case TokenType::PLUS_EQUAL: 
        {
            printf("HERE");
            break;
        }
        default: return; // Unreachable.
    }
}

// variable support

void Compiler::namedVariable(Token name,bool canAssign) 
{
    ResolvedVar rv = resolve(name);
    if (canAssign && parser->match(TokenType::EQUAL)) 
    {
        expression();
        emitBytes(rv.setOp, (uint16_t)rv.arg);
    } 
    else if (canAssign && parser->match(TokenType::PLUS_EQUAL)) 
    {
        emitBytes(rv.getOp, (uint16_t)rv.arg);
        expression();
        emitByte(OpCode::OP_ADD);
        emitBytes(rv.setOp, (uint16_t)rv.arg);
    } 
    else if (canAssign && parser->match(TokenType::MINUS_EQUAL)) 
    {
        emitBytes(rv.getOp, (uint16_t)rv.arg);
        expression();
        emitByte(OpCode::OP_SUBTRACT);
        emitBytes(rv.setOp, (uint16_t)rv.arg);
    } 
    else 
    {
        emitBytes(rv.getOp, (uint16_t)rv.arg);
    }
}


void Compiler::declareVariable(Token* name) 
{
    if (scopeDepth == 0) return; // var is global

    for (int i = (int) locals.size() - 1; i >= 0; i--) 
    {
        Local* local = &locals[i];
        if (local->depth != -1 && local->depth < scopeDepth) 
        {
            break; 
        }

        if(name->equals(local->name))
        {
            parser->error("Already a variable with this name in this scope.");
        }
    }    
    addLocal(*name);
}

void Compiler::markInitialized() 
{
    if (scopeDepth == 0) return; // var is global
    locals[locals.size() - 1].depth = scopeDepth;
}

void Compiler::defineVariable(cindex_t global) 
{
    if (scopeDepth > 0) 
    {
        markInitialized();
        return;
    }

    // global variable
    emitBytes(OpCode::OP_DEFINE_GLOBAL, global);
}

ResolvedVar Compiler::resolve(Token& var)
{
    OpCode getOp;
    OpCode setOp;
    cindex_t arg = (cindex_t) resolveLocal(&var);
    if (arg != 65535)  //-1
    {
        getOp = OpCode::OP_GET_LOCAL;
        setOp = OpCode::OP_SET_LOCAL;
    } 
    else if ((arg = (cindex_t) resolveUpvalue(&var)) != 65535) // -1 
    {
        getOp = OpCode::OP_GET_UPVALUE;
        setOp = OpCode::OP_SET_UPVALUE;
    }    
    else 
    {
        arg = identifierConstant(&var);            
        getOp = OpCode::OP_GET_GLOBAL;
        setOp = OpCode::OP_SET_GLOBAL;
    }
    return ResolvedVar {arg,getOp,setOp};
}


// local variables 

void Compiler::addLocal(Token name) 
{
    locals.push_back(Local{name,-1});
}

int Compiler::resolveLocal(Token* name) 
{
    for (int i = (int) locals.size() - 1; i >= 0; i--) 
    {
        Local* local = &locals[i];
        if(name->equals(local->name))
        {
            if (local->depth == -1) 
            {
                parser->error("Can't read local variable in its own initializer.");
            }            
            return i;
        }
    }

    return -1;
}

// upvalues

int Compiler::resolveUpvalue(Token* name) 
{
    if (enclosing == NULL) return -1;

    int local = enclosing->resolveLocal( name);
    if (local != -1) //  -1
    {
        enclosing->locals[local].isCaptured = true;
        return addUpvalue((cindex_t)local, true);
    }

    int upvalue = enclosing->resolveUpvalue(name);
    if (upvalue != -1) // -1 
    {
        return addUpvalue((cindex_t)upvalue, false);
    }

    return -1;
}

int Compiler::addUpvalue(cindex_t index, bool isLocal) 
{
    return (int) function->addUpvalue(*this, (uint8_t)index,isLocal);
}

// constants

cindex_t Compiler::identifierConstant(Token* name) 
{
    return makeConstant(new ObjString(vm, name->str()));
}

void Compiler::emitConstant(Value value) 
{
    emitBytes(OpCode::OP_CONSTANT, makeConstant(value));
}

cindex_t Compiler::makeConstant(Value value) 
{
    currentChunk()->constants.push_back(value);
    int constant = (int) currentChunk()->constants.size()-1;
    if (constant > UINT16_MAX) 
    {
        parser->error("Too many constants in one chunk.");
        return 0;
    }

    return (cindex_t)constant;
}

// scopes

void Compiler::beginScope() 
{
    scopeDepth++;
}

void Compiler::endScope() 
{
    scopeDepth--;

    while (locals.size() > 0 && locals[locals.size() - 1].depth > scopeDepth) 
    {
        if (locals[locals.size() - 1].isCaptured) 
        {
            emitByte(OpCode::OP_CLOSE_UPVALUE);
        } else 
        {
            emitByte(OpCode::OP_POP);
        }        
        locals.pop_back();
    }    
}

// compiler support

Chunk* Compiler::currentChunk() 
{
    return &function->chunk;    
}

ObjFunction* Compiler::endCompiler() 
{
    emitReturn();

#ifdef DEBUG_PRINT_CODE
    if (!parser->hadError) 
    {
        disassembleChunk( 
            *currentChunk(), 
            function->name() != NULL ? function->name()->toString().c_str() : "<script>"
        );
    }
#endif

    ObjFunction* fun = function;
    return fun;
}

// BYTE CODE emitters

void Compiler::emitByte(uint8_t byte) 
{
    currentChunk()->writeChunk(byte, parser->previous.line);
}

void Compiler::emitBytes(uint16_t s) 
{
    uint8_t byte1 = (s >> 8) & 0xff;
    uint8_t byte2 = s & 0xff;

    currentChunk()->writeChunk(byte1, parser->previous.line);
    currentChunk()->writeChunk(byte2, parser->previous.line);
}

void Compiler::emitByte(OpCode op) 
{
    currentChunk()->writeChunk(op, parser->previous.line);
}

void Compiler::emitBytes(OpCode op1, OpCode op2) 
{
    emitByte(op1);
    emitByte(op2);
}

void Compiler::emitBytes(OpCode op, uint8_t byte2) 
{
    emitByte(op);
    emitByte(byte2);
}

void Compiler::emitBytes(OpCode op, uint16_t s) 
{
    emitByte(op);

    uint8_t byte1 = (s >> 8) & 0xff;
    uint8_t byte2 = s & 0xff;
    emitBytes(byte1,byte2);
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2) 
{
    emitByte(byte1);
    emitByte(byte2);
}


int Compiler::emitTry()
{
    emitByte(OpCode::OP_TRY_BEGIN);
    emitByte((uint8_t)0xff);
    emitByte((uint8_t)0xff);
    emitByte((uint8_t)0xff);
    emitByte((uint8_t)0xff);
    return (int) currentChunk()->code.size() - 4;
}

void Compiler::emitEndTry()
{
    emitByte(OpCode::OP_TRY_END);
}


void Compiler::emitReturn() 
{
    for(size_t i = 0; i< function->exHandlers.size(); i++)
    {
        emitByte(OpCode::OP_TRY_END);
    }
    if (type == FunctionType::TYPE_CONSTRUCTOR) 
    {
        emitBytes(OpCode::OP_GET_LOCAL, (uint16_t)0);
    } 
    else 
    {
        emitByte(OpCode::OP_NIL);
    }
    emitByte(OpCode::OP_RETURN);
}

int Compiler::emitJump(OpCode instruction) 
{
    emitByte(instruction);
    emitByte((uint8_t)0xff);
    emitByte((uint8_t)0xff);
    return (int) currentChunk()->code.size() - 2;
}

void Compiler::patchJump(int offset) 
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = (int) currentChunk()->code.size() - offset - 2;

    if (jump > UINT16_MAX) 
    {
        parser->error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

void Compiler::patchTry(int offset) 
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = (int) currentChunk()->code.size() - offset - 4;

    if (jump > UINT16_MAX) 
    {
        parser->error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

void Compiler::patchTryFinal(int offset) 
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = (int) currentChunk()->code.size() - offset - 2;

    if (jump > UINT16_MAX) 
    {
        parser->error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

void Compiler::patchLoop(int offset, int dest)
{
    if (dest > UINT16_MAX) 
    {
        parser->error("Too much code to jump over.");
    }

    int target = offset - dest + 2;

    currentChunk()->code[offset] = (target >> 8) & 0xff;
    currentChunk()->code[offset + 1] = target & 0xff;
}

void Compiler::emitLoop(int loopStart) 
{
    emitByte(OpCode::OP_LOOP);

    size_t offset =  currentChunk()->code.size() - loopStart + 2;
    if (offset > UINT16_MAX) parser->error("Loop body too large.");

    emitBytes( (uint16_t)offset);
}


