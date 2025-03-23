#ifndef moc_com_rtm_object_h
#define moc_com_rtm_object_h

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
    MOSER marshaler for WinRT COM calls
*/

inline GUID string_to_guid(const std::wstring& str)
{
    GUID guid;
    ::CLSIDFromString(str.c_str(), &guid);
    return guid;
}

inline std::wstring guid_to_string(const GUID& guid)
{
    wchar_t buf[256];
    size_t len = ::StringFromGUID2(guid, buf, 256);
    if (len == 0)
    {
        return L"";
    }
    return std::wstring(buf, len - 1);
}


inline void** vtable(void* that)
{
    void*** vt = (void***)that;
    return *vt;
}

inline void* vtable_slot(void* that, size_t index)
{
    void** vt = vtable(that);
    return vt[index];
}

class WinRtMarshaller
{
public:

    WinRtMarshaller()
    {}

    static ffi_type* marshall(const std::string& paramType, Value& val, void*& dest)
    {
        if (paramType == "Int16")
        {
            int16_t c = (int16_t)val.to_int();
            *(int16_t*)dest = c;
            return &ffi_type_sint16;
        }
        if (paramType == "UInt16")
        {
            uint16_t c = (uint16_t)val.to_int();
            *(uint16_t*)dest = c;
            return &ffi_type_uint16;
        }
        if (paramType == "Int32")
        {
            int32_t c = (int32_t)val.to_int();
            *(int32_t*)dest = c;
            return &ffi_type_sint32;
        }
        if (paramType == "UInt32")
        {
            uint32_t c = (uint32_t)val.to_int();
            *(uint32_t*)dest = c;
            return &ffi_type_uint32;
        }
        if (paramType == "Int64")
        {
            int64_t c = (int64_t)val.to_int();
            *(int64_t*)dest = c;
            return &ffi_type_sint64;
        }
        if (paramType == "UInt64")
        {
            uint64_t c = (uint64_t)val.to_int();
            *(uint64_t*)dest = c;
            return &ffi_type_uint64;
        }
        if (paramType == "UInt8")
        {
            uint8_t c = (uint8_t)val.to_int();
            *(uint8_t*)dest = c;
            return &ffi_type_uint8;
        }
        if (paramType == "Int8")
        {
            int8_t c = (int8_t)val.to_int();
            *(int8_t*)dest = c;
            return &ffi_type_sint8;
        }
        if (paramType == "Char16")
        {
            uint16_t c = (uint16_t)val.to_int();
            *(uint16_t*)dest = c;
            return &ffi_type_sint16;
        }
        if (paramType == "Boolean")
        {
            int8_t b = (int8_t)val.to_int();
            *(int8_t*)dest = b;
            return &ffi_type_sint8;
        }
        if (paramType == "Single")
        {
            float f = (float)val.to_double();
            *(float*)dest = f;
            return &ffi_type_float;
        }
        if (paramType == "Double")
        {
            double d = (double)val.to_double();
            *(double*)dest = d;
            return &ffi_type_double;
        }
        if (paramType == "String")
        {
            std::wstring str = to_wstring(val.to_string());
            HSTRING h = nullptr;
            ::WindowsCreateString(str.c_str(), (UINT32)str.size(), &h);
            *(HSTRING*)dest = h;
            return &ffi_type_pointer;
        }

        auto wstr = as<ObjWideString>(val);
        if (wstr)
        {
            *(void**)dest = wstr->pointer();
        }
        // assume ptr

        // Interface?
        auto comPtr = as<ComObject>(val);
        if (comPtr)
        {
            *(IUnknown**)dest = comPtr->ptr();
        }
        else
        {
            auto ptr = as<ObjPointer>(val);
            if (ptr)
            {
                *(void**)dest = ptr->pointer();
            }
            else if (IS_OBJ(val))
            {
                std::map<std::string, std::shared_ptr<native::StructDesc>>& cache = native::theStructCache();
                if (cache.count(paramType))
                {
                    auto& st = cache[paramType];
                    size_t s = st->size;
                    if (s <= 8)
                    {
                        *(size_t*)dest = *(size_t*)val.as.obj->pointer();
                        return st->type;
                    }
                }
                *(void**)dest = val.as.obj->pointer();
                return &ffi_type_pointer;
            }
            else
            {
                *(uint32_t*)dest = (uint32_t)val.to_int();
                return &ffi_type_uint32;
            }
        }
        return &ffi_type_pointer;
    }

