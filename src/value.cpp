#include "pch.h"
#include "value.h"
#include "object.h"
#include "foreign.h"

#include <sstream>

std::string Value::to_string() const
{
    switch(type)
    {
        case ValueType::VAL_BOOL :
        {
            return as.boolean ? "true" : "false";
            break;
        }
        case ValueType::VAL_NIL :
        {
            return "nil";
            break;
        }
        case ValueType::VAL_INT :
        {
            std::ostringstream oss;
            oss << as.integer;
            return oss.str();
            break;
        }
        case ValueType::VAL_NUMBER :
        {
            std::ostringstream oss;
            oss << as.number;
            return oss.str();
            break;
        }
        case ValueType::VAL_OBJ :
        {
            if(as.obj == nullptr) return "";
            return as.obj->toString();
            break;
        }
    }
    return "<undef>";
}

Value& Value::operator=(const Value& rhs)
{
    if( this == &rhs) return *this;

    type = rhs.type;
    switch(type)
    {
        case ValueType::VAL_BOOL :
        {
            as.boolean  = rhs.as.boolean;
            break;
        }
        case ValueType::VAL_NIL :
        {
            as.integer = 0;
            break;
        }
        case ValueType::VAL_INT :
        {
            as.integer = rhs.as.integer;
            break;
        }
        case ValueType::VAL_NUMBER :
        {
            as.number = rhs.as.number;
            break;
        }
        case ValueType::VAL_OBJ :
        {
            as.obj = rhs.as.obj;
            break;
        }
    }
    return *this;
}


ptrdiff_t Value::to_int() const
{
    switch(type)
    {
        case ValueType::VAL_BOOL :
        {
            return as.boolean ? 1 : 0;
            break;
        }
        case ValueType::VAL_NIL :
        {
            return 0;
            break;
        }
        case ValueType::VAL_INT :
        {
            return as.integer;
            break;
        }
        case ValueType::VAL_NUMBER :
        {
            ptrdiff_t i = (ptrdiff_t)as.number;
            return i;
            break;
        }
        case ValueType::VAL_OBJ :
        {
            auto buf = ::as<ObjBuffer>(as.obj);
            if( buf )
            {
                return buf->size();
            }

            std::string s = to_string();
            std::istringstream iss(s);
            ptrdiff_t i = 0;
            iss >> i;
            return i;
            break;
        }
    }
    return 0;
}

double Value::to_double() const
{
    switch(type)
    {
        case ValueType::VAL_BOOL :
        {
            return as.boolean ? 1 : 0;
            break;
        }
        case ValueType::VAL_NIL :
        {
            return 0;
            break;
        }
        case ValueType::VAL_INT :
        {
            double d = (double)as.integer;
            return d;
            break;
        }
        case ValueType::VAL_NUMBER :
        {
            return as.number;
            break;
        }
        case ValueType::VAL_OBJ :
        {
            auto buf = ::as<ObjBuffer>(as.obj);
            if( buf )
            {
                return (double)buf->size();
            }

            std::string s = to_string();
            std::istringstream iss(s);
            double d = 0;
            iss >> d;
            return d;
            break;
        }
    }
    return 0;
}


bool Value::isEqual(const Value& rhs) const
{
    switch (rhs.type) 
    {
        case ValueType::VAL_BOOL:   return equals(rhs.as.boolean);
        case ValueType::VAL_NIL:    return IS_NIL(*this);
        case ValueType::VAL_INT:    return equals(rhs.as.integer);
        case ValueType::VAL_NUMBER: return equals(rhs.as.number);
        case ValueType::VAL_OBJ: 
        {
            if(isa<ObjString>(rhs.as.obj))
            {
                std::string s1 = to_string();
                std::string s2 = rhs.to_string();
                return s1 == s2;
            }
            return as.obj == rhs.as.obj;
        }       
         
        default:         return false; // Unreachable.
    }

}

