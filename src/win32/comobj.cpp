//#include "pch.h"
#ifdef _WIN32

#include <vm.h>
#include "win32/comobj.h"
#include "win32/uni.h"
#include "win32/tinfo.h"
#include "foreign.h"
#include "native.h"
#include <ffi.h>
#include "gc.h"
#include "win32/xaml.h"
#include <iostream>

#include "win32/winrtmarshal.h"
#include "win32/winrtdelegate.h"
#include "win32/async.h"


ComObject::ComObject(VM& vm) : ObjBuiltin(vm) {}

ComObject::ComObject(VM& vm, IUnknown* d) : ObjBuiltin(vm), ptr_(d)
{
    if(ptr_)
        ptr_->AddRef();
}

ComObject::~ComObject()
{
    if (ptr_)
    {
        ptr_->Release();
        ptr_ = nullptr;
    }
}

bool ComObject::create(const std::string& s, int ctx)
{
    if (ptr_)
    {
        ptr_->Release();
        ptr_ = nullptr;
    }

    std::wstring str = to_wstring(s);

    CLSID clsid;
    HRESULT hr = ::CLSIDFromString(str.c_str(), &clsid);
    if (hr != S_OK)
    {
        return false;
    }

    hr = ::CoCreateInstance(clsid, nullptr, ctx, IID_IUnknown, (void**)&ptr_);
    if (hr != S_OK)
    {
        return false;
    }

    return true;
}

Value ComObject::invoke( int argCount, Value* args)
{
    if(argCount < 4) return NIL_VAL;

    int vtable_index = (int) args[0].to_int();
    std::string retType = args[1].to_string();
    auto paramTypes = as<ObjArray>(args[2].as.obj);
    if(!paramTypes) return NIL_VAL;
    auto params = as<ObjArray>(args[3].as.obj);
    if(!params) return NIL_VAL;

    int paramCount = (int) paramTypes->size().to_int();

    if(paramTypes->size().to_int() != paramCount) return NIL_VAL;

    std::vector<HSTRING> hstrings;
    std::vector<IUnknown*> unknowns;

    void* slot = vtable_slot(this->ptr_,vtable_index);

    void* resultPtr = nullptr;

    Parameters parameters;
    parameters.addThis(this->ptr());
    parameters.addParameters(paramTypes, params);

    if (retType != "void")
    {
        parameters.addOutParam(&resultPtr);
    }


    HRESULT hr = make_rt_call(slot, parameters.values, parameters.types);
    
    Value rval = NIL_VAL;
    if(hr != S_OK)
    {
        return NIL_VAL;
        //std::cout << "FFI  CALL FAILED HR: " << hr << std::endl;
    }
    else
    {
        // marshal out parameter result if non void

        if(retType != "void")
        {
            rval = WinRtMarshaller::unmarshall(vm, retType,resultPtr);

        }
    }
    return rval;
}




Value ComObject::asyncAction( int argCount, Value* args)
{
    if(argCount < 6) return NIL_VAL;

    int vtable_index = (int) args[0].to_int();
    std::string retType = args[1].to_string();
    std::string handlerIID = args[2].to_string();
    auto paramTypes = as<ObjArray>(args[3].as.obj);
    if(!paramTypes) return NIL_VAL;
    auto params = as<ObjArray>(args[4].as.obj);
    if(!params) return NIL_VAL;

    Value cb = args[5];

    int paramCount = (int) paramTypes->size().to_int();

    if(paramTypes->size().to_int() != paramCount) return NIL_VAL;

    std::vector<HSTRING> hstrings;
 
    void* slot = vtable_slot(this->ptr_,vtable_index);

    Parameters parameters;
    parameters.addThis(this->ptr());
    parameters.addParameters(paramTypes, params);

    ABI::Windows::Foundation::IAsyncAction* asyncAction = nullptr;
    parameters.addOutParam((void**)&asyncAction);

    Value rval = NIL_VAL;

    HRESULT hr = make_rt_call(slot, parameters.values, parameters.types);
    if(hr != S_OK)
    {
        resolve_rt_callback(vm, "Int32", ABI::Windows::Foundation::AsyncStatus::Error, &hr, cb);
    }
    else
    {
        ABI::Windows::Foundation::IAsyncActionCompletedHandler* handler = new AsyncActionHandler(vm, cb);
        HRESULT hr2 = asyncAction->put_Completed(handler);
        if (hr2 != S_OK)
        {
            resolve_rt_callback(vm, "Int32", ABI::Windows::Foundation::AsyncStatus::Error, &hr2, cb);
        }
        asyncAction->Release();
    }
    return NIL_VAL;
}

