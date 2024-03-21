#include "foreign.h"
#include "vm.h"
#include "gc.h"
#include "win32/uni.h"
#include "native.h"
#include <sstream>


class FFIMarshaller
{
public:

    static std::string getType(Value v)
    {
        switch(v.type)
        {
            case ValueType::VAL_NIL : 
            {
                return "ptr";
                break;
            }
            case ValueType::VAL_BOOL : 
            {
                return "byte";
                break;
            }
            case ValueType::VAL_INT : 
            {
                return "int";
            }
            case ValueType::VAL_NUMBER : 
            {
                return "double";
                break;
            }
            case ValueType::VAL_OBJ : 
            {
                auto str = as<ObjString>(v);
                if(str) return "str";
#ifdef _WIN32
                auto wstr = as<ObjWideString>(v);
                if(wstr) return "wstr";
#endif
                return "ptr";
                break;
            }
        }
        return "ptr";
    }


    static Value to_value(VM& vm, ffi_type* type, const std::string& name, void* arg)
    {
        if(type == &ffi_type_void) return NIL_VAL;
        if (type == &ffi_type_sint8)
        {
            int8_t c = *(int8_t*)(arg);
            return Value((ptrdiff_t)c);
        }
        if (type == &ffi_type_sint16)
        {
            int16_t c = *(int16_t*)(arg);
            return Value((ptrdiff_t)c);
        }
        if (type == &ffi_type_sint32)
        {
            int32_t c = *(int32_t*)(arg);
            return Value((ptrdiff_t)c);
        }
        if (type == &ffi_type_sint64)
        {
            int64_t c = *(int64_t*)(arg);
            return Value((ptrdiff_t)c);
        }
        if (type == &ffi_type_uint8)
        {
            uint8_t c = *(uint8_t*)(arg);
            return Value((ptrdiff_t)c);
        }
        if (type == &ffi_type_uint16)
        {
            uint16_t c = *(uint16_t*)(arg);
            return Value((ptrdiff_t)c);
        }
        if (type == &ffi_type_uint32)
        {
            uint32_t c = *(uint32_t*)(arg);
            return Value((ptrdiff_t)c);
        }
        if (type == &ffi_type_uint64)
        {
            uint64_t c = *(uint64_t*)(arg);
            return Value((ptrdiff_t)c);
        }
        if (type == &ffi_type_float)
        {
            float c = *(float*)arg;
            return Value((double)c);
        }
        if (type == &ffi_type_double)
        {
            double c = *(double*)arg;
            return Value((double)c);
        }
        if (type == &ffi_type_pointer)
        {
            if (name == "str")
            {
                char* c = *(char**)arg;
                if(c==0)
                    return new ObjString(vm, "");
                return Value( new ObjString(vm, c));
            }
#ifdef _WIN32            
            if (name == "wstr")
            {
                wchar_t* c = *(wchar_t**)arg;
                return Value(new ObjWideString(vm, c));
            }
#endif            
            return Value( new ObjPointer( vm, *(void**)arg));
        }

        //if(native::theStructCache().count(name))
        {
             return Value( new ObjPointer(vm,  (void*)arg));
        }

        return NIL_VAL;
    }

