//#include "pch.h"
#include "lib.h"
#include "foreign.h"
#include "gc.h"
#include "vm.h"
#include "must.h"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <chrono>
#include "serialize.h"

#include "nlohmann/json.hpp"
#include "dirent.h"
#ifdef _WIN32
#include <stdio.h>
#include "shlwapi.h"
#define popen _popen
#define pclose _pclose
#include "win32/winrtmarshal.h"
#include "win32/winrtdelegate.h"
#include "win32/comobj.h"
#include "win32/disp.h"
#include "win32/uni.h"
#include "roapi.h"
#include <windows.foundation.h>
#include "win32/xaml.h"
#include "ddeml.h"
#else
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef ENABLE_MOSER_XAML
#include "MoserX.h"
#endif

using namespace nlohmann;

json toJson(VM& vm, Value value);

#ifdef _WIN32

static HMENU makeSubMenu(ObjArray* array)
{
    HMENU menu = CreatePopupMenu();

    size_t len = array->size().to_int();
    for(size_t i = 0; i < len; i++)
    {
        auto sub = as<ObjArray>(array->index(i));
        if(sub && sub->size().to_int() > 1)
        {
            std::string label = sub->index(0).to_string();
            Value value = sub->index(1);

            auto a = as<ObjArray>(value);
            if(a)
            {
                HMENU subMenu = makeSubMenu(a);
                ::AppendMenuW( menu, MF_STRING | MF_POPUP, (UINT_PTR)subMenu, to_wstring(label).c_str());
            }
            else
            {
                ::AppendMenuW(menu, MF_STRING, (UINT_PTR)value.to_int(), to_wstring(label).c_str());
            }
        }
    }

    return menu;
}

Value win32MenuNative(VM& vm, int argCount, Value* args)
{
    if(argCount<1) return NIL_VAL;
    Value v = args[0];
    auto array = as<ObjArray>(v);
    if(!array) return NIL_VAL;

    HMENU hMenu = ::CreateMenu();

    size_t len = array->size().to_int();
    for(size_t i = 0; i < len; i++)
    {
        Value val = array->index(i);
        auto sub = as<ObjArray>(val);
        if(sub)
        {
            ptrdiff_t subLen = sub->size().to_int();
            if(subLen >= 2)
            {
                std::string label = sub->index(0).to_string();
                Value value = sub->index(1);
                
                auto a = as<ObjArray>(value);
                if(a)
                {
                    HMENU subMenu = makeSubMenu(sub);
                    ::AppendMenuW( hMenu, MF_STRING | MF_POPUP, (UINT_PTR)subMenu, to_wstring(label).c_str());
                }
            }
        }
    }
    return new ObjPointer( vm, (void*) hMenu);
}

#endif

Value isDirectoryNative (VM& , int argCount, Value* args)
{
    if(argCount <1) return false;
    std::string path = args[0].to_string();

    struct stat statbuf;
   if (stat(path.c_str(), &statbuf) != 0)
       return false;
   return S_ISDIR(statbuf.st_mode) != 0;
}

Value dirnameNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    std::string path = args[0].to_string();

#ifdef _WIN32
    size_t pos = path.find_last_of("\\/");
    std::string dir = path;
    if( pos != std::string::npos)
    {
        dir = path.substr(0,pos);
    }
#else
    char* c = strdup(path.c_str());
    std::string dir = dirname(c);
    free(c);
#endif
    return new ObjString(vm, dir);
}

Value basenameNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    std::string path = args[0].to_string();
#ifdef _WIN32

    size_t pos = path.find_last_of("\\/");
    std::string base = path;
    if( pos != std::string::npos)
    {
        base = path.substr(pos+1);
    }
#else    
    char* c = strdup(path.c_str());
    std::string base = basename(c);
    free(c);
#endif
    return new ObjString(vm, base);    
}

Value extNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1) return NIL_VAL;

    std::string f = args[0].to_string();
    size_t pos = f.find_last_of(".");
    if (pos == std::string::npos) return new ObjString(vm, "");

    return new ObjString(vm, f.substr(pos));
}

Value path2selfNative(VM& vm, int /* argCount */, Value* /* args */)
{
    if(!vm.compiler) return NIL_VAL;

#ifdef _WIN32
    char full[_MAX_PATH];
    _fullpath( full, vm.compiler->filename.c_str(), _MAX_PATH );
    std::string s = full;

#else
    char* c = realpath(vm.compiler->filename.c_str(),NULL);
    std::string s = c;
    free(c);
#endif
    return new ObjString(vm, s);
}

Value typeofNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    Value arg = args[0];
    if(IS_NIL(arg)) return new ObjString(vm, "nil");
    if(IS_BOOL(arg)) return new ObjString(vm, "boolean");
    if(IS_INT(arg)) return new ObjString(vm, "int");
    if(IS_NUMBER(arg)) return new ObjString(vm, "double");
    if(IS_OBJ(arg)) 
    {
        Obj* obj = arg.as.obj;
        return new ObjString( vm, obj->type() );
    }
    return new ObjString(vm, "nil");
}

Value funcNameNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    Value arg = args[0];

    auto closure = as<ObjClosure>(arg);
    if(closure)
    {
        return closure->function->name();
    }

    auto method = as<ObjBoundMethod>(arg);
    if(method)
    {
        auto closure2 = as<ObjClosure>(method->method);
        if(closure2)
        {
            return closure2->function->name();
        }
        auto p = as<ObjDecorator>(method->method);
        if(p)
        {
            return as<ObjClosure>(p->getProperty("target"))->function->name();
        }
    }

    auto nf = as<ObjNativeFun>(arg);
    if(nf)
    {
        return new ObjString(vm, arg.to_string());
    }

    auto nm = as<ObjNativeMethod>(arg);
    if(nm)
    {
        return new ObjString(vm, arg.to_string());
    }

    return NIL_VAL;
}

Value globalNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    Value arg = args[0];
    std::string key = arg.to_string();

    if(argCount>1)
    {
        Value val = args[1];
        vm.globals[key] = val;
       // return NIL_VAL;
    }

    if(vm.globals.count(key))
    {
        return vm.globals[key];
    }
    return NIL_VAL;
}

