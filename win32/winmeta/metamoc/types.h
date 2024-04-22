#pragma once
#include "cache.h"

GUID get_generic(const std::string& gen);
inline std::vector<std::string> split_generic(const std::string& gen);


enum class async_mode {
    NONE,
    ACTION,
    ACTION_WITH_PROGRESS,
    OPERATION,
    OPERATION_WITH_PROGRESS
};

struct DefaultInterface
{
    std::string ns;
    std::string name;
    GUID guid;
};



struct generic_instance
{
    std::string name;
    std::vector<generic_instance> params;

    friend bool operator<(const generic_instance& lhs, const generic_instance& rhs)
    {
        if (lhs.name != rhs.name)
        {
            return strcmp(lhs.name.c_str(), rhs.name.c_str()) > 0;
        }
        if (lhs.params.size() != rhs.params.size())
        {
            return lhs.params.size() < rhs.params.size();
        }
        for (size_t i = 0; i < lhs.params.size(); i++)
        {
            auto s1 = lhs.params[i];
            auto s2 = rhs.params[i];
            if (s1.name != s2.name)
            {
                return s1 < s2;
            }
        }
        return false;
    }

    std::string toString() const
    {
        std::ostringstream oss;

        oss << name;
        if (!params.empty()) oss << "<";
        std::vector<std::string> v;
        for (auto& it : params)
        {
            v.push_back(it.toString());
        }
        oss << join(v, ",");
        if (!params.empty()) oss << ">";
        return oss.str();
    }

    // map`2<string,vector`1<int>>    : string,vector`1<int>
    // map`2<vector`1<int>,string>    : vector`1<int>,string

    static void parse_args(generic_instance& result, std::string& input, size_t& pos)
    {
        if (input.empty()) return;

        if (pos >= input.size()) return;

        std::string stream = input.substr(pos);

        size_t p1 = stream.find_first_of("`<,>");
        if (p1 == std::string::npos)
        {
            generic_instance param;
            param.name = stream;
            result.params.push_back(param);
            pos += stream.size();
            return;
        }
        if (stream[p1] == '`')
        {
            generic_instance param;
            param.name = stream.substr(0, p1 + 2);
            std::string args = stream.substr(p1 + 3);
            pos += p1 + 3;
            parse_args(param, input, pos);
            result.params.push_back(param);
            parse_args(result, input, pos);
            return;
        }
        if (stream[p1] == '>')
        {
            if (p1 != 0)
            {
                generic_instance param;
                param.name = stream.substr(0, p1);
                result.params.push_back(param);
                std::string args = stream.substr(p1 + 1);
            }
            pos += p1 + 1;
            parse_args(result, input, pos);
            return;
        }
        if (stream[p1] == ',')
        {
            if (p1 != 0)
            {
                generic_instance param;
                param.name = stream.substr(0, p1);
                result.params.push_back(param);
            }
            pos += p1 + 1;
            parse_args(result, input, pos);
            return;
        }
    }

    static generic_instance parse(std::string type)
    {
        generic_instance result;
        size_t p1 = type.find("`");
        if (p1 == std::string::npos)
        {
            result.name = type;
            return result;
        }
        result.name = type.substr(0, p1 + 2);
        std::string args = type.substr(p1 + 3);
        size_t pos = 0;
        parse_args(result, args, pos);
        return result;
    }
};

extern std::set<generic_instance> generic_instances;
extern std::set<generic_instance> generic_delegates;


extern std::map<async_mode, std::string> asyncModeOperation;

inline std::string make_struct_type(const ElementType& t)
{
    static std::map<ElementType, std::string> theMap = {
        { ElementType::Void, "Void"},
        { ElementType::Boolean, "int"},
        { ElementType::I1, "byte" },
        { ElementType::U1, "ubyte" },
        { ElementType::I2, "short" },
        { ElementType::U2, "ushort" },
        { ElementType::I4, "int" },
        { ElementType::U4, "uint" },
        { ElementType::I8, "long" },
        { ElementType::U8, "ulong" },
        { ElementType::R4, "float" },
        { ElementType::R8, "double" },
        { ElementType::String, "ptr" },
        { ElementType::Ptr, "ptr" },
        { ElementType::ByRef, "ptr" },
        { ElementType::ValueType, "ptr" },
        { ElementType::Class, "ptr" },
        { ElementType::Var, "ptr" },
        { ElementType::Array, "ptr" },
        { ElementType::GenericInst, "ptr" },
        { ElementType::TypedByRef, "ptr" },
        { ElementType::I, "ptr" },
        { ElementType::U, "ptr" },
        { ElementType::FnPtr, "ptr" },
        { ElementType::Object, "ptr"}, //"IInspectable" },
        { ElementType::SZArray, "ptr" },
        { ElementType::MVar, "ptr" }
    };
    if (theMap.count(t) == 0)
    {
        return "ptr";
    }
    return theMap[t];
}


