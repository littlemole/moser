//#include "pch.h"
#ifdef _WIN32

#include <vm.h>
#include "win32/disp.h"
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

void addVariantArg(VM& vm, VARIANT& v, Value value, VARTYPE vt);


Value getVariantResult(VM& vm, VARIANT& varResult)
{
    switch (varResult.vt)
    {
    case VT_BOOL:
    {
        return (varResult.boolVal == VARIANT_TRUE) ? true : false;
    }
    case VT_BSTR:
    {
        size_t len = ::SysStringLen(varResult.bstrVal);
        std::wstring ws((wchar_t*)(varResult.bstrVal), len);
        return new ObjString(vm, to_string(ws));
    }
    case VT_DISPATCH:
    {
        return new DispatchObject(vm, varResult.pdispVal);
    }
    case VT_EMPTY:
    {
        return NIL_VAL;
    }
    case VT_I1:
    {
        return ptrdiff_t((long)varResult.cVal);
    }
    case VT_I2:
    {
        return ptrdiff_t((long)varResult.iVal);
    }
    case VT_I4:
    {
        return ptrdiff_t((long)varResult.intVal);
    }
    case VT_I8:
    {
        return ptrdiff_t((long)varResult.llVal);
    }
    case VT_UI1:
    {
        return ptrdiff_t((long)varResult.bVal);
    }
    case VT_UI2:
    {
        return ptrdiff_t((long)varResult.uiVal);
    }
    case VT_UI4:
    {
        return ptrdiff_t((long)varResult.uintVal);
    }
    case VT_UI8:
    {
        return ptrdiff_t((long)varResult.ullVal);
    }
    case VT_R4:
    {
        return (double)varResult.fltVal;
    }
    case VT_R8:
    {
        return varResult.dblVal;
    }
    }
    return NIL_VAL;
}


class FunWrapper : public ::IDispatch
{
public:

    VM& vm;
    Value fun;

    FunWrapper(VM& v, Value f) : vm(v), fun(f)
    {
        AddRef();
    }

    virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (::IsEqualIID(IID_IUnknown, riid))
        {
            AddRef();
            *ppvObject = (void*)(IUnknown*)this;
        }
        if (::IsEqualIID(IID_IDispatch, riid))
        {
            AddRef();
            *ppvObject = (void*)(IDispatch*)this;
        }
        return E_NOINTERFACE;
    }

    virtual ULONG __stdcall AddRef(void) override
    {
        cnt++;
        return (ULONG)cnt;
    }

    virtual ULONG __stdcall Release(void) override
    {
        cnt--;
        ptrdiff_t c = cnt;
        if (cnt == 0)
        {
            delete this;
        }
        return (ULONG)c;
    }

    virtual HRESULT __stdcall GetTypeInfoCount(UINT* /*pctinfo*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT __stdcall GetTypeInfo(UINT /*iTInfo*/, LCID, ITypeInfo**) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT __stdcall GetIDsOfNames(REFIID, LPOLESTR* /*rgszNames*/, UINT /*cNames*/, LCID, DISPID* /*rgDispId*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT __stdcall Invoke(DISPID dispIdMember, REFIID, LCID, WORD /*wFlags*/, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* /*pExcepInfo*/, UINT* /*puArgErr*/) override
    {
        if (dispIdMember == DISPID_VALUE)
        {
            std::vector<Value> args;
            for (ULONG i = 0; i < pDispParams->cArgs; i++)
            {
                Value val = getVariantResult(vm, pDispParams->rgvarg[i]);
                args.push_back(val);
            }
            auto fu = ::as<ObjClosure>(fun.as.obj);
            if (fu)
            {
                //bool b = 
                fu->callValue((int)args.size());
                // TODO: call

                Value r = NIL_VAL; // todo: get result
                if (pVarResult)
                {
                    VARIANT v;
                    ::VariantInit(&v);
                    addVariantArg(vm, v, r, VT_VARIANT);
                    *pVarResult = v;
                }
            }

            return S_OK;
        }
        return E_NOTIMPL;
    }

    ptrdiff_t cnt = 0;
};

