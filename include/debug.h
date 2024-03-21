#ifndef moc_debug_h
#define moc_debug_h

#include <cstddef>
#include <cstdio>

class Chunk;

void disassembleChunk(Chunk& chunk, const char* name);
size_t disassembleInstruction(Chunk& chunk, size_t offset);


#ifdef DEBUG_TRACE_EXECUTION
#define DEBUG_TRACE_EXECUTION_PREAMBLE \
    printf("\n=== run ===\n"); 
#else
#define DEBUG_TRACE_EXECUTION_PREAMBLE 
#endif

#ifdef DEBUG_TRACE_EXECUTION
#define DEBUG_TRACE_EXECUTION_TRACE \
        if(!stack.empty()) \
        {\
            printf("e: %zu", exHandlers.size());\
            printf("      ");\
            for (Value* slot = &stack[0]; slot - &stack[0] < stack.size(); slot++) \
            {\
                printf("[ ");\
                slot->print();\
                printf(" ]");\
            }\
            printf("\n");\
        }\
\
        disassembleInstruction(\
            frame->closure->function->chunk,\
            (int)(frame->ip - &frame->closure->function->chunk.code[0]));        
#else
#define DEBUG_TRACE_EXECUTION_TRACE 
#endif

#endif