bool Value::equals(bool rhs) const
{
    switch (type) 
    {
        case ValueType::VAL_BOOL:   return as.boolean == rhs;
        case ValueType::VAL_NIL:    return !rhs;
        case ValueType::VAL_INT:    return (bool)(as.integer) == rhs;
        case ValueType::VAL_NUMBER: return (bool)(as.number) == rhs;
        case ValueType::VAL_OBJ: 
        {            
            std::string s = to_string();
            return s == (rhs ? "true" : "false");
        }       
         
        default:         return false; // Unreachable.
    }
}

bool Value::equals(ptrdiff_t rhs) const // where is this called?
{
    switch (type) 
    {
        case ValueType::VAL_BOOL:   return as.boolean == (bool)rhs;
        case ValueType::VAL_NIL:    return !rhs;
        case ValueType::VAL_INT:    return as.integer == rhs;
        case ValueType::VAL_NUMBER: return (ptrdiff_t)(as.number) == rhs;
        case ValueType::VAL_OBJ: 
        {            
            std::string s = to_string();
            std::istringstream iss(s);
            ptrdiff_t i = 0;
            iss >> i;
            return i == rhs;
        }       
         
        default:         return false; // Unreachable.
    }
}

bool Value::equals(double rhs) const
{
    switch (type) 
    {
        case ValueType::VAL_BOOL:   return as.boolean == (bool)rhs;
        case ValueType::VAL_NIL:    return !rhs;
        case ValueType::VAL_INT:    return (double)(as.integer) == rhs;
        case ValueType::VAL_NUMBER: return as.number == rhs;
        case ValueType::VAL_OBJ: 
        {            
            std::string s = to_string();
            std::istringstream iss(s);
            double d = 0;
            iss >> d;
            return d == rhs;
        }       
         
        default:         return false; // Unreachable.
    }
}

bool Value::equals(const Obj* rhs) const
{
    if( type != ValueType::VAL_OBJ) return false;

    auto lhstr = ::as<ObjString>(as.obj);
    auto rhstr = ::as<ObjString>(rhs);
    if(lhstr && rhstr)
    {
        return lhstr->toString() == rhstr->toString();
    }
    return as.obj == rhs;
}


bool Value::isFalsey() const
{
    if(IS_INT(*this))
    {
        return !as.integer;
    }
    if(IS_NUMBER(*this))
    {
        return !as.number;
    }
    bool r = IS_NIL(*this) || (IS_BOOL(*this) && !as.boolean);
    return r;
}

void Value::print() const
{
    switch (type) 
    {
        case ValueType::VAL_BOOL:
            printf(this->as.boolean ? "true" : "false");
            break;
        case ValueType::VAL_NIL: printf("nil"); break;
        case ValueType::VAL_INT: 
        {
            ptrdiff_t val = this->as.integer;
            ptrdiff_t i = val;
            printf("%ti", i); 
            break;
        } 
        case ValueType::VAL_NUMBER:
        {
            double val = this->as.number;
            printf("%lf", val); 
            break;
        } 
        case ValueType::VAL_OBJ:
        {
            std::string s = as.obj->toString();
            printf("%s", s.c_str());
            break;
        }
    }
}

Value Value::pointer(VM& vm)
{
    switch (type) 
    {
        case ValueType::VAL_NIL: return new ObjPointer(vm);            
        case ValueType::VAL_BOOL: return new ObjPointer(vm,&as.boolean);
        case ValueType::VAL_INT: return new ObjPointer(vm,&as.integer);
        case ValueType::VAL_NUMBER: return new ObjPointer(vm,&as.number);
        case ValueType::VAL_OBJ:
        {
            auto ptr = dynamic_cast<ObjPointer*>(as.obj);
            if(ptr)
            {
                return new ObjPointer(vm,ptr->addressOf());
            }
            return new ObjPointer(vm,as.obj->pointer());
        }
    }
    return new ObjPointer(vm);
}