    static void to_foreign(ffi_type* type, void* ret, Value val)
    {
        Value r = val;
        auto proxy = as<ObjProxy>(r);
        while(proxy)
        {
            r = proxy->getProperty("target");
            proxy = as<ObjProxy>(r);
        }
        
        //*(size_t*)ret = 0;
        if(type == &ffi_type_void) return;

        if (type == &ffi_type_sint8)
        {
            char i = (char)r.to_int();
            *(char*)ret = i;
        }
        if (type == &ffi_type_sint16)
        {
            short i = (short)r.to_int();
            *(short*)ret = i;
        }
        if (type == &ffi_type_sint32)
        {
            int32_t i = (int32_t)r.to_int();
            *(int32_t*)ret = i;
        }
        if (type == &ffi_type_sint64)
        {
            int64_t i = (int64_t)r.to_int();
            *(int64_t*)ret = i;
        }
        if (type == &ffi_type_uint8)
        {
            unsigned char i = (unsigned char)r.to_int();
            *(unsigned char*)ret = i;
        }
        if (type == &ffi_type_uint16)
        {
            unsigned short i = (unsigned short)r.to_int();
            *(unsigned short*)ret = i;
        }
        if (type == &ffi_type_uint32)
        {
            uint32_t i = (uint32_t)r.to_int();
            *(uint32_t*)ret = i;
        }
        if (type == &ffi_type_uint64)
        {
            uint64_t i = (uint64_t)r.to_int();
            *(uint64_t*)ret = i;
        }
        if (type == &ffi_type_float)
        {
            float i = (float)r.to_double();
            *(float*)ret = i;
        }
        if (type == &ffi_type_double)
        {
            double i = (double)r.to_double();
            *(double*)ret = i;
        }
        if (type == &ffi_type_pointer)
        {
            *(size_t*)ret = 0;

            if(IS_INT(r))
            {
                *(ptrdiff_t*)ret = r.as.integer;
                return;
            }

            if(IS_NUMBER(r))
            {
                *(ptrdiff_t*)ret = r.to_int();
                return;
            }

            auto p = as<ObjPointer>(r);
            if(p)
            {
                *(void**)ret = p->pointer();
                return;
            }

            auto b = as<ObjBuffer>(r);
            if(b)
            {
                *(void**)ret = b->pointer();
                return;
            }

            auto rec = as<ObjRecord>(r);
            if(rec)
            {
                *(void**)ret = rec->pointer();
                return;
            }

            auto str = as<ObjString>(r);
            if(str)
            {
                *(void**)ret = (void*) (str->toString().c_str());
                return;
            }

#ifdef _WIN32
            auto wstr = as<ObjWideString>(r);
            if(wstr)
            {
                *(void**)ret = (void*) (wstr->pointer());
                return;
            }
#endif
            auto cb = as<ObjCallback>(r);
            if(cb)
            {
                *(void**)ret = cb->pointer();
                return;
            }

            auto instance = as<ObjInstance>(r);
            if(instance)
            {
                *(void**)ret = instance->pointer();
                return;
            }

        }
    }

};

#ifdef _WIN32
ObjWideString::ObjWideString(VM& v) : Obj(v)
{
    init();
}

ObjWideString::ObjWideString(VM& v, const char* s, size_t length)
    : Obj(v)
{
    if(s && length > 0)
    {
        chars = std::wstring(to_wstring(s, (int) length));
    }
    init();
}

ObjWideString::ObjWideString(VM& v, const std::string& str)
    : Obj(v), chars(to_wstring(str))
{
    init();
}

ObjWideString::ObjWideString(VM& v, const wchar_t* s, size_t length)
    : Obj(v)
{
    if(s && length > 0)
    {
        chars = std::wstring(s,length);
    }
    init();
}

ObjWideString::ObjWideString(VM& v, const std::wstring& str)
    : Obj(v), chars(str)
{
    init();
}

ObjWideString::~ObjWideString()
{}

void ObjWideString::mark_gc()
{    
    vm.gc.markMap(fields);
}


void ObjWideString::init()
{
    fields["length"] = chars.size();

    auto meth = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto str = as<ObjWideString>(that.as.obj);
            if(!str) return NIL_VAL;
            for( int i = 0; i < argCount; i++)
            {
                auto s =args[i].to_string();
                std::wstring w = str->chars + to_wstring(s);
                str = new ObjWideString(str->vm, w);
            }
            return str;
        }
    );
    fields["add"] = meth;
}


Value ObjWideString::index(ptrdiff_t i) const
{
    if(chars.empty() || i > (ptrdiff_t)chars.size() || i < 0)
    {
        return NIL_VAL;
    }
    return new ObjWideString( vm, chars.substr(i,1) );
}

Value ObjWideString::slice(ptrdiff_t index, ptrdiff_t len) const
{
    if(chars.empty() || index > (ptrdiff_t)chars.size() || index < 0)
    {
        return NIL_VAL;
    }
    if(len == -1) len = chars.size() - index;
    if(index+len> (ptrdiff_t)chars.size()) len = chars.size()-index;

    auto s = new ObjWideString( vm, chars.substr(index,len));
    return s;
}