inline async_mode is_async(const std::string& retType)
{
    if (retType.starts_with("Windows.Foundation.IAsyncActionWithProgress"))
    {
        return async_mode::ACTION_WITH_PROGRESS;
    }
    if (retType.starts_with("Windows.Foundation.IAsyncAction"))
    {
        return async_mode::ACTION;
    }
    if (retType.starts_with("Windows.Foundation.IAsyncOperationWithProgress"))
    {
        return async_mode::OPERATION_WITH_PROGRESS;
    }
    if (retType.starts_with("Windows.Foundation.IAsyncOperation"))
    {
        return async_mode::OPERATION;
    }
    return async_mode::NONE;
}


inline std::string print_param_type(TypeSig& typeSig, std::pair<ParamAttributes, std::string>& param)
{
    auto ts = typeSig.Type();

    std::ostringstream oss;

    //    if (param.first.In()) std::cout << "[in] ";
    if (param.first.Out()) oss << "[out] ";

//    if (typeSig.is_szarray()) std::cout << "array(";
    std::visit([&param, &oss](auto&& arg)
        {
            using typ = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<typ, ElementType>)
            {
                oss << getTypeName(arg);
            }
            if constexpr (std::is_same_v<typ, coded_index<TypeDefOrRef>>)
            {
                auto t = find(arg);
                if (t)
                {
                    std::string n(t.TypeName());
                    oss << t.TypeNamespace() << "." << n;
                }
                else
                {
                    int idx = arg.index();
                    if (idx == 174)
                        oss << "GUID";
                    else
                        oss << "ptr";
                }
            }
            if constexpr (std::is_same_v<typ, GenericTypeIndex>)
            {
                oss << "T" << arg.index;
            }
            if constexpr (std::is_same_v<typ, GenericTypeInstSig>)
            {
                bool isDelegate = false;
                auto tmp = arg.GenericType();
                if (tmp)
                {
                    TypeDef x = find(tmp);
                    //print_attrs(x);
                    std::string n(x.TypeName());
                    oss << x.TypeNamespace() << "." << n;

                    auto cat = get_category(x);
                    if (cat == category::delegate_type)
                    {
                        isDelegate = true;
                    }
                }
                else
                {
                    oss << "UnknownGenType";
                }

                if (arg.GenericArgCount() > 0)
                {
                    std::string n = oss.str();
                    oss << "<";
                    std::vector<std::string> v;
                    bool is_instantiation = true;
                    generic_instance gi{ n };
                    for (auto ga : arg.GenericArgs())
                    {
                        auto t = print_param_type(ga, param);
                        if (t[0] == 'T' && (('0' <= t[1]) && (t[1] <= '9'))) is_instantiation = false;

                        v.push_back(t);
                        generic_instance g{ t };
                        gi.params.push_back(g);
                    }
                    oss << join(v, ",") << ">";

                    if (is_instantiation)
                    {
                        if (isDelegate)
                        {
                            generic_delegates.insert(gi);

                        }
                        else
                        {
                            generic_instances.insert(gi);
                        }
                    }
                }
            }
            if constexpr (std::is_same_v<typ, GenericMethodTypeIndex>)
            {
                oss << "genmethindex(" << arg.index << ")";
            }
        },
        ts);

//    if (typeSig.is_szarray()) std::cout << ")";
    if (typeSig.is_array())
    {
        oss << "[";
        for (auto i : typeSig.array_sizes())
        {
            oss << i << ",";
        }
        oss << "]";
    }

    for (int i = 0; i < typeSig.ptr_count(); i++)
    {
        oss << "*";
    }

    //if (param.first.Out()) std::cout << "*";
    return oss.str();
}

inline std::vector<std::string> get_param_types(MethodDefSig& methodDefSig, std::pair<Param, Param>& params)
{
    //  auto params = methodDefSig.Params();
    std::vector<std::string> result;
    if (methodDefSig.Params().first == methodDefSig.Params().second) return result;

    std::vector<std::pair<ParamAttributes, std::string>> paramNames;
    for (auto&& jt : params)
    {
        if (jt.Sequence() == 0) continue;
        std::string n(jt.Name());
        ParamAttributes flags = jt.Flags();
        paramNames.push_back({ flags,n });
    }
    int n = 0;
    for (auto&& jt : methodDefSig.Params())
    {
        auto t = jt.Type();

        std::string s = print_param_type(t, paramNames[n]);
        result.push_back(s);
        n++;
    }
    return result;
}


