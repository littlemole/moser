//#include "pch.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "gc.h"

#include <sstream>

/* LOOKUP ORDER

 property lookup:

    - fields
    - methods
    - static methods

method lookup

    - methods
    . static methods
    - fields

*/


Obj::Obj(VM& v) : vm(v)
{
    vm.allocations++;

    vm.objects.push_back(this);

#ifdef DEBUG_VERBOSE_GC
    printf("%p allocate  \n", (void*)this);
#endif
}

Obj::~Obj()
{
    vm.allocations--;
#ifdef DEBUG_VERBOSE_GC
    printf("%p deallocate  %d\n", (void*)this);
#endif
}

bool Obj::callValue(int /* argCount */)
{
    vm.runtimeError("Can only call functions and classes.");
    exit(67);
}


ObjString::ObjString(VM& v) : Obj(v)
{
    init();
}

ObjString::ObjString(VM& v, const char* s, size_t length)
    : Obj(v)
{
    if(s && length > 0)
    {
        chars = std::string(s,length);
    }
    init();
}

ObjString::ObjString(VM& v, const std::string& str)
    : Obj(v), chars(str)
{
    init();
}

ObjString::~ObjString()
{}

void ObjString::mark_gc()
{
    vm.gc.markMap(fields);
    
}


void ObjString::init()
{
    fields["length"] = Value(chars.size());

    auto add = new ObjNativeMethod( vm,
        [](Value that, const std::string& , int argCount, Value* args) -> Value
        {
            auto str = as<ObjString>(that.as.obj);
            if(!str) return NIL_VAL;
            for( int i = 0; i < argCount; i++)
            {
                str = as<ObjString>(str->add(args[i]).as.obj);
            }
            return str;
        }
    );
    fields["add"] = add;

    auto substr = new ObjNativeMethod( vm, 
        [](Value that, const std::string& , int argCount, Value* args) -> Value
        {
            auto str = as<ObjString>(that.as.obj);
            if(!str) return NIL_VAL;

            ptrdiff_t start = 0;
            if(argCount > 0)
            {
                start = args[0].to_int();
            }
            ptrdiff_t n = str->toString().size() - start;
            if(argCount > 1)
            {
                n = args[1].to_int();
            }

            str = new ObjString( str->vm, str->toString().substr(start,n) );
            return str;
        }
    );
    fields["substr"] = substr;

    auto find = new ObjNativeMethod( vm, 
        [](Value that, const std::string& , int argCount, Value* args) -> Value
        {
            auto str = as<ObjString>(that.as.obj);
            if(!str) return NIL_VAL;

            if(argCount == 0)
            {
                return NIL_VAL;
            }

            std::string what = args[0].to_string();
            size_t pos = str->toString().find(what);
            if(pos == std::string::npos)
            {
                return Value(-1);
            }
            return Value(pos);
        }
    );
    fields["find"] = find;

    auto splitStr = new ObjNativeMethod( vm, 
        [](Value that, const std::string& , int argCount, Value* args) -> Value
        {
            auto str = as<ObjString>(that.as.obj);
            if(!str) return NIL_VAL;

            if(argCount == 0)
            {
                return str;
            }

            std::string by = args[0].to_string();
            std::vector<std::string> v = split(str->toString(),by);
            auto array = new ObjArray(str->vm);
            for(auto& it : v)
            {
                array->add(new ObjString(str->vm, it));
            }
            return array;
        }
    );
    fields["split"] = splitStr;

    auto replace_all = new ObjNativeMethod( vm, 
        [](Value that, const std::string& , int argCount, Value* args) -> Value
        {
            auto str = as<ObjString>(that.as.obj);
            if(!str) return NIL_VAL;

            if(argCount < 2)
            {
                return str;
            }

            std::string what = args[0].to_string();
            std::string with = args[1].to_string();
            std::regex rgx(what);
            std::string r = std::regex_replace(str->toString(),rgx,with);
            return new ObjString(str->vm, r);
        }
    );
    fields["replace_all"] = replace_all;

    auto replace = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto str = as<ObjString>(that.as.obj);
            if(!str) return NIL_VAL;

            if(argCount < 2)
            {
                return str;
            }

            std::string what = args[0].to_string();
            std::string with = args[1].to_string();
            std::regex rgx(what);
            std::string r = std::regex_replace(
                str->toString(),
                rgx,
                with,
                std::regex_constants::match_default | std::regex_constants::format_first_only
            );
            return new ObjString(str->vm, r);
        }
    );
    fields["replace"] = replace;

}


Value ObjString::index(ptrdiff_t i) const
{
    if(chars.empty() || i > (ptrdiff_t)chars.size() || i < 0)
    {
        return NIL_VAL;
    }
    return new ObjString( vm, chars.substr(i,1) );
}

Value ObjString::slice(ptrdiff_t index, ptrdiff_t len) const
{
    if(chars.empty() || index > (ptrdiff_t)chars.size() || index < 0)
    {
        return NIL_VAL;
    }
    if(len == -1) len = chars.size() - index;
    if(index+len> (ptrdiff_t)chars.size()) len = chars.size()-index;

    auto s = new ObjString( vm, chars.substr(index,len));
    return s;
}

Value ObjString::index(ptrdiff_t i, Value val)
{
    if(chars.empty() || i > (ptrdiff_t)chars.size() || i < 0)
    {
        return NIL_VAL;
    }
    std::string s = val.to_string();
    if(s.empty()) return NIL_VAL;
    std::string dest = chars;
    dest[i] = s.substr(0,1)[0];
    return new ObjString(vm, dest);
}

Value ObjString::add(Value val)
{
    std::string s = val.to_string();
    if(s.empty()) return new ObjString(vm, s);

    std::string dest = chars;
    dest += s;
    return new ObjString(vm, dest);
}

Value ObjString::size() const
{
    return chars.size();
}


Value ObjString::getProperty(const std::string& name)
{
    if( fields.count(name))
    {
        Value value = fields[name];
        return value;
    }

    return NIL_VAL;
}


