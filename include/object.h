#ifndef moc_object_h
#define moc_object_h

#include "value.h"
#include "chunk.h"

#include <regex>
#include <unordered_map>
#include <unordered_set>

// forwards
class VM;
class ObjInstance;


/*
    MOSER builtin interfaces used by MOSER builtin objects
*/

class Indexable
{
public:

    virtual Value index(ptrdiff_t i) const = 0;
    virtual Value slice(ptrdiff_t index, ptrdiff_t len) const = 0;
    virtual Value index(ptrdiff_t i, Value val) = 0;
    virtual Value add(Value val) = 0;
    virtual Value size() const = 0;

};

class Methodable
{
public:

    virtual Value getMethod(const std::string& name) = 0;
    virtual bool  invokeMethod(const std::string& name, int argCount) = 0;
};

class Propertyable
{
public:

    virtual Value getProperty(const std::string& name) = 0;
    virtual void setProperty(const std::string& name, Value val) = 0;
    virtual void deleteProperty(const std::string& name) = 0;
    virtual std::vector<std::string> keys() = 0;
};


class Callable 
{
public:
    virtual bool callValue(int argCount) = 0;
};

/*
    MOSER builtin objects
*/


/*
    every MOSER builtin object is a object
*/


class Obj 
{
public:
    Obj(VM&);
    virtual ~Obj();

    bool isMarked = false;

    virtual std::string type() const = 0;
    virtual const std::string& toString() const = 0;
    virtual bool callValue(int argCount);
    virtual void mark_gc() = 0;
    virtual void* pointer() { return this; }
    virtual void finalize() {};

    Value metadata;
    VM& vm;
};

// object helpers

template<class T>
T* as(Obj* ob)
{
    return dynamic_cast<T*>(ob);
}

template<class T>
T* as(Value value)
{
    if(value.type != ValueType::VAL_OBJ) return nullptr;
    return dynamic_cast<T*>(value.as.obj);
}


template<class T>
bool isa(Obj* ob)
{
    return as<T>(ob) != nullptr;
}

template<class T>
bool isa(Value value)
{
    if(value.type != ValueType::VAL_OBJ) return false;
    return as<T>(value.as.obj) != nullptr;
}

template<class T>
const T* as(const Obj* ob)
{
    return dynamic_cast<const T*>(ob);
}


template<class T>
bool isa(const Obj* ob)
{
    return as<const T>(ob) != nullptr;
}

/*
    MOSER String object (utf8)
*/

class ObjString : public Obj, public Indexable, public Propertyable, public Methodable
{
public:
    ObjString(VM&);
    ObjString(VM&, const std::string& str);
    ObjString(VM&, const char* s, size_t length);
    virtual ~ObjString() override;

    virtual void mark_gc() override;

    void init();

    virtual Value index(ptrdiff_t i) const override;
    virtual Value slice(ptrdiff_t index, ptrdiff_t len) const override;
    virtual Value index(ptrdiff_t i, Value val) override;
    virtual Value add(Value val) override;
    virtual Value size() const override;

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value val) override;
    virtual void deleteProperty(const std::string& name) override;
    virtual std::vector<std::string> keys() override; 

    virtual Value getMethod(const std::string& name);
    virtual bool  invokeMethod(const std::string& name, int argCount);

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "str"; }

    virtual void* pointer() override { return (void*)chars.c_str(); }

private:
    std::string chars;
    std::unordered_map<std::string,Value> fields;
};

/*
    MOSER function
*/

class ObjFunction : public Obj 
{
friend class Compiler;
friend std::ostream& operator<<(std::ostream& os, ObjFunction& fun);
friend std::istream& operator>>(std::istream& is, ObjFunction** fun);
public:

    ObjFunction(VM&, ObjString* n, int cnt, int arity = 0);
    
    virtual ~ObjFunction() {}
    virtual void mark_gc() override;
    
    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "function"; }

    ObjString* name() { return name_; }
    int upvalueCount() { return upvalueCount_;} 
    int arity() { return arity_; }

    int addUpvalue(Compiler& compiler, uint8_t index, bool isLocal);

    virtual void* pointer() override { return this; }

    Chunk chunk;

private:

    ObjString* name_ = nullptr;
    int upvalueCount_ = 0;

    int arity_ = 0;

    std::vector<std::vector<int>> breakes;
    std::vector<std::vector<int>> loops;
    std::vector<int> exHandlers;
    std::string fun;
};

