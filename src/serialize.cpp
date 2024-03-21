#include "serialize.h"
#include "foreign.h"

enum ObjType
{
    O_STR,
    O_FUN,
    O_NATIVE,
    O_VARIADIC,
    O_STRUCT,
    O_CLASS,
    O_MAP,
    O_ARRAY,
    O_REGEX,
    O_DECORATOR,
    O_PROXY,
    O_CALLBACK
};

std::ostream& serialize(std::ostream& os, Value& v);
std::istream& deserialize(VM& vm, std::istream& is, Value& v);
std::ostream& serialize(std::ostream& os, std::vector<Value>& v);
std::istream& deserialize (VM& vm, std::istream& is, std::vector<Value>& v);

template<class T>
static void write(std::ostream& os, T& t)
{
    os.write( (char*)&t, sizeof(T));
}

template<class T>
static void read(std::istream& is, T& t)
{
    is.read( (char*)&t, sizeof(T));
}

std::ostream& serialize(std::ostream& os, const std::string& s)
{
    size_t len = s.size();
    write(os,len);
    os.write(s.data(),len);
    return os;
}

static std::istream& deserialize( VM&, std::istream& is, std::string& s)
{
    size_t len = 0;
    read(is,len);

    if(len>0)
    {
        std::vector<char> buf(len,0);
        is.read(&buf[0],len);
        s = std::string(&buf[0],len);
    }
    else
    {
        s = std::string();
    }
    return is;
}

std::ostream& serialize(std::ostream& os, std::vector<std::string>& v)
{
    size_t len = v.size();
    write(os,len);
    for(auto& it : v)
    {
        serialize(os,it);
    }
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, std::vector<std::string>& v)
{
    v.clear();
    size_t len = 0;
    read(is,len);
    for(size_t i = 0; i < len; i++)
    {
        std::string s;
        deserialize(vm, is,s);
        v.push_back(s);
    }
    return is;
}


static std::ostream& serialize(std::ostream& os, ObjString* s)
{
    if(s == 0)
    {
        serialize(os,"");
        Value nil = NIL_VAL;
        serialize(os, nil);
    }
    else
    {
        serialize(os,s->toString());
        serialize(os,s->metadata );
    }
    
    return os;
}

std::istream& deserialize( VM& vm, std::istream& is, ObjString** result)
{
    size_t len = 0;
    read(is,len);

    if(len>0)
    {
        std::vector<char> buf(len,0);
        is.read(&buf[0],len);
        *result = new ObjString(vm, &buf[0],len);
        deserialize(vm, is, (*result)->metadata);
    }
    else
    {
        *result = new ObjString(vm, "", 0);
        deserialize( vm, is, (*result)->metadata );
    }
    return is;
}

std::ostream& serialize(std::ostream& os, ObjForeignFunction* fun)
{
    serialize(os,fun->lib);    
    serialize(os,fun->retType);    
    serialize(os,fun->name);
    serialize(os, fun->args);
    serialize(os, fun->metadata);
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, ObjForeignFunction** fun)
{
    std::string lib;
    std::string retType;
    std::string name;
    deserialize(vm, is,lib);    
    deserialize(vm, is,retType);    
    deserialize(vm, is,name);    

    std::vector<std::string> args;
    deserialize(vm, is, args);
    *fun = new ObjForeignFunction(vm,lib,retType,name,args);
    deserialize(vm,is, (*fun)->metadata);

    return is;
}


std::ostream& serialize(std::ostream& os, ObjFunctionPtr* fun)
{
    serialize(os,fun->retType);    
    serialize(os, fun->args);
    serialize(os, fun->metadata);
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, ObjFunctionPtr** fun)
{
    std::string retType;
    deserialize(vm, is,retType);    

    std::vector<std::string> args;
    deserialize(vm, is, args);
    *fun = new ObjFunctionPtr(vm,0,retType,args);
    deserialize(vm,is, (*fun)->metadata);

    return is;
}

std::ostream& serialize(std::ostream& os, ObjCallbackType* fun)
{
    serialize(os, fun->name_);
    serialize(os, fun->retType_);
    serialize(os,fun->args_);
    serialize(os,fun->metadata);
    return os;
}

std::istream& deserialize( VM& vm, std::istream& is, ObjCallbackType** fun)
{
    std::string name;
    deserialize(vm, is, name);

    std::string retType;
    deserialize(vm, is, retType);

    std::vector<std::string> args;
    deserialize(vm, is, args);
    *fun = new ObjCallbackType( vm, name, retType, args);
    deserialize(vm, is, (*fun)->metadata );

    return is;
}