void ObjString::setProperty(const std::string& /* key */, Value /* val */)
{
}

void ObjString::deleteProperty(const std::string& /* name */)
{
    
}

Value ObjString::getMethod(const std::string& mname)
{
    return fields[mname];
}

std::vector<std::string> ObjString::keys()
{
    std::vector<std::string> empty;
    return empty;
}

bool ObjString::invokeMethod(const std::string& name, int argCount)
{
    Value val = fields[name];
    if(IS_OBJ(val))
    {
        return val.as.obj->callValue(argCount);
    }
    return false;
}


const std::string& ObjString::toString() const
{
    return chars;
}

ObjFunction::ObjFunction(VM& v, ObjString* n, int cnt, int a) 
: Obj(v), name_(n), upvalueCount_(cnt), arity_(a)
{
    if(name_)
    {
        std::ostringstream oss;
        oss << "<fn " << name_->toString() << ">";
        fun = oss.str();
    }
    else 
    {
        fun = "<script>";
    }
}


int ObjFunction::addUpvalue(Compiler& compiler, uint8_t index, bool isLocal) 
{
    for (int i = 0; i < upvalueCount_; i++) 
    {
        Upvalue* upvalue = &compiler.upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) 
        {
            return i;
        }
    }    

    compiler.upvalues.push_back(Upvalue{index,isLocal});
    return upvalueCount_++;
}


const std::string& ObjFunction::toString() const
{
    if (fun.empty()) 
    {
        static std::string s("<script>");
        return s;
    }
    return fun;
}

void ObjFunction::mark_gc()
{
    vm.gc.markObject(name_);
    vm.gc.markArray(chunk.constants);
}



ObjNativeFun::ObjNativeFun(VM& v) : Obj(v) {}

ObjNativeFun::ObjNativeFun(VM& v, NativeFn f) : Obj(v), function(f)
{}



const std::string& ObjNativeFun::toString()  const
{
    static std::string s("<nativeFun>");
    return s;    
}

bool ObjNativeFun::callValue(int argCount)
{
    //unused: auto frame = &vm.frames.back();
    Value result = function(vm, argCount, &vm.stack.back() - argCount+ 1);

    for( int i = 0; i < argCount;i++)
    {
        vm.pop();
    }
    vm.pop();
    vm.push(result);
    return true;
}

void ObjNativeFun::mark_gc()
{
}

ObjNativeMethod::ObjNativeMethod(VM& v) : Obj(v) {}

ObjNativeMethod::ObjNativeMethod(VM& v, NativeMeth f) : Obj(v), function(f)
{}

ObjNativeMethod::ObjNativeMethod(VM& v, const std::string& n, NativeMeth f) : Obj(v), function(f), name(n)
{}

const std::string& ObjNativeMethod::toString() const
{
    static std::string s("<nativeMethod>");
    return s;
}

void ObjNativeMethod::mark_gc()
{
    
}


bool ObjNativeMethod::callValue(int argCount)
{
    Value* thet = &vm.stack.back() - argCount;
    auto frame = &vm.frames.back();
    Value result = function( *thet, this->name, argCount, &vm.stack.back() - argCount +1);

    // support eval which changes frame
    if( &vm.frames.back() == frame )
    {
        for( int i = 0; i < argCount;i++)
        {
            vm.pop();
        }
        vm.pop();
        vm.push(result);
    }
    else
    {
        //throw "unexpected never reached";
    }
    return true;
}

ObjUpvalue::ObjUpvalue(VM& v, Value* val)
    : Obj(v),
    location(val),
    closed(NIL_VAL)
{}


ObjClosure::ObjClosure(VM& v, ObjFunction* f)
    : Obj(v), function(f), upvalues(f->upvalueCount(), nullptr)
{}


const std::string& ObjClosure::toString() const
{
    return function->toString();
}

void ObjClosure::mark_gc()
{
    vm.gc.markObject((Obj*)function);
    for (size_t i = 0; i < upvalues.size(); i++) 
    {
        vm.gc.markObject((Obj*)upvalues[i]);
    }
}


bool ObjClosure::callValue(int argCount)
{
    return vm.call(this, argCount);      
}

const std::string& ObjUpvalue::toString() const
{
    static std::string s("upvalue");
    return s;
}

bool ObjUpvalue::callValue(int /* argCount */)
{
    return false;  
}

void ObjUpvalue::mark_gc()
{
    vm.gc.markValue(closed);
    
}


std::string ObjUpvalue::type()  const
{ 
    return "upvalue"; 
}

ObjClass::ObjClass(VM& v, ObjString* n) 
    : Obj(v), name(n)
{
    init();
}


const std::string& ObjClass::toString() const
{
    return name->toString();
}

void ObjClass::mark_gc()
{
    vm.gc.markObject((Obj*)name);
    vm.gc.markObject((Obj*)super);
    vm.gc.markMap(methods);
    vm.gc.markMap(getters);
    vm.gc.markMap(setters);
    vm.gc.markMap(fields);
}


bool ObjClass::callValue(int argCount)
{
    vm.stack[vm.stack.size()-argCount - 1] = new ObjInstance(vm, this);

    if(methods.count(name->toString()))
    {
        Value ctor = methods[name->toString()];
        vm.call(as<ObjClosure>(ctor), argCount);
        vm.frames.back().returnToCallerOnReturn = true;
        //unused: Value r = 
        vm.run();
    }
    else if (argCount != 0) 
    {
        vm.runtimeError("Expected 0 arguments but got %d.",argCount);
        exit(77);
    }


    if(!IS_NIL(metadata))
    {
        GC::Lock guard(vm);
        auto map = as<ObjMap>(metadata);
        if(map)
        {
            Value pm = map->item("Proxy");
            if(!IS_NIL(pm))
            {
                auto array = as<ObjArray>(pm);
                if(array)
                {
                    Value r = vm.pop();
                    Value p;
                    for( int i = 0; i < array->size().to_int(); i++)
                    {
                        auto val = vm.globals[array->index(i).to_string()];
                        auto clazz = as<ObjClass>(val);
                        if(clazz)
                        {
                            vm.push(val);
                            clazz->callValue(0);
                            Value v = vm.pop();
                            p = new ObjProxy(
                                vm,
                                r.as.obj,
                                v.as.obj
                            );
                            r = p;
                        }
                    }
                    vm.push(p);
                }
                else
                {
                    Value r = vm.pop();

                    auto val = vm.globals[pm.to_string()];
                    auto clazz = as<ObjClass>(val);
                    if(clazz)
                    {
                        vm.push(val);
                        clazz->callValue(0);
                        Value i = vm.pop();
                    
                        Value p = new ObjProxy(vm, r.as.obj,i.as.obj);
                        vm.push(p);
                    }
                }
            }
        }
    }


    return true;
}