Value ComObject::asyncOperation(int argCount, Value* args)
{
    if (argCount < 6) return NIL_VAL;

    int vtable_index = (int) args[0].to_int();
    std::string retType = args[1].to_string();
    std::string handlerIID = args[2].to_string();
    auto paramTypes = as<ObjArray>(args[3].as.obj);
    if (!paramTypes) return NIL_VAL;
    auto params = as<ObjArray>(args[4].as.obj);
    if (!params) return NIL_VAL;

    Value cb = args[5];

    int paramCount = (int) paramTypes->size().to_int();

    if (paramTypes->size().to_int() != paramCount) return NIL_VAL;

    if (retType == "void") return NIL_VAL;

    void* resultPtr = nullptr;

    Parameters parameters;
    parameters.addThis(this->ptr());
    parameters.addParameters(paramTypes,params);
    parameters.addOutParam(&resultPtr);

    std::vector<HSTRING> hstrings;

    void* slot = vtable_slot(this->ptr_, vtable_index);


    HRESULT hr = make_rt_call(slot, parameters.values, parameters.types);

    if (hr != S_OK)
    {
        std::cout << "FFI  CALL FAILED HR: " << hr << std::endl;
        resolve_rt_callback(vm, "Int32", ABI::Windows::Foundation::AsyncStatus::Error, &hr, cb);
    }
    else
    {
        IUnknown* asyncOp = (IUnknown*)resultPtr;
        IUnknown* handler = new AsyncOperationHandler(vm, retType, handlerIID, cb);

        Parameters params2;
        params2.addThis(resultPtr);
        params2.addParam(handler);

        void* rslot = vtable_slot(resultPtr, 6); // 6 := put_COmpleted

        HRESULT hr2 = make_rt_call(rslot, params2.values, params2.types);
        if (hr2 != S_OK)
        {
            resolve_rt_callback(vm, "Int32", ABI::Windows::Foundation::AsyncStatus::Error, &hr2, cb);
        }
        asyncOp->Release();
    }
    return NIL_VAL;
}

Value ComObject::asyncOperationWithProgress(int argCount, Value* args)
{
    if (argCount < 6) return NIL_VAL;

    int vtable_index = (int) args[0].to_int();
    std::string retType = args[1].to_string();
    std::string handlerIID = args[2].to_string();
    auto paramTypes = as<ObjArray>(args[3].as.obj);
    if (!paramTypes) return NIL_VAL;
    auto params = as<ObjArray>(args[4].as.obj);
    if (!params) return NIL_VAL;

    Value cb = args[5];

    int paramCount = (int) paramTypes->size().to_int();
    if (paramTypes->size().to_int() != paramCount) return NIL_VAL;

    void* slot = vtable_slot(this->ptr_, vtable_index);
    void* resultPtr = nullptr;

    Parameters parameters;
    parameters.addThis(this->ptr());
    parameters.addParameters(paramTypes, params);
    parameters.addOutParam(&resultPtr);

    HRESULT hr = make_rt_call(slot, parameters.values, parameters.types);
    if (hr != S_OK)
    {
        resolve_rt_callback(vm, "Int32", ABI::Windows::Foundation::AsyncStatus::Error, &hr, cb);
    }
    else
    {
        IUnknown* handler = new AsyncOperationWithProgressHandler(vm, retType, handlerIID, cb);

        Parameters rparams;
        rparams.addThis(resultPtr);
        rparams.addParam(handler);

        void* rslot = vtable_slot(resultPtr, 8); // 6 := put_COmpleted

        HRESULT hr2 = make_rt_call(rslot, rparams.values, rparams.types);
        if (hr2 != S_OK)
        {
            resolve_rt_callback(vm, "Int32", ABI::Windows::Foundation::AsyncStatus::Error, &hr2, cb);
        }
        ((IUnknown*)resultPtr)->Release();
    }

    return NIL_VAL;
}


Value ComObject::asyncActionWithProgress(int argCount, Value* args)
{
    if (argCount < 6) return NIL_VAL;

    int vtable_index = (int) args[0].to_int();
    std::string retType = args[1].to_string();
    std::string handlerIID = args[2].to_string();
    auto paramTypes = as<ObjArray>(args[3].as.obj);
    if (!paramTypes) return NIL_VAL;
    auto params = as<ObjArray>(args[4].as.obj);
    if (!params) return NIL_VAL;

    Value cb = args[5];

    int paramCount = (int) paramTypes->size().to_int();

    if (paramTypes->size().to_int() != paramCount) return NIL_VAL;

    void* slot = vtable_slot(this->ptr_, vtable_index);
    void* resultPtr = nullptr;

    Parameters parameters;
    parameters.addThis(this->ptr());
    parameters.addParameters(paramTypes, params);
    parameters.addOutParam(&resultPtr);

    HRESULT hr = make_rt_call(slot, parameters.values, parameters.types);
    if (hr != S_OK)
    {
        resolve_rt_callback( vm, "Int32", ABI::Windows::Foundation::AsyncStatus::Error, &hr, cb);
    }
    else
    {
        IUnknown* handler = new AsyncOperationWithProgressHandler(vm, retType, handlerIID, cb);

        Parameters rparams;
        rparams.addThis(resultPtr);
        rparams.addParam(handler);

        void* rslot = vtable_slot(resultPtr, 6); // 6 := put_COmpleted

        HRESULT hr2 = make_rt_call(rslot, rparams.values, rparams.types);
        if (hr2 != S_OK)
        {
            resolve_rt_callback( vm, "Int32", ABI::Windows::Foundation::AsyncStatus::Error, &hr2, cb);
        }
        ((IUnknown*)resultPtr)->Release();
    }
    return NIL_VAL;
}


