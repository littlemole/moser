#pragma once
#include "common.h"

extern std::map<ElementType, std::string> typenames;

inline int gen_args(TypeDef& typeDef)
{
    std::string n(typeDef.TypeName());
    size_t pos = n.find("`");
    if (pos == std::string::npos) return 0;
    int s = atoi(n.substr(pos + 1).c_str());
    return s;
}

inline std::string getTypeName(const ElementType& t)
{
    if (typenames.count(t) == 0) return "unk";
    return typenames[t];
}

inline GUID get_guid(TypeDef& typeDef)
{
    GUID result;
    memcpy(&result, &GUID_NULL, sizeof(GUID));

    for (auto&& attr : typeDef.CustomAttribute())
    {
        std::string name(attr.TypeNamespaceAndName().second);
        if (name == "GuidAttribute")
        {
            CustomAttributeSig cas = attr.Value();
            auto vfas = cas.FixedArgs();
            unsigned long data1 = std::get<uint32_t>(std::get<0>(vfas[0].value).value);
            unsigned short data2 = std::get<unsigned short>(std::get<0>(vfas[1].value).value);
            unsigned short data3 = std::get<unsigned short>(std::get<0>(vfas[2].value).value);
            unsigned char data4[8] =
            {
                std::get<uint8_t>(std::get<0>(vfas[3].value).value),
                std::get<uint8_t>(std::get<0>(vfas[4].value).value),
                std::get<uint8_t>(std::get<0>(vfas[5].value).value),
                std::get<uint8_t>(std::get<0>(vfas[6].value).value),
                std::get<uint8_t>(std::get<0>(vfas[7].value).value),
                std::get<uint8_t>(std::get<0>(vfas[8].value).value),
                std::get<uint8_t>(std::get<0>(vfas[9].value).value),
                std::get<uint8_t>(std::get<0>(vfas[10].value).value)
            };

            result.Data1 = data1;
            result.Data2 = data2;
            result.Data3 = data3;
            memcpy(&result.Data4[0], &data4[0], sizeof(unsigned char) * 8);
        }
    }
    return result;
}

inline bool is_default(const InterfaceImpl& impl)
{
    auto attrs = impl.CustomAttribute();
    for (auto&& attr : attrs)
    {
        std::string name(attr.TypeNamespaceAndName().second);
        if (name == "DefaultAttribute")
        {
            return true;
        }
    }
    return false;
}

inline std::string get_type(const TypeSig& typeSig)
{
    auto ts = typeSig.Type();
    auto i = ts.index();

    std::ostringstream oss;

    std::visit([&oss](auto&& arg)
        {
            using type = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v < type, ElementType>)
            {
                oss << getTypeName(arg);
            }
            if constexpr (std::is_same_v < type, coded_index<TypeDefOrRef>>)
            {
                TypeDef t = find(arg);
                oss << std::string(t.TypeNamespace()) << "." << std::string(t.TypeName());
            }
            if constexpr (std::is_same_v < type, GenericTypeIndex>)
            {
                oss << "GENERIC_TYPE_INDEX ";
            }
            if constexpr (std::is_same_v < type, GenericTypeInstSig>)
            {
                auto tmp = arg.GenericType();
                if (tmp)
                {

                    auto x = find(tmp);
                    oss << x.TypeNamespace() << "." << x.TypeName();
                }
                if (arg.GenericArgCount() > 0)
                {
                    oss << "<";
                    std::vector<std::string> v;
                    for (auto ga : arg.GenericArgs())
                    {
                        v.push_back(get_type(ga));
                    }
                    oss << join(v, ",");
                    oss << ">";
                }

            }
            if constexpr (std::is_same_v < type, GenericMethodTypeIndex>)
            {
                oss << "GENERIC_METHOD_TYPE_INDEX ";
            }
        },
        ts);
    return oss.str();
}

inline void visit_elemSig(ElemSig& elemSig)
{
    std::visit([](auto&& av)
        {
            if constexpr (std::is_same_v< std::decay_t<decltype(av)>, ElemSig::SystemType>)
            {
                std::cout << av.name << ", ";
            }
            else if constexpr (std::is_same_v< std::decay_t<decltype(av)>, ElemSig::EnumValue>)
            {
                std::visit([](auto&& x)
                    {
                        std::cout << x << ", ";
                    }, av.value);
            }
            else
            {
                std::cout << av << ", ";
            }
        }, elemSig.value);
}