Value ObjClass::bindMethod(ObjInstance* instance, const std::string& mname) 
{
    if(!methods.count(mname))
    {
        return NIL_VAL;
    }

    Value method = methods[mname];

    ObjBoundMethod* bound = new ObjBoundMethod( vm, instance, method.as.obj);
    return bound;
}


Value ObjClass::getMethod(const std::string& mname)
{
    if(!fields.count(mname))
    {
        return NIL_VAL;
    }

    Value method = fields[mname];
    auto closure = as<ObjClosure>(method);
    if(!closure)
    {
        auto bound = as<ObjBoundMethod>(method);
        if(bound)
        {
            return bound;
        }
        auto meth = as<ObjNativeMethod>(method);
        if(meth)
        {
            return meth;
        }
        return NIL_VAL;
    }

    ObjBoundMethod* bound = new ObjBoundMethod( vm, this, closure);
    return bound;
}

bool ObjClass::invokeMethod(const std::string& mname, int argCount)
{
    Value prop = getMethod(mname);
    if(IS_OBJ(prop))
    {
        if(prop.as.obj->callValue(argCount))
        {
            return true;
        }
        return false;
    }

    return false;
}


Value ObjClass::getProperty(const std::string& pname)
{
    if( fields.count(pname))
    {
        Value value = fields[pname];
        return value;
    }
    if( methods.count(pname))
    {
        Value value = methods[pname];
        return value;
    }
    return NIL_VAL;
}

void ObjClass::setProperty(const std::string& key, Value val)
{
    fields[key] = val;
}

void ObjClass::deleteProperty(const std::string& pname)
{
    if(fields.count(pname))
    {
        fields.erase(pname);
    }
}


std::vector<std::string> ObjClass::keys()
{
    std::vector<std::string> k;
    for(auto& it : fields)
    {
        k.push_back(it.first);
    }
    return k;
}

void ObjClass::init()
{
    auto keys = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int /* argCount */, Value* /* args */) -> Value
        {
            auto map = as<ObjClass>(that.as.obj);
            if(!map) return NIL_VAL;

            ObjArray* array = new ObjArray(map->vm);
            auto k = map->keys();
            for( auto& it : k)
            {
                if(it.starts_with("_")) continue;
                Value v = map->getProperty(it);
                if(IS_OBJ(v))
                {
                    auto m = as<ObjClosure>(v);
                    if(m) continue;
                    auto b = as<ObjBoundMethod>(v);
                    if(b) continue;
                    auto n = as<ObjNativeMethod>(v);
                    if(n) continue;
                }
                array->add(new ObjString(map->vm, it));
            }
            return array;
        }
    );
    fields["keys"] = keys;

    auto meths = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int /* argCount */, Value* /* args */) -> Value
        {
            auto clazz = as<ObjClass>(that.as.obj);
            if(!clazz) return NIL_VAL;

            ObjArray* array = new ObjArray(clazz->vm);
            auto& meths = clazz->methods;
            for( auto& it : meths)
            {
                if(it.first.starts_with("_")) continue;
                array->add(new ObjString(clazz->vm, it.first));
            }
            auto k = clazz->keys();
            for( auto& it : k)
            {
                if(it.starts_with("_")) continue;
                array->add(new ObjString(clazz->vm, it));
            }
            return array;
        }
    );
    fields["methods"] = meths;

}

void ObjClass::inherit(ObjClass* superClass)
{
    if(!superClass) return;
    super = superClass;
    fields["base"] = super;

    for( auto& it : superClass->methods)
    {
        methods[it.first] = it.second;
    }        
    for( auto& it : superClass->getters)
    {
        getters[it.first] = it.second;
    }        
    for( auto& it : superClass->setters)
    {
        setters[it.first] = it.second;
    }        
}

ObjInstance::ObjInstance(VM& v) : Obj(v) 
{ 
    init(); 
}


ObjInstance::ObjInstance(VM& v, ObjClass* k)  
    : Obj(v), klass(k)
{
    std::ostringstream oss;
    oss << klass->name->toString() << " instance";
    name = oss.str();

    init();
}

void ObjInstance::finalize() 
{
    if(  klass)
    {
        std::string dtor("~");
        dtor += klass->name->toString();
        if(klass->methods.count(dtor))
        {
            vm.push(this);
            invokeMethod(dtor,0);
            vm.frames.back().returnToCallerOnReturn = true;
            //unused: Value v = 
            vm.run();
            vm.pop();
        }
    }
}