Value ObjWideString::index(ptrdiff_t i, Value val)
{
    if(chars.empty() || i > (ptrdiff_t)chars.size() || i < 0)
    {
        return NIL_VAL;
    }
    std::string s = val.to_string();
    if(s.empty()) return NIL_VAL;
    std::wstring dest = chars;
    dest[i] = s.substr(0,1)[0];
    return new ObjWideString(vm, dest);
}

Value ObjWideString::add(Value val)
{
    std::wstring s = to_wstring(val.to_string());
    if(s.empty()) return new ObjWideString(vm, s);

    std::wstring dest = chars;
    dest += s;
    return new ObjWideString(vm, dest);
}

Value ObjWideString::size() const
{
    return chars.size();
}

ObjWideString* copyString(VM& vm, const char* chars, size_t length) 
{
    return new ObjWideString(vm, chars,length);
}


Value ObjWideString::getProperty(const std::string& name)
{
    if( fields.count(name))
    {
        Value value = fields[name];
        return value;
    }

    return NIL_VAL;
}


void ObjWideString::setProperty(const std::string& /* key */, Value /* val */)
{
}

void ObjWideString::deleteProperty(const std::string& /* name */)
{
    
}

Value ObjWideString::getMethod(const std::string& mname)
{
    return fields[mname];
}

std::vector<std::string> ObjWideString::keys()
{
    std::vector<std::string> empty;
    return empty;
}

bool ObjWideString::invokeMethod(const std::string& mname, int argCount)
{
    Value val = fields[mname];
    if(IS_OBJ(val))
    {
        return val.as.obj->callValue(argCount);
    }
    return false;
}


const std::string& ObjWideString::toString() const
{
    buffer = to_string(chars);
    return buffer;;
}

#endif
ObjBuffer::ObjBuffer(VM& v) : ObjBuiltin(v), values_(1024, '\0')
{
    init();
}

ObjBuffer::ObjBuffer(VM& v,size_t s) : ObjBuiltin(v), values_(s, '\0')
{
    init();
}

ObjBuffer::ObjBuffer(VM& v, std::string s) : ObjBuiltin(v), values_(s.size(), '\0')
{
    for (size_t i = 0; i < s.size(); i++)
    {
        values_[i] = s[i];
    }
    init();
}

void ObjBuffer::mark_gc()
{
    vm.gc.markMap(fields);
    vm.gc.markMap(methods);
}

 
void ObjBuffer::init()
{
    fields["length"] = values_.size();

    auto asString = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto buf = as<ObjBuffer>(that.as.obj);
            if(!buf) return NIL_VAL;

            if(argCount == 0) return new ObjString(buf->vm, buf->asString());

            ptrdiff_t len = args[0].to_int();

            return new ObjString( buf->vm, buf->asString(len));
        }
    );
    methods["asString"] = asString;

#ifdef _WIN32
    auto asWideString = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto buf = as<ObjBuffer>(that.as.obj);
            if (!buf) return NIL_VAL;

            if (argCount == 0) return new ObjWideString( buf->vm, buf->asWideString());

            ptrdiff_t len = args[0].to_int();

            return new ObjWideString(buf->vm, buf->asWideString(len));
        }
    );

    methods["asWideString"] = asWideString;
#endif
}

std::string ObjBuffer::asString()
{
    std::ostringstream oss;
    for (auto& i : values_)
    {
        if (i == '\0') break;
        oss.write(&i, 1);
    }
    return oss.str();
}

std::string ObjBuffer::asString(size_t s)
{
    return std::string( &values_[0], s);
}

#ifdef _WIN32

std::wstring ObjBuffer::asWideString()
{
    size_t pos = values_.size();
    for(size_t i = 0; i < values_.size()-1; i++)
    {
        if( values_[i] == '\0' && values_[i+1] == '\0' )
        {
            pos = i+1;
            break;
        }
    }

    std::wstring ws( (wchar_t*)&values_[0], pos / 2 );
    return ws;
}

std::wstring ObjBuffer::asWideString(size_t s)
{
    return std::wstring( (wchar_t*)&values_[0], s);
}

#endif

size_t ObjBuffer::size() const
{
    return values_.size();
}