/*
    Moser native function - functions implmented in native code.
*/

typedef Value (*NativeFn)( VM& vm, int argCount, Value* args);

class ObjNativeFun : public Obj, Callable
{
public:
    ObjNativeFun(VM&);

    ObjNativeFun(VM&, NativeFn f);

    virtual ~ObjNativeFun() {}
    virtual void mark_gc() override;

    NativeFn function;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "native function"; }

    virtual bool callValue(int argCount) override;

    virtual void* pointer() override { return (void*)function; }

};

/*
    Moser native member function - builtin object member functions implmented in native code.
*/

class ObjNativeMethod;
typedef Value (*NativeMeth)( Value that, const std::string& name,  int argCount, Value* args);

class ObjNativeMethod : public Obj, Callable
{
public:
    ObjNativeMethod(VM&);
    ObjNativeMethod(VM&, NativeMeth f);
    ObjNativeMethod(VM&, const std::string& n, NativeMeth f);

    virtual ~ObjNativeMethod() {}
    virtual void mark_gc() override;

    NativeMeth function;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "native method"; }

    virtual bool callValue(int argCount) override;

    virtual void* pointer() override { return (void*) function; }

    std::string name;
};

/*
    Upvalues are objects - used to prolong lifetime of plain stack Values when
    captured by a closure.
*/

class ObjUpvalue : public Obj 
{
friend class VM;
public:
    ObjUpvalue(VM& v, Value* val);

    virtual ~ObjUpvalue() {}
    virtual void mark_gc() override;
    virtual bool callValue(int argCount) override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override;

    virtual void* pointer() override { return location; }

private:
    Value* location = nullptr;
    Value closed;  
};

/*
    Moser closure - all functions are wrapped in a closure, which also
    holds on any upvalue variables closed over
*/

class ObjClosure : public Obj, Callable
{
friend class VM;
public:
    ObjClosure(VM& v, ObjFunction* f);

    virtual ~ObjClosure() {}
    virtual void mark_gc() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "closure"; }

    virtual bool callValue(int argCount) override;

    virtual void* pointer() override { return function; }

    ObjFunction* function = nullptr;

private:
    std::vector<ObjUpvalue*> upvalues;
};

/*
    MOSER class object representing a user defined class 
*/

class ObjClass : public Obj, public Callable, public Propertyable, public Methodable
{
public:
    ObjClass(VM& vm, ObjString* n);

    virtual ~ObjClass() {}
    virtual void mark_gc() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "class"; }

    virtual bool callValue(int argCount) override;    
    virtual Value bindMethod(ObjInstance* instance, const std::string& name) ;

    virtual void* pointer() override { return this; }

    ObjString* name = nullptr;
    ObjClass* super = nullptr;

    void inherit(ObjClass* superClass);

    void defineStaticMethod(ObjString* mname, Value method) 
    {
        fields[mname->toString()] = method;
    }

    void defineMethod(ObjString* mname, Value method) 
    {
        methods[mname->toString()] = method;
    }

    void defineGetter(ObjString* gname, Value method) 
    {
        getters[gname->toString()] = method;
    }

    void defineSetter(ObjString* sname, Value method) 
    {
        setters[sname->toString()] = method;
    }

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value val) override;
    virtual void deleteProperty(const std::string& name) override;
    virtual std::vector<std::string> keys() override;

    void init();
    std::unordered_map<std::string,Value> fields;

    std::unordered_map<std::string, Value> methods;
    std::unordered_map<std::string, Value> getters;
    std::unordered_map<std::string, Value> setters;
};

/*
    Moser builtin object representing a concrete instance of a user 
    defined class.
*/

class ObjInstance : public Obj, public Methodable, public Propertyable
{
public:

    ObjInstance(VM& v);
    ObjInstance( VM& v, ObjClass* k);

    virtual ~ObjInstance() {};
    virtual void mark_gc() override;

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value val) override;
    virtual void deleteProperty(const std::string& name) override;
    virtual std::vector<std::string> keys() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "object"; }

    virtual void* pointer() override { return this; }

    virtual void finalize() override;

    ObjClass* klass = nullptr;

    void init();
    std::string name;
    std::unordered_map<std::string,Value> fields;
};

/*
    MOSER object wrapping a closure that is bound to an object instance and
    has a this pointer
*/

class ObjBoundMethod : public Obj, Callable
{
public:

