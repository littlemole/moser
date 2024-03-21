#ifndef moc_com_rtm_delegate_object_h
#define moc_com_rtm_delegate_object_h

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

/*
    WinRT COM delegates for MOSER
*/


template<class ... Args>
class Delegate : public IUnknown
{
public:

    VM& vm;
    std::atomic<int> refCnt_;

    std::vector<std::string> args_;
    GUID handlerIID_;
    Value cb_;

    Delegate(VM& vm, const std::vector<std::string>& args, std::string handlerIID, Value cb);
    ~Delegate();

    virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    virtual HRESULT __stdcall Invoke(Args ...);
};

template<class ... Args>
Delegate<Args...>::Delegate(VM& v, const std::vector<std::string>& args, std::string handlerIID, Value cb)
    : vm(v), args_(args), cb_(cb)
{
    handlerIID_ = string_to_guid(to_wstring(handlerIID));
    refCnt_ = 1;

    vm.gc.pin(cb_.as.obj);
}

template<class ... Args>
Delegate<Args...>::~Delegate()
{
    vm.gc.unpin(cb_.as.obj);
}

template<class ... Args>
HRESULT STDMETHODCALLTYPE Delegate<Args...>::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject) return E_INVALIDARG;

    if (::IsEqualGUID(IID_IUnknown, riid))
    {
        *ppvObject = (IUnknown*)this;
        refCnt_++;
        return S_OK;
    }
    if (::IsEqualGUID(handlerIID_, riid))
    {
        *ppvObject = (IUnknown*)this;
        refCnt_++;
        return S_OK;
    }
    return E_NOINTERFACE;
}

template<class ... Args>
ULONG STDMETHODCALLTYPE Delegate<Args...>::AddRef()
{
    refCnt_++;
    return 2;
}

template<class ... Args>
ULONG STDMETHODCALLTYPE Delegate<Args...>::Release()
{
    bool shutdown = refCnt_ == 1;
    refCnt_--;
    if (shutdown)
    {
        delete this;
    }
    return 1;
}

template<class ... Args>
HRESULT STDMETHODCALLTYPE Delegate<Args...>::Invoke(Args ... arg)
{
    std::vector<void*> params { ((void*)arg) ... };
    std::vector<Value> values;
    for (size_t i = 0; i < params.size(); i++)
    {
        Value v = WinRtMarshaller::unmarshall(vm, args_[i], params[i]);
        values.push_back(v);
    }

    auto obj = as<Obj>(cb_);
    if (obj)
    {
        vm.push(cb_);
        for (auto& it : values) vm.push(it);
        obj->callValue((int)values.size());
        vm.frames.back().returnToCallerOnReturn = true;
        vm.run();
        vm.pop(); // ?
    }

    //this->Release();
    return S_OK;
}

inline ComObject* make_delegate(VM& vm, const std::vector<std::string> argTypes, const std::string& handlerIID, Value cb)
{
    IUnknown* delegate = nullptr;
    switch (argTypes.size())
    {
    case 0:
    {
        delegate = new Delegate<>(vm, argTypes, handlerIID, cb);
        break;
    }
    case 1:
    {
        delegate = new Delegate<void*>(vm, argTypes, handlerIID, cb);
        break;
    }
    case 2:
    {
        delegate = new Delegate<void*, void*>(vm, argTypes, handlerIID, cb);
        break;
    }
    case 3:
    {
        delegate = new Delegate<void*, void*, void*>(vm, argTypes, handlerIID, cb);
        break;
    }
    case 4:
    {
        delegate = new Delegate<void*, void*, void*, void*>(vm, argTypes, handlerIID, cb);
        break;
    }
    }
    auto comPtr = new ComObject(vm, delegate);
    delegate->Release();
    return comPtr;
}



#endif