Value metaNative(VM& , int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    if(!IS_OBJ(args[0])) return NIL_VAL;

    auto closure = ::as<ObjClosure>(args[0]);
    if(closure)
    {
        auto f = closure->function;
        return f->metadata;
    }
    auto instance = ::as<ObjInstance>(args[0]);
    if(instance)
    {
        if(!instance->klass) return NIL_VAL;
        return instance->klass->metadata;
    }
    auto method = ::as<ObjBoundMethod>(args[0]);
    if(method)
    {
        auto closure2 = as<ObjClosure>(method->method);
        if(closure2)
        {
            return closure2->function->metadata;
        }
        auto p = as<ObjDecorator>(method->method);
        if(p)
        {
            p->getProperty("target").as.obj->metadata;
        }
    }
    auto obj = ::as<Obj>(args[0]);
    Value m = obj->metadata;
    return m;
}

Value charNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return new ObjString(vm, "");

    auto str = as<ObjString>(args[0]);
    if(str)
    {
        if(str->toString().empty()) return NIL_VAL;
        return ptrdiff_t( str->toString()[0] );
    }

    char c = (char) args[0].to_int();

    return new ObjString(vm, &c,1);
}

Value globNative(VM& vm, int argCount, Value* args)
{
    auto result = new ObjArray(vm);
    if(argCount<1)
        return result;

    auto dir = args[0].to_string();

    DIR* folder = opendir(dir.c_str());
    if(folder == NULL)
    {
        return result;
    }

    struct dirent* entry = nullptr;
    while( (entry = readdir(folder)) != 0)
    {
        std::string f = entry->d_name;
        result->add( new ObjString(vm, f) );
    }

    closedir(folder);     

    return result;
}

Value foreignNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 4)
    {
        return NIL_VAL;
    }

    std::string lib = args[0].to_string();
    std::string retType = args[1].to_string();
    std::string funName = args[2].to_string();
    auto array = as<ObjArray>(args[3]);
    if(!array) array = new ObjArray(vm);

    std::vector<std::string> v;
    for (ptrdiff_t i = 0; i < array->size().as.integer; i++)
    {
        v.push_back(array->index(i).to_string());
    }

    auto r = new ObjForeignFunction(vm, lib,retType,funName,v);
    if (r->pointer() == 0) return NIL_VAL;
    return r;
}


Value variadicNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 4)
    {
        return NIL_VAL;
    }

    std::string lib = args[0].to_string();
    std::string retType = args[1].to_string();
    std::string funName = args[2].to_string();
    auto array = as<ObjArray>(args[3]);
    if(!array) array = new ObjArray(vm);

    std::vector<std::string> v;
    for (ptrdiff_t i = 0; i < array->size().as.integer; i++)
    {
        v.push_back(array->index(i).to_string());
    }

    auto r = new ObjVariadicForeignFunction(vm, lib,retType,funName,v);

    if (r->pointer() == 0) return NIL_VAL;
    return r;

}


Value structNative(VM& vm, int argCount, Value* args)
{
    if (argCount != 0)
    {
        std::vector<std::pair<std::string, std::string>> v;

        auto array = as<ObjArray>(args[0]);
        if(array)
        {
            for (ptrdiff_t i = 0; i < array->size().to_int(); i++)
            {
                std::string s = array->index(i).to_string();
                size_t pos = s.find(":");
                if (pos != std::string::npos)
                {
                    std::string name = s.substr(0, pos);
                    std::string type = s.substr(pos + 1);
                    v.push_back({ name, type });
                }
            }
        }
        else
        {
            for (int i = 0; i < argCount; i++)
            {
                std::string s = args[i].to_string();
                size_t pos = s.find(":");
                if (pos != std::string::npos)
                {
                    std::string name = s.substr(0, pos);
                    std::string type = s.substr(pos + 1);
                    v.push_back({ name, type });
                }
            }
        }
        return new ObjStruct(vm, v);
    }
    return NIL_VAL;
}


Value namedStructNative(VM& vm, int argCount, Value* args)
{
    if (argCount > 1)
    {
        std::vector<std::pair<std::string, std::string>> v;

        std::string name = args[0].to_string();

        auto array = as<ObjArray>(args[1]);
        if(array)
        {
            for (ptrdiff_t i = 0; i < array->size().to_int(); i++)
            {
                std::string s = array->index(i).to_string();
                size_t pos = s.find(":");
                if (pos != std::string::npos)
                {
                    std::string sname = s.substr(0, pos);
                    std::string type = s.substr(pos + 1);
                    v.push_back({ sname, type });
                }
            }
        }
        else
        {
            for (int i = 1; i < argCount; i++)
            {
                std::string s = args[i].to_string();
                size_t pos = s.find(":");
                if (pos != std::string::npos)
                {
                    std::string sname = s.substr(0, pos);
                    std::string type = s.substr(pos + 1);
                    v.push_back({ sname, type });
                }
            }
        }
        return new ObjStruct(vm, name,v);
    }
    return NIL_VAL;
}

Value callbackNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 2)
    {
        return NIL_VAL;
    }

    if (argCount == 2)
    {
        std::string retType = args[0].to_string();
        auto array = as<ObjArray>(args[1]);
        if(!array) return NIL_VAL;

        std::vector<std::string> v;
        for (ptrdiff_t i = 0; i < array->size().as.integer; i++)
        {
            v.push_back(array->index(i).to_string());
        }

        return new ObjCallbackType(vm, "callbackType", retType,v);
    }

    std::string name = args[0].to_string();   
    std::string retType = args[1].to_string();
    auto array = as<ObjArray>(args[2]);
    if(!array) return NIL_VAL;

    std::vector<std::string> v;
    for (ptrdiff_t i = 0; i < array->size().as.integer; i++)
    {
        v.push_back(array->index(i).to_string());
    }

    return new ObjCallbackType(vm, name, retType,v);

}