    ObjBoundMethod(VM& v, Value r, Obj* m);

    virtual ~ObjBoundMethod() {}
    virtual void mark_gc() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "method"; }

    virtual bool callValue(int argCount) override;    
    virtual void* pointer() override { return method; }

    Obj* method = nullptr;

//private:
    Value receiver;
};

/*
    Intermediate type for builtins
*/

class ObjBuiltin : public ObjInstance
{
public:

    ObjBuiltin(VM& v);

    virtual ~ObjBuiltin() {}

    std::unordered_map<std::string, Value> methods;
};

/*
    A JavaScript like Array object
*/

class ObjArray : public ObjBuiltin, public Indexable
{
public:

    ObjArray(VM& v);
    virtual ~ObjArray() {}
    virtual void mark_gc() override;

    virtual Value index(ptrdiff_t i) const override;
    virtual Value slice(ptrdiff_t index, ptrdiff_t len) const override;
    virtual Value index(ptrdiff_t i, Value val) override;
    virtual Value add(Value val) override;
    virtual Value size() const override;
    
    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual Value getProperty(const std::string& name) override;
    virtual std::vector<std::string> keys() override;
    virtual void deleteProperty(const std::string& name) override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "array"; }

    virtual void* pointer() override { return &values[0]; }

    Value pop();


    std::vector<Value> values;
    mutable std::string displayName;
};

/*
    A JavaScript like Object / Hashmap
*/

class ObjMap : public ObjBuiltin
{
public:

    ObjMap(VM& vm);

    virtual ~ObjMap() {}
    virtual void mark_gc() override;

    Value item(const std::string& s);
    Value item(const std::string& key, Value val);
    Value size();

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value val) override;
    virtual void deleteProperty(const std::string& name) override;
    virtual std::vector<std::string> keys() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "map"; }

    virtual void* pointer() override { return this; }


    std::unordered_map<std::string,Value> elements;
    mutable std::string displayName;

};

/*
    A simple regex obkect encapsulation std c++ regexes
*/

class ObjRegex : public ObjBuiltin
{
public:

    ObjRegex(VM& v, const std::string& r);
    ObjRegex(VM& v, const std::string& r, const std::string& o);

    virtual ~ObjRegex() {}
    virtual void mark_gc() override;

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value val) override;
    virtual void deleteProperty(const std::string& name) override;
    virtual std::vector<std::string> keys() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "<regex>>"; }

    virtual void* pointer() override { return this; }



    void init();

    std::string value;
    std::smatch smatch_;
    std::regex regex_;
    std::string suffix_;
    std::string options_;
};

/*
    MOSER object providing a python like decorator.
*/

class ObjDecorator : public ObjBuiltin
{
public:

    ObjDecorator(VM& v);
    ObjDecorator(VM& v, Obj* target, Obj* proxy);

    virtual ~ObjDecorator() {}
    virtual void mark_gc() override;

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual bool callValue(int argCount) override;    
    virtual bool callValue(Value receiver,int argCount);    

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value val) override;
    virtual void deleteProperty(const std::string& name) override;
    virtual std::vector<std::string> keys() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "<Decorator>>"; }

    virtual void* pointer() override { return this; }

};

/*
    MOSER object representing a Java like runtime proxy.
*/

class ObjProxy : public ObjDecorator
{
public:

    ObjProxy(VM& v);
    ObjProxy(VM&, Obj* target, Obj* proxy);

    virtual ~ObjProxy() {}
    virtual void mark_gc() override;

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual bool callValue(int argCount) override;    
    virtual bool callValue(Value receiver,int argCount);    

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value val) override;
    virtual void deleteProperty(const std::string& name) override;
    virtual std::vector<std::string> keys() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "<Proxy>"; }

    virtual void* pointer() override { return this; }

};


/*
    A simple Promise obkect encapsulation std c++ regexes
*/
/*
class ObjPromise : public ObjBuiltin
{
public:

    ObjPromise(VM& v);

    virtual ~ObjPromise() {}
    virtual void mark_gc() override;

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value val) override;
    virtual void deleteProperty(const std::string& name) override;
    virtual std::vector<std::string> keys() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "<Promise>>"; }

    virtual void* pointer() override { return this; }

    void init();

	Value result;
	Obj* onResolve = nullptr;
	Obj* onReject = nullptr;
	ObjPromise* chain = nullptr;
};
*/
#endif
