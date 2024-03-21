#ifndef moc_com_disp_object_h
#define moc_com_disp_object_h

#ifdef _WIN32
#include "object.h"
#include <map>

/*
    MOSER support for IDispatch based Scripting Objects
*/

struct IDispatch;
struct IUnknown;

class DispatchObject : public ObjBuiltin
{
public:

    DispatchObject(VM&);
    DispatchObject(VM&, IDispatch* disp);
    ~DispatchObject();

    bool create(const std::string& s, int ctx = 0x01 | 0x02 | 0x04 | 0x10); // CLSCTX_ALL
    bool prepare(const std::wstring& progid);

    virtual void mark_gc() override;

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value item) override;
    virtual void deleteProperty(const std::string& name) override;

    virtual const std::string& toString() const override;
    virtual std::string type() const override { return "disp"; }

    virtual void* pointer() override;
    ::IDispatch* ptr();

    Value enumerate();

private:

    struct ComFun
    {
        int kind = 0;
        long dispid = 0;
        unsigned short vt_result = 0;
        std::vector<unsigned short> params;
    };

    std::map<std::string, ComFun> methods;
    std::map<std::string, ComFun> getters;
    std::map<std::string, ComFun> setters;

    std::string progid_;
    ::IDispatch* disp_ = nullptr;
};

#endif
#endif