Value ObjBuffer::getMethod(const std::string& mname)
{
    if(methods.count(mname)) return methods[mname];
    return NIL_VAL;
}

bool ObjBuffer::invokeMethod(const std::string& mname, int argCount)
{
    Value m = getMethod(mname);
    if(IS_OBJ(m))
    {
        return m.as.obj->callValue(argCount);
    }
    return false;
}


const std::string& ObjBuffer::toString() const 
{
    static std::string s("<buffer>");
    return s;
}

Value ObjBuffer::getProperty(const std::string& pname)
{
    if( fields.count(pname))
    {
        Value value = fields[pname];
        return value;
    }

    if(!methods.count(pname) )
    {
        return NIL_VAL;
    }
    return methods[pname];
}

void ObjBuffer::deleteProperty(const std::string& /* name */)
{
}

const std::string& ObjForeignFunction::toString() const 
{
    return name;
}

ObjForeignFunction::ObjForeignFunction(VM& v) : Obj(v)
{}


ObjForeignFunction::ObjForeignFunction(
    VM& v,
    const std::string& l,
    const std::string& r,
    const std::string& f,
    std::vector<std::string>& a
) : Obj(v), lib(l), retType(r), name(f), args(a)
{
    fm_ = std::make_shared<native::FunMaker>(lib);
    fun_ = fm_->makeFun(retType, name, args);
}



void ObjForeignFunction::mark_gc()
{
    
}

void* ObjForeignFunction::pointer() 
{ 
    return (void*)(fun_->pointer());
}

bool ObjForeignFunction::callValue(int argCount)
{
    if (argCount != (int)fun_->nargs() || fun_->pointer() == 0) 
    {  
        for (int i = 0; i < argCount; i++)
        {
            vm.pop();
        }
        vm.pop();
        vm.push(NIL_VAL);
        return false;
    }

    for (int i = 0; i < argCount; i++)
    {
        Value v = *(&vm.stack.back() - argCount +1 +i);
        //unused: const std::string& paramType = fun_->argument(i);

        size_t r = 0;
        void* p = &r;
        FFIMarshaller::to_foreign( fun_->type(i), p, v);
        fun_->value((size_t)r);
    }

    ffi_arg r = fun_->invoke();

    Value retVal = FFIMarshaller::to_value(vm, fun_->returnType(),fun_->returnTypeStr(),&r);

    for( int i = 0; i < argCount;i++)
    {
        vm.pop();
    }
    vm.pop();
    vm.push(retVal);

    return true;
}

//////////////////////////////////////



const std::string& ObjFunctionPtr::toString() const 
{
    static std::string n("functionptr");
    return n;
}

ObjFunctionPtr::ObjFunctionPtr(VM& v) : Obj(v)
{}


ObjFunctionPtr::ObjFunctionPtr(
    VM& v, void* ptr,
    const std::string& r,
    std::vector<std::string>& a
) : Obj(v),  retType(r), args(a)
{
    fm_ = std::make_shared<native::FunMaker>();
    fun_ = fm_->makeFun(retType, ptr, args);
}



void ObjFunctionPtr::mark_gc()
{
    
}

void* ObjFunctionPtr::pointer() 
{ 
    return (void*)(fun_->pointer());
}

bool ObjFunctionPtr::callValue(int argCount)
{
    if (argCount != (int)fun_->nargs() || fun_->pointer() == 0) 
    {  
        for (int i = 0; i < argCount; i++)
        {
            vm.pop();
        }
        vm.pop();
        vm.push(NIL_VAL);
        return false;
    }

    for (int i = 0; i < argCount; i++)
    {
        Value v = *(&vm.stack.back() - argCount +1 +i);
        //unused: const std::string& paramType = fun_->argument(i);

        size_t r = 0;
        void* p = &r;
        FFIMarshaller::to_foreign( fun_->type(i), p, v);
        fun_->value((size_t)r);
    }

    ffi_arg r = fun_->invoke();

    Value retVal = FFIMarshaller::to_value(vm, fun_->returnType(),fun_->returnTypeStr(),&r);

    for( int i = 0; i < argCount;i++)
    {
        vm.pop();
    }
    vm.pop();
    vm.push(retVal);

    return true;
}