void ComObject::mark_gc()
{
}


IUnknown* ComObject::ptr()
{
    return ptr_;
}

void* ComObject::pointer()
{
    return ptr_;
}

Value ComObject::getMethod(const std::string& mname)
{
    return getProperty(mname);
}

bool ComObject::invokeMethod(const std::string& mname, int argCount)
{
    Value val = getMethod(mname);
    if(IS_OBJ(val))
    {
        return val.as.obj->callValue(argCount);
    }
    
    return false;
}

void ComObject::doRelease()
{
    if(ptr_)
    {
        this->ptr_->Release();
        this->ptr_ = nullptr;
    }
}

Value ComObject::getProperty(const std::string& pname)
{
    if(pname == "release")
    {
        auto release = new ObjNativeMethod( vm, 
            [](Value that, const std::string&, int /*argCount*/, Value* /*args*/) -> Value
            {
                auto comPtr = as<ComObject>(that.as.obj);
                if(!comPtr) return NIL_VAL;
                if (!comPtr->ptr()) return NIL_VAL;
                comPtr->doRelease();
                return NIL_VAL;
            }
        );        
        return release;
    }
    if(pname == "queryInterface")
    {
        auto queryInterface = new ObjNativeMethod( vm, 
            [](Value that, const std::string&, int argCount, Value* args) -> Value
            {
                if(argCount == 0) return NIL_VAL;
                auto comPtr = as<ComObject>(that.as.obj);
                if(!comPtr) return  new ComObject(comPtr->vm, nullptr);
                if (!comPtr->ptr()) return  new ComObject(comPtr->vm, nullptr);

                std::string guidStr = args[0].to_string();
                GUID guid = string_to_guid(to_wstring(guidStr));   
                IUnknown* unk = nullptr;             
                HRESULT hr = comPtr->ptr()->QueryInterface( guid, (void**)&unk);
                if(hr == S_OK && unk)
                {
                    auto result = new ComObject(comPtr->vm, unk); // refCount now = 2
                    unk->Release(); // refCount = 1
                    return result;                    
                }
                return  new ComObject(comPtr->vm, nullptr);
            }
        );        
        return queryInterface;
    }
    if(pname == "invoke")
    {
        auto invoke = new ObjNativeMethod( vm,
            []( Value that, const std::string&, int argCount, Value* args) -> Value
            {                
                if(argCount == 0) return NIL_VAL;
                auto comPtr = as<ComObject>(that.as.obj);
                if(!comPtr) return NIL_VAL;
                if (!comPtr->ptr()) return NIL_VAL;
                return comPtr->invoke(argCount,args);
            }
        );        
        return invoke;
    }
    if (pname == "asyncAction")
    {
        auto invoke = new ObjNativeMethod( vm,
            [](Value that, const std::string&, int argCount, Value* args) -> Value
        {
            if (argCount == 0) return NIL_VAL;
            auto comPtr = as<ComObject>(that.as.obj);
            if (!comPtr) return NIL_VAL;
            if (!comPtr->ptr()) return NIL_VAL;
            return comPtr->asyncAction(argCount, args);
        }
        );
        return invoke;
    }
    if (pname == "asyncActionWithProgress")
    {
        auto invoke = new ObjNativeMethod( vm,
            []( Value that, const std::string&, int argCount, Value* args) -> Value
            {
                if (argCount == 0) return NIL_VAL;
                auto comPtr = as<ComObject>(that.as.obj);
                if (!comPtr) return NIL_VAL;
                if (!comPtr->ptr()) return NIL_VAL;
                return comPtr->asyncActionWithProgress(argCount, args);
            }
        );
        return invoke;
    }
    if (pname == "asyncOperation")
    {
        auto invoke = new ObjNativeMethod( vm,
            []( Value that, const std::string&, int argCount, Value* args) -> Value
            {
                if (argCount == 0) return NIL_VAL;
                auto comPtr = as<ComObject>(that.as.obj);
                if (!comPtr) return NIL_VAL;
                return comPtr->asyncOperation(argCount, args);
            }
        );
        return invoke;
    }
    if (pname == "asyncOperationWithProgress")
    {
        auto invoke = new ObjNativeMethod( vm,
            [](Value that, const std::string&, int argCount, Value* args) -> Value
            {
                if (argCount == 0) return NIL_VAL;
                auto comPtr = as<ComObject>(that.as.obj);
                if (!comPtr) return NIL_VAL;
                if (!comPtr->ptr()) return NIL_VAL;
                return comPtr->asyncOperationWithProgress(argCount, args);
            }
        );
        return invoke;
    }
    return NIL_VAL;
}

void ComObject::setProperty(const std::string& /*key*/, Value /*val*/)
{
}

void ComObject::deleteProperty(const std::string& /*name*/)
{
}

const std::string& ComObject::toString() const
{
    static std::string id = "IUnknown";
    return id;
}


///////////////////////////////////////////////////////////////

#endif
