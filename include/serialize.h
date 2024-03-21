#ifndef moc_serialize_h
#define moc_serialize_h

#include "object.h"
#include <sstream>

/*
    MOSER bytecode serialization / deserialization
*/


// serialize

std::ostream& serialize(std::ostream& os, ObjFunction& s);

// deserialize

std::istream& deserialize(VM& vm, std::istream& is, ObjFunction** fun);


#endif 