#ifndef moc_com_rtm_async_object_h
#define moc_com_rtm_async_object_h

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
    WinRT async method invocation support
*/

class AsyncActionHandler : public ABI::Windows::Foundation::IAsyncActionCompletedHandler
{
public:
    VM& vm;

    std::atomic<int> refCnt_;

    Value cb_;

    AsyncActionHandler(VM& v, Value cb)
        : vm(v), cb_(cb)
    {
        refCnt_ = 1;

        vm.gc.pin(cb_.as.obj);
    }

    ~AsyncActionHandler()
    {
        vm.gc.unpin(cb_.as.obj);
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (!ppvObject) return E_INVALIDARG;

        if (::IsEqualGUID(IID_IUnknown, riid))
        {
            *ppvObject = (IUnknown*)this;
            refCnt_++;
            return S_OK;
        }
        if (::IsEqualGUID(__uuidof(ABI::Windows::Foundation::IAsyncActionCompletedHandler), riid))
        {
            *ppvObject = (IUnknown*)this;
            refCnt_++;
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef() override
    {
        refCnt_++;
        return 2;
    }

    virtual ULONG STDMETHODCALLTYPE Release() override
    {
        bool shutdown = refCnt_ == 1;
        refCnt_--;
        if (shutdown)
        {
            delete this;
        }
        return 1;
    }

    virtual HRESULT STDMETHODCALLTYPE Invoke(ABI::Windows::Foundation::IAsyncAction* /*asyncInfo*/, ABI::Windows::Foundation::AsyncStatus status)  override
    {
        switch (status)
        {
        case ABI::Windows::Foundation::AsyncStatus::Started: { return S_OK; }
        case ABI::Windows::Foundation::AsyncStatus::Canceled: // fall thru
        case ABI::Windows::Foundation::AsyncStatus::Error: // fall thru // { resolve_rt_callback(status, cb_); this->Release(); return S_OK; }
        case ABI::Windows::Foundation::AsyncStatus::Completed:
        default: break;
        }

        resolve_rt_callback(vm, status, cb_);

        this->Release();
        return S_OK;
    }

};



class AsyncOperationHandler : public IUnknown
{
public:

    VM& vm;
    std::atomic<int> refCnt_;

    std::string retType_;
    GUID handlerIID_;
    Value cb_;

    AsyncOperationHandler(VM& v, std::string retType, std::string handlerIID, Value cb)
        : vm(v), retType_(retType), cb_(cb)
    {
        handlerIID_ = string_to_guid(to_wstring(handlerIID));
        refCnt_ = 1;

        vm.gc.pin(cb_.as.obj);
    }

    ~AsyncOperationHandler()
    {
        vm.gc.unpin(cb_.as.obj);
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
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

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        refCnt_++;
        return 2;
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        bool shutdown = refCnt_ == 1;
        refCnt_--;
        if (shutdown)
        {
            delete this;
        }
        return 1;
    }

    virtual HRESULT STDMETHODCALLTYPE Invoke(IUnknown* asyncInfo, ABI::Windows::Foundation::AsyncStatus status)
    {
        switch (status)
        {
        case ABI::Windows::Foundation::AsyncStatus::Started: { return S_OK; }
        case ABI::Windows::Foundation::AsyncStatus::Canceled: // fall thru
        case ABI::Windows::Foundation::AsyncStatus::Error:
        {
            IAsyncInfo* ai = nullptr;
            asyncInfo->QueryInterface(&ai);
            HRESULT hr = S_OK;
            ai->get_ErrorCode(&hr);
            ai->Release();
            resolve_rt_callback(vm, "Int32", status, &hr, cb_);
            this->Release();
            return S_OK;
        }
        case ABI::Windows::Foundation::AsyncStatus::Completed:
        default: break;
        }

        void* result = nullptr;
        IUnknown* outparam = nullptr;
        void* slot = vtable_slot(asyncInfo, 8); // GetResults is slot 8 

        Parameters parameters;
        parameters.addThis(asyncInfo);
        parameters.addOutParam((void**)&outparam);

        HRESULT hr = make_rt_call(slot, parameters.values, parameters.types);

        if (hr != S_OK)
        {
            std::cout << "FFI  CALL FAILED HR: " << result << std::endl;
        }
        else
        {
            resolve_rt_callback(vm, retType_, status, outparam, cb_);
        }
        this->Release();
        return S_OK;
    }

};

class AsyncActionWithProgressHandler : public IUnknown
{
public:
    VM& vm;
    std::atomic<int> refCnt_;

    std::string retType_;
    GUID handlerIID_;
    Value cb_;

    AsyncActionWithProgressHandler(VM& v, std::string retType, std::string handlerIID, Value cb)
        : vm(v), retType_(retType), cb_(cb)
    {
        handlerIID_ = string_to_guid(to_wstring(handlerIID));
        refCnt_ = 1;

        vm.gc.pin(cb_.as.obj);
    }

    ~AsyncActionWithProgressHandler()
    {
        vm.gc.unpin(cb_.as.obj);
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
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

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        refCnt_++;
        return 2;
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        bool shutdown = refCnt_ == 1;
        refCnt_--;
        if (shutdown)
        {
            delete this;
        }
        return 1;
    }

    virtual HRESULT STDMETHODCALLTYPE Invoke(IUnknown* asyncInfo, void* /*progress */)
    {
        IAsyncInfo* ai = nullptr;
        asyncInfo->QueryInterface(&ai);
        ABI::Windows::Foundation::AsyncStatus status;
        ai->get_Status(&status);
        ai->Release();

        switch (status)
        {
        case ABI::Windows::Foundation::AsyncStatus::Started: { return S_OK; }
        case ABI::Windows::Foundation::AsyncStatus::Canceled: // fall thru
        case ABI::Windows::Foundation::AsyncStatus::Error:
        {
            IAsyncInfo* ai2 = nullptr;
            asyncInfo->QueryInterface(&ai2);
            HRESULT hr = S_OK;
            ai2->get_ErrorCode(&hr);
            ai2->Release();
            resolve_rt_callback(vm, "Int32", status, &hr, cb_);
            this->Release();
            return S_OK;
        }
        case ABI::Windows::Foundation::AsyncStatus::Completed:
        default: break;
        }

        resolve_rt_callback(vm, status, cb_);
        this->Release();
        return S_OK;
    }

};

class AsyncOperationWithProgressHandler : public IUnknown
{
public:
    VM& vm;
    std::atomic<int> refCnt_;

    std::string retType_;
    GUID handlerIID_;
    Value cb_;

    AsyncOperationWithProgressHandler(VM& v, std::string retType, std::string handlerIID, Value cb)
        : vm(v), retType_(retType), cb_(cb)
    {
        handlerIID_ = string_to_guid(to_wstring(handlerIID));
        refCnt_ = 1;

        vm.gc.pin(cb_.as.obj);
    }

    ~AsyncOperationWithProgressHandler()
    {
        vm.gc.unpin(cb_.as.obj);
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
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

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        refCnt_++;
        return 2;
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        bool shutdown = refCnt_ == 1;
        refCnt_--;
        if (shutdown)
        {
            delete this;
        }
        return 1;
    }

    virtual HRESULT STDMETHODCALLTYPE Invoke(IUnknown* asyncInfo, void* /*progress */)
    {
        IAsyncInfo* ai = nullptr;
        asyncInfo->QueryInterface(&ai);
        ABI::Windows::Foundation::AsyncStatus status;
        ai->get_Status(&status);
        ai->Release();

        switch (status)
        {
        case ABI::Windows::Foundation::AsyncStatus::Started: { return S_OK; }
        case ABI::Windows::Foundation::AsyncStatus::Canceled: // fall thru
        case ABI::Windows::Foundation::AsyncStatus::Error:
        {
            IAsyncInfo* ai2 = nullptr;
            asyncInfo->QueryInterface(&ai2);
            HRESULT hr = S_OK;
            ai2->get_ErrorCode(&hr);
            ai2->Release();
            resolve_rt_callback(vm, "Int32", status, &hr, cb_);
            this->Release();
            return S_OK;
        }
        case ABI::Windows::Foundation::AsyncStatus::Completed:
        default: break;
        }

        void* result = nullptr;
        IUnknown* outparam = nullptr;
        void* slot = vtable_slot(asyncInfo, 10); // GetResults is slot 10

        Parameters parameters;
        parameters.addThis(asyncInfo);
        parameters.addOutParam((void**)&outparam);

        HRESULT hr = make_rt_call(slot, parameters.values, parameters.types);
        if (hr != S_OK)
        {
            std::cout << "FFI  CALL FAILED HR: " << result << std::endl;
        }
        else
        {
            resolve_rt_callback(vm, retType_, status, outparam, cb_);
        }

        this->Release();
        return S_OK;
    }

};




#endif
