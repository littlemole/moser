#ifndef moc_foreign_h
#define moc_foreign_h

#include "object.h"
#include <functional>

/*
    MOSER bultin objects for native  invocation support.
*/

// forward
class ObjRecord;


/*
 forwards for the natice libffi interface. see native.h for detals
*/

namespace native {
    class FunMaker;
    class Fun;
    class VarFun;
    class StructDesc;
    class Struct;
    class Callback;
}


#ifdef _WIN32

/*
    Support for windows 16 bit unicode encoding.
*/

class ObjWideString : public Obj, public Indexable, public Propertyable, public Methodable
{
public:
    ObjWideString(VM&);
    ObjWideString(VM& , const std::string& str);
    ObjWideString(VM&, const std::wstring& str);
    ObjWideString(VM&, const char* s, size_t length);
    ObjWideString(VM&, const wchar_t* s, size_t length);
    virtual ~ObjWideString() override;

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
    virtual std::string type() const override { return "wstr"; }

    virtual void* pointer() override { return (void*)chars.c_str(); }

private:
    mutable std::string buffer;
    std::wstring chars;
    std::unordered_map<std::string,Value> fields;
};
#endif

/*
    MOSER object representing some native OS memory buffer of a specific size.
*/

class ObjBuffer : public ObjBuiltin
{
public:

    ObjBuffer(VM&);
    ObjBuffer(VM&, std::string s);
    ObjBuffer(VM&, size_t s);

    virtual void mark_gc() override;
    virtual void* pointer() override { return &values_[0]; }

#ifdef _WIN32
    std::wstring asWideString();
    std::wstring asWideString(size_t s);
#endif    

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "buffer"; }

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;
    virtual Value getProperty(const std::string& name) override;
    virtual void deleteProperty(const std::string& name) override;

    std::string asString();
    std::string asString(size_t s);

    size_t size() const;

private:
    void init();

    std::vector<char> values_;
};

/*
    MOSER object to represent a C style pointer.
*/

class ObjPointer : public Obj, public Methodable
{
public:

    ObjPointer(VM&);
    ObjPointer(VM&, void* p);
    ObjPointer(VM&, void* p, Value d);
    ~ObjPointer();

    virtual void mark_gc() override;
    virtual void* pointer() override { return ptr_; }

    void free();

    void** addressOf();

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "ptr"; }

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

private:

    void init();

    void* ptr_ = nullptr;
    void* deletor_ = nullptr;

    std::unordered_map<std::string,Value> fields;
};

/*
    MOSER object to represent a C function to be loaded from a shared lib aka DLL.
*/

class ObjForeignFunction : public Obj
{
friend std::ostream& serialize(std::ostream& os, ObjForeignFunction* fun);
friend std::istream& deserialize(VM&, std::istream& os, ObjForeignFunction** fun);
public:
    ObjForeignFunction(VM& v);

    ObjForeignFunction(
        VM&,
        const std::string& l,
        const std::string& r,
        const std::string& f,
        std::vector<std::string>& a
    );

    virtual ~ObjForeignFunction() {}
    virtual void mark_gc() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "foreign function"; }

    virtual bool callValue(int argCount) override;

    virtual void* pointer() override;

    std::shared_ptr<native::Fun> fun_;

protected:

    std::string lib;
    std::string retType;
    std::string name;
    std::vector<std::string> args;

    std::shared_ptr<native::FunMaker> fm_;
};

/*
    MOSER object to represent a native C function ptr, like handed out from
    some C API. Or from MS COM.
*/

class ObjFunctionPtr : public Obj
{
friend std::ostream& serialize(std::ostream& os, ObjFunctionPtr* fun);
friend std::istream& deserialize(VM&, std::istream& os, ObjFunctionPtr** fun);
public:
    ObjFunctionPtr(VM& v);

    ObjFunctionPtr(
        VM&,
        void* ptr,
        const std::string& r,
        std::vector<std::string>& a
    );

