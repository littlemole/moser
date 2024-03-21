#include "native.h"

#ifdef _WIN32
#include <windows.h>
#define RTLD_NEXT 0
#define RTLD_LAZY 0
#endif


namespace native {

    StructMemberDesc::~StructMemberDesc()
    {
        if(0)//cleanup && type)
        {
            free(type->elements);
            free(type);
            type = nullptr;
        }
    }


    std::map<std::string,std::shared_ptr<StructDesc>>& theStructCache()
    {
        static std::map<std::string,std::shared_ptr<StructDesc>> c;
        return c;
    }

    std::map<std::string, ffi_type*>& t_lookup()
    {
        static std::map<std::string, ffi_type*> theMap = {
            { "byte"   , &ffi_type_sint8},
            { "short"  , &ffi_type_sint16},
            { "int"    , &ffi_type_sint32},
            { "long"   , &ffi_type_sint64},
            { "ubyte"  , &ffi_type_uint8},
            { "ushort" , &ffi_type_uint16},
            { "uint"   , &ffi_type_uint32},
            { "ulong"  , &ffi_type_uint64},
            { "float"  , &ffi_type_float},
            { "double" , &ffi_type_double},
            { "ptr"    , &ffi_type_pointer},
            { "str"    , &ffi_type_pointer},
            { "wstr"   , &ffi_type_pointer},
            { "void"   , &ffi_type_void}
        };
        return theMap;
    }

    ffi_type* type_lookup(const std::string& name)
    {
        if(t_lookup().count(name))
        {
            return t_lookup()[name];
        }
        if(theStructCache().count(name))
        {
            return theStructCache()[name]->type;
        }
        return nullptr;
    }


    Struct::Struct(std::map<std::string, StructMemberDesc>& els, size_t s) 
        : size(s), elements(els)
    {
        ptr = malloc(size);
        ptr_to_free = ptr;
        memset(ptr, 0, size);
    }


    Struct::Struct(std::map<std::string, StructMemberDesc>& els, size_t s,void* p) 
        : size(s), elements(els)
    {
        ptr = p;
        ptr_to_free = nullptr;
    }

    Struct::~Struct()
    {
        if (ptr_to_free)
        {
            free(ptr_to_free);
            ptr_to_free = nullptr;
        }
    }



    StructDesc::StructDesc(
        std::vector<std::pair<std::string, StructMemberDesc>>& els, 
        ffi_type* t)
      : type(t), size(t->size)
    {
        for (auto& el : els)
        {
            elements[el.first] = el.second;
        }
    }

    StructDesc::~StructDesc()
    {
        if(type )
        {
            free(type->elements);
            free(type);
            type = nullptr;
        }        
    }

    std::shared_ptr<Struct> StructDesc::create()
    {
        return std::make_shared<Struct>(elements, size);
    }


    std::shared_ptr<Struct> StructDesc::attach(void* p)
    {
        return std::make_shared<Struct>(elements, size, p);
    }



    MakeStruct::MakeStruct(size_t ws) : word_size(ws)
    {}

    MakeStruct::MakeStruct(const std::string& name, size_t ws = 8) : name_(name), word_size(ws)
    {}

    void MakeStruct::add(const std::string& n, const std::string&  typeName, ffi_type* t) //, bool cleanup = false)
    {
        size_t pos = n.find("[");
        if( pos == std::string::npos)
        {
            elements.push_back(
                std::make_pair(n, StructMemberDesc{ 0,t,n,typeName,false })
            );
            return;
        }
        std::string as = n.substr(pos+1);
        int size = atoi(as.c_str());

        ffi_type* array_type = (ffi_type*)malloc(sizeof(ffi_type));
        ffi_type **els;
        els = (ffi_type**)malloc ((size + 1) * sizeof (ffi_type *));
        for (int i = 0; i < size; ++i)
        {
            els[i] = t;
        }
        els[size] = NULL;
        array_type->size = array_type->alignment = 0;
        array_type->type = FFI_TYPE_STRUCT;
        array_type->elements = els;

        elements.push_back(
            std::make_pair(n.substr(0,pos), StructMemberDesc{ 0,array_type,n.substr(0,pos),typeName,true })
        );
    }

    void MakeStruct::add(const std::string& n, const std::string& typ)
    {
        ffi_type* t = type_lookup(typ);
        add(n,typ,t);
    }