Value stringNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1)
    {
        return new ObjString(vm, "");
    }
    if(IS_OBJ(args[0]))
    {
        auto buf = as<ObjBuffer>(args[0]);
        if(buf)
        {
            return new ObjString(vm, buf->asString());
        }
        auto ptr = as<ObjPointer>(args[0]);
        if(ptr)
        {
            char* c = (char*)ptr->pointer();
            ptrdiff_t len = 0;
            if(argCount>1)
            {
                len = args[1].to_int();
            }
            else
            {
                len = strlen(c);
            }
            if(c == 0 || len == 0)
                return new ObjString(vm, "");
            return new ObjString(vm, c,len);
        }
    }
    return new ObjString(vm, args[0].to_string());
}

#ifdef _WIN32
Value encodeNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1)
    {
        return new ObjString(vm,"");
    }
    if (argCount == 1 )
    {
        return args[0];
    }

    auto str = args[0].to_string();
    long cp = (long) args[1].to_int();

    if (cp == CP_WINUNICODE)
    {
        std::wstring ws = to_wstring(str);
        return new ObjWideString(vm,ws);
    }
    if (cp == CP_UTF8)
    {
        return args[0];
    }
    std::wstring ws = to_wstring(str);
    std::string r = to_string(ws, cp);
    return new ObjString(vm,r);
}

Value decodeNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1)
    {
        return new ObjString(vm,"");
    }
    if (argCount == 1)
    {
        return args[0];
    }

    auto str = args[0].to_string();
    long cp = (long) args[1].to_int();

    if (cp == CP_WINUNICODE)
    {
        std::wstring ws((const wchar_t*)str.data(), str.size() / 2);
        return new ObjWideString(vm,ws);
    }
    if (cp == CP_UTF8)
    {
        return args[0];
    }
    std::wstring ws = to_wstring(str, cp);
    std::string r = to_string(ws);
    return new ObjString(vm,r);
}

#endif 

#ifdef _WIN32

Value wstringNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1)
    {
        return new ObjWideString(vm,"");
    }
    if(IS_OBJ(args[0]))
    {
        auto buf = as<ObjBuffer>(args[0]);
        if(buf)
        {
            return new ObjWideString(vm, buf->asWideString());
        }
        auto ptr = as<ObjPointer>(args[0]);
        if(ptr)
        {
            wchar_t* c = (wchar_t*)ptr->pointer();
            ptrdiff_t len = 0;
            if(argCount>1)
            {
                len = args[1].to_int();
            }
            else
            {
                len = wcslen(c);
            }
            return new ObjWideString( vm, c,len);
        }
    }
    return new ObjWideString( vm, to_wstring( args[0].to_string()));
}
#endif


Value bufferNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1)
    {
        return  new ObjBuffer(vm);
    }
    if (IS_OBJ(args[0]))
    {
        auto s = as<ObjString>(args[0]);
        if (s)
        {
            return new ObjBuffer(vm,args[0].to_string());
        }
    }
    return new ObjBuffer(vm,args[0].to_int());
}


Value pointerNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1)
    {
        return  new ObjPointer(vm);
    }
    if( argCount == 1)
    {
        auto p = as<ObjPointer>(args[0]);
        if(p)
        {
            return new ObjPointer(vm, p->addressOf());
        }
        auto obj = as<Obj>(args[0]);
        if(obj)
        {
            return new ObjPointer( vm, obj->pointer() );
        }

        return new ObjPointer(vm, (void*)args[0].to_int());
    }

    auto deletor = args[1];

    auto p = as<ObjPointer>(args[0]);
    if(p)
    {
        return new ObjPointer(vm, p->addressOf(),deletor);
    }
    auto obj = as<Obj>(args[0]);
    if(obj)
    {
        return new ObjPointer( vm, obj->pointer(),deletor );
    }

    return new ObjPointer(vm, (void*)args[0].to_int(), deletor);
}

Value gc_runNative(VM& vm, int /* argCount */, Value* /* args */)
{
    vm.gc.collectGarbage();
	return NIL_VAL;
}

Value addLibPathNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1) return NIL_VAL;
    Value f = args[0];
    std::string str = f.to_string();

#ifdef _WIN32
    char full[_MAX_PATH];
    _fullpath(full, str.c_str(), _MAX_PATH);
    std::string s = full;

#else
    char* c = realpath(str.c_str(), NULL);
    std::string s = c;
    free(c);
#endif

    vm.include_path.push_back(s);
    return NIL_VAL;
}




Value evalNative(VM& vm, int argCount, Value* args)
{
    if(argCount<1) return NIL_VAL;
    Value f = args[0];
    std::string str = f.to_string();

	if(str.empty()) return NIL_VAL;

    ObjFunction* function = nullptr;

	{
		GC::Lock lock(vm);
		Compiler compiler(vm, vm.compiler,FunctionType::TYPE_FUNCTION);

		function = compiler.compile(compiler.filename.c_str(),str.c_str());
		vm.push(function);
    }
    {
        GC::Lock lock(vm);
        ObjClosure* closure = nullptr;
        closure = new ObjClosure(vm,function);
        vm.pop();
        vm.push(closure);
        vm.call(closure,0);
    }

    vm.top_frame().returnToCallerOnReturn = true;
    Value r = vm.run();
    if(vm.hasException())
    {
        printf("unhandlex exception");
        vm.printPendingException();
		return NIL_VAL;
    }
    vm.pop();
    return r;
}

bool file_exists(const std::string& file)
{
#ifdef _WIN32
    DWORD dwAttrib = GetFileAttributesW( to_wstring(file).c_str() );

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat buffer;
    return (stat(file.c_str(), &buffer) == 0);
#endif
//    return false;
}

#ifdef _WIN32
const char* sep = "\\";
#else
const char* sep = "/";
#endif