//////////////////////////////////////

ObjPointer::ObjPointer(VM& v) : Obj(v), ptr_(0),deletor_(0)
{
    init();
};

ObjPointer::ObjPointer(VM& v, void* p) : Obj(v), ptr_(p),deletor_(0)
{
    init();
};

ObjPointer::ObjPointer(VM& v, void* p, Value d) : Obj(v), ptr_(p) 
{
    if(IS_OBJ(d))
    {
        auto ff = as<ObjForeignFunction>(d);
        if(ff)
        {
            deletor_ = ff->fun_->pointer();
        }
    }
    init();
};

typedef void(*finalizer)(void*);

ObjPointer::~ObjPointer()
{
    if( (deletor_ != 0) && (ptr_ != 0) )
    {
       // printf("killroy\n");
        finalizer fin = (finalizer)deletor_;
        (fin)(ptr_);
        ptr_ = 0;
        deletor_ = 0;
    }
}

Value ObjPointer::getMethod(const std::string& name)
{
    return fields[name];
}

bool ObjPointer::invokeMethod(const std::string& name, int argCount)
{
    Value val = fields[name];
    if(IS_OBJ(val))
    {
        return val.as.obj->callValue(argCount);
    }
    return false;
}


void ObjPointer::init()
{
    auto freemem = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int /* argCount */, Value* /* args */) -> Value
        {
            auto ptr = as<ObjPointer>(that.as.obj);
            if(!ptr) return NIL_VAL;
            ptr->free();
            return NIL_VAL;
        }
    );
    fields["free"] = freemem;

    auto detach = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int /* argCount */, Value* /* args */) -> Value
        {
            auto ptr = as<ObjPointer>(that.as.obj);
            if(!ptr) return NIL_VAL;
            //printf("detach\n");
            ptr->deletor_= 0;
            ptr->ptr_ = 0;
            return NIL_VAL;
        }
    );
    fields["detach"] = detach;


    auto addressOf = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int /* argCount*/, Value* /* args */) -> Value
        {
            auto ptr = as<ObjPointer>(that.as.obj);
            if(!ptr) return NIL_VAL;
            
            void ** p = &ptr->ptr_;
            void* r = (void*)p;
            if(!r) return NIL_VAL;
            return new ObjPointer(ptr->vm, r);
        }
    );
    fields["addressOf"] = addressOf;

    auto asObj = new ObjNativeMethod( vm, 
        []( Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto ptr = as<ObjPointer>(that.as.obj);
            if(!ptr) return NIL_VAL;

            if(argCount == 1)
            {
                ptr->ptr_ = args[0].as.obj;
                return NIL_VAL;
            }
            
            Obj* obj = (Obj*)ptr->pointer();
            if(!obj) return NIL_VAL;
            return obj;
        }
    );
    fields["object"] = asObj;

    auto asBool = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto ptr = as<ObjPointer>(that.as.obj);
            if(!ptr) return false;

            if(argCount == 1)
            {
                *(bool*)(ptr->ptr_) = args[0].to_int() != 0;
                return NIL_VAL;
            }

            bool* p = (bool*)ptr->pointer();
            if(!p) return Value(0);
            int r = *p;
            return Value(r);
        }
    );
    fields["bool"] = asBool;

    auto asInt = new ObjNativeMethod( vm, 
        []( Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto ptr = as<ObjPointer>(that.as.obj);
            if(!ptr) return 0;

            if(argCount == 1)
            {
                *(ptrdiff_t*)(ptr->ptr_) = args[0].to_int();
                return NIL_VAL;
            }

            ptrdiff_t* p = (ptrdiff_t*)ptr->pointer();
            if(!p) return Value(0);
            ptrdiff_t r = *p;
            return Value(r);
        }
    );
    fields["integer"] = asInt;

    auto asDouble = new ObjNativeMethod( vm, 
        []( Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto ptr = as<ObjPointer>(that.as.obj);
            if(!ptr) return 0.0;
            
            if(argCount == 1)
            {
                *(double*)(ptr->ptr_) = args[0].to_double();
                return NIL_VAL;
            }

            double* d = (double*)ptr->pointer();
            if(!d) return Value(0.0);
            double r = *d;
            return Value(r);
        }
    );
    fields["double"] = asDouble;

    auto asStr = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto ptr = as<ObjPointer>(that.as.obj);
            if(!ptr) return new ObjString(ptr->vm, "");

            char* s = (char*)ptr->pointer();
            if(!s) return new ObjString(ptr->vm, "");

            ptrdiff_t len = 0;
            if(argCount > 0)
            {
                len = args[0].to_int();
            }
            else
            {
                len = strlen(s);
            }
            return new ObjString(ptr->vm, s,len);
        }
    );
    fields["string"] = asStr;

