#ifndef moc_chunk_h
#define moc_chunk_h

#include "common.h"
#include "value.h"

/* the available VMs OpCodes */

enum class OpCode : uint8_t
{
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,    
    OP_POP,    
    OP_GET_LOCAL,
    OP_GET_LOCAL_ADDR,
    OP_GET_META,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,    
    OP_GET_GLOBAL_ADDR,    
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,    
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,    
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,    
    OP_BIN_AND,
    OP_BIN_OR,
    OP_NOT,    
    OP_NEGATE,
    OP_BIN_NEGATE,
    OP_MODULO,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,    
    OP_CLASS,
    OP_ARRAY_INIT,
    OP_ARRAY_GET,
    OP_ARRAY_SET,
    OP_ARRAY_SLICE,
    OP_MAP_INIT,
    OP_MAP_GET,
    OP_MAP_SET,
    OP_DELETE,
    OP_INHERIT,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_METHOD,
    OP_INVOKE,
    OP_GET_SUPER,
    OP_TRY_BEGIN,
    OP_TRY_END,
    OP_THROW,
    OP_FINALLY,
    OP_SET_META,
	OP_SHIFT_LEFT,
	OP_SHIFT_RIGHT,
    OP_RETURN,
    OP_ISA,
    OP_GETTER,
    OP_SETTER,
    OP_DUP,
    OP_DECORATOR,
    OP_STATIC_METHOD,
	OP_CO_AWAIT,
};

/*
    Chunk is the basic building block for storing bytecode.
    it stores the bytecode itself and any constants the code
    refers to, and additionally debugy info like line of 
    (source-) code and the filename the bytecode was read from,
    if any.
*/

class Chunk 
{
public:

    Chunk();
    ~Chunk();

    void writeChunk(uint8_t byte, int line);
    void writeChunk(OpCode op, int line);

    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;
    std::string filename;
};

#endif