    static Value unmarshall(VM& vm, const std::string& paramType, void* result)
    {
        Value rval = NIL_VAL;

        void* resultPtr = &result;

        if (paramType == "Int16")
        {
            int16_t retVal = *(int16_t*)(void*)resultPtr;
            rval = Value((size_t)retVal);
        }
        else if (paramType == "UInt16")
        {
            uint16_t retVal = *(uint16_t*)(void*)resultPtr;
            rval = Value((size_t)retVal);
        }
        else if (paramType == "Int32")
        {
            int32_t retVal = *(int32_t*)(void*)resultPtr;
            rval = Value((size_t)retVal);
        }
        else if (paramType == "UInt32")
        {
            uint32_t retVal = *(uint32_t*)(void*)resultPtr;
            rval = Value((size_t)retVal);
        }
        else if (paramType == "Int64")
        {
            int64_t retVal = *(int64_t*)(void*)resultPtr;
            rval = Value((size_t)retVal);
        }
        else if (paramType == "UInt64")
        {
            uint64_t retVal = *(uint64_t*)(void*)resultPtr;
            rval = Value((size_t)retVal);
        }
        else if (paramType == "Int8")
        {
            int8_t retVal = *(int8_t*)(void*)resultPtr;
            rval = Value((size_t)retVal);
        }
        else if (paramType == "UInt8")
        {
            uint8_t retVal = *(uint8_t*)(void*)resultPtr;
            rval = Value((size_t)retVal);
        }
        else if (paramType == "Char16")
        {
            uint16_t retVal = *(uint16_t*)(void*)resultPtr;
            rval = Value((size_t)retVal);
        }
        else if (paramType == "Single")
        {
            float retVal = *(float*)(void*)resultPtr;
            rval = Value((double)retVal);
        }
        else if (paramType == "Double")
        {
            double retVal = *(double*)(void*)resultPtr;
            rval = Value((double)retVal);
        }
        else if (paramType == "String")
        {
            HSTRING retVal = *(HSTRING*)(void*)resultPtr;
            UINT32 len = 0;
            const wchar_t* wc = ::WindowsGetStringRawBuffer(retVal, &len);
            std::wstring ws(wc, len);
            ::WindowsDeleteString(retVal);
            rval = Value(new ObjString(vm, to_string(ws)));
        }
        else if (paramType == "Boolean")
        {
            uint32_t retVal = *(uint32_t*)(void*)resultPtr;
            rval = Value((ptrdiff_t)retVal != 0);
        }
        else if (paramType == "Pointer" || paramType == "ptr")
        {
            rval = Value(new ObjPointer(vm, *(void**)resultPtr));
        }
        else if (paramType == "wstr" || paramType == "wstring")
        {
            wchar_t* retVal = *(wchar_t**)resultPtr;
            rval = Value(new ObjWideString(vm, retVal));
        }
        else
        {
            std::map<std::string, std::shared_ptr<native::StructDesc>>& cache = native::theStructCache();
            if (cache.count(paramType))
            {
                rval = Value(new ObjPointer(vm, *(void**)resultPtr));
                return rval;
            }
            rval = Value(new ComObject(vm, *(IUnknown**)resultPtr));
        }
        return rval;
    }
};



inline Value unwrap(Value& v)
{
    if (!IS_OBJ(v)) return v;

    auto proxy = as<ObjProxy>(v);
    if (proxy && proxy->fields.count("target"))
    {
        Value val = proxy->fields["target"];
        return unwrap(val);
    }

    auto instance = as<ObjInstance>(v);
    if (instance)
    {
        if (instance->klass)
        {
            Value val = instance->getProperty("comPtr");
            return unwrap(val);
        }
    }
    return v;
}


class Parameters
{
public:
    std::vector<size_t> values;
    std::vector<ffi_type*> types;
    std::vector<HSTRING> hstrings;

    ~Parameters()
    {
        for (auto& it : hstrings)
        {
            ::WindowsDeleteString(it);
        }
    }

    void addThis(void* that)
    {
        types.push_back(&ffi_type_pointer);
        values.push_back((size_t)that);
    }

    void addParam(void* p)
    {
        types.push_back(&ffi_type_pointer);
        values.push_back((size_t)p);
    }

    void addParameters(ObjArray* paramTypes, ObjArray* params)
    {
        ptrdiff_t paramCount = paramTypes->size().to_int();

        for (ptrdiff_t i = 0; i < paramCount; i++)
        {
            auto paramType = paramTypes->index(i).to_string();
            Value val = params->index(i);
            Value unwrapped = unwrap(val);
            size_t s = 0;
            void* p = &s;
            ffi_type* ffiType = WinRtMarshaller::marshall(paramType, unwrapped, p);
            types.push_back(ffiType);
            values.push_back(s);
            if (paramType == "String")
            {
                hstrings.push_back(*(HSTRING*)p);
            }
        }
    }

    void addOutParam(void** outParam)
    {
        types.push_back(&ffi_type_pointer);
        values.push_back((size_t)outParam);
    }
};


inline HRESULT make_rt_call(void* slot, std::vector<size_t>& buf, std::vector<ffi_type*>& v)
{
    std::vector<void*> values;

    unsigned int cifArgs = (unsigned int)buf.size();

    // put values into a void* array for ffi
    for (unsigned int i = 0; i < cifArgs; i++)
    {
        void* p = &buf[i];
        values.push_back(p);
    }

    ffi_type** aTypes = 0;
    if (cifArgs)
    {
        aTypes = &v[0];
    }

    Value rval = NIL_VAL;

    // prepare the FFI call
    ffi_cif cif;
    ffi_type rType = ffi_type_uint32;

    ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, cifArgs, &rType, aTypes);
    if (status == FFI_OK)
    {
        void** vals = 0;
        if (values.size())
        {
            vals = &values[0];
        }

        size_t result = 0;

        // make the FFI call
        ffi_call(&cif, FFI_FN(slot), &result, vals);

        // for winrt, result always is a HRESULT
        HRESULT hr = (HRESULT)result;
        return hr;
    }
    return E_FAIL;
}

inline void resolve_rt_callback(VM& vm, const std::string& retType, ABI::Windows::Foundation::AsyncStatus status, void* outparam, Value cb)
{
    Value v = WinRtMarshaller::unmarshall(vm, retType, outparam);
    auto obj = as<Obj>(cb);
    if (obj)
    {
        vm.push(obj);
        vm.push((size_t)status);
        vm.push(v);
        obj->callValue(2);
        vm.top_frame().returnToCallerOnReturn = true;
        vm.run();
        vm.pop(); // ?
    }
}


inline void resolve_rt_callback(VM& vm, ABI::Windows::Foundation::AsyncStatus status, Value cb)
{
    auto obj = as<Obj>(cb);
    if (obj)
    {
        vm.push(obj);
        vm.push((size_t)status);
        obj->callValue(1);
        vm.top_frame().returnToCallerOnReturn = true;
        vm.run();
        vm.pop(); // ?
    }
}



#endif


