#ifndef moc_compiler_h
#define moc_compiler_h

#include "object.h"
#include "parser.h"

/*
    the MOSER compiler
*/

class VM;

using cindex_t = uint16_t;

enum class FunctionType
{
  TYPE_FUNCTION,
  TYPE_METHOD,
  TYPE_CONSTRUCTOR,
  TYPE_DESTRUCTOR,
  TYPE_SCRIPT
};

struct Upvalue 
{
    uint16_t index = 0;
    bool isLocal = false;
};

struct Local
{
    Token name;
    int depth = 0;
    bool isCaptured = false;  
};

struct ClassCompiler 
{
    ClassCompiler* enclosing = nullptr;
    Token name;
    bool hasSuperclass = false;
};

struct ResolvedVar
{
    uint16_t arg = (uint16_t) - 1;
    OpCode getOp = OpCode::OP_GET_GLOBAL;
    OpCode setOp = OpCode::OP_SET_GLOBAL;
};


class Compiler
{
friend class Parser;
public:

    VM& vm;

    Compiler(VM&);
    Compiler(VM& ,Compiler* parent, FunctionType t, bool async);
    Compiler(VM& ,Compiler* parent, FunctionType t,Scanner* s, Parser* p, bool async);
    ~Compiler();

    Compiler* enclosing = nullptr;
    ObjFunction* function = nullptr;
    std::vector<Upvalue> upvalues;
    std::vector<Local> locals;
    Value metadata;
    int scopeDepth = 0;

    ObjFunction* compile(const char* file, std::string source);

    std::string filename;

private:

    FunctionType type = FunctionType::TYPE_SCRIPT;

    Scanner* scanner = nullptr;
    Parser* parser = nullptr;

    void declaration();
    void metaDeclaration();
    void statement();    
    void expressionStatement();
    void expression();

    void classDeclaration();
	void asyncDeclaration() ;	
    void funDeclaration();
    void varDeclaration();
    void externDeclaration();
    void nativeDeclaration(const std::string& lib);
    void callbackDeclaration();
    void structDeclaration();

    void tryStatement();
    void throwStatement();
    void printStatement();
    void returnStatement();
    void ifStatement();
    void whileStatement();    
    void doWhileStatement();    
    void forStatement();    
    void forOfStatement();
    void forInStatement();
    void deleteStatement();
    void switchStatement();

    void block();

    void variable(bool);
    void dot(bool);
    void number(bool);
    void literal(bool);    
    void string(bool);
    void grouping(bool);    
    void call(bool);
    void this_(bool);
    void address(bool);
    void meta(bool);
    void and_(bool);
    void or_(bool);
    void unary(bool);
    void binary(bool);
    void break_(bool);
    void continue_(bool);
    void super_(bool);
    void arrayInit(bool);
    void arrayAccess(bool);
    void mapInit(bool);
    void mapAccess(bool);
    void funExpression(bool);

    void emitByte(OpCode op);
    void emitByte(uint8_t byte);
    void emitBytes(uint16_t byte);
    void emitBytes(OpCode op, uint8_t byte2);
    void emitBytes(OpCode op, uint16_t byte2);
    void emitBytes(uint8_t byte1, uint8_t byte2);    
    void emitBytes(OpCode op1, OpCode op2);

    void emitReturn();      
    int  emitTry();      
    void emitEndTry();      
    int  emitJump(OpCode instruction);
    void emitLoop(int loopStart);    
    void emitConstant(Value value);    

    void patchJump(int offset);
    void patchTry(int offset);
    void patchTryFinal(int offset);
    void patchLoop(int offset, int dest);

    void parseFunction(FunctionType type, bool async);
    void method(OpCode op);
    cindex_t argumentList();    

    cindex_t parseVariable(const char* errorMessage);
    void namedVariable(Token name, bool canAssign);    
    void declareVariable(Token* name);    
    void defineVariable(cindex_t global);

    int addUpvalue(cindex_t index, bool isLocal); 
    void addLocal(Token name);
    int resolveLocal(Token* name);    
    int resolveUpvalue(Token* name);
    void markInitialized();    

    void beginScope();
    void endScope();

    cindex_t identifierConstant(Token* name);
    cindex_t makeConstant(Value value);

    Chunk* currentChunk();
    ObjFunction* endCompiler();

    ResolvedVar resolve(Token& var);

};



#endif