#ifdef _WIN32
    auto asWStr = new ObjNativeMethod( vm, 
        []( Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto ptr = as<ObjPointer>(that.as.obj);
            if(!ptr) return new ObjWideString(ptr->vm, L"");
            
            wchar_t* s = (wchar_t*)ptr->pointer();
            if(!s) return new ObjWideString(ptr->vm, L"");

            int len = 0;
            if(argCount > 0)
            {
                len = (int)args[0].to_int();
            }
            else
            {
                len = (int)wcslen(s);
            }
            return new ObjWideString(ptr->vm, s,len);
        }
    );
    fields["wstring"] = asWStr;
#endif
}

void ObjPointer::free()
{
    if(ptr_)
    {
        std::free(ptr_);
        ptr_ = nullptr;
    }
}

void** ObjPointer::addressOf()
{
    return &ptr_;   
}

const std::string& ObjPointer::toString() const 
{
    static std::string s("<ffi ptr>");
    return s;
}

void ObjPointer::mark_gc()
{    
    vm.gc.markMap(fields);
}


const std::string& ObjVariadicForeignFunction::toString() const
{
    return name;
}

ObjVariadicForeignFunction::ObjVariadicForeignFunction(VM& v) : Obj(v)
{}


ObjVariadicForeignFunction::ObjVariadicForeignFunction(
    VM& vm, 
    const std::string& l,
    const std::string& r,
    const std::string& f,
    std::vector<std::string>& a
) : Obj(vm),lib(l), retType(r), name(f), args(a)
{
    fm_ = std::make_shared<native::FunMaker>(lib);

    std::vector<std::string> v;
    size_t nfixed = 0;
    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] == "...")
        {
            nfixed = i;
            continue;
        }
        v.push_back(args[i]);
    }
    fun_ = fm_->makeVarFun(retType, name, v, nfixed);
}


void ObjVariadicForeignFunction::mark_gc()
{
    
}

void* ObjVariadicForeignFunction::pointer() 
{ 
    return (void*)(fun_->pointer());
}

bool ObjVariadicForeignFunction::callValue(int argCount)
{
    if (fun_->pointer() == 0)
    {
        for (int i = 0; i < argCount; i++)
        {
            vm.pop();
        }
        vm.pop();
        vm.push(NIL_VAL);
        return false;
    }

    for (int i = 0; i < argCount; i++)
    {
        Value v = *(&vm.stack.back() - argCount +1 +i);
        auto ts = FFIMarshaller::getType(v);
        //unused: const std::string& paramType = i < fun_->nfixed ? fun_->argument(i) : ts;

        size_t r = 0;
        void* p = &r;

        ffi_type* tp = i < (int)fun_->nfixed ? fun_->type(i) : native::t_lookup()[ts];
        FFIMarshaller::to_foreign( tp, p, v);

        fun_->value((size_t)r, ts );
    }

    ffi_arg r = fun_->invoke();
    Value retVal = FFIMarshaller::to_value(vm, fun_->returnType(),fun_->returnTypeStr(),&r);

    for( int i = 0; i < argCount;i++)
    {
        vm.pop();
    }
    vm.pop();
    vm.push(retVal);

    return true;
}


ObjStruct::ObjStruct(VM& v,std::vector<std::pair<std::string, std::string>>& desc) 
    : ObjBuiltin(v), members_(desc)
{
    ::native::MakeStruct sm(sizeof(size_t));
    for (auto& it : desc)
    {
        sm.add(it.first, it.second);
    }

    structDesc_ = sm.describe();

    fields["size"] = structDesc_->size;
}