std::ostream& serialize(std::ostream& os, ObjVariadicForeignFunction* fun)
{
    serialize(os,fun->lib);    
    serialize(os,fun->retType);    
    serialize(os,fun->name);
    serialize(os,fun->args);
    serialize(os,fun->metadata);
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, ObjVariadicForeignFunction** fun)
{
    std::string lib;
    std::string retType;
    std::string name;
    deserialize(vm,is,lib);    
    deserialize(vm,is,retType);    
    deserialize(vm,is,name);    

    std::vector<std::string> args;
    deserialize(vm, is, args);

    *fun = new ObjVariadicForeignFunction(vm,lib,retType,name,args);
    deserialize(vm, is,  (*fun)->metadata );
    return is;
}

std::ostream& serialize(std::ostream& os, ObjStruct* s)
{
    serialize(os, s->name_);
    size_t len = s->members_.size();
    write(os,len);
    for(auto& it : s->members_)
    {
        serialize(os,it.first);
        serialize(os,it.second);
    }
    serialize(os, s->metadata);
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, ObjStruct** s)
{
    std::string name;
    deserialize(vm, is, name);
    size_t len = 0;
    read(is,len);

    std::vector<std::pair<std::string,std::string>> v;
    for(size_t i = 0; i < len; i++)
    {
        std::string n;
        deserialize(vm,is,n);
        std::string t;
        deserialize(vm,is,t);
        v.push_back({n,t});
    }

    if (name.empty())
    {
        *s = new ObjStruct(vm,v);
    }
    else
    {
        *s = new ObjStruct(vm,name,v);
    }
    deserialize(vm, is, (*s)->metadata );
    return is;
}

std::ostream& serialize(std::ostream& os, std::unordered_map<std::string,Value>& m)
{
    size_t s = m.size();
    write(os, s);
    for (auto& it : m)
    {
        serialize(os, it.first);
        serialize(os, it.second);
    }
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, std::unordered_map<std::string, Value>& m)
{
    size_t s = 0;
    read(is, s);
    for (size_t i = 0; i < s; i++)
    {
        std::string key;
        deserialize(vm, is, key);
        Value v;
        deserialize(vm, is, v);
        m[key] = v;
    }
    return is;
}

std::ostream& serialize(std::ostream& os, ObjClass* clazz)
{
    serialize( os, clazz->name);
    if (clazz->super)
    {
        os << true;
        serialize(os, clazz->super);
    }
    else
    {
        os << false;
    }
    serialize(os, clazz->fields);
    serialize(os, clazz->getters);
    serialize(os, clazz->setters);
    serialize(os, clazz->methods);
    serialize(os, clazz->metadata);
    return os;
}


std::istream& deserialize(VM& vm, std::istream& is, ObjClass** clazz)
{
    ObjString* name = nullptr;
    deserialize(vm, is, &name);
    *clazz = new ObjClass(vm, name);

    bool hasSuper = false;
    is >> hasSuper;

    if (hasSuper)
    {
        ObjClass* super = nullptr;
        deserialize(vm, is, &super);
        (*clazz)->inherit(super);
    }

    deserialize(vm, is, (*clazz)->fields);
    deserialize(vm, is, (*clazz)->getters);
    deserialize(vm, is, (*clazz)->setters);
    deserialize(vm, is, (*clazz)->methods);
    deserialize(vm, is, (*clazz)->metadata);
    return is;
}

std::ostream& serialize(std::ostream& os, ObjMap* map)
{
    serialize(os, map->elements);
    serialize(os, map->metadata);
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, ObjMap** map)
{
    *map = new ObjMap(vm);

    deserialize(vm,is, (*map)->elements);
    deserialize(vm, is, (*map)->metadata);
    return is;
}


std::ostream& serialize(std::ostream& os, ObjArray* a)
{
    serialize(os, a->values);
    serialize(os, a->metadata);
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, ObjArray** a)
{
    *a = new ObjArray(vm);

    deserialize(vm, is, (*a)->values );
    deserialize(vm, is, (*a)->metadata);
    return is;
}


std::ostream& serialize(std::ostream& os, ObjRegex* a)
{
    serialize(os, a->options_);
    serialize(os, a->value);
    serialize(os, a->metadata);
    return os;
}


std::istream& deserialize(VM& vm, std::istream& is, ObjRegex** r)
{
    std::string options;
    deserialize(vm, is, options);
    std::string value;
    deserialize(vm, is, value);

    *r = new ObjRegex(vm,value,options);
    deserialize(vm, is,  (*r)->metadata);
    return is;
}