std::string get_file(const std::string& dir, const std::string& file)
{
    std::string mom = dir + sep + file;
    std::string compiled = dir + sep +file + ".mbc";

    long stime = get_mtime(mom.c_str());
    long ctime = get_mtime(compiled.c_str());

    if (stime <= ctime)
    {
        if (file_exists(compiled)) return compiled;
    }
    if (file_exists(mom)) return mom;

    std::string lib = dir + sep + file + ".lib";
    std::string lib_compiled = dir + sep + file + ".lib.mbc";

    stime = get_mtime(lib.c_str());
    ctime = get_mtime(lib_compiled.c_str());

    if (stime <= ctime)
    {
        if (file_exists(lib_compiled)) return lib_compiled;
    }
    if (file_exists(lib)) return lib;

    return "";
}


std::string find_file(VM& vm, const std::string& file)
{
    for (auto& it : vm.include_path)
    {
        auto f = get_file(it, file);
        if (!f.empty())
        {
            return f;
        }
    }

    std::string current_dir = vm.compiler->filename;

    size_t pos = current_dir.find_last_of("\\/");
    if (pos != std::string::npos)
    {
        current_dir = current_dir.substr(0, pos + 1);
    }
    else
    {
        current_dir = ".";
    }

    auto f = get_file(current_dir, file);
    if (!f.empty())
    {
        return f;
    }

    return "";
}

// holy cow, refactor all file access to std::filesystem on next view

Value importNative(VM& vm, int argCount, Value* args)
{
    if(argCount<1) return NIL_VAL;
    Value f = args[0];
    std::string filename = f.to_string();

    std::string file = find_file(vm, filename);
    if (file.empty()) return NIL_VAL;

    ObjFunction* function = nullptr;

    std::string content = slurp(file.c_str());
    if (content.empty()) return NIL_VAL;

    if( file.ends_with(".mbc") )
    {
        GC::Lock lock(vm);

        std::istringstream iss(content);
        deserialize(vm, iss, &function);

        vm.push(function);
    }
    else
    {
        GC::Lock lock(vm);
        Compiler compiler(vm,vm.compiler,FunctionType::TYPE_FUNCTION);

        function = compiler.compile(file.c_str(),content.c_str());
        vm.push(function);
    }

    {
        GC::Lock lock(vm);
        ObjClosure* closure = nullptr;
        closure = new ObjClosure(vm,function);
        vm.gc.pin(closure); // ok global import, but unpin anyway?
        vm.pop();
        vm.push(closure);
        vm.call(closure,0);
    }

    vm.top_frame().returnToCallerOnReturn = true;
    Value r = vm.run();
    if(vm.hasException())
    {
        printf("unhandled exception");
        vm.printPendingException();
        exit(1);
    }
    vm.pop();
    return r;
}


Value clockNative(VM&, int /* argCount */, Value* /* args */)
{
    return Value(time(0));
}


Value timeNative(VM&, int /* argCount */, Value* /* args */)
{
    using namespace std::chrono;
    milliseconds ms = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    );

    return Value((size_t)ms.count());
}



Value argumentsNative(VM& vm, int /* argCount */, Value* /* args */)
{
	return vm.top_frame().arguments(vm);
/*
    auto array = new ObjArray(vm);

    int argc = 0;
    int slot = vm.top_frame().argBaseIndex;
    argc = (int)vm.stack.size()-1-slot-1;

    for(int i = 0; i < argc; i++)
    {
        array->add( *(&vm.stack.back() -argc  +i) );
    }

    if(!vm.top_frame().varargs.empty())
    {
        for( size_t i = 0; i < vm.top_frame().varargs.size(); i++)
        {
            array->add(vm.top_frame().varargs[i]);
        }
    }

   // array->isMarked = true;
    return array;
	*/
}


Value intNative(VM&, int argCount, Value* args)
{
    if(argCount == 0) return size_t(0);
    switch(args[0].type)
    {
        case ValueType::VAL_NIL: return NIL_VAL;
        case ValueType::VAL_BOOL : return (ptrdiff_t)(args[0].as.boolean);
        case ValueType::VAL_INT : return (size_t)(args[0].as.integer);
        case ValueType::VAL_NUMBER : return (size_t)(args[0].as.number);
        case ValueType::VAL_OBJ :
        {
            auto str = as<ObjString>(args[0].as.obj);
            if(str)
            {
                std::istringstream iss(str->toString());
                ptrdiff_t i = 0;
                iss >> i;
                return i;
            }
            auto ptr = as<ObjPointer>(args[0].as.obj);
            if(ptr)
            {
                void* p = ptr->pointer();
                size_t i = (size_t)p;
                return i;
            }
            void* p = args[0].as.obj->pointer();
            size_t i = (size_t)p;
            return i;
        }
        default: 
        {

        }
    }
    return NIL_VAL;
}


Value floatNative(VM&, int argCount, Value* args)
{
    if(argCount == 0) return 0.0;
    switch(args[0].type)
    {
        case ValueType::VAL_NIL: return NIL_VAL;
        case ValueType::VAL_BOOL : return (double)(args[0].as.boolean);
        case ValueType::VAL_INT : return  (double)(args[0].as.integer);
        case ValueType::VAL_NUMBER : return args[0].as.number;
        case ValueType::VAL_OBJ :
        {
            auto str = as<ObjString>(args[0].as.obj);
            if(str)
            {
                std::istringstream iss(str->toString());
                double d = 0;
                iss >> d;
                return d;
            }
            return NIL_VAL;
        }
        default: return NIL_VAL;
    }
//    return NIL_VAL;
}

Value randNative(VM&, int argCount, Value* args) 
{
    if(argCount == 0) return 0.0;

    size_t r = ::rand();
    if (argCount == 0)
    {
        return r;
    }
    size_t upper = args[0].to_int();
    size_t lower = 0;
    if (argCount > 1)
    {
        lower = upper;
        upper = args[1].to_int() -1;
    }
    if (upper == 0) return lower;
    size_t v = (r % (upper)) + lower;
    return v;
}

Value errnoNative(VM&, int /*argCount*/, Value* /*args*/ ) 
{
    return errno;
}

Value readlineNative(VM& vm, int /* argCount */, Value* /* args */)
{
    std::string line;
    std::getline(std::cin, line);
    return new ObjString(vm, line);
}