inline std::vector<std::string> get_param_names(MethodDefSig& methodDefSig, std::pair<Param, Param>& params)
{
    //  auto params = methodDefSig.Params();
    std::vector<std::string> result;
    if (methodDefSig.Params().first == methodDefSig.Params().second) return result;

    std::vector<std::pair<ParamAttributes, std::string>> paramNames;
    for (auto&& jt : params)
    {
        if (jt.Sequence() == 0) continue;
        std::string n(jt.Name());
        ParamAttributes flags = jt.Flags();
        paramNames.push_back({ flags,n });
    }
    int n = 0;
    for (auto&& jt : methodDefSig.Params())
    {
        auto t = jt.Type();

        std::string s = paramNames[n].second;
        result.push_back(s);
        n++;
    }
    return result;
}


inline bool skip(MethodDefSig& sig, std::pair<winmd::reader::Param, winmd::reader::Param>& params)
{
    auto ret = sig.ReturnType();
    if (ret)
    {
        auto r = ret.Type();
        if (r.is_array()) return true;
        if (r.is_szarray()) return true;
    }
    auto psig = sig.Params();
    for (auto&& p : psig)
    {
        if (p.Type().is_array()) return true;
        if (p.Type().is_szarray()) return true;

    }
    for (auto&& p : params)
    {
        if (p.Flags().Out())
        {
            return true;
        }
        // if(p.ty)
    }
    return false;
}

inline std::string get_retType(const MethodDef& methodDef)
{
    std::string retType = "void";
    auto rt = methodDef.Signature().ReturnType();
    if (rt)
    {
        auto retTypeSig = rt.Type();

        ParamAttributes attr{};
        std::pair<ParamAttributes, std::string> p{ attr, "" };
        retType = print_param_type(retTypeSig, p);
    }
    return retType;
}


inline GUID get_handler_guid(std::string type)
{
    GUID result;
    TypeDef typeDef = theCache.find(type);
    if (!typeDef)
    {
        return IID_NULL;
    }

    for (auto&& m : typeDef.MethodList())
    {
        if (m.Name() == "put_Completed")
        {
            auto sig = m.Signature();
            auto param = sig.Params().first;
            auto t = param->Type();
            ParamAttributes attr{};
            std::pair<ParamAttributes, std::string> p{ attr, "" };
            auto type = print_param_type(t, p);
            size_t pos = type.find("<");
            if (pos != std::string::npos)
            {
                type = type.substr(0, pos);
            }
            auto td = theCache.find(type);
            result = get_guid(td);
        }
    }

    return result;
}

inline GUID get_generic_handler_guid(std::string type)
{
    GUID result;
    TypeDef typeDef = theCache.find(type);
    if (!typeDef) return IID_NULL;

    for (auto&& m : typeDef.MethodList())
    {
        if (m.Name() == "put_Completed")
        {
            auto sig = m.Signature();
            auto param = sig.Params().first;
            auto t = param->Type();
            ParamAttributes attr{};
            std::pair<ParamAttributes, std::string> p{ attr, "" };
            auto type = print_param_type(t, p);
            size_t pos = type.find("<");
            if (pos != std::string::npos)
            {
                // type = type.substr(0, pos);
            }
            auto td = theCache.find(type);
            result = get_guid(td);
        }
    }

    return result;
}

inline std::string get_callback(const std::string& genType, const std::string& fullname)
{
    std::ostringstream oss;
    if (genType.starts_with("Windows.Foundation.IAsyncActionWithProgress"))
    {
        std::string arg = get_first_gen_arg(fullname);
        oss << "Windows.Foundation.AsyncActionWithProgressCompletedHandler`1<" << arg << ">";
        return oss.str();
    }
    if (genType.starts_with("Windows.Foundation.IAsyncAction"))
    {
        oss << "Windows.Foundation.AsyncActionCompletedHandler";
        return oss.str();
    }
    if (genType.starts_with("Windows.Foundation.IAsyncOperationWithProgress"))
    {
        std::string arg1 = get_first_gen_arg(fullname);
        std::string arg2 = get_second_gen_arg(fullname);
        oss << "Windows.Foundation.AsyncOperationWithProgressCompletedHandler`2<" << arg1 << "," << arg2 << ">";
        return oss.str();
    }
    if (genType.starts_with("Windows.Foundation.IAsyncOperation"))
    {
        std::string arg = get_first_gen_arg(fullname);
        oss << "Windows.Foundation.AsyncOperationCompletedHandler`1<" << arg << ">";
        return oss.str();
    }
    return "";
}


inline DefaultInterface get_default_interface(TypeDef& typeDef)
{
    DefaultInterface result;

    for (auto impl : typeDef.InterfaceImpl())
    {
        bool def = is_default(impl);
        if (def)
        {
            auto iface = find(impl.Interface());
            result.name = iface.TypeName();
            result.ns = iface.TypeNamespace();
            result.guid = get_guid(iface);
            return result;
        }
    }

    return result;
}