std::ostream& serialize(std::ostream& os, ObjDecorator* d)
{
    serialize(os, d->name);
    serialize(os, d->klass);
    serialize(os, d->fields);
    serialize(os, d->metadata);
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, ObjDecorator** d)
{
    *d = new ObjDecorator(vm);
    deserialize(vm, is, (*d)->name);
    deserialize(vm, is, &(*d)->klass);
    deserialize(vm, is, (*d)->fields);
    deserialize(vm, is, (*d)->metadata);
    return is;
}

std::ostream& serialize(std::ostream& os, ObjProxy* d)
{
    serialize(os, d->name);
    serialize(os, d->klass);
    serialize(os, d->fields);
    serialize(os, d->metadata);
    return os;
}


std::istream& deserialize(VM& vm, std::istream& is, ObjProxy** d)
{
    *d = new ObjProxy(vm);
    deserialize(vm, is, (*d)->name);
    deserialize(vm, is, &(*d)->klass);
    deserialize(vm, is, (*d)->fields);
    deserialize(vm, is, (*d)->metadata);
    return is;
}

std::ostream& serialize(std::ostream& os, Value& v)
{
    size_t typ = (size_t)v.type;
    write(os,typ);
    switch(v.type)
    {
        case ValueType::VAL_NIL :
        {
            break;
        }
        case ValueType::VAL_BOOL : 
        {
            write(os,v.as.boolean);
            break;
        }
        case ValueType::VAL_INT : 
        {
            write(os,v.as.integer);
            break;
        }
        case ValueType::VAL_NUMBER : 
        {
            write(os,v.as.number);
            break;
        }
        case ValueType::VAL_OBJ : 
        {
            auto str = ::as<ObjString>(v);
            if(str)
            {
                size_t ot = O_STR;
                write(os,ot);
                serialize(os, str);
                break;
            }
            auto fun = ::as<ObjFunction>(v);
            if(fun)
            {
                size_t ot = O_FUN;
                write(os,ot);
                serialize(os, *fun);
                break;
            }
            auto native = ::as<ObjForeignFunction>(v);
            if(native)
            {
                size_t ot = O_NATIVE;
                write(os,ot);
                serialize(os, native);
                break;
            }
            auto variadic = ::as<ObjVariadicForeignFunction>(v);
            if(variadic)
            {
                size_t ot = O_VARIADIC;
                write(os,ot);
                serialize(os, variadic);
                break;
            }
            auto s = ::as<ObjStruct>(v);
            if(s)
            {
                size_t ot = O_STRUCT;
                write(os,ot);
                serialize(os, s);
                break;
            }
            auto cb = ::as<ObjCallbackType>(v);
            if (cb)
            {
                size_t ot = O_CALLBACK;
                write(os, ot);
                serialize(os, cb);
                break;
            }
            auto clazz = ::as<ObjClass>(v);
            if (clazz)
            {
                size_t ot = O_CLASS;
                write(os, ot);
                serialize(os, clazz);
                break;
            }
            auto map = ::as<ObjMap>(v);
            if (map)
            {
                size_t ot = O_MAP;
                write(os, ot);
                serialize(os, map);
                break;
            }
            auto a = ::as<ObjArray>(v);
            if (a)
            {
                size_t ot = O_ARRAY;
                write(os, ot);
                serialize(os, a);
                break;
            }
            auto r = ::as<ObjRegex>(v);
            if (r)
            {
                size_t ot = O_REGEX;
                write(os, ot);
                serialize(os, r);
                break;
            }
            auto proxy = ::as<ObjProxy>(v);
            if (proxy)
            {
                size_t ot = O_PROXY;
                write(os, ot);
                serialize(os, proxy);
                break;
            }
            auto decorator = ::as<ObjDecorator>(v);
            if (decorator)
            {
                size_t ot = O_DECORATOR;
                write(os, ot);
                serialize(os, decorator);
                break;
            }

            break;
        }
    }
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, Value& v)
{
    size_t typ = 0;
    read(is,typ);
    v.type = (ValueType)typ;
    switch(v.type)
    {
        case ValueType::VAL_NIL :
        {
            v = NIL_VAL;
            break;
        }
        case ValueType::VAL_BOOL : 
        {
            read(is,v.as.boolean);
            break;
        }
        case ValueType::VAL_INT : 
        {
            read(is,v.as.integer);       
            break;
        }
        case ValueType::VAL_NUMBER : 
        {
            read(is,v.as.number);
            break;
        }
        case ValueType::VAL_OBJ : 
        {
            size_t t = 0;
            read(is,t);
            ObjType ot = (ObjType)t;

            switch(ot)
            {
                case O_STR : 
                {
                    ObjString* str = 0;
                    deserialize(vm, is, &str );
                    v.as.obj = (Obj*)str;
                    break;
                }
                case O_FUN : 
                {
                    ObjFunction* fun = 0;
                    deserialize(vm, is, &fun);
                    v.as.obj = (Obj*)fun;
                    break;
                }
                case O_NATIVE : 
                {
                    ObjForeignFunction* fun = 0;
                    deserialize(vm, is, &fun);
                    v.as.obj = (Obj*)fun;
                    break;
                }
                case O_VARIADIC : 
                {
                    ObjVariadicForeignFunction* fun = 0;
                    deserialize(vm, is, &fun);
                    v.as.obj = (Obj*)fun;
                    break;
                }
                case O_STRUCT : 
                {
                    ObjStruct* s = 0;
                    deserialize(vm, is, &s);
                    v.as.obj = (Obj*)s;
                    break;
                }
                case O_CALLBACK:
                {
                    ObjCallbackType* cb = 0;
                    deserialize(vm, is, &cb);
                    v.as.obj = (Obj*)cb;
                    break;
                }
                case O_CLASS :
                {
                    ObjClass* s = 0;
                    deserialize(vm, is, &s);
                    v.as.obj = (Obj*)s;
                    break;
                }
                case O_MAP:
                {
                    ObjMap* s = 0;
                    deserialize(vm, is, &s);
                    v.as.obj = (Obj*)s;
                    break;
                }
                case O_ARRAY:
                {
                    ObjArray* s = 0;
                    deserialize(vm, is, &s);
                    v.as.obj = (Obj*)s;
                    break;
                }
                case O_REGEX:
                {
                    ObjRegex* s = 0;
                    deserialize(vm, is, &s);
                    v.as.obj = (Obj*)s;
                    break;
                }
                case O_DECORATOR:
                {
                    ObjDecorator* s = 0;
                    deserialize(vm, is, &s);
                    v.as.obj = (Obj*)s;
                    break;
                }
                case O_PROXY:
                {
                    ObjProxy* s = 0;
                    deserialize(vm, is, &s);
                    v.as.obj = (Obj*)s;
                    break;
                }
            }
        }
    }
    return is;
}