    void MakeStruct::add(const std::string& n)
    {
        size_t pos = n.find(":");
        if (pos == std::string::npos) {
            return;
        }
        std::string key = n.substr(0, pos);
        std::string typ = n.substr(pos + 1);

        ffi_type* t = type_lookup(typ);

        add(key,typ,t);
        return;
    }

    void MakeStruct::add(const std::vector<std::string>& v)
    {
        for (auto& i : v)
        {
            add(i);
        }
    }

    std::shared_ptr<native::StructDesc> MakeStruct::describe()
    {
        if(theStructCache().count(name_) > 0)
        {
            return theStructCache()[name_];
        }


        type_ = (ffi_type*)malloc(sizeof(ffi_type));
        ffi_type** types = (ffi_type**)malloc(sizeof(ffi_type)*(elements.size()+1));

        for (size_t i = 0; i < elements.size(); i++)
        {
            types[i] = elements[i].second.type;
        }
        types[elements.size()] = nullptr; // :( oh my dear c-api

        type_->size = 0;
        type_->alignment = 0;
        type_->type = FFI_TYPE_STRUCT;
        type_->elements = types;

        ffi_cif cif;
        if (ffi_prep_cif (&cif, FFI_DEFAULT_ABI, 0, type_, 0) == FFI_OK)
        {
            std::vector<size_t> offsets(elements.size()+1,0);
            ffi_get_struct_offsets(FFI_DEFAULT_ABI,type_,&offsets[0]);
            for(size_t i = 0; i < elements.size(); i++)
            {
                elements[i].second.offset = offsets[i];
            }
        }

        auto result = std::make_shared<StructDesc>(elements,type_);
        if(!name_.empty())
        {
            theStructCache()[name_] = result;
        }
        return result;
    }


    ffi_type& MakeStruct::type()
    {
        return *type_;
    }



    Fun::Fun(const std::string& returnType, void* f, std::vector<std::string> params) 
        :  
          params_(std::move(params)), 
          intbuf(params_.size(),0),
          fun(f),
          returnTypeStr_(returnType)
    {
        
        returnType_ = t_lookup()[returnType];

        for (auto& p : params_)
        {
            ffi_type* ft = t_lookup()[p];

            types_.push_back(ft);
        }
        prepare();
    }

    void* Fun::pointer() 
    {
        return fun;
    }

    bool Fun::prepare()
    {
        ffi_type** t = types_.empty() ? 0 : &types_[0];
        status_ = ffi_prep_cif(&cif_, FFI_DEFAULT_ABI, (unsigned int)types_.size(), returnType_,t);
        if (status_ != FFI_OK) 
        {
            return false;
        }
        return true;
    }

    ffi_arg Fun::invoke()
    {
        std::vector<void*> v(values_.begin(), values_.end());
        std::vector<void*> args;
        for (auto& t : v)
        {
            args.push_back(&t);
        }
        clear();

        ffi_arg result;
        void** p = v.empty() ? 0 : &v[0];
        ffi_call(&cif_, FFI_FN(fun), &result, p);
        
        return result;
    }


    void Fun::clear()
    {
        values_.clear();
    }

    size_t Fun::nargs()
    {
        return types_.size();
    }

    const std::string& Fun::argument(size_t i)
    {
        static std::string empty = "";
        if (i >= nargs())
        {
            return empty;
        }
        return params_[i];
    }

    ffi_type* Fun::type(size_t i)
    {
        if (i >= nargs())
        {
            return nullptr;
        }
        return types_[i];
    }

    const std::string& Fun::returnTypeStr()
    {
        return returnTypeStr_;
    }

    ffi_type* Fun::returnType()
    {
        return returnType_;
    }



    VarFun::VarFun(const std::string& returnType, void* f, std::vector<std::string> params, size_t nfix)
        : Fun(params.size()+16), nfixed(nfix)
    {
        fun = f;
        params_ = params;

        returnType_ = t_lookup()[returnType];

        for (auto& p : params_)
        {
            ffi_type* ft = t_lookup()[p];

            types_.push_back(ft);
        }

        vartypes_ = types_;
    }

    Fun& VarFun::value(size_t i, std::string typ)
    {
        intbuf[values_.size()] = i;
        void* t = &intbuf[values_.size()];
        values_.push_back((void*)t);
        if(values_.size() > types_.size())
        {
            vartypes_.push_back( t_lookup()[typ] );
        }
        return *this;
    }