struct RoMetaDataLocator : public IRoMetaDataLocator
{
    HRESULT Locate(PCWSTR nameElement, IRoSimpleMetaDataBuilder& metaDataDestination) const
    {
        std::string nameElementUtf8 = to_utf8(nameElement);
        size_t pos = nameElementUtf8.find("<");
        if (pos != std::string::npos)
        {
            GUID guid = get_generic(nameElementUtf8);
            metaDataDestination.SetWinRtInterface(guid);
            return S_OK;
        }
        if (nameElementUtf8 == "ptr")
        {
            GUID guid = IID_IInspectable;
            metaDataDestination.SetWinRtInterface(guid);
            return S_OK;
        }
        if (nameElementUtf8 == "IInspectable")
        {
            GUID guid = IID_IInspectable;
            metaDataDestination.SetWinRtInterface(guid);
            return S_OK;
        }
        TypeDef typeDef = theCache.find(nameElementUtf8);
        if (!typeDef) return E_FAIL;

        category cat = get_category(typeDef);
        switch (cat)
        {
        case category::interface_type:
        {
            GUID guid = get_guid(typeDef);
            auto gen = typeDef.GenericParam();
            int cnt = 0;
            for (auto&& unused : gen)
            {
                cnt++;
            }
            if (cnt)
            {
                metaDataDestination.SetParameterizedInterface(guid, cnt);
            }
            else
            {
                metaDataDestination.SetWinRtInterface(guid);
            }
            break;
        }
        case category::class_type:
        {
            auto defaultIface = get_default_interface(typeDef);
            std::string name = defaultIface.ns + "." + defaultIface.name;
            std::wstring defaultIfaceName = to_wstring(name);

            metaDataDestination.SetRuntimeClassSimpleDefault(nameElement, defaultIfaceName.c_str(), &defaultIface.guid);
            break;
        }
        case category::struct_type:
        {
            int n = 0;
            auto fieldList = typeDef.FieldList();
            std::vector<std::wstring> strs;
            std::vector<const wchar_t*> names;
            for (auto&& field : fieldList)
            {
                n++;
                TypeSig sig = field.Signature().Type();

                std::string name = get_type(sig);
                size_t pos = name.find_first_of("<");
                if (pos == std::string::npos)
                {
                    strs.push_back(to_wstring(name));
                }
                else
                {
                    auto v = split_generic(name);
                    for (auto& it : v)
                    {
                        strs.push_back(to_wstring(it));
                    }
                }
            }
            for (auto& it : strs)
            {
                names.push_back(it.c_str());
            }
            metaDataDestination.SetStruct(nameElement, (UINT32)strs.size(), &names[0]);
            break;
        }
        case category::enum_type:
        {
            std::string base = get_enum_base(typeDef);
            std::wstring baseType = to_wstring(base);
            metaDataDestination.SetEnum(nameElement, baseType.c_str());
            break;
        }
        case category::delegate_type:
        {
            GUID iid = get_guid(typeDef);
            auto gen = typeDef.GenericParam();
            int cnt = 0;
            for (auto&& unused : gen)
            {
                cnt++;
            }
            if (cnt)
            {
                metaDataDestination.SetParameterizedDelegate(iid, cnt);
            }
            else
            {
                metaDataDestination.SetDelegate(iid);
            }
            break;
        }
        default:
        {
            std::cout << "##########################################" << std::endl;
            break;
        }
        }

        //return E_ABORT;

       // metaDataDestination.SetDelegate()
        return S_OK;
    }
};

inline GUID get_generic(const std::string& gen)
{
    GUID guid = GUID_NULL;

    auto wgen = to_wstring(gen);
    HSTRING hstr = nullptr;
    ::WindowsCreateString(wgen.c_str(), (UINT32)wgen.size(), &hstr);

    DWORD cnt = 0;
    HSTRING* parts = nullptr;
    HRESULT hr = ::RoParseTypeName(hstr, &cnt, &parts);
    if (hr != S_OK)
    {

        return guid;
    }

    std::vector<const wchar_t*> str;
    for (unsigned int i = 0; i < cnt; i++)
    {
        str.push_back(WindowsGetStringRawBuffer(parts[i], nullptr));
    }

    RoMetaDataLocator locator;
    ROPARAMIIDHANDLE pih;
    hr = RoGetParameterizedTypeInstanceIID(cnt, &str[0], locator, &guid, &pih);
    if (hr != S_OK)
    {
        int x = 1;
    }
    for (unsigned int i = 0; i < cnt; i++)
    {
        //std::wcout << WindowsGetStringRawBuffer(parts[i], nullptr) << std::endl;
        ::WindowsDeleteString(parts[i]);
    }
    ::CoTaskMemFree(parts);
    WindowsDeleteString(hstr);
    return guid;
}