Value cwdNative(VM& vm, int /* argCount */, Value* /* args */)
{
    std::string cwd = current_working_directory();
    return new ObjString(vm, cwd);
}


static Value mallocNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;
    void* c = std::malloc(args[0].to_int());
    auto p = new ObjPointer(vm, c);
    return p;
}

static Value freeNative(VM&, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    auto p = ::as<ObjPointer>(args[0]);
    if(p)
    {
        p->free();
    }
    return NIL_VAL;
}

static Value toDosNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1) return NIL_VAL;

    std::string s = args[0].to_string();
    std::string r = toDos(s);

    auto result = new ObjString(vm, r);
    return result;
}


static Value toUnixNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1) return NIL_VAL;

    std::string s = args[0].to_string();
    std::string r = toUnix(s);

    auto result = new ObjString(vm,r);
    return result;
}

Value slurpNative(VM& vm, int argCount, Value* args)
{
    if(argCount == 0) return NIL_VAL;

    std::string file = args[0].to_string();
    std::string content = slurp(file.c_str());
    return new ObjString(vm, content);
}

Value flushNative(VM&, int argCount, Value* args)
{
    if(argCount < 2) return NIL_VAL;

    std::string file = args[0].to_string();
    std::string content = args[1].to_string();

    flush(file,content);
    return NIL_VAL;
}


Value toHexNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1) return NIL_VAL;

    auto buf = as<ObjBuffer>(args[0]);
    if (buf)
    {
        std::string str((const char*)buf->pointer(), buf->size());
        std::string hex = toHex(str);

        return new ObjString(vm, hex);
    }

    if (argCount > 1)
    {
        auto ptr = as<ObjPointer>(args[0]);
        if (ptr)
        {
            ptrdiff_t len = args[1].to_int();
            std::string str = std::string((const char*)ptr->pointer(), len);
            std::string hex = toHex(str);

            return new ObjString(vm, hex);
        }
    }

    std::string str = args[0].to_string();
    std::string hex = toHex(str);

    return new ObjString(vm, hex);
}


Value fromHexNative(VM& vm, int argCount, Value* args)
{
    if (argCount < 1) return NIL_VAL;

    std::string hex = args[0].to_string();
    std::string str = fromHex(hex);

    return new ObjBuffer(vm, str);
}

Value mustacheNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 2) return NIL_VAL;

    std::string tpl = args[0].to_string();

    Value data = args[1];

    mustache must(tpl);
    std::string result = must.render(data);
    return new ObjString(vm,result);
}

Value regexNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    std::string rgx = args[0].to_string();

    if(argCount == 1)
    {
        return new ObjRegex(vm, rgx);
    }

    std::string opt = args[1].to_string();

    return new ObjRegex(vm, rgx,opt);
}

Value cliargsNative(VM& vm, int /* argCount */, Value* /* args */)
{
    auto array = new ObjArray(vm);
    for( size_t i = 0; i < vm.cliArgs.size(); i++)
    {
        array->add( new ObjString(vm, vm.cliArgs[i]));
    }
    return array;
}

Value strcmpNative(VM&, int argCount, Value* args)
{
    if(argCount < 2) return NIL_VAL;

    std::string s1 = args[0].to_string();
    std::string s2 = args[1].to_string();

    return size_t(strcmp(s1.c_str(),s2.c_str()));
}

Value popenNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    constexpr int BUFSIZE=4096;

    std::string cmd = args[0].to_string();

    char buf[BUFSIZE];
    FILE *fp;

    std::ostringstream oss;
    if ((fp = popen(cmd.c_str(), "r")) == NULL) 
    {
        return NIL_VAL;
    }

    while (fgets(buf, BUFSIZE, fp) != NULL) 
    {
        oss << buf;
    }

    if (pclose(fp)) 
    {}

    return new ObjString(vm, oss.str());
}

Value offsetofNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    auto rec = ::as<ObjRecord>(args[0]);
    if(!rec) return NIL_VAL;

    if(argCount < 2)
    {
        return new ObjPointer(vm, rec->pointer());
    }

    std::string key = args[1].to_string();
    void* p = rec->offset_of(key);
    return new ObjPointer(vm, p);
}





json toJson(VM& vm, Value value);
json toJson(Propertyable* map);

json toJson(VM& vm, Indexable* indexable)
{
    json j;
    size_t s = indexable->size().to_int();
    for (size_t i = 0; i < s; i++)
    {
        Value v = indexable->index(i);
        j[i] = toJson(vm, v);
    }
    return j;
}

json toJson(VM& vm, Propertyable* map)
{
    json j;

    auto keys = map->keys();

    size_t len = keys.size();
    for (size_t i = 0; i < len; i++)
    {
        std::string key = keys[i];
        Value val = map->getProperty(key);
        j[key] = toJson(vm,val);
    }
    return j;
}

json toJson(VM& vm, Value value)
{
    switch(value.type)
    {
        case ValueType::VAL_NIL:
        {
            json j;
            return j;
        }
        case ValueType::VAL_BOOL:
        {
            json::boolean_t r(value.to_int() != 0);
            return r;
        }
        case ValueType::VAL_INT:
        {
            json::number_integer_t i(value.to_int());
            return i;
        }
        case ValueType::VAL_NUMBER:
        {
            json::number_float_t f(value.to_double());
            return f;
        }
        case ValueType::VAL_OBJ:
        {
            auto str = ::as<ObjString>(value.as.obj);
            if(str)
            {
                json::string_t s(str->toString());
                return s;
            }

            auto array = ::as<Indexable>(value.as.obj);
            if(array)
            {
                return toJson(vm,array);
            }

            auto map = ::as<Propertyable>(value.as.obj);
            if(map)
            {
                return toJson(vm,map);
            }
            break;
        }
    }
    json j;
    return j;
}

Value toJsonNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    json j = toJson(vm,args[0]);
    std::string s = j.dump();

    return new ObjString(vm, s);
}


Value fromJson(VM& vm, json& j);
Value fromJsonArray(VM& vm, json& j);

