#ifndef moc_ffi_h
#define moc_ffi_h

#include "common.h"
#include <map>
#include <memory>
#include <functional>

#include "win32/uni.h"
#include "win32/dlopen.h"
#include <ffi.h>

/*
    MOSER libffi interface for native calls.

    Here be Dragons.
*/


namespace native {

    class StructDesc;
    std::map<std::string,std::shared_ptr<StructDesc>>& theStructCache();

    std::map<std::string, ffi_type*>& t_lookup();
    size_t pad(size_t current, size_t s, size_t align);

    class StructMemberDesc
    {
    public:

        ~StructMemberDesc();

        size_t offset = 0;
        ffi_type* type = 0;
        std::string name;
        std::string typeName;
        bool cleanup = false;

    private:
    };

    class Struct
    {
    public:

        void* ptr = nullptr;
        size_t size = 0;

        Struct(std::map<std::string, StructMemberDesc>& els, size_t s);
        Struct(std::map<std::string, StructMemberDesc>& els, size_t s,void* p);
        ~Struct();

        template<class T>
        T get(const std::string& key)
        {
            T* src = (T*)((char*)ptr + elements[key].offset);
            return *src;
        }

        void* get_ptr(const std::string& key)
        {
            void* src = (void*)((char*)ptr + elements[key].offset);
            return src;
        }

        template<class T>
        void set(const std::string& key, T c)
        {
            T* target = (T*)((char*)ptr + elements[key].offset);

            if (sizeof(T) != elements[key].type->size) {
                printf("error mismatched size for %s %zu != %zu\n", key.c_str(), sizeof(T), elements[key].type->size);
                exit(1);
            }

            *target = c;
        }

        bool exists(const std::string& key) { return elements.count(key); }

        StructMemberDesc& desc(const std::string& key) { return elements[key]; }

        std::vector<std::string> keys()
        {
            std::vector<std::string> result;
            for(auto& it : elements)
            {
                result.push_back(it.first);
            }
            return result;
        }

    private:
        void* ptr_to_free = nullptr;
        std::map<std::string, StructMemberDesc> elements;
    };


    class StructDesc
    {
    public:
        ffi_type* type = nullptr;
        size_t size = 0;

        StructDesc(std::vector<std::pair<std::string, StructMemberDesc>>& els,ffi_type* t);
        ~StructDesc();

        std::shared_ptr<Struct> create();
        std::shared_ptr<Struct> attach(void* p);

    private:
        std::map<std::string, StructMemberDesc> elements;
    };

    std::map<std::string,std::shared_ptr<StructDesc>>& theStructCache();


    class MakeStruct
    {
    public:

        MakeStruct(size_t ws);
        MakeStruct(const std::string& n, size_t ws);

        void add(const std::string& n, const std::string& typ, ffi_type* t); //, bool cleanup);
        void add(const std::string& n, const std::string& type);
        void add(const std::string& n);
        void add(const std::vector<std::string>& v);

        std::shared_ptr<native::StructDesc> describe();

        ffi_type& type();

    private:
        std::string name_;
        std::vector<std::pair<std::string, StructMemberDesc>> elements;
        size_t word_size = 8;
        ffi_type* type_ = nullptr;
    };

    class Fun
    {
    public:

        Fun(size_t n) : intbuf(n,0) {}

        Fun(const std::string& returnType, void* f, std::vector<std::string> params);

        Fun& value(size_t i)
        {
            intbuf[values_.size()] = i;
            void* t = &intbuf[values_.size()];
            values_.push_back((void*)t);
            return *this;
        }

        

        void* pointer();

        virtual bool prepare();

        ffi_arg invoke();

        void clear();
        size_t nargs();

        const std::string& argument(size_t i);
        ffi_type* type(size_t i);
        const std::string& returnTypeStr();
        ffi_type* returnType();


        std::vector<void*> values_;

    protected:

        std::vector<std::string> params_;
        ffi_status status_ = (ffi_status)-1;
        std::vector<size_t> intbuf;

        void* fun = nullptr;
        std::string returnTypeStr_;
        ffi_type* returnType_;
        ffi_cif cif_;
        std::vector<ffi_type*> types_;
    };

    class Callback : public Fun
    {
    public:

        Callback(const std::string& returnType, std::vector<std::string> params);

        ~Callback() 
        {
            ffi_closure_free(addr_);
            addr_ = nullptr;
            closure_ = nullptr;
        }

        virtual bool prepare();

        void* closure() { return closure_; }

        template<class F>
        void callback( F fun )
        {
            cb_ = fun;
        }

    protected:

        static void callbackfun(ffi_cif *cif, void *ret, void **args, void *user_data);

        void cbfun(ffi_cif *cif, void *ret, void **args);

        std::function<void(ffi_cif*,void*,void**)> cb_;

        void* addr_ = nullptr;
        void* closure_ = nullptr;
    };

    class VarFun : public Fun
    {
    public:

        VarFun(const std::string& returnType, void* f, std::vector<std::string> params, size_t nfix);

        virtual bool prepare();

        ffi_arg invoke();

        Fun& value(size_t i, std::string typ);

        size_t nfixed = 0;

        std::vector<ffi_type*> vartypes_;
    };

    class FunMaker
    {
    public:

        FunMaker();
        FunMaker(std::string lib);
        ~FunMaker();

        std::shared_ptr<Fun> makeFun(const std::string& returnType, void* ptr, const std::vector<std::string>& params);
        std::shared_ptr<Fun> makeFun(const std::string& returnType, const std::string& name, const std::vector<std::string>& params);
        std::shared_ptr<VarFun> makeVarFun(const std::string& returnType, const std::string& name, const std::vector<std::string>& params, size_t nvariadic);
        std::shared_ptr<Callback> makeCallback(const std::string& returnType, const std::vector<std::string>& params);


    private:
        void* libhandle_ = nullptr;
    };

}

#endif