    virtual ~ObjFunctionPtr() {}
    virtual void mark_gc() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "function ptr"; }

    virtual bool callValue(int argCount) override;

    virtual void* pointer() override;

    std::shared_ptr<native::Fun> fun_;

protected:

    std::string retType;
    std::vector<std::string> args;

    std::shared_ptr<native::FunMaker> fm_;
};


/*
    MOSER object to represent a variadic C function with arbitary count of arguments
    (think printf)
*/

class ObjVariadicForeignFunction : public Obj
{
friend std::ostream& serialize(std::ostream& os, ObjVariadicForeignFunction* fun);
public:
    ObjVariadicForeignFunction(VM& v);
    ObjVariadicForeignFunction(
        VM&,
        const std::string& l,
        const std::string& r,
        const std::string& f,
        std::vector<std::string>& a
    );

    virtual ~ObjVariadicForeignFunction() {}
    virtual void mark_gc() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "variadic function"; }

    virtual bool callValue(int argCount) override;

    virtual void* pointer() override;

protected:

    std::string lib;
    std::string retType;
    std::string name;
    std::vector<std::string> args;

    std::shared_ptr<native::FunMaker> fm_;
    std::shared_ptr<native::VarFun> fun_;

};

/*
    MOSER object representing a C style struct definition.
*/

class ObjStruct : public ObjBuiltin
{
friend std::ostream& serialize(std::ostream& os, ObjStruct* s);
public:

    ObjStruct(VM&, std::vector<std::pair<std::string, std::string>>& desc);
    ObjStruct(VM&, const std::string& name, std::vector<std::pair<std::string, std::string>>& desc);
    ObjStruct(VM&, const std::string& name);

    virtual void mark_gc() override;

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value val) override;
    virtual void deleteProperty(const std::string& name) override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "typedef"; }

    virtual bool callValue(int argCount) override;

    size_t size() const;

    Value create();

private:
    std::string name_;
    std::shared_ptr<::native::StructDesc> structDesc_;
    std::vector<std::pair<std::string, std::string>> members_;
};

/*
    MOSER object representing a specific instance of a C style struct
*/

class ObjRecord : public ObjBuiltin
{
public:

    ObjRecord(VM&, std::shared_ptr<native::Struct> r);
    virtual void mark_gc() override;

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value val) override;
    virtual void deleteProperty(const std::string& name) override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "struct"; }

    virtual bool callValue(int argCount) override;

    size_t size() const;

    virtual void* pointer() override;

    void* offset_of(const std::string& key);

private:
    std::shared_ptr<::native::Struct> struct_;
    ObjPointer* ptr_ = nullptr;

};

/*
    MOSER object representing the type of a C style function callback,
    where MOSER code hands out a function ptr to native code.
*/

class ObjCallbackType : public Obj
{
public:

    ObjCallbackType(      
        VM&,
        const std::string& name,
        const std::string& retType,
        const std::vector<std::string> args
    );

    virtual void mark_gc() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override;

    virtual bool callValue(int argCount) override;

//private:
    std::string retType_;
    std::vector<std::string> args_;
    std::string name_;
};

/*
    MOSER object representing a specific C style callback.
    It wraps a MOSER function and makes it available to native
    code for callback.

    beware the GC - the native code does not now it gets a MOSER
    function reference, client code must make sure the reference
    will not be garbage collected until the callback actually happens.
*/
class ObjCallback : public Obj
{
public:

    ObjCallback(        
        VM&,
        Obj* cb,
        const std::string& name,
        const std::string& retType,
        const std::vector<std::string> args
    );

    virtual ~ObjCallback();

    virtual void mark_gc() override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override ;
    
    virtual bool callValue(int argCount) override;

    template<class F>
    void callback(F fun)
    {
        std::function<void(void*, void*, void**)> f = fun;
        setCb(f);
    }

    virtual void* pointer() override;

private:

    void setCb(std::function<void(void*, void*, void**)>& f);

    std::shared_ptr<native::FunMaker> fm_;
    std::shared_ptr<native::Callback> cb_;

    Obj* cbFun_ = nullptr;
    std::string name_;
};

#endif