Value fromJsonObject(VM& vm, json& j)
{
    auto map = new ObjMap(vm);

    for (json::iterator it = j.begin(); it != j.end(); ++it) 
    {
        json v = it.value();
        map->item(it.key(), fromJson(vm,v));
    }

    return map;
}

Value fromJsonArray(VM& vm, json& j)
{
    auto array = new ObjArray(vm);
    size_t s = j.size();
    for (size_t i = 0; i < s; i++)
    {
        array->add(fromJson(vm, j[i]));
    }
    return array;
}

Value fromJson(VM& vm, json& j)
{
    if (j.is_string())
    {
        std::string s = j.get<std::string>();
        return new ObjString(vm,s);
    }
    if (j.is_number())
    {
        double d = j.get<double>();
        return d;
    }
    if (j.is_boolean())
    {
        bool b = j.get<bool>();
        return b;
    }
    if (j.is_array())
    {        
        return fromJsonArray(vm,j);
    }
    if (j.is_object())
    {
        return fromJsonObject(vm,j);
    }
    return NIL_VAL;
}

Value fromJsonNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    std::string s = args[0].to_string();
    json j = json::parse(s);

    return fromJson(vm,j);
}

Value jsonNative(VM& vm )
{
    auto map = new ObjMap(vm);

    map->item("parse", new ObjNativeFun(vm,fromJsonNative));
    map->item("stringify", new ObjNativeFun(vm,toJsonNative));

    return map;
}


#ifdef _WIN32

Value createComPtrNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    ObjPointer* ptr = as<ObjPointer>(args[0].as.obj);
    if(ptr)
    {
        auto comPtr = new ComObject(vm, (IUnknown*)ptr->pointer());
        return comPtr;
    }
    DispatchObject* disp = as<DispatchObject>(args[0].as.obj);
    if(disp)
    {
        IUnknown* unk = nullptr;
        HRESULT hr = disp->ptr()->QueryInterface(IID_IUnknown,(void**)&unk);
        if(hr == S_OK && unk)
        {
            auto comPtr = new ComObject(vm,unk);
            unk->Release();
            return comPtr;
        }
        return NIL_VAL;
    }    
    ComObject* com = as<ComObject>(args[0].as.obj);
    if(com)
    {
        auto comPtr = new ComObject(vm,com->ptr());
        return comPtr;
    }
    return NIL_VAL;
}


Value createDispPtrNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    ObjPointer* ptr = as<ObjPointer>(args[0].as.obj);
    if(ptr)
    {
        auto dispPtr = new DispatchObject(vm, (IDispatch*)ptr->pointer());
        return dispPtr;
    }
    DispatchObject* disp = as<DispatchObject>(args[0].as.obj);
    if(disp)
    {
        auto dispPtr = new DispatchObject(vm, disp->ptr());
        return dispPtr;
    }
    ComObject* com = as<ComObject>(args[0].as.obj);
    if(com)
    {
        IDispatch* d = nullptr;
        HRESULT hr = com->ptr()->QueryInterface(IID_IDispatch, (void**)&d);
        if(hr == S_OK && d)
        {
            auto dispPtr = new DispatchObject(vm, d);
            d->Release();
            return dispPtr;
        }
        return NIL_VAL;
    }
    return NIL_VAL;
}

Value createObjectNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    std::string progid = args[0].to_string();
    if (!progid.empty() && progid[0] == '{')
    {
        auto co = new ComObject(vm);
        if (argCount > 1)
        {
            if (co->create(progid, (int)args[1].to_int()))
            {
                return co;
            }
        }
        else
        {
            if (co->create(progid))
            {
                return co;
            }
        }
        return NIL_VAL;
    }

    auto co = new DispatchObject(vm);
    if (argCount > 1)
    {
        if (co->create(progid, (int)args[1].to_int()))
        {
            return co;
        }
    }
    else
    {
        if (co->create(progid))
        {
            return co;
        }
    }
    return NIL_VAL;
}

Value createActivationFactoryNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    std::wstring rtname = to_wstring(args[0].to_string());

    HSTRING hstr = nullptr;
    ::WindowsCreateString(rtname.c_str(), (UINT32)rtname.size(),&hstr);

    IActivationFactory* factory = nullptr;
    HRESULT hr = ::RoGetActivationFactory(hstr,IID_IActivationFactory,(void**)&factory);

    ComObject* result = nullptr;
    if(hr == S_OK)
    {
        result = new ComObject(vm, factory);
        factory->Release();
    }

    ::WindowsDeleteString(hstr);

    if(result)
    {
        return result;
    }
    return NIL_VAL;
}

Value ActivateInstanceNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    std::wstring rtname = to_wstring(args[0].to_string());

    HSTRING hstr = nullptr;
    ::WindowsCreateString(rtname.c_str(),(UINT32)rtname.size(),&hstr);

    IInspectable* insp = nullptr;
    HRESULT hr = ::RoActivateInstance(hstr,&insp);
    ::WindowsDeleteString(hstr);
    if(hr != S_OK)
    {
        return NIL_VAL;
    }

    ComObject* result = nullptr;
    if(insp)
    {
        result = new ComObject(vm, insp);
        insp->Release();
    }

    if(result)
    {
        return result;
    }
    return NIL_VAL;
}


Value delegateNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 3) return NIL_VAL;
    Value argTypes = args[0];
    std::string handlerIID = args[1].to_string();
    Value cb = args[2];

    auto array = as<ObjArray>(argTypes);
    std::vector<std::string> v;
    if(array)
    {
        for(int i = 0; i < array->size().to_int(); i++)
        {
            v.push_back(array->index(i).to_string());
        }
    }

    return make_delegate(vm, v,handlerIID,cb);
}


Value enumerateNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    auto co = ::as<DispatchObject>(args[0]);
    if(!co) 
    {
        auto result = new ObjArray(vm);
        return result;
    }

    return co->enumerate();
}

Value comInitNative(VM& vm, int /* argCount */, Value* /* args */)
{
    if (vm.coinit == CO_INIT_NONE)
    {
        ::CoInitialize(0);
        vm.coinit = CO_INIT_COM;
    }
    return NIL_VAL;
}