void addVariantArg(VM& vm, VARIANT& v, Value val, VARTYPE vt)
{
    ::VariantInit(&v);
    v.vt = VT_EMPTY;

    switch (val.type)
    {
    case ValueType::VAL_NIL:
    {
        v.vt = VT_EMPTY;
        ::VariantChangeType(&v, &v, 0, vt);
        break;
    }
    case ValueType::VAL_BOOL:
    {
        v.vt = VT_BOOL;
        v.boolVal = val.as.boolean ? VARIANT_TRUE : VARIANT_FALSE;
        ::VariantChangeType(&v, &v, 0, vt);
        break;
    }
    case ValueType::VAL_INT:
    {
        v.vt = v.vt = VT_I4;
        v.intVal = (INT)val.as.integer;
        ::VariantChangeType(&v, &v, 0, vt);
    }
    case ValueType::VAL_NUMBER:
    {
        v.vt = v.vt = VT_R8;
        v.dblVal = val.as.number;
        ::VariantChangeType(&v, &v, 0, vt);
    }
    case ValueType::VAL_OBJ:
    {
        auto str = ::as<ObjString>(val);
        if (str)
        {
            std::wstring ws = to_wstring(str->toString());
            v.vt = VT_BSTR;
            v.bstrVal = ::SysAllocStringLen(ws.c_str(), (UINT)ws.size());
            ::VariantChangeType(&v, &v, 0, vt);
        }
        auto co = ::as<DispatchObject>(val);
        if (co)
        {
            IDispatch* disp = co->ptr();
            v.vt = VT_DISPATCH;
            v.pdispVal = disp;
            disp->AddRef();
        }
        auto closure = ::as<ObjClosure>(val);
        if (closure)
        {
            FunWrapper* fw = new FunWrapper(vm, val);
            v.vt = VT_DISPATCH;
            v.pdispVal = (::IDispatch*)fw;
        }
        break;
    }
    }

}


DispatchObject::DispatchObject(VM& v) : ObjBuiltin(v) {}

DispatchObject::DispatchObject(VM& v, IDispatch* d) : ObjBuiltin(v), disp_(d)
{
    disp_->AddRef();
    prepare(L"<progid>");
}

DispatchObject::~DispatchObject()
{
    if (disp_)
    {
        disp_->Release();
        disp_ = nullptr;
    }
}


bool DispatchObject::prepare(const std::wstring& progid)
{
    ITypeInfo* ti = nullptr;
    HRESULT hr = disp_->GetTypeInfo(0, LOCALE_SYSTEM_DEFAULT, &ti);
    if (hr != S_OK)
    {
        return false;
    }

    typelib::MetaInterface mi;

    bool r = typelib::parse_typeinfo(ti, progid, mi);
    if (!r)
    {
        return false;
    }

    progid_ = to_string(progid);

    for (size_t i = 0; i < mi.functions.size(); i++)
    {
        auto f = mi.functions[i];
        ComFun fu;
        fu.dispid = f.dispid;
        fu.vt_result = f.vt_result;
        fu.kind = f.invokeKind;
        for (size_t j = 0; j < f.params.size(); j++)
        {
            VARTYPE vt = f.params[j].vt;

            if (f.params[j].isRetVal)
            {
                fu.vt_result = vt;
            }
            else
            {
                fu.params.push_back(vt);
            }
        }
        if (f.isPropGet && fu.params.empty())
        {
            getters[to_string(f.name)] = fu;
        }
        else if (f.isPropPut)
        {
            setters[to_string(f.name)] = fu;
        }
        else
        {
            methods[to_string(f.name)] = fu;
        }
    }
    return true;
}

bool DispatchObject::create(const std::string& s, int ctx)
{
    std::wstring progid = to_wstring(s);

    CLSID clsid;
    HRESULT hr = ::CLSIDFromProgID(progid.c_str(), &clsid);
    if (hr != S_OK)
    {
        return false;
    }

    hr = ::CoCreateInstance(clsid, nullptr, ctx, IID_IDispatch, (void**)&disp_);
    if (hr != S_OK)
    {
        return false;
    }

    prepare(progid);

    return true;
}

void DispatchObject::mark_gc()
{
}


IDispatch* DispatchObject::ptr()
{
    return disp_;
}

void* DispatchObject::pointer()
{
    return disp_;
}

Value DispatchObject::getMethod(const std::string& mname)
{
    return getProperty(mname);
}

bool DispatchObject::invokeMethod(const std::string& mname, int argCount)
{
    Value val = getMethod(mname);
    if (IS_OBJ(val))
    {
        return val.as.obj->callValue(argCount);
    }

    return false;
}