ObjStruct::ObjStruct(VM& v, const std::string& name, std::vector<std::pair<std::string, std::string>>& desc) 
    : ObjBuiltin(v), name_(name), members_(desc)
{
    ::native::MakeStruct sm(name,sizeof(size_t));
    for (auto& it : desc)
    {
        sm.add(it.first, it.second);
    }

    structDesc_ = sm.describe();

    fields["size"] = structDesc_->size;
}


ObjStruct::ObjStruct(VM& v, const std::string& name) 
    : ObjBuiltin(v), name_(name)
{
    structDesc_ = native::theStructCache()[name];
    fields["size"] = structDesc_->size;
}


void ObjStruct::mark_gc()
{    
    vm.gc.markMap(fields);
}


bool ObjStruct::callValue(int argCount)
{
    if(argCount > 0)
    {
        Value v = vm.stack[vm.stack.size() - argCount];
        auto ptr = as<ObjPointer>(v);
        if(ptr)
        {
            auto s = this->structDesc_->attach(ptr->pointer());
            vm.pop();
            vm.pop();
            vm.push(new ObjRecord(vm, s));
            return true;
        }
        vm.pop();
        vm.pop();
        vm.push(NIL_VAL);
        return false;
    }
    Value s = create();
    vm.pop();
    vm.push(s);

    return true;
}

Value ObjStruct::create()
{
    auto s = this->structDesc_->create();
    return new ObjRecord(vm, s);
}

size_t ObjStruct::size() const 
{
    return structDesc_->size;
}

Value ObjStruct::getMethod(const std::string& /* name */)
{
    return NIL_VAL;
}

bool ObjStruct::invokeMethod(const std::string& mname, int argCount)
{
    Value m = getProperty(mname);
    if(IS_OBJ(m))
    {
        return m.as.obj->callValue(argCount);
    }
    return false;
}


Value ObjStruct::getProperty(const std::string& pname)
{
    if( fields.count(pname))
    {
        Value value = fields[pname];
        return value;
    }

    if(!methods.count(pname) )
    {
        return NIL_VAL;
    }
    return methods[pname];
}


void ObjStruct::setProperty(const std::string& key, Value val)
{
    fields[key] = val;
}

void ObjStruct::deleteProperty(const std::string& /* name */)
{
}

const std::string& ObjStruct::toString() const 
{
    static std::string s("struct");
    return s;
}

ObjRecord::ObjRecord(VM& v, std::shared_ptr<native::Struct> r) : ObjBuiltin(v), struct_(r)
{}

void ObjRecord::mark_gc()
{
    vm.gc.markMap(fields);
    vm.gc.markMap(methods);
    vm.gc.markObject(ptr_);
}


bool ObjRecord::callValue(int /* argCount */)
{
    return false;
}

size_t ObjRecord::size() const
{
    return struct_->size;
}

// unused
Value ObjRecord::getMethod(const std::string& /* name */)
{
    return NIL_VAL;
}

bool ObjRecord::invokeMethod(const std::string& mname, int argCount)
{
    Value m = getProperty(mname);
    if(IS_OBJ(m))
    {
        return m.as.obj->callValue(argCount);
    }
    return false;
}


Value ObjRecord::getProperty(const std::string& pname)
{
    if (struct_->exists(pname))
    {
        void* p = struct_->get_ptr(pname);
        native::StructMemberDesc& desc = struct_->desc(pname);
        if (native::theStructCache().count(desc.typeName))
        {
            auto r = native::theStructCache()[pname]->attach(p);
            return new ObjRecord(vm, r);
        }

        return FFIMarshaller::to_value(vm, desc.type,"",p);
    }

    if( fields.count(pname))
    {
        Value value = fields[pname];
        return value;
    }

    if(!methods.count(pname) )
    {
        return NIL_VAL;
    }
    return methods[pname];
}

#include <iostream>