    bool VarFun::prepare()
    {

        ffi_type** t = vartypes_.empty() ? 0 : &vartypes_[0];
        status_ = ffi_prep_cif_var(&cif_, FFI_DEFAULT_ABI, (unsigned int)nfixed, (unsigned int)vartypes_.size(),  returnType_,t);
        if (status_ != FFI_OK) 
        {
            return false;
        }
        return true;
    }

    ffi_arg VarFun::invoke()
    {
        prepare();

        ffi_arg r = Fun::invoke();

        values_.clear();
        vartypes_.clear();
        vartypes_ = types_;
        return r;
    }

    void Callback::callbackfun(ffi_cif *cif, void *ret, void **args, void *user_data)
    {
        Callback* cb = (Callback*)user_data;
        cb->cbfun(cif,ret,args);
    }

    void Callback::cbfun(ffi_cif *cif, void *ret, void **args)
    {
        if(cb_)
        {
            cb_(cif,ret,args);
        }
    }

    Callback::Callback(const std::string& returnType, std::vector<std::string> params) 
        : Fun(params.size())
    {
        returnType_ = t_lookup()[returnType];
        params_ = params;

        for (auto& p : params_)
        {
            ffi_type* ft = t_lookup()[p];

            types_.push_back(ft);
        }

        prepare();            

        addr_ = ffi_closure_alloc ( sizeof(ffi_closure ), &closure_);
    
        status_ = ffi_prep_closure_loc ((ffi_closure *)addr_, &cif_, callbackfun, this, closure_);
        if (status_ != FFI_OK) 
        {
            printf("ffi_prep_closure: cb failed\n");
            exit(1);
        }

    }
    
    bool Callback::prepare()
    {
        ffi_type** t = types_.empty() ? 0 : &types_[0];
        status_ = ffi_prep_cif(&cif_, FFI_DEFAULT_ABI, (unsigned int)types_.size(), returnType_, t);
        if (status_ != FFI_OK) 
        {
            return false;
        }
        return true;
    }


    FunMaker::FunMaker()
    {
    }

    FunMaker::FunMaker( std::string lib)
    {
#ifdef _WIN32
        if(lib.empty() || lib == "libc.so" || lib == "libc")
        {
            lib = "ucrtbase.dll";
        }
        libhandle_ = dlopen(lib.c_str(), RTLD_LAZY);
        if (!libhandle_) 
        {
           // printf("dlopen error %s\n", dlerror());
           // exit(1);
        }
#else            
        if(lib.empty() || lib == "libc.so" || lib == "libc")
        {
            libhandle_ = RTLD_NEXT;
        }
        else
        {
            libhandle_ = dlopen(lib.c_str(), RTLD_LAZY);
            if (!libhandle_) 
            {
             //   printf("dlopen error %s\n", dlerror());
             //   exit(1);
            }
        }
#endif            
    }

    std::shared_ptr<Fun> FunMaker::makeFun(const std::string& returnType, void* ptr, const std::vector<std::string>& params)
    {

        void* fun = ptr;
        if (!fun)
        {
            //printf(" dlsym error %s\n",name.c_str());
            //exit(1);
        }

        return std::make_shared<Fun>(returnType, fun, params);
    }

    std::shared_ptr<Fun> FunMaker::makeFun(const std::string& returnType, const std::string& name, const std::vector<std::string>& params)
    {

        void* fun = dlsym(libhandle_, name.c_str());
        if (!fun)
        {
            //printf(" dlsym error %s\n",name.c_str());
            //exit(1);
        }

        return std::make_shared<Fun>(returnType, fun, params);
    }

    std::shared_ptr<VarFun> FunMaker::makeVarFun(const std::string& returnType, const std::string& name, const std::vector<std::string>& params, size_t nfixed)
    {

        void* fun = dlsym(libhandle_, name.c_str());
        if (!fun)
        {
            //printf(" dlsym error %s \n", name.c_str());
            //exit(1);
        }

        return std::make_shared<VarFun>(returnType, fun, params, nfixed);
    }

    std::shared_ptr<Callback> FunMaker::makeCallback(const std::string& returnType, const std::vector<std::string>& params)
    {
        return std::make_shared<Callback>(returnType, params);
    }


    FunMaker::~FunMaker()
    {
#ifdef _WIN32        
        if(libhandle_ != RTLD_NEXT)
        {
            dlclose(libhandle_);
        }
#endif        
        libhandle_ = nullptr;
    }

}