inline void visit_elemSig(ElemSig& elemSig, std::string& result)
{
    std::visit([&result](auto&& av)
        {
            if constexpr (std::is_same_v< std::decay_t<decltype(av)>, ElemSig::SystemType>)
            {
                result = av.name;
            }
            else if constexpr (std::is_same_v< std::decay_t<decltype(av)>, ElemSig::EnumValue>)
            {
                std::visit([&result](auto&& x)
                    {
                        std::ostringstream oss;
                        oss << x;
                        result = oss.str();
                    }, av.value);
            }
            /*
            else if constexpr (std::is_same_v< std::decay_t<decltype(av)>, unsigned int>)
            {
                result = "long";
            }
            */
            else
            {
                result = "";
            }
        }, elemSig.value);
}


inline void visit_elemSig(ElemSig& elemSig, std::vector<std::string>& result)
{
    std::visit([&result](auto&& av)
        {
            if constexpr (std::is_same_v< std::decay_t<decltype(av)>, ElemSig::SystemType>)
            {
                result.push_back(std::string(av.name));
            }
            else if constexpr (std::is_same_v< std::decay_t<decltype(av)>, ElemSig::EnumValue>)
            {
                std::visit([&result](auto&& x)
                    {
                        std::ostringstream oss;
                        oss << x;
                        result.push_back(oss.str());
                    }, av.value);
            }
            else
            {
                std::ostringstream oss;
                oss << av;
                result.push_back(oss.str());
            }
        }, elemSig.value);
}

inline void visit_fixArgSig(FixedArgSig fixedArgSig)
{
    if (fixedArgSig.value.index() == 0)
    {
        auto v = std::get<0>(fixedArgSig.value);
        visit_elemSig(v);
    }
    if (fixedArgSig.value.index() == 1)
    {
        auto vv = std::get<1>(fixedArgSig.value);
        if (!vv.empty())
        {
            visit_elemSig(vv[0]);
        }
    }
}


inline void visit_fixArgSig(FixedArgSig fixedArgSig, std::string& result)
{
    if (fixedArgSig.value.index() == 0)
    {
        auto v = std::get<0>(fixedArgSig.value);
        visit_elemSig(v, result);
    }
    if (fixedArgSig.value.index() == 1)
    {
        auto vv = std::get<1>(fixedArgSig.value);
        for (auto& v : vv)
        {
            visit_elemSig(v, result);
        }
    }
}

inline void visit_fixArgSig(FixedArgSig fixedArgSig, std::vector<std::string>& result)
{
    if (fixedArgSig.value.index() == 0)
    {
        auto v = std::get<0>(fixedArgSig.value);
        visit_elemSig(v, result);
    }
    if (fixedArgSig.value.index() == 1)
    {
        auto vv = std::get<1>(fixedArgSig.value);
        for (auto& v : vv)
        {
            visit_elemSig(v, result);
        }
    }
}

template<class T>
std::map<std::string, std::vector<std::vector<std::string>>> get_attrs(T& typeDef)
{
    std::map<std::string, std::vector<std::vector<std::string>>> result;

    auto attrs = typeDef.CustomAttribute();
    for (auto&& attr : attrs)
    {
        std::vector<std::string> v;

        if (attr.TypeNamespaceAndName().second == "OverloadAttribute")
        {
            int x = 0;
        }

        auto n = std::string(attr.TypeNamespaceAndName().second);
        auto value = attr.Value();
        std::vector<FixedArgSig> fixedArgs = value.FixedArgs();

        if (!fixedArgs.empty())
        {
            for (auto&& arg : fixedArgs)
            {
                visit_fixArgSig(arg, v);
            }
        }
        if (result.count(n) == 0)
        {
            result[n] = std::vector<std::vector<std::string>>{};
        }
        result[n].push_back(v);
    }
    return result;
}


