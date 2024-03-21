#ifndef moc_com_object_h
#define moc_com_object_h

#ifdef _WIN32
#include "object.h"
#include <map>

/*
    MOSER support for WinRT COM Objects
*/

struct IDispatch;
struct IUnknown;

Value unwrap(Value& v);


class ComObject : public ObjBuiltin
{
public:

    ComObject(VM&);
    ComObject(VM& , ::IUnknown* ptr);
    ~ComObject();

    virtual void mark_gc() override;

    bool create(const std::string& s, int ctx = 0x01 | 0x02 | 0x04 | 0x10); // CLSCTX_ALL

    virtual Value getMethod(const std::string& name) override;
    virtual bool  invokeMethod(const std::string& name, int argCount) override;

    virtual Value getProperty(const std::string& name) override;
    virtual void setProperty(const std::string& name, Value item) override;
    virtual void deleteProperty(const std::string& name) override;

    virtual const std::string& toString() const override;    
    virtual std::string type() const override { return "comptr"; }    

    virtual void* pointer() override;
    ::IUnknown* ptr();

    Value invoke( int argCount, Value* args);
    Value asyncAction(int argCount, Value* args);
    Value asyncActionWithProgress(int argCount, Value* args);
    Value asyncOperation( int argCount, Value* args);
    Value asyncOperationWithProgress(int argCount, Value* args);

private:

    void doRelease();

    ::IUnknown* ptr_ = nullptr;
};


//ComObject* make_delegate(VM& vm, const std::vector<std::string> argTypes, const std::string& handlerIID, Value cb);

#endif
#endif