void ObjRecord::setProperty(const std::string& key, Value val)
{
    if (struct_->exists(key))
    {
        void* p = struct_->get_ptr(key);

        native::StructMemberDesc& desc = struct_->desc(key);
        if(native::theStructCache().count(desc.typeName))
        {
            if( IS_OBJ(val))
            {
                auto rec = dynamic_cast<ObjRecord*>(val.as.obj);
                if(rec)
                {
                    auto keys = rec->struct_->keys();
                    //for( auto& key : keys)
                    {
                       // auto desc = rec->struct_->desc(key);                        
                        size_t s = rec->size();
                        std::cout << "copy struct: " << key << " " << s << std::endl;
                        memcpy(p,rec->pointer(),s);
                    }
                    return;
                }
            }
        }

        FFIMarshaller::to_foreign(desc.type,p,val);
        return;
    }
    fields[name] = val;
}

void ObjRecord::deleteProperty(const std::string& /* name */)
{
}


const std::string& ObjRecord::toString() const 
{
    static std::string s("record");
    return s;
}

void* ObjRecord::pointer()
{
    return this->struct_->ptr;
}

void* ObjRecord::offset_of(const std::string& key)
{
    if (!struct_->exists(key)) return nullptr;
    return struct_->get_ptr(key);
}


ObjCallbackType::ObjCallbackType(        
    VM& v, 
    const std::string& name,
    const std::string& retType,
    const std::vector<std::string> args
) : Obj(v),
    retType_(retType),
    args_(args),
    name_(name)
{}

void ObjCallbackType::mark_gc()
{
    
}

const std::string& ObjCallbackType::toString() const 
{
    return name_;
}

std::string ObjCallbackType::type() const 
{ 
    return "callbackType"; 
}

bool ObjCallbackType::callValue(int argCount)
{
    if(argCount != 1)
    {
        return false;
    }

    Value funVal = *(&vm.stack.back() );
    auto fun = as<Obj>(funVal);
    if(!fun)
    {
        return false;
    }

    Obj* cb = nullptr;

    auto ptr = as<ObjPointer>(funVal);
    if(ptr)
    {
        cb = new ObjFunctionPtr(vm,ptr->pointer(),retType_, args_);
    }
    else
    {
        cb = new ObjCallback(vm, fun, name_, retType_, args_);
    }

    for( int i = 0; i < argCount;i++)
    {
        vm.pop();
    }
    vm.pop();
    vm.push(cb);

    return true;
}

ObjCallback::ObjCallback(      
    VM& v,
    Obj* cb,  
    const std::string& name,
    const std::string& retType,
    const std::vector<std::string> args
) : Obj(v), cbFun_(cb), name_(name)
{
    fm_ = std::make_shared<native::FunMaker>();
    cb_ = fm_->makeCallback(retType, args);

    this->callback([this](void* cifp, void* ret, void** args)
    {
        ffi_cif* cif = (ffi_cif*)cifp;
        {
            for (size_t i = 0; i < cif->nargs; i++)
            {
                Value arg;
                if (this->cb_->argument(i) == "str")
                {
                    arg = new ObjString(vm, *(char**)args[i]);
                }
    #ifdef _WIN32            
                else if (this->cb_->argument(i) == "wstr")
                {
                    arg = new ObjWideString(vm, *(wchar_t**)args[i]);
                }
    #endif            
                else
                {
                    arg = FFIMarshaller::to_value(vm, cb_->type(i),cb_->argument(i),args[i]);
                }
                vm.push(arg);
            }
        }
        this->cbFun_->callValue(cif->nargs);
        vm.frames.back().returnToCallerOnReturn = true;
        Value r = vm.run();
        if(!vm.pendingEx.empty())
        {
            vm.pendingEx.back().print();
            printf("unhandled exception in callback");
            exit(1);
        }
        FFIMarshaller::to_foreign(cif->rtype,ret,r);
    });
}

ObjCallback::~ObjCallback()
{
    //GC::unpin(cbFun_);
}

void ObjCallback::mark_gc()
{    
    vm.gc.markObject(cbFun_);
}

const std::string& ObjCallback::toString() const 
{
    return name_;
}

std::string ObjCallback::type() const 
{ 
    return "callback"; 
}

bool ObjCallback::callValue(int /* argCount */)
{
    return false;
}

void ObjCallback::setCb(std::function<void(void*, void*, void**)>& f)
{
    cb_->callback(f);
}

void* ObjCallback::pointer()
{
    return cb_->closure();
}