Value makeWordNative(VM& , int argCount, Value* args)
{
    if(argCount != 2) return NIL_VAL;
    int a = (int)args[0].to_int();
    int b = (int)args[1].to_int();
    int r = MAKEWORD(a,b);
    return size_t(r);
}

Value makeLongNative(VM&, int argCount, Value* args) 
{
    if(argCount != 2) return NIL_VAL;
    int a = (int)args[0].to_int();
    int b = (int)args[1].to_int();
    int r = MAKELONG(a,b);
    return size_t(r);
}


Value loWordNative(VM&, int argCount, Value* args) 
{
    if(argCount != 1) return NIL_VAL;
    int a = (int)args[0].to_int();
    int r = LOWORD(a);
    return size_t(r);
}

Value hiWordNative(VM&, int argCount, Value* args) 
{
    if(argCount != 1) return NIL_VAL;
    int a = (int)args[0].to_int();
    int r = HIWORD(a);
    return size_t(r);
}

Value loByteNative(VM&, int argCount, Value* args) 
{
    if(argCount != 1) return NIL_VAL;
    int a = (int)args[0].to_int();
    int r = LOBYTE(a);
    return size_t(r);
}

Value hiByteNative(VM&, int argCount, Value* args) 
{
    if(argCount != 1) return NIL_VAL;
    int a = (int)args[0].to_int();
    int r = HIBYTE(a);
    return size_t(r);
}

Value makeIntResourceNative(VM& vm, int argCount, Value* args)
{
    if(argCount != 1) return NIL_VAL;
    int a = (int)args[0].to_int();
    wchar_t* w = MAKEINTRESOURCEW(a);
    return  new ObjPointer(vm, w);
}

#endif

Value decoratorNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 2) return NIL_VAL;

    if(!IS_OBJ(args[0])) return NIL_VAL;
    if(!IS_OBJ(args[1])) return NIL_VAL;

    auto p = new ObjDecorator(vm, args[0].as.obj,args[1].as.obj);

    return p;
}

Value proxyNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 2) return NIL_VAL;

    if(!IS_OBJ(args[0])) return NIL_VAL;
    if(!IS_OBJ(args[1])) return NIL_VAL;

    auto p = new ObjProxy(vm, args[0].as.obj,args[1].as.obj);

    return p;
}

Value invokeNative(VM& vm, int argCount, Value* args)
{
    if(argCount < 1) return NIL_VAL;

    Value target = args[0];

    if(!IS_OBJ(target)) return NIL_VAL;

    if(argCount == 3)
    {
        Value name = args[1];
        auto p = as<ObjProxy>(target);
        if(p)
        {
            vm.push(target);
            int cnt = 0;
            if(argCount > 2) 
            {
                auto array = as<ObjArray>(args[2]);
                if(array)
                {
                    for(int i = 0; i < array->size().to_int(); i++)
                    {
                        vm.push(array->index(i));
                        cnt++;
                    }
                }
            }
            p->invokeMethod(name.to_string(),cnt);
            Value r = vm.pop();
            return r;
        }
        auto obj = as<ObjInstance>(target);
        if(obj)
        {
            target = obj->getProperty(name.to_string());
            vm.push(target);

            int cnt = 0;
            if(argCount > 2) 
            {
                auto array = as<ObjArray>(args[2]);
                if(array)
                {
                    for(int i = 0; i < array->size().to_int(); i++)
                    {
                        vm.push(array->index(i));
                        cnt++;
                    }
                }
            }

            target.as.obj->callValue(cnt);
            vm.top_frame().returnToCallerOnReturn = true;
            Value r = vm.run();
            vm.pop();
            return r;
        }
    }

    vm.push(target);

    int cnt = 0;
    if(argCount > 1) 
    {
        auto array = as<ObjArray>(args[1]);
        if(array)
        {
            for(int i = 0; i < array->size().to_int(); i++)
            {
                vm.push(array->index(i));
                cnt++;
            }
        }
    }

    auto foreign = as<ObjForeignFunction>(target);
    if(foreign)
    {
        foreign->callValue(cnt);
        Value r = vm.peek(0);
        vm.pop(); 
        return r;
    }

    auto variadic = as<ObjVariadicForeignFunction>(target);
    if(variadic)
    {
        variadic->callValue(cnt);
        Value r = vm.peek(0);
        vm.pop(); 
        return r;
    }

    auto strct = as<ObjStruct>(target);
    if(strct)
    {
        strct->callValue(cnt);
        Value r = vm.peek(0);
        vm.pop();
        return r;
    }

    target.as.obj->callValue(cnt);
    vm.top_frame().returnToCallerOnReturn = true;
    Value r = vm.run();
    vm.pop();
    return r;
}

#ifdef _WIN32

Value wrtInitNative(VM& vm, int /* argCount */, Value* /* args */)
{
    if (vm.coinit == CO_INIT_NONE)
    {
        if (S_OK != ::RoInitialize(RO_INIT_SINGLETHREADED)) exit(-1);
        vm.coinit = CO_INIT_WINRT;
    }
    return NIL_VAL;
}

#ifdef ENABLE_MOSER_XAML

MoserX xmos;

Value xamlInitNative(VM& /*vm*/, int /* argCount */, Value* /* args */)
{
    xmos.init();
    return NIL_VAL;
}

#endif

#endif