inline std::string get_impl(const TypeDef& typeDef, const MethodDef& methodDef)
{
    std::string result;

    auto impls = methodDef.get_database().MethodImpl;
    for (auto& impl : impls)
    {

        // std::cout << "[" << impl.index() << "]";
        int bindex = 0;
        auto body = impl.MethodBody();
        if (body)
        {
            bindex = body.index();
            // if (bindex != typeDef.index()) continue;

            std::string tn;
            std::string ns;
            if (body.type() == MethodDefOrRef::MethodDef)
            {
                MethodDef m;
                m = body.MethodDef();
                tn = m.Parent().TypeName();
                ns = m.Parent().TypeNamespace();

                auto attrs = get_attrs(m);
                if (attrs.count("OverloadAttribute"))
                {
                    tn = attrs["OverloadAttribute"].front().front();
                }
            }
            else
            {
                auto ref = body.MemberRef();
                tn = ref.Class().TypeDef().TypeName();
                ns = ref.Class().TypeDef().TypeNamespace();

                auto td = ref.Class().TypeDef();
                auto attrs = get_attrs(td);
                if (attrs.count("OverloadAttribute"))
                {
                    tn = attrs["OverloadAttribute"].front().front();
                }
            }

            //std::cout << tn << " =?=" << typeDef.TypeName() << std::endl;
            if (ns != typeDef.TypeNamespace()) continue;
            if (tn != typeDef.TypeName()) continue;

        }
        //std::cout << "--------------------" << std::endl;
        auto decl = impl.MethodDeclaration();
        if (decl)
        {

            if (decl.type() == MethodDefOrRef::MemberRef)
            {
                auto ref = decl.MemberRef();
                if (ref)
                {
                    //std::cout << ref.Name() << " =?= " << methodDef.Name()  << std::endl;

                    if (ref.Name() != methodDef.Name()) continue;

                    //print_attrs(ref);

                    auto c = ref.Class();

                    if (c.type() == MemberRefParent::TypeRef)
                    {
                        auto r = c.TypeRef();
                        result = std::string(c.TypeRef().TypeNamespace()) + "." + std::string(c.TypeRef().TypeName());
                    }
                    else if (c.type() == MemberRefParent::TypeDef)
                    {
                        result = std::string(ref.Class().TypeDef().TypeNamespace()) + "." + std::string(ref.Class().TypeDef().TypeName());
                    }
                    else if (c.type() == MemberRefParent::MethodDef)
                    {
                        result = std::string(ref.Class().TypeDef().TypeNamespace()) + "." + std::string(ref.Class().TypeDef().TypeName());
                    }
                    else if (c.type() == MemberRefParent::ModuleRef)
                    {
                        result = std::string(ref.Class().TypeDef().TypeNamespace()) + "." + std::string(ref.Class().TypeDef().TypeName());
                    }
                    else if (c.type() == MemberRefParent::TypeSpec)
                    {
                        auto d = c.get_database().TypeSpec[c.index()];
                        auto gen = d.Signature().GenericTypeInst();
                        std::ostringstream oss;
                        oss << gen.GenericType().TypeRef().TypeNamespace() << "." << gen.GenericType().TypeRef().TypeName();
                        if (gen.GenericArgCount())
                        {
                            oss << "<";
                            std::vector<std::string> v;
                            for (auto arg : gen.GenericArgs())
                            {
                                v.push_back(get_type(arg));
                            }
                            oss << join(v, ",");
                        }
                        oss << ">";
                        result = oss.str();

                        //                       std::cout << ref.Class()..TypeRef().TypeName() << " : ";
                    }

                    // std::cout << ref.Name() << std::endl;
                    break;
                }
            }
            else
            {
                auto def = decl.MethodDef();
                if (def)
                {
                    if (def.Name() != methodDef.Name()) continue;
                    //print_attrs(def);
                    result = std::string(def.Parent().TypeNamespace()) + "." + std::string(def.Parent().TypeName());
                    //                    std::cout << def.Name() << ": " << def.Parent().TypeName() << std::endl;
                    break;
                }
            }
        }
        //        break;
    }
    return result;
}



inline bool is_iface(std::string s)
{
    if (s.empty()) return false;

    size_t pos = s.find(".");
    if (pos == std::string::npos) return false;

    pos = s.find("`");
    if (pos != std::string::npos) return true;

    auto typeDef = theCache.find(s);
    if (!typeDef) return false;

    auto cat = get_category(typeDef);
    if (cat == category::interface_type) return true;
    return false;
}

inline bool is_enum(std::string s)
{
    if (s.empty()) return false;

    size_t pos = s.find(".");
    if (pos == std::string::npos) return false;

    auto typeDef = theCache.find(s);
    if (!typeDef) return false;

    auto cat = get_category(typeDef);
    if (cat == category::enum_type) return true;
    return false;
}

inline bool is_class(std::string s)
{
    size_t pos = s.find(".");
    if (pos == std::string::npos) return false;

    auto typeDef = theCache.find(s);
    if (!typeDef) return false;

    auto cat = get_category(typeDef);
    if (cat == category::class_type) return true;
    return false;
}

inline bool is_delegate(std::string s)
{
    size_t pos = s.find(".");
    if (pos == std::string::npos) return false;

    pos = s.find("`", pos);
    if (pos != std::string::npos)
    {
        s = s.substr(0, pos + 2);
    }

    auto typeDef = theCache.find(s);
    if (!typeDef)
    {
        size_t pos = s.find("Handler");
        if (pos != std::string::npos) return true;
        return false;
    }

    auto cat = get_category(typeDef);
    if (cat == category::delegate_type) return true;

    pos = s.find("Handler");
    if (pos != std::string::npos) return true;

    return false;
}