template<class T>
std::ostream& serialize(std::ostream& os, std::vector<T>& v)
{
    size_t len = v.size();
    write(os,len);
    for(auto& it : v)
    {
        write(os,it);
    }
    return os;
}

std::ostream& serialize(std::ostream& os, std::vector<Value>& v)
{
    size_t len = v.size();
    write(os,len);
    for(auto& it : v)
    {
        serialize(os, it);
    }
    return os;
}

template<class T>
std::istream& deserialize(VM& , std::istream& is, std::vector<T>& v)
{
    v.clear();
    size_t len = 0;
    read(is,len);
    for(size_t i = 0; i < len; i++)
    {
        T t;
        read(is,t);
        v.push_back(t);
    }
    return is;
}

std::istream& deserialize(VM& vm, std::istream& is, std::vector<Value>& v)
{
    v.clear();
    size_t len = 0;
    read(is,len);
    for(size_t i = 0; i < len; i++)
    {
        Value t;
        deserialize(vm, is, t);
        v.push_back(t);
    }
    return is;
}

std::ostream& serialize(std::ostream& os, Chunk& chunk)
{
    serialize(os, chunk.filename);
    serialize(os, chunk.code);
    serialize(os, chunk.constants);
    serialize(os, chunk.lines);
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, Chunk& chunk)
{
    deserialize(vm, is, chunk.filename);
    deserialize(vm, is, chunk.code);
    deserialize(vm, is, chunk.constants);
    deserialize(vm, is, chunk.lines);
    return is;
}

std::ostream& serialize(std::ostream& os, ObjFunction& fun)
{
    serialize(os, fun.name());
    int cnt = fun.upvalueCount();
    write(os,cnt);
    int arity = fun.arity();
    write(os,arity);
    serialize(os, fun.chunk);
    serialize(os, fun.metadata);
    return os;
}

std::istream& deserialize(VM& vm, std::istream& is, ObjFunction** fun)
{
    ObjString* s = 0;
    deserialize(vm, is, &s);
    int cnt = 0;
    read(is,cnt);
    int arity = 0;
    read(is,arity);
    ObjFunction* f = new ObjFunction(vm,s,cnt,arity);
    deserialize(vm, is, f->chunk);
    *fun = f;
    deserialize(vm, is, f->metadata);
    return is;
}