void ObjInstance::init()
{
    if(klass)
    {
        fields["className"] = klass->name;
        fields["type"] = klass;
    }
    auto keys = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int /* argCount */, Value* /* args */) -> Value
        {
            auto map = as<ObjInstance>(that.as.obj);
            if(!map) return NIL_VAL;

            ObjArray* array = new ObjArray(map->vm);
            auto k = map->keys();
            for( auto& it : k)
            {
                if(it.starts_with("_")) continue;
                Value v = map->getProperty(it);
                if(IS_OBJ(v))
                {
                    auto m = as<ObjClosure>(v);
                    if(m) continue;
                    auto b = as<ObjBoundMethod>(v);
                    if(b) continue;
                    auto n = as<ObjNativeMethod>(v);
                    if(n) continue;
                }
                array->add(new ObjString(map->vm, it));
            }
            return array;
        }
    );
    fields["keys"] = keys;

    auto meths = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int /* argCount */, Value* /* args */) -> Value
        {
            auto instance = as<ObjInstance>(that.as.obj);
            if(!instance) return NIL_VAL;

            ObjMap* map = new ObjMap(instance->vm);
            auto& meths = instance->klass->methods;
            for( auto& it : meths)
            {
                if(it.first.starts_with("_")) continue;
                Value v = instance->klass->bindMethod(instance,it.first);                
                map->item(it.first,v);
            }
            auto k = instance->keys();
            for( auto& it : k)
            {
                if(it.starts_with("_")) continue;
                Value v = instance->getProperty(it);
                if(IS_OBJ(v))
                {
                    auto m = as<ObjClosure>(v);
                    if(m) {
                        map->item( it,v);
                        continue;
                    }
                    auto b = as<ObjBoundMethod>(v);
                    if(b) map->item( it,v);
                }                
            }
            return map;
        }
    );
    fields["methods"] = meths;

}


Value ObjInstance::getMethod(const std::string& mname)
{
    if(!fields.count(mname))
    {
        return NIL_VAL;
    }

    Value method = fields[mname];
    auto closure = as<ObjClosure>(method);
    if(!closure)
    {
        auto bound = as<ObjBoundMethod>(method);
        if(bound)
        {
            return bound;
        }
        auto meth = as<ObjNativeMethod>(method);
        if(meth)
        {
            return meth;
        }
        return NIL_VAL;
    }

    ObjBoundMethod* bound = new ObjBoundMethod( vm, this, closure);
    return bound;
}

bool ObjInstance::invokeMethod(const std::string& mname, int argCount)
{
    Value prop = getMethod(mname);
    if(IS_OBJ(prop))
    {
        if(prop.as.obj->callValue(argCount))
        {
            return true;
        }
        return false;
    }

    Value m = klass->bindMethod(this,mname);
    if(IS_NIL(m))
    {
        return false;
    }
    if(IS_OBJ(m))
    {
        return m.as.obj->callValue(argCount);
    }
    return false;
}

Value ObjInstance::getProperty(const std::string& pname)
{
    if(klass->getters.count(pname))
    {
        Value getter = klass->getters[pname];
        auto meth = as<ObjClosure>(getter);
        if(!meth) return NIL_VAL;

        ObjBoundMethod* bound = new ObjBoundMethod( vm, this, as<ObjClosure>(meth));
        vm.push(bound);
        bound->callValue(0);
//        vm.call(bound,0);
        vm.frames.back().returnToCallerOnReturn = true;
        Value r = vm.run();
        vm.pop();
        return r;
    }
    if( fields.count(pname))
    {
        Value value = fields[pname];

        auto bound = as<ObjBoundMethod>(value);
        if(bound)
        {
            return bound;
        }
        auto meth = as<ObjNativeMethod>(value);
        if(meth)
        {
            return meth;
        }

        auto closure = as<ObjClosure>(value);
        if(closure)
        {
            ObjBoundMethod* bound2 = new ObjBoundMethod( vm, this, closure);
            return bound2;
        }
        return value;
    }

    Value bound = klass->bindMethod(this,pname);
    return bound;
}

void ObjInstance::setProperty(const std::string& key, Value val)
{
    auto bound = as<ObjBoundMethod>(val);
    if(bound)
    {
        val = bound->method;
    }
    if(klass->setters.count(key))
    {
        Value setter = klass->setters[key];
        auto meth = as<ObjClosure>(setter);
        if(!meth) return;

        ObjBoundMethod* bound2 = new ObjBoundMethod( vm, this, as<ObjClosure>(meth));

        vm.push(bound2);
        vm.push(val);
        bound2->callValue(1);
//        vm.call(bound,1);
        vm.frames.back().returnToCallerOnReturn = true;
        //unused: Value r = 
        vm.run();
        vm.pop();
        return;
    }
    fields[key] = val;
}

void ObjInstance::deleteProperty(const std::string& pname)
{
    if(fields.count(pname))
    {
        fields.erase(pname);
    }
}


std::vector<std::string> ObjInstance::keys()
{
    std::vector<std::string> k;
    for(auto& it : fields)
    {
        k.push_back(it.first);
    }
    return k;
}


const std::string& ObjInstance::toString() const
{
    if(name.empty())
    {
        static std::string s("<klass>");
        return s;
    }
    return name;
}

void ObjInstance::mark_gc()
{
    vm.gc.markObject((Obj*)klass);
    vm.gc.markMap(fields);
}

ObjBoundMethod::ObjBoundMethod(VM& v, Value r, Obj* m)
    : Obj(v), method(m), receiver(r)
{}


ObjBuiltin::ObjBuiltin(VM& v) : ObjInstance(v) {}