inline std::string ungen_arg(std::string& arg, std::vector<std::string>& genArgs)
{
    if (isGenArg(arg))
    {
        arg = genArgs[atoi(arg.substr(1).c_str())];
        return arg;
    }
    size_t pos = arg.find("<");
    if (pos == std::string::npos) return arg;

    TypeDef typeDef = theCache.find(arg.substr(0, pos));
    if (!typeDef) return arg;

    int n = 0;
    for (auto&& genParam : typeDef.GenericParam())
    {
        std::string name("T");
        name += std::to_string(n);
        replace(arg, name, genArgs[n]);
        n++;
    }

    return arg;
}

inline std::vector<std::string> get_statics(TypeDef& typeDef)
{
    std::vector<std::string> result;
    auto attrs = typeDef.CustomAttribute();
    for (auto&& attr : attrs)
    {
        if (attr.TypeNamespaceAndName().second == "StaticAttribute")
        {
            auto value = attr.Value();
            std::vector<FixedArgSig> fixedArgs = value.FixedArgs();

            if (!fixedArgs.empty())
            {
                std::string s;
                for (auto&& arg : fixedArgs)
                {
                    visit_fixArgSig(arg, s);
                    break;
                }
                result.push_back(s);
            }
        }
    }
    return result;
}

inline std::vector<std::string> get_factories(TypeDef& typeDef)
{
    if (fullName(typeDef.TypeNamespace(), typeDef.TypeName()) == "Windows.Storage.Pickers.FileOpenPicker")// "Microsoft.UI.Dispatching.DispatcherExitDeferral")
    {
        int x = 1;
    }
    std::vector<std::string> result;
    auto attrs = typeDef.CustomAttribute();
    for (auto&& attr : attrs)
    {
        if (attr.TypeNamespaceAndName().second == "ActivatableAttribute")
        {
            std::string n(fullName(attr.TypeNamespaceAndName().first, attr.TypeNamespaceAndName().second));

            auto value = attr.Value();
            std::vector<FixedArgSig> fixedArgs = value.FixedArgs();

            if (!fixedArgs.empty())
            {
                std::string s;
                for (auto&& arg : fixedArgs)
                {
                    visit_fixArgSig(arg, s);
                    break;
                }
                if (s.empty() || s[0] == 0 || s == "\x1")
                {
                    result.push_back("");
                }
                else
                {
                    result.push_back(s);
                }
            }
        }
    }
    return result;
}


inline std::vector<std::string> split_generic(const std::string& gen)
{
    std::vector<std::string> result;

    auto wgen = to_wstring(gen);
    HSTRING hstr = nullptr;
    ::WindowsCreateString(wgen.c_str(), wgen.size(), &hstr);

    DWORD cnt = 0;
    HSTRING* parts = nullptr;
    HRESULT hr = ::RoParseTypeName(hstr, &cnt, &parts);
    if (hr != S_OK)
    {
        WindowsDeleteString(hstr);
        return result;
    }

    for (int i = 0; i < cnt; i++)
    {
        result.push_back(to_utf8(WindowsGetStringRawBuffer(parts[i], nullptr)));
    }

    for (int i = 0; i < cnt; i++)
    {
        ::WindowsDeleteString(parts[i]);
    }
    ::CoTaskMemFree(parts);
    WindowsDeleteString(hstr);
    return result;
}

inline std::string get_enum_base(TypeDef& typeDef)
{
    auto fields = typeDef.FieldList();
    for (auto&& field : fields)
    {
        if (field.Name() == "value__")
        {
            return getTypeName(field.Signature().Type().element_type());
        }
    }
    return "";
}

inline void import_namespace(const std::string& ns)
{
    auto nsr = nsRoot(ns);
    std::string file_path;
    if (nsr == "Windows")
    {
        std::filesystem::path winmd_dir = get_local_winmd_path();
        std::string s = winmd_dir.string() + "\\" + ns + ".winmd";
        file_path = find_winmd_path_for_ns(s);
    }
    else
    {
        CHAR NPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, NPath);
        std::string cwd = NPath;
        file_path = cwd + "\\" + ns + ".winmd";
    }

    //std::cout << "FP: " << file_path << std::endl;
    if (file_path.empty()) return;

    theCache.add_database(file_path);
}

inline void import_platform_metadata(const std::string& version)
{
    std::ostringstream oss;
    oss << "C:\\Program Files (x86)\\Windows Kits\\10\\UnionMetadata\\" << version << "\\Windows.winmd";

    auto winmd = oss.str();

    theCache.add_database(winmd);
}