void init_stdlib(VM& vm)
{
    GC::Lock lock(vm);

    vm.defineNative("typeof",typeofNative);
    vm.defineNative("int",   intNative);
    vm.defineNative("float", floatNative);
    vm.defineNative("string", stringNative);
    vm.defineNative("char",   charNative);
    vm.defineNative("regex", regexNative);
    vm.defineNative("mustache", mustacheNative);

#ifdef _WIN32
    vm.defineNative("wstring", wstringNative);
#endif

    vm.defineNative("import", importNative);
    vm.defineNative("arguments", argumentsNative);
    vm.defineNative("strcmp", strcmpNative);

    // runtime

    auto rt = new ObjMap(vm);
    rt->item("fn", new ObjNativeFun(vm, funcNameNative));
    rt->item("invoke", new ObjNativeFun(vm, invokeNative));
    rt->item("Decorator", new ObjNativeFun(vm, decoratorNative));
    rt->item("RuntimeProxy", new ObjNativeFun(vm, proxyNative));
    rt->item("global", new ObjNativeFun(vm, globalNative));
    rt->item("eval", new ObjNativeFun(vm, evalNative));
    rt->item("meta", new ObjNativeFun(vm, metaNative));
    rt->item("gc_run", new ObjNativeFun(vm, gc_runNative));
    rt->item("add_lib_path", new ObjNativeFun(vm, addLibPathNative));

    vm.defineGlobal("runtime", rt);

#ifdef _WIN32

    // win32
    auto win32 = new ObjMap(vm);
    win32->item("menue", new ObjNativeFun(vm, win32MenuNative));
    win32->item("encode", new ObjNativeFun(vm, encodeNative));
    win32->item("decode", new ObjNativeFun(vm, decodeNative));

    win32->item("MAKEWORD", new ObjNativeFun(vm, makeWordNative));
    win32->item("MAKELONG", new ObjNativeFun(vm, makeLongNative));
    win32->item("LOWORD", new ObjNativeFun(vm, loWordNative));
    win32->item("HIWORD", new ObjNativeFun(vm, hiWordNative));
    win32->item("LOBYTE", new ObjNativeFun(vm, loByteNative));
    win32->item("HIBYTE", new ObjNativeFun(vm, hiByteNative));
    win32->item("MAKEINTRESOURCE", new ObjNativeFun(vm, makeIntResourceNative));
    vm.defineGlobal("win32", win32);
#endif
    

    // io
    auto io = new ObjMap(vm);
    io->item("self",new ObjNativeFun(vm, path2selfNative));
    io->item("is_directory",new ObjNativeFun(vm, isDirectoryNative));
    io->item("dirname",new ObjNativeFun(vm, dirnameNative));
    io->item("basename",new ObjNativeFun(vm, basenameNative));
    io->item("ext", new ObjNativeFun(vm, extNative));
    io->item("glob",new ObjNativeFun(vm, globNative));
    io->item("readline",new ObjNativeFun(vm, readlineNative));
    io->item("cwd",new ObjNativeFun(vm, cwdNative));
    io->item("slurp",new ObjNativeFun(vm, slurpNative));
    io->item("flush",new ObjNativeFun(vm, flushNative));
    io->item("toHex", new ObjNativeFun(vm, toHexNative));
    io->item("fromHex", new ObjNativeFun(vm, fromHexNative));

    vm.defineGlobal("io", io);

    // sys
    auto sys = new ObjMap(vm);
    sys->item("clock",new ObjNativeFun(vm, clockNative));
    sys->item("time", new ObjNativeFun(vm, timeNative));
    sys->item("args",new ObjNativeFun(vm, cliargsNative));
    sys->item("popen",new ObjNativeFun(vm, popenNative));
    sys->item("buffer",new ObjNativeFun(vm, bufferNative));
    sys->item("pointer",new ObjNativeFun(vm, pointerNative));
    sys->item("rand", new ObjNativeFun(vm, randNative));
    sys->item("errno", new ObjNativeFun(vm, errnoNative));
    vm.defineGlobal("sys", sys);

    // foreign
    auto foreign = new ObjMap(vm);
    foreign->item("native",new ObjNativeFun(vm, foreignNative));
    foreign->item("variadic",new ObjNativeFun(vm, variadicNative));
    foreign->item("struct",new ObjNativeFun(vm, structNative));
    foreign->item("named_struct",new ObjNativeFun(vm, namedStructNative));
    foreign->item("callback",new ObjNativeFun(vm, callbackNative));
    foreign->item("offset_of",new ObjNativeFun(vm, offsetofNative));
    vm.defineGlobal("foreign", foreign);
	
	// OS
    auto os = new ObjMap(vm);

    os->item("malloc", new ObjNativeFun(vm, mallocNative));
    os->item("free", new ObjNativeFun(vm, freeNative));
    os->item("toDos", new ObjNativeFun(vm, toDosNative));
    os->item("toUnix", new ObjNativeFun(vm, toUnixNative));

#ifdef _WIN32    
    os->item("system", new ObjString(vm, "nt"));
    os->item("linesep", new ObjString(vm, "\r\n"));
    os->item("pathsep", new ObjString(vm, "\\"));
#else
    os->item("system", new ObjString(vm, "posix"));
    os->item("linesep", new ObjString(vm, "\n"));
    os->item("pathsep", new ObjString(vm, "/"));
#endif
	vm.defineGlobal("os", os);

    // JSON 
    vm.defineGlobal("JSON", jsonNative(vm));

#ifdef _WIN32

    // com
    auto com = new ObjMap(vm);
    com->item("Ptr", new ObjNativeFun(vm, createComPtrNative));
    com->item("DispatchPtr", new ObjNativeFun(vm, createDispPtrNative));
    com->item("createObject", new ObjNativeFun(vm, createObjectNative));
    com->item("enumerate", new ObjNativeFun(vm, enumerateNative));
    com->item("init", new ObjNativeFun(vm, comInitNative));
    vm.defineGlobal("com", com);

    // winrt
    auto winrt = new ObjMap(vm);
    winrt->item("activationFactory", new ObjNativeFun(vm, createActivationFactoryNative));
    winrt->item("activate", new ObjNativeFun(vm, ActivateInstanceNative));
    winrt->item("Delegate", new ObjNativeFun(vm, delegateNative));
    winrt->item("init", new ObjNativeFun(vm, wrtInitNative));
    vm.defineGlobal("winrt", winrt);

#ifdef ENABLE_MOSER_XAML

    // xaml
    auto xaml = new ObjMap(vm);
    xaml->item("init", new ObjNativeFun(vm, xamlInitNative));
    winrt->item("xaml", xaml);
#endif

#endif
}