ObjArray::ObjArray(VM& v) : ObjBuiltin(v)
{
    fields["length"] = 0;

    auto contains = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto array = as<ObjArray>(that.as.obj);
            if(!array) return NIL_VAL;

            if(argCount < 1) return NIL_VAL;

            Value key = args[0];

            for( auto& it : array->values)
            {
                if(it.isEqual(key)) return true;
            }
            return false;
        }
    );
    methods["contains"] = contains;

    auto joinArray = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto array = as<ObjArray>(that.as.obj);
            if(!array) return NIL_VAL;

            if(argCount < 1) return NIL_VAL;

            Value key = args[0];
            std::vector<std::string> v;

            for( auto& it : array->values)
            {
                v.push_back(it.to_string());
            }
            std::string r = join(v,key.to_string());

            return new ObjString(array->vm, r);
        }
    );
    methods["join"] = joinArray;

    auto pop = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int /* argCount */, Value* /* args */) -> Value
        {
            auto array = as<ObjArray>(that.as.obj);
            if(!array) return NIL_VAL;

            return array->pop();
        }
    );
    methods["pop"] = pop;    

    auto back = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int /* argCount */, Value* /* args */) -> Value
        {
            auto array = as<ObjArray>(that.as.obj);
            if(!array) return NIL_VAL;

            ptrdiff_t len = array->size().to_int();
            if(len == 0) return NIL_VAL;

            return array->index(len-1);
        }
    );
    methods["back"] = back;    

    auto add = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto array = as<ObjArray>(that.as.obj);
            if(!array) return NIL_VAL;
            for( int i = 0; i < argCount; i++)
            {
                array->add(args[i]);
            }
            return NIL_VAL;
        }
    );
    methods["add"] = add;
    methods["push"] = add;

    auto forEach = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            if(argCount == 0) return NIL_VAL;

            auto f = as<ObjClosure>(args[0]);
            if(!f) return NIL_VAL;

            auto array = as<ObjArray>(that.as.obj);
            if(!array) return NIL_VAL;

            ptrdiff_t len = array->size().to_int();
            for(ptrdiff_t i = 0; i < len; i++)
            {
                Value v = array->index(i);
                f->vm.execute(f, v);
            }
            return NIL_VAL;
        }
    );
    methods["forEach"] = forEach;

    auto filter = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            if(argCount == 0) return NIL_VAL;

            auto f = as<ObjClosure>(args[0]);
            if(!f) return NIL_VAL;

            auto array = as<ObjArray>(that.as.obj);
            if(!array) return NIL_VAL;

            ptrdiff_t len = array->size().to_int();

            ObjArray* result = nullptr;
            result = new ObjArray(f->vm);
            f->vm.gc.pin(result);
            for( int i = 0; i < len; i++)
            {
                Value r = f->vm.execute(f,array->index(i));
                if(r.to_int() != 0)
                {
                    result->add(array->index(i));
                }
            }
            f->vm.gc.unpin(result);
            return result;
        }
    );
    methods["filter"] = filter;

    auto transform = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            if(argCount == 0) return NIL_VAL;

            auto f = as<ObjClosure>(args[0]);
            if(!f) return NIL_VAL;

            auto array = as<ObjArray>(that.as.obj);
            if(!array) return NIL_VAL;

            ptrdiff_t len = array->size().to_int();

            ObjArray* result = nullptr;
            result = new ObjArray(f->vm);
            f->vm.gc.pin(result);
            for( int i = 0; i < len; i++)
            {
                Value r = f->vm.execute(f,array->index(i));
                if(!IS_NIL(r))
                {
                    result->add(r);
                }
            }
            f->vm.gc.unpin(result);
            return result;
        }
    );
    methods["transform"] = transform;

    auto sort = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            auto array = as<ObjArray>(that.as.obj);
            if(!array) return NIL_VAL;

            ptrdiff_t len = array->size().to_int();

            std::vector<Value> tobesorted;
            for( int i = 0; i < len; i++)
            {
                tobesorted.push_back(array->index(i));
            }

            if(argCount == 0)
            {
                std::sort(
                    tobesorted.begin(), 
                    tobesorted.end(),
                    [that](Value lhs, Value rhs) mutable
                    {
                        if( IS_OBJ(lhs))
                        {
                            auto s1 = lhs.to_string();
                            auto s2 = rhs.to_string();
                            return s1 < s2;
                        }
                        return lhs.to_double() < rhs.to_double();
                    }
                );
            }
            else
            {
                auto f = as<ObjClosure>(args[0]);
                if(!f) return NIL_VAL; 

                std::sort(
                    tobesorted.begin(), 
                    tobesorted.end(),
                    [that,array,f](Value lhs, Value rhs) mutable
                    {
                        return array->vm.execute(f,lhs,rhs).to_int();
                    }
                );
            }

            auto result = new ObjArray(array->vm);
            array->vm.gc.pin(result);
            for( auto& it : tobesorted)
            {
                result->add(it);
            }
            array->vm.gc.unpin(result);
            return result;
        }
    );
    methods["sort"] = sort;

}

void ObjArray::mark_gc()
{    
    vm.gc.markMap(fields);
    vm.gc.markMap(methods);
    vm.gc.markArray(values);
}

Value ObjArray::pop()
{
    if(values.empty()) return NIL_VAL;
    Value v = values.back();
    values.pop_back();
    fields["length"] = values.size();
    return v;
}

Value ObjArray::index(ptrdiff_t i) const
{
    if(values.empty() || i > (ptrdiff_t)values.size() || i < 0)
    {
        return NIL_VAL;
    }
    return values[i];
}

Value ObjArray::slice(ptrdiff_t index, ptrdiff_t len) const
{
    if(values.empty() || index > (ptrdiff_t)values.size() || index < 0)
    {
        return new ObjArray(vm);
    }
    if(len == -1) len = values.size() - index;
    ptrdiff_t s = index + len;
    if( s > (ptrdiff_t)values.size() ) s = values.size();
    auto a = new ObjArray(vm);
    for(ptrdiff_t i = index; i < s; i++)
    {
        a->add(values[i]);
    }
    return a;
}

Value ObjArray::index(ptrdiff_t i, Value val)
{
    if(values.empty() || i > (ptrdiff_t)values.size() || i < 0)
    {
        return NIL_VAL;
    }
    values[i] = val;
    return val;
}

Value ObjArray::add(Value val)
{
    values.push_back(val);
    fields["length"] = values.size();
    return this;
}

Value ObjArray::size() const
{
    return values.size();
}

Value ObjArray::getMethod(const std::string& mname)
{
    if (methods.count(mname) == 0) return NIL_VAL;
    return methods[mname];
}

bool ObjArray::invokeMethod(const std::string& mname, int argCount)
{
    if(methods.count(mname))
    {
        Value val = methods[mname];
        if(IS_OBJ(val))
        {
            auto m = as<Methodable>(val.as.obj);
            if(m)
            {
                return val.as.obj->callValue(argCount);
            }
            auto n = as<ObjNativeMethod>(val.as.obj);
            if(n)
            {
                return val.as.obj->callValue(argCount);
            }

        }
    }
    return false;
}


Value ObjArray::getProperty(const std::string& pname)
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

void ObjArray::deleteProperty(const std::string& /* name */)
{
}