Value DispatchObject::getProperty(const std::string& pname)
{
    if (methods.count(pname))
    {
        auto cometh = new ObjNativeMethod(vm, pname,
            [](Value that, const std::string& pname, int argCount, Value* args) -> Value
            {
                auto co = ::as<DispatchObject>(that);
                if (!co) return NIL_VAL;

                ComFun& fu = co->methods[pname];
                if (argCount != fu.params.size())
                {
                    // return NIL_VAL;
                }

                std::vector<VARIANT> vars;

                int len = argCount;
                if (len)
                    for (int i = len - 1; i >= 0; i--)
                    {
                        VARIANT v;
                        addVariantArg(co->vm, v, args[i], fu.params[argCount - 1 - i]);
                        vars.push_back(v);
                    }

                DISPPARAMS params = { 0,0,0,0 };
                params.cArgs = (UINT)len;// fu.params.size();
                if (!vars.empty())
                {
                    params.rgvarg = &vars[0];
                }

                VARIANT varResult;
                ::VariantInit(&varResult);
                EXCEPINFO ex;
                ::ZeroMemory(&ex, sizeof(ex));
                UINT errArg = 0;
                //fu.kind

                int kind = fu.kind;// | DISPATCH_METHOD;
                HRESULT hr = co->disp_->Invoke(fu.dispid, IID_NULL, LOCALE_USER_DEFAULT, (WORD)kind, &params, &varResult, &ex, &errArg);

                if (hr != S_OK)
                {
                    IErrorInfo* ie = nullptr;
                    GetErrorInfo(0, &ie);
                    if (ie)
                    {
                        BSTR err = nullptr;
                        ie->GetDescription(&err);
                        ::SysFreeString(err);
                        ie->Release();
                    }
                    return NIL_VAL;
                }

                Value result;
                if (hr == S_OK)
                {
                    result = getVariantResult(co->vm, varResult);
                }
                ::VariantClear(&varResult);
                for (auto& v : vars)
                {
                    ::VariantClear(&v);
                }
                return result;
            }
        );
        return cometh;
    };

    if (!getters.count(pname)) return NIL_VAL;

    ComFun& fu = getters[pname];

    DISPPARAMS params = { 0,0,0,0 };

    VARIANT varResult;
    ::VariantInit(&varResult);

    HRESULT hr = disp_->Invoke(fu.dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &params, &varResult, 0, 0);

    Value result;
    if (hr == S_OK)
    {
        result = getVariantResult(vm, varResult);
    }
    ::VariantClear(&varResult);
    return result;
}

void DispatchObject::setProperty(const std::string& key, Value val)
{
    if (setters.count(key))
    {
        ComFun& fu = setters[key];

        DISPPARAMS params = { 0,0,0,0 };

        DISPID dispidNamed = DISPID_PROPERTYPUT;
        params.cNamedArgs = 1;
        params.rgdispidNamedArgs = &dispidNamed;

        VARIANT param;
        addVariantArg(vm, param, val, fu.params[0]);
        params.cArgs = 1;
        params.rgvarg = &param;

        VARIANT varResult;
        ::VariantInit(&varResult);

        disp_->Invoke(fu.dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUT, &params, &varResult, 0, 0);
        ::VariantClear(&varResult);
    }
}

void DispatchObject::deleteProperty(const std::string& /*name*/)
{
}

const std::string& DispatchObject::toString() const
{
    return progid_;
}

Value DispatchObject::enumerate()
{
    VARIANT r;
    ::VariantInit(&r);

    DISPPARAMS params = { 0,0,0,0 };
    //    0xfffffffc
    HRESULT hr = disp_->Invoke(DISPID_NEWENUM, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, &r, 0, 0);
    if (hr != S_OK) return NIL_VAL;

    IEnumVARIANT* ev = 0;
    hr = r.punkVal->QueryInterface(IID_IEnumVARIANT, (void**)&ev);
    if (hr != S_OK)
    {
        ::VariantClear(&r);
        return NIL_VAL;
    }

    auto result = new ObjArray(vm);
    while (hr == S_OK)
    {
        VARIANT v;
        ::VariantInit(&v);
        ULONG fetched = 0;
        hr = ev->Next(1, &v, &fetched);

        if ((hr == S_OK) && fetched)
        {
            Value val = getVariantResult(vm, v);
            result->add(val);
        }
        ::VariantClear(&v);
        if (!fetched) break;
    }
    ev->Release();
    ::VariantClear(&r);
    return result;
}



#endif
