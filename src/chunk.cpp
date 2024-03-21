#include "chunk.h"


Chunk::Chunk()
{}

Chunk::~Chunk()
{
    code.clear();
}


void Chunk::writeChunk(uint8_t byte, int line) 
{
    code.push_back(byte);
    lines.push_back(line);
}

void Chunk::writeChunk(OpCode op,int line) 
{
    code.push_back(static_cast<uint8_t>(op));
    lines.push_back(line);
}