std::vector<std::string> ObjArray::keys()
{
    std::vector<std::string> k;
    for(auto& it : fields)
    {
        k.push_back(it.first);
    }
    return k;
}

const std::string& ObjArray::toString() const
{
    std::ostringstream oss;
    oss << "array [ ";
    for( size_t i = 0; i < values.size(); i++)
    {
        oss << index(i).to_string() << ", ";
    }
    oss << "]";
    displayName = oss.str();
    return displayName;
}



ObjMap::ObjMap(VM& vm) : ObjBuiltin(vm)
{
    fields["length"] = 0;

    auto exists = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args ) -> Value
        {
            auto map = as<ObjMap>(that.as.obj);
            if(!map) return NIL_VAL;

            if(argCount < 1) return NIL_VAL;

            std::string key = args[0].to_string();

            for( auto& it : map->elements)
            {
                if(it.first == key) return true;
            }
            return false;
        }
    );
    methods["exists"] = exists;

    auto keys = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int /* argCount */, Value* /* args */) -> Value
        {
            auto map = as<ObjMap>(that.as.obj);
            if(!map) return NIL_VAL;

            ObjArray* array = new ObjArray(map->vm);
            for( auto& it : map->elements)
            {
                array->add(new ObjString(map->vm, it.first));
            }
            return array;
        }
    );
    methods["keys"] = keys;

    auto forEach = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            if(argCount == 0) return NIL_VAL;

            auto f = as<ObjClosure>(args[0]);
            if(!f) return NIL_VAL;

            auto map = as<ObjMap>(that.as.obj);
            if(!map) return NIL_VAL;

            for( auto& it : map->elements)
            {
                //unused: Value r = 
                map->vm.execute(f,Value(new ObjString(map->vm, it.first)),it.second);
            }
            return NIL_VAL;
        }
    );
    methods["forEach"] = forEach;

    auto filter = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            if(argCount == 0) return NIL_VAL;

            auto f = as<ObjClosure>(args[0]);
            if(!f) return NIL_VAL;

            auto map = as<ObjMap>(that.as.obj);
            if(!map) return NIL_VAL;

            ObjMap* result = new ObjMap(map->vm);
            map->vm.gc.pin(result);
            
            for( auto& it : map->elements )
            {
                Value r = map->vm.execute(f,Value(new ObjString(map->vm,it.first)),it.second);
                if(r.to_int() != 0)
                {
                    result->item(it.first,it.second);
                }
            }
            map->vm.gc.unpin(result);
            return result;
        }
    );
    methods["filter"] = filter;

    auto transform = new ObjNativeMethod( vm,
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            if(argCount == 0) return NIL_VAL;

            auto f = as<ObjClosure>(args[0]);
            if(!f) return NIL_VAL;

            auto map = as<ObjMap>(that.as.obj);
            if(!map) return NIL_VAL;

            ObjMap* result = new ObjMap(map->vm);
            map->vm.gc.pin(result);
            
            for( auto& it : map->elements )
            {
                Value r = map->vm.execute(f,Value(new ObjString(map->vm, it.first)),it.second);
                if(!IS_NIL(r))
                {
                    auto rm = ::as<ObjMap>(r);
                    if(rm)
                    {
                        for( auto& j : rm->elements)
                        {
                            result->item( j.first, j.second);
                        }
                    }
                    else
                    {
                        if(r.to_int())
                        {
                            result->item(it.first,it.second);
                        }
                    }
                }
            }
            map->vm.gc.unpin(result);
            return result;
        }
    );
    methods["transform"] = transform;


}

void ObjMap::mark_gc()
{    
    vm.gc.markMap(elements);
    vm.gc.markMap(methods);
    vm.gc.markMap(fields);
}


Value ObjMap::item(const std::string& s)
{
    if(elements.empty() || elements.count(s) == 0)
    {
        return NIL_VAL;
    }
    return elements[s];
}

Value ObjMap::item(const std::string& key, Value val)
{
    elements[key] = val;
    fields["length"] = elements.size();
    return val;
}

Value ObjMap::size()
{
    return elements.size();
}

Value ObjMap::getMethod(const std::string& mname)
{
    if(methods.count(mname)) return methods[mname];
    if(elements.count(mname)) return elements[mname];
    return NIL_VAL;
}

bool ObjMap::invokeMethod(const std::string& mname, int argCount)
{
    Value m = getMethod(mname);
    if(IS_OBJ(m)) 
    {
        return m.as.obj->callValue(argCount);
    }
    return false;
}


