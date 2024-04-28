#include "pch.h"
#include "debug.h"
#include "value.h"
#include "object.h"
#include "chunk.h"

void disassembleChunk(Chunk& chunk, const char* name)
{
    printf("== %s ==\n", name);

    for(size_t offset = 0; offset < chunk.code.size(); )
    {
        offset = disassembleInstruction(chunk,offset);
    }
}

static size_t simpleInstruction(const char* name, size_t offset)
{
    printf("%s\n", name);
    return offset + 1;
}



static size_t constantInstruction(const char* name, Chunk& chunk, size_t offset)
{
    uint16_t constant = read_short(&(chunk.code[offset + 1]));
    printf("%-16s %4d '", name, constant);
    chunk.constants[constant].print();
    printf("'\n");
    return offset + 3;
}

static size_t shortInstruction(const char* name, Chunk& chunk, size_t offset)
{
    uint16_t slot = read_short( &(chunk.code[offset+1]) );
    printf("%-16s %4d\n", name, slot);
    return offset + 3; 
}

static size_t byteInstruction(const char* name, Chunk& chunk, size_t offset)
{
    uint8_t slot = chunk.code[offset+1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2; 
}

static size_t jumpInstruction(const char* name, int sign, Chunk& chunk, size_t offset)
{
    uint16_t jump = (uint16_t)(chunk.code[offset + 1] << 8);
    jump |= chunk.code[offset + 2];
    printf("%-16s %4zd -> %zd\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static size_t tryInstruction(const char* name, int sign, Chunk& chunk, size_t offset)
{
    uint16_t jump = (uint16_t)(chunk.code[offset + 1] << 8);
    jump |= chunk.code[offset + 2];
    printf("%-16s %4zd -> %zd\n", name, offset,offset + 5 + sign * jump);

    jump = (uint16_t)(chunk.code[offset + 3] << 8);
    jump |= chunk.code[offset + 4];
    printf("      final: %4zd -> %zd\n", offset, offset + 5 + sign * jump);

    return offset + 5;
}

static size_t invokeInstruction(const char* name, Chunk& chunk, size_t offset)
{
    uint16_t constant = read_short( &(chunk.code[offset+1]) );
    uint8_t argCount = chunk.code[offset+3];

    printf("%-16s (%d args) %4d '", name, argCount, constant);
    chunk.constants[constant].print();
    printf("'\n");
    return offset + 4;
}

static size_t arrayInitInstruction(const char* name, Chunk& chunk, size_t offset)
{
    uint16_t n = read_short( &(chunk.code[offset+1]) );
    printf("%-16s (%d args) \n", name, n);
    return offset + 3;
}

size_t disassembleInstruction(Chunk& chunk, size_t offset)
{
    printf("%04d ", (int) offset);

    if (offset > 0 && chunk.lines[offset] == chunk.lines[offset - 1]) 
    {
        printf("   | ");
    } else 
    {
        printf("%4d ", chunk.lines[offset]);
    }  

  OpCode instruction = static_cast<OpCode>(chunk.code[offset]);
  switch (instruction) 
  {
    case OpCode::OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);
    case OpCode::OP_NIL:
      return simpleInstruction("OP_NIL", offset);
    case OpCode::OP_DECORATOR:
      return simpleInstruction("OP_DECORATOR", offset);
    case OpCode::OP_TRUE:
      return simpleInstruction("OP_TRUE", offset);
    case OpCode::OP_FALSE:
      return simpleInstruction("OP_FALSE", offset);
    case OpCode::OP_POP:
      return simpleInstruction("OP_POP", offset);      
    case OpCode::OP_GET_LOCAL:
      return shortInstruction("OP_GET_LOCAL", chunk, offset);
    case OpCode::OP_GET_LOCAL_ADDR:
      return shortInstruction("OP_GET_LOCAL_ADDR", chunk, offset);
    case OpCode::OP_GET_META:
      return simpleInstruction("OP_GET_META", offset);
    case OpCode::OP_SET_LOCAL:
      return shortInstruction("OP_SET_LOCAL", chunk, offset);      
    case OpCode::OP_GET_GLOBAL:
      return constantInstruction("OP_GET_GLOBAL", chunk, offset);      
    case OpCode::OP_GET_GLOBAL_ADDR:
      return constantInstruction("OP_GET_GLOBAL_ADDR", chunk, offset);      
    case OpCode::OP_DEFINE_GLOBAL:  
      return constantInstruction("OP_DEFINE_GLOBAL", chunk,offset);    
    case OpCode::OP_SET_GLOBAL:
      return constantInstruction("OP_SET_GLOBAL", chunk, offset);   
    case OpCode::OP_GET_UPVALUE:
      return shortInstruction("OP_GET_UPVALUE", chunk, offset);
    case OpCode::OP_SET_UPVALUE:
      return shortInstruction("OP_SET_UPVALUE", chunk, offset);           
    case OpCode::OP_EQUAL:
      return simpleInstruction("OP_EQUAL", offset);
    case OpCode::OP_GREATER:
      return simpleInstruction("OP_GREATER", offset);
    case OpCode::OP_LESS:
      return simpleInstruction("OP_LESS", offset);      
    case OpCode::OP_ISA:
      return simpleInstruction("OP_ISA", offset);
    case OpCode::OP_ADD:
      return simpleInstruction("OP_ADD", offset);
    case OpCode::OP_BIN_AND:
      return simpleInstruction("OP_BIN_AND", offset);
    case OpCode::OP_BIN_OR:
      return simpleInstruction("OP_BIN_OR", offset);
    case OpCode::OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
    case OpCode::OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
    case OpCode::OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);      
    case OpCode::OP_NOT:
      return simpleInstruction("OP_NOT", offset);      
    case OpCode::OP_MODULO:
      return simpleInstruction("OP_MODULO", offset);     
    case OpCode::OP_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);     
    case OpCode::OP_BIN_NEGATE:
      return simpleInstruction("OP_BIN_NEGATE", offset);     
    case OpCode::OP_PRINT:
      return simpleInstruction("OP_PRINT", offset);    
    case OpCode::OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OpCode::OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);        
    case OpCode::OP_LOOP:
      return jumpInstruction("OP_LOOP", -1, chunk, offset);  
    case OpCode::OP_CALL:
      return byteInstruction("OP_CALL", chunk, offset);  
    case OpCode::OP_CLOSURE: 
    {
        offset++;
        uint16_t constant = read_short( &(chunk.code[offset]) );
        offset+=2;

//        uint8_t constant = chunk.code[offset++];
        printf("%-16s %4d ", "OP_CLOSURE", constant);
        chunk.constants[constant].print();
        printf("\n");

        ObjFunction* function = as<ObjFunction>(chunk.constants[constant]);
        for (int j = 0; j < function->upvalueCount(); j++) 
        {
          int isLocal = chunk.code[offset++];
          uint16_t index = read_short( &(chunk.code[offset]) );          
          offset+=2;
//          int index = chunk.code[offset++];
          printf("%04zd      |                     %s %d\n",
               offset - 2, isLocal ? "local" : "upvalue", index);
        }        
        return offset;
    }               
    case OpCode::OP_DUP:
      return simpleInstruction("OP_DUP", offset);    
    case OpCode::OP_CLOSE_UPVALUE:
      return simpleInstruction("OP_CLOSE_UPVALUE", offset);    
    case OpCode::OP_CLASS:
      return constantInstruction("OP_CLASS", chunk, offset);    
    case OpCode::OP_SET_META:
      return constantInstruction("OP_SET_META", chunk, offset);    
    case OpCode::OP_INHERIT:
      return simpleInstruction("OP_INHERIT", offset);      
    case OpCode::OP_GET_PROPERTY:
      return constantInstruction("OP_GET_PROPERTY", chunk, offset);
    case OpCode::OP_SET_PROPERTY:
      return constantInstruction("OP_SET_PROPERTY", chunk, offset);   
    case OpCode::OP_METHOD:
      return constantInstruction("OP_METHOD", chunk, offset);           
    case OpCode::OP_STATIC_METHOD:
      return constantInstruction("OP__STATIC_METHOD", chunk, offset);           
    case OpCode::OP_SETTER:
      return constantInstruction("OP_SETTER", chunk, offset);           
    case OpCode::OP_GETTER:
      return constantInstruction("OP_GETTER", chunk, offset);           
    case OpCode::OP_INVOKE:
      return invokeInstruction("OP_INVOKE", chunk, offset);      
    case OpCode::OP_GET_SUPER:
      return constantInstruction("OP_GET_SUPER", chunk, offset);      
    case OpCode::OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    case OpCode::OP_TRY_BEGIN:
      return tryInstruction("OP_TRY_BEGIN",1,chunk, offset);
    case OpCode::OP_TRY_END:
      return simpleInstruction("OP_TRY_END", offset);
    case OpCode::OP_THROW:
      return simpleInstruction("OP_THROW", offset);
    case OpCode::OP_FINALLY:
      return simpleInstruction("OP_FINALLY", offset);
    case OpCode::OP_ARRAY_INIT:
      return arrayInitInstruction("OP_ARRAY_INIT",chunk, offset);
    case OpCode::OP_DELETE:
      return simpleInstruction("OP_DELETE", offset);
    case OpCode::OP_ARRAY_GET:
      return simpleInstruction("OP_ARRAY_GET", offset);
    case OpCode::OP_ARRAY_SLICE:
      return simpleInstruction("OP_ARRAY_SLICE", offset);
    case OpCode::OP_ARRAY_SET:
      return simpleInstruction("OP_ARRAY_SET", offset);
    case OpCode::OP_MAP_INIT:
      return arrayInitInstruction("OP_MAP_INIT",chunk, offset);
    case OpCode::OP_MAP_GET:
      return simpleInstruction("OP_MAP_GET", offset);
    case OpCode::OP_MAP_SET:
      return simpleInstruction("OP_MAP_SET", offset);
    default:
      printf("Unknown opcode %d\n", (int)instruction);
      return offset + 1;
  }
//  return 0;
}

