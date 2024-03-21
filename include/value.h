#ifndef moc_value_h
#define moc_value_h

#include "common.h"

/*
    All MOSER variables and constants are values
*/

class VM;

enum class ValueType : uint8_t 
{
    VAL_BOOL,
    VAL_NIL, 
    VAL_INT,
    VAL_NUMBER,
    VAL_OBJ
};

struct Value 
{
	Value() : type(ValueType::VAL_NIL), as(0) {}
	Value(bool b) : type(ValueType::VAL_BOOL), as(b) {}
	Value(int i) : type(ValueType::VAL_INT), as(i) {}
	Value(size_t i) : type(ValueType::VAL_INT), as(i) {}
	Value(ptrdiff_t i) : type(ValueType::VAL_INT), as(i) {}
	Value(double d) : type(ValueType::VAL_NUMBER), as(d) {}
	Value(Obj* obj) : type(ValueType::VAL_OBJ), as(obj) {}

	Value(const Value& rhs)
		: type(rhs.type), as(rhs.as)
	{
	
	}

    Value& operator=(const Value& rhs);

    ValueType type;
    union val
    {
        val() : obj(0) {}
        val(bool b) : boolean(b) {}
        val(ptrdiff_t i) : integer(i) {}
        val(size_t i) : integer(i) {}
        val(int i) : integer(i) {}
        val(double d) : number(d) {}
        val(Obj* o) : obj(o) {}
        bool boolean;
        ptrdiff_t integer;
        double number;
        Obj* obj;
    } as; 

    bool isEqual(const Value& rhs) const;
    bool equals(bool rhs) const;
    bool equals(ptrdiff_t rhs) const;
    bool equals(double rhs) const;
    bool equals(const Obj* rhs) const;    

    bool isFalsey() const;

    std::string to_string() const;
    ptrdiff_t to_int() const;
    double to_double() const;
    void print() const;

    Value pointer( VM& );
};

inline bool IS_NIL   (const Value& val) { return val.type == ValueType::VAL_NIL; }
inline bool IS_BOOL  (const Value& val) { return val.type == ValueType::VAL_BOOL; }
inline bool IS_INT   (const Value& val) { return val.type == ValueType::VAL_INT; }
inline bool IS_NUMBER(const Value& val) { return val.type == ValueType::VAL_NUMBER; }
inline bool IS_OBJ   (const Value& val) { return val.type == ValueType::VAL_OBJ; }

#define NIL_VAL           Value{}


#endif