Value ObjMap::getProperty(const std::string& pname)
{
    if(elements.count(pname))
    {
        return elements[pname];
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


void ObjMap::setProperty(const std::string& key, Value val)
{
    elements[key] = val;
}

void ObjMap::deleteProperty(const std::string& pname)
{
    if(elements.count(pname))
    {
        elements.erase(pname);
    }
}

std::vector<std::string> ObjMap::keys()
{
    std::vector<std::string> k;
    for(auto& it : elements)
    {
        k.push_back(it.first);
    }
    return k;
}

const std::string& ObjMap::toString() const
{
    std::ostringstream oss;
    oss << "map { ";
    for( auto& it : elements)
    {
        oss << it.first << " : " << it.second.to_string() << ", ";
    }
    oss << "}";
    displayName = oss.str();
    return displayName;
}

const std::string& ObjBoundMethod::toString() const
{
    auto closure = as<ObjClosure>(method);
    if(closure)
    {
        return closure->function->toString();
    }
    auto p = as<ObjDecorator>(method);
    if(p)
    {
        auto closure2 = as<ObjClosure>(p->getProperty("target").as.obj);
        if(closure2)
        {
            return closure2->function->toString();
        }
    }
    static std::string unk = "unk";
    return unk;
}

bool ObjBoundMethod::callValue(int argCount)
{
    vm.stack[vm.stack.size()-argCount -1 ] = receiver;                

    auto closure = as<ObjClosure>(method);
    if(closure)
    {
        return vm.call(closure, argCount);
    }

    auto p = as<ObjDecorator>(method);
    if(p)
    {
        return p->callValue(receiver,argCount);
    }
    return false;
}

void ObjBoundMethod::mark_gc()
{    
    vm.gc.markValue(receiver);
    vm.gc.markObject((Obj*)method);
}


static auto regopts(const std::string& s)
{
    auto flags = std::regex_constants::ECMAScript;

    size_t pos = std::string::npos;
    pos = s.find("i");
    if (pos != std::string::npos) flags |= std::regex_constants::icase;
    return flags;
}

ObjRegex::ObjRegex(VM& v, const std::string& r)
    : ObjBuiltin(v), value(r),regex_(r)
{
    init();
}

ObjRegex::ObjRegex(VM& vm, const std::string& r, const std::string& o)
    : ObjBuiltin(vm), value(r),regex_(r, regopts(o)), options_(o)
{
    init();
}

void ObjRegex::init()
{
    fields["suffix"] =  new ObjString(vm, "");

    auto match = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {            
            if(argCount == 0) return NIL_VAL;

            auto rgx = as<ObjRegex>(that);
            if(!rgx) return NIL_VAL;

            std::string s = args[0].to_string();
            bool found = std::regex_match(s, rgx->smatch_, rgx->regex_);
            if (!found) return NIL_VAL;

            auto array = new ObjArray(rgx->vm);
            array->add(  new ObjString(rgx->vm, rgx->smatch_[0]));

            for (size_t i = 1; i < rgx->smatch_.size(); i++)
            {
                array->add( new ObjString(rgx->vm, rgx->smatch_[i]));
            }
            return array;
        }
    );
    methods["match"] = match;

    auto search = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {            
            if(argCount == 0) return NIL_VAL;

            auto rgx = as<ObjRegex>(that);
            if(!rgx) return NIL_VAL;

            std::string s = args[0].to_string();
            bool found = std::regex_search(s, rgx->smatch_, rgx->regex_);
            if (!found) return NIL_VAL;

            auto array = new ObjArray(rgx->vm);
            array->add( new ObjString(rgx->vm, rgx->smatch_[0]));

            for (size_t i = 1; i < rgx->smatch_.size(); i++)
            {
                array->add( new ObjString(rgx->vm, rgx->smatch_[i]));
            }

            rgx->suffix_ = rgx->smatch_.suffix();
            rgx->fields["suffix"] = new ObjString(rgx->vm, rgx->suffix_ );
            return array;
        }
    );
    methods["search"] = search;

    auto replace = new ObjNativeMethod( vm, 
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {            
            if(argCount < 2) return NIL_VAL;

            auto rgx = as<ObjRegex>(that);
            if(!rgx) return NIL_VAL;

            std::string s = args[0].to_string();
            std::string with = args[1].to_string();
            std::string r = std::regex_replace(s,rgx->regex_,with,
                std::regex_constants::match_default | std::regex_constants::format_first_only
            );

            return new ObjString(rgx->vm, r);
        }
    );
    methods["replace"] = replace;

    auto replace_all = new ObjNativeMethod( vm,
        [](Value that, const std::string&, int argCount, Value* args) -> Value
        {            
            if(argCount < 2) return NIL_VAL;

            auto rgx = as<ObjRegex>(that);
            if(!rgx) return NIL_VAL;

            std::string s = args[0].to_string();
            std::string with = args[1].to_string();
            std::string r = std::regex_replace(s,rgx->regex_,with);

            return new ObjString(rgx->vm, r);
        }
    );
    methods["replace_all"] = replace_all;

}

Value ObjRegex::getMethod(const std::string& mname)
{
    if (methods.count(mname) == 0) return NIL_VAL;
    return methods[mname];
}

bool ObjRegex::invokeMethod(const std::string& mname, int argCount)
{
    if (methods.count(mname) == 0) return false;

    Value val = methods[mname];
    if(IS_OBJ(val))
    {
        return val.as.obj->callValue(argCount);
    }
    return false;
}

Value ObjRegex::getProperty(const std::string& pname)
{
    if(!fields.count(pname)) return NIL_VAL;
    return fields[pname];
}

void ObjRegex::setProperty(const std::string& /* key */, Value /* val */)
{
//    fields[key] = val;
}


void ObjRegex::deleteProperty(const std::string& /* name */)
{
}

std::vector<std::string> ObjRegex::keys()
{
    std::vector<std::string> k;
    for(auto& it : fields)
    {
        k.push_back(it.first);
    }
    return k;
}

const std::string& ObjRegex::toString() const
{
    if(value.empty())
    {
        static std::string s("<regex>");
        return s;
    }
    return value;
}

void ObjRegex::mark_gc()
{    
    vm.gc.markMap(fields);
    vm.gc.markMap(methods);
}


////////////////////////////////////////////

ObjFunction* get_fun(Obj* f)
{
    auto closure = as<ObjClosure>(f);
    if(closure)
    {
        return closure->function;
    }
    auto bm = as<ObjBoundMethod>(f);
    if(bm)
    {
        return get_fun(bm->method);
    }
    auto p = as<ObjDecorator>(f);
    if(p)
    {
        return get_fun(p->getProperty("target").as.obj);
    }
    return nullptr;
}

ObjDecorator::ObjDecorator(VM& v) : ObjBuiltin(v)
{}


ObjDecorator::ObjDecorator(VM& v, Obj* target, Obj* proxy)
    : ObjBuiltin(v)
{
    fields["target"] =  target;
    fields["proxy"] =  proxy;
}

Value ObjDecorator::getMethod(const std::string& mname)
{
    if (methods.count(mname) == 0) return NIL_VAL;
    return methods[mname];
}

bool ObjDecorator::callValue(int argCount)
{
    Value target = fields["target"];
    Value proxy  = fields["proxy"];

    std::vector<Value> args;
    for(int i = 0; i < argCount; i++)
    {
        args.push_back(vm.stack[vm.stack.size()-argCount+i]);
    }
    for(int i = 0; i < argCount; i++)
    {
        vm.pop();
    }

    vm.push(target);

    for(int i = 0; i < argCount; i++)
    {
        vm.push(args[i]);
    }

    proxy.as.obj->callValue(argCount+1);
    return true;   
}

bool ObjDecorator::callValue(Value receiver, int argCount)
{
    Value target = fields["target"];
    Value proxy  = fields["proxy"];

    auto closure = as<ObjClosure>(target);
    if(closure)
    {
        target = new ObjBoundMethod(vm, receiver,target.as.obj);
    }

    std::vector<Value> args;
    for(int i = 0; i < argCount; i++)
    {
        args.push_back(vm.stack[vm.stack.size()-argCount+i]);
    }
    for(int i = 0; i < argCount; i++)
    {
        vm.pop();
    }


    vm.push(target);
    ObjString* fname = nullptr;

    auto fun = get_fun(target.as.obj);
    if(fun)
    {
        fname = fun->name();
    }

    if(!fname)
    {
        fname = new ObjString(vm, target.to_string());
    }

    vm.push(fname);
    for(int i = 0; i < argCount; i++)
    {
        vm.push(args[i]);
    }

    proxy.as.obj->callValue(argCount+2);
    return true;   
}

bool ObjDecorator::invokeMethod(const std::string& /*mname*/, int /*argCount*/)
{
    return false;
}

Value ObjDecorator::getProperty(const std::string& pname)
{
    Value target = fields["target"];
//    Value proxy  = fields["proxy"];

    auto obj = as<ObjInstance>(target);
    if(obj)
    {
        Value m = obj->getProperty(pname);
        return m;
//        return new ObjDecorator(vm, m.as.obj,proxy.as.obj);
    }

    auto p = as<ObjDecorator>(target);
    if(p)
    {
        return p->getProperty(pname);
    }
    return NIL_VAL;
}

void ObjDecorator::setProperty(const std::string& /* key */, Value /* val */)
{
}


void ObjDecorator::deleteProperty(const std::string& /* name */)
{
}

std::vector<std::string> ObjDecorator::keys()
{
    std::vector<std::string> k;
    for(auto& it : fields)
    {
        k.push_back(it.first);
    }
    return k;
}

const std::string& ObjDecorator::toString() const
{
    static std::string s("<Decorator>");
    return s;
}

void ObjDecorator::mark_gc()
{
    vm.gc.markMap(fields);
    vm.gc.markMap(methods);
}


////////////////////////////////////////////////////////////////////////

ObjProxy::ObjProxy(VM& v) : ObjDecorator(v)
{}

ObjProxy::ObjProxy(VM& v, Obj* target, Obj* proxy)
    :ObjDecorator(v, target,proxy)
{
}

Value ObjProxy::getMethod(const std::string& mname)
{
    if (methods.count(mname) == 0) return NIL_VAL;
    // not used
    return methods[mname];
}

bool ObjProxy::callValue(int /* argCount */ )
{
    // not used
    return false;
}

bool ObjProxy::callValue(Value /* receiver */, int /* argCount */)
{
    // not used
    return false;
}

bool ObjProxy::invokeMethod(const std::string& mname, int argCount)
{
    Value target = fields["target"];
    Value proxy  = fields["proxy"];

    std::vector<Value> args;
    for(int i = 0; i < argCount; i++)
    {
        args.push_back(vm.stack[vm.stack.size()-argCount+i]);
    }
    for(int i = 0; i < argCount; i++)
    {
        vm.pop();
    }

    auto obj = as<ObjInstance>(target);
    if(obj)
    {
       // target = obj->getProperty(name);
    }

    {
        GC::Lock guard(vm);

        vm.push(proxy);
        vm.push(target);
        vm.push(new ObjString(vm, mname));
        for(int i = 0; i < argCount; i++)
        {
            vm.push(args[i]);
        }
    }

    obj = as<ObjInstance>(proxy);
    if(obj)
    {
        vm.gc.pin(this);
        obj->invokeMethod("invoke",argCount+2);
        vm.frames.back().returnToCallerOnReturn = true;
        Value r = vm.run();
        vm.pop(); 
        vm.pop();
        vm.push(r);
        vm.gc.unpin(this);
    }

    return true;
}

Value ObjProxy::getProperty(const std::string& pname)
{
    Value target = fields["target"];
    Value proxy  = fields["proxy"];

    if (pname == "target") return target;
    if (pname == "proxy") return proxy;

    auto obj = as<ObjInstance>(target);
    if(obj)
    {
      //  target = obj->getProperty(name);
    }

    vm.push(proxy);
    vm.push(target);
    vm.push(new ObjString(vm, pname));

    obj = as<ObjInstance>(proxy);
    if(obj)
    {
        obj->invokeMethod("getter",2);
        vm.frames.back().returnToCallerOnReturn = true;
        Value r = vm.run();
        vm.pop();
        vm.pop();
        vm.push(r);
        
        return r;
    }
    return NIL_VAL;
}

void ObjProxy::setProperty(const std::string& key, Value val)
{
    Value target = fields["target"];
    Value proxy  = fields["proxy"];

    auto obj = as<ObjInstance>(target);
    if(obj)
    {
      //  target = obj->getProperty(name);
    }

    vm.push(proxy);
    vm.push(target);
    vm.push(new ObjString(vm, key));
    vm.push(val);

    obj = as<ObjInstance>(proxy);
    if(obj)
    {
        obj->invokeMethod("setter",3);
        vm.frames.back().returnToCallerOnReturn = true;
        //unused: Value r = 
        vm.run();
        vm.pop();
        //return r;
    }
//    return NIL_VAL;
}


void ObjProxy::deleteProperty(const std::string& /* name */)
{
}

std::vector<std::string> ObjProxy::keys()
{
    Value target = fields["target"];

    std::vector<std::string> k;

    auto obj = as<ObjInstance>(target);
    if(obj)
    {
        for(auto& it : fields)
        {
            k.push_back(it.first);
        }
    }
    return k;
}

const std::string& ObjProxy::toString() const
{
    static std::string s("<Proxy>");
    return s;
}

void ObjProxy::mark_gc()
{    
    vm.gc.markMap(fields);
    vm.gc.markMap(methods);
}





