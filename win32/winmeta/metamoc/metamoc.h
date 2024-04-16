#pragma once
#include "types.h"



inline void make_promise_cb(async_mode asyncMode)
{
    if (asyncMode == async_mode::OPERATION || asyncMode == async_mode::OPERATION_WITH_PROGRESS)
    {
        std::cout << "        var _promise = Future();" << std::endl;
        std::cout << "        var _cb = fun(status, result)" << std::endl;
        std::cout << "        {" << std::endl;
        std::cout << "            if (status == Windows.Foundation.AsyncStatus.Completed)" << std::endl;
        std::cout << "            {" << std::endl;
        std::cout << "                _promise.resolve(result);" << std::endl;
        std::cout << "            } else {" << std::endl;
        std::cout << "                _promise.reject(status);" << std::endl;
        std::cout << "            }" << std::endl;
        std::cout << "        };" << std::endl;
    }
    else if (asyncMode == async_mode::ACTION || asyncMode == async_mode::ACTION_WITH_PROGRESS)
    {
        std::cout << "        var _promise = Future();" << std::endl;
        std::cout << "        var _cb = fun(status)" << std::endl;
        std::cout << "        {" << std::endl;
        std::cout << "            if (status == Windows.Foundation.AsyncStatus.Completed)" << std::endl;
        std::cout << "            {" << std::endl;
        std::cout << "                _promise.resolve();" << std::endl;
        std::cout << "            } else {" << std::endl;
        std::cout << "                _promise.reject(status);" << std::endl;
        std::cout << "            }" << std::endl;
        std::cout << "        };" << std::endl;
    }
}

void write_enum(TypeDef& typeDef)
{
    std::string ns(typeDef.TypeNamespace());

    std::cout << std::endl << ns << "." << typeDef.TypeName() << " = {" << std::endl << "    ";

    std::vector<std::string> entries;
    for (auto& field : typeDef.FieldList())
    {
        std::string n(field.Name());
        if (n == "value__") continue;

        std::string entry = "\"";
        entry += n + "\" : ";

        auto c = field.Constant();
        if (c)
        {
            std::visit([&entry](auto arg)
                {
                    using type = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<type, std::nullptr_t>)
                    {
                        entry += " nil";
                    }
                    else if constexpr (std::is_same_v<type, std::u16string_view>)
                    {
                        std::u16string_view v = arg;
                        std::wstring ws(v.begin(), v.end());
                        entry += to_utf8(ws);
                    }
                    else if constexpr (std::is_same_v<type, char16_t>)
                    {
                        entry += "unk";
                    }
                    else
                    {
                        entry += std::to_string(arg);
                    }
                }, c.Value());
        }
        else
        {
            entry = getTypeName(field.Signature().Type().element_type());
        }
        entries.push_back(entry);
    }
    std::cout << join(entries, ", \n    ");
    std::cout << std::endl << "};" << std::endl << std::endl;
}


void write_struct(TypeDef& typeDef)
{
    std::string ns(typeDef.TypeNamespace());

    std::cout << std::endl << ns << "." << typeDef.TypeName() << " = foreign.named_struct( \"";
    std::cout << ns << "." << typeDef.TypeName() << "\", [ " << std::endl << "    \"";

    std::vector<std::string> fields;
    for (auto& field : typeDef.FieldList())
    {
        std::string n(field.Name());
        // if (n == "value__") continue;

        std::string entry = n;
        entry += ":";

        auto c = field.Constant();
        std::string typeName;
        if (c)
        {
            std::visit([&typeName](auto arg)
                {
                    using type = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<type, std::nullptr_t>)
                    {
                        typeName = "nil";
                    }
                    else if constexpr (std::is_same_v<type, std::u16string_view>)
                    {
                        std::u16string_view v = arg;
                        std::wstring ws(v.begin(), v.end());
                        typeName = to_utf8(ws);
                    }
                    else if constexpr (std::is_same_v<type, char16_t>)
                    {
                        typeName = "unk";
                    }
                    else
                    {
                        typeName = std::to_string(arg);
                    }
                }, c.Value());
        }
        else
        {
            typeName = make_struct_type(field.Signature().Type().element_type());
        }
        entry += typeName;
        fields.push_back(entry);
    }
    std::cout << join(fields, "\", \r\n    \"");
    std::cout << "\"" << std::endl << "]); " << std::endl << std::endl;
}



inline void print_param(TypeSig& typeSig, std::pair<ParamAttributes, std::string>& param)
{
    auto ts = typeSig.Type();

    std::ostringstream oss;

    //    if (param.first.In()) std::cout << "[in] ";
      //  if (param.first.Out()) std::cout << "[out] ";

  //  if (typeSig.is_szarray()) std::cout << "array(";
    std::visit([&param, &oss](auto&& arg)
        {
            using typ = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<typ, ElementType>)
            {
                oss << ungen(getTypeName(arg));
            }
            if constexpr (std::is_same_v<typ, coded_index<TypeDefOrRef>>)
            {
                auto t = find(arg);
                if (t)
                {
                    std::string n(t.TypeName());
                    oss << t.TypeNamespace() << "." << ungen(n);
                }
                else
                {
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
                    oss << x.TypeNamespace() << "." << ungen(n);

                    auto cat = get_category(x);
                    if (cat == category::delegate_type)
                    {
                        isDelegate = true;
                    }
                }

                if (arg.GenericArgCount() > 0)
                {
                    std::string n = oss.str();
                    oss << "_";
                    bool is_instantiation = true;
                    generic_instance gi{ n };
                    std::vector<std::string> v;
                    for (auto ga : arg.GenericArgs())
                    {
                        auto t = print_param_type(ga, param);
                        if (t[0] == 'T' && (('0' <= t[1]) && (t[1] <= '9'))) is_instantiation = false;

                        v.push_back(t);
                        generic_instance g{ t };
                        gi.params.push_back(g);
                    }
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
                    oss << join(v, ",") << "_";
                }
            }
            if constexpr (std::is_same_v<typ, GenericMethodTypeIndex>)
            {
                std::cout << "genmethindex(" << arg.index << ")";
            }
        },
        ts);

  //  if (typeSig.is_szarray()) std::cout << ")";
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
    std::cout << oss.str();
}

inline void print_retType(RetTypeSig& retTypeSig)
{
    std::cout << "    //@Returns(\"";
    if (!retTypeSig)
    {
        std::cout << "void\")" << std::endl;;
        return;
    }

    auto t = retTypeSig.Type();

    ParamAttributes attr{};
    std::pair<ParamAttributes, std::string> p{ attr, "" };
    std::string s = print_param_type(t, p);
    std::cout << s;
    std::cout << "\")" << std::endl;

    //    std::cout << getTypeName(retTypeSig.Type().element_type()) << " ";

}


inline void print_param_types(MethodDefSig& methodDefSig, std::pair<Param, Param>& params)
{
    //  auto params = methodDefSig.Params();

    if (methodDefSig.Params().first == methodDefSig.Params().second) return;

    std::cout << "    //@Parameters(\"";

    std::vector<std::pair<ParamAttributes, std::string>> paramNames;
    std::vector<std::string> p;
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
        p.push_back(s);
        n++;
    }
    std::cout << join(p, "\",\"");
    std::cout << "\")" << std::endl;
}

inline void print_params(MethodDefSig& methodDefSig, std::pair<Param, Param>& params, bool cb = false)
{
    //  auto params = methodDefSig.Params();
    std::vector<std::pair<ParamAttributes, std::string>> paramNames;
    for (auto&& jt : params)
    {
        if (jt.Sequence() == 0) continue;
        std::string n(jt.Name());
        ParamAttributes flags = jt.Flags();
        paramNames.push_back({ flags,n });
    }
    int n = 0;
    std::vector<std::string> args;
    for (auto&& jt : methodDefSig.Params())
    {
        auto t = jt.Type();

        std::string s = paramNames[n].second;
        args.push_back(s);
        // std::cout << getTypeName(t.element_type()) << ", ";
        n++;
    }
    
    if (cb)
    {
        args.push_back("cb");
    }
    
    std::cout << join(args, ",");
}

inline void print_signature(const std::string_view& name, MethodDefSig& methodDefSig, std::pair<Param, Param>& params, bool cb = false)
{
    auto ret = methodDefSig.ReturnType();
    print_retType(ret);
    print_param_types(methodDefSig, params);
    std::cout << "    " << name << "(";
    print_params(methodDefSig, params, cb);
    std::cout << ")" << std::endl;
}

inline void print_ctor_signature(std::string resultType, std::string fullname, int vtable_start_index, const MethodDef& methodDef, MethodDefSig& methodDefSig, std::pair<Param, Param>& params)
{
    auto ret = methodDefSig.ReturnType();
    auto t = ret.Type();
    ParamAttributes attr{};
    std::pair<ParamAttributes, std::string> p{ attr, "" };
    std::string retType = print_param_type(t, p);
    auto asyncMode = is_async(retType);
    print_retType(ret);
    print_param_types(methodDefSig, params);

    std::vector<std::string> pnames;
    bool first = true;
    for (auto&& p : params)
    {
        if (first) { first = false; continue; }
        pnames.push_back(std::string(p.Name()));
    }

    std::string ctorName( methodDef.Name());
    if (ctorName == "Create")
    {
        std::ostringstream oss;
        oss << "Create_" << join(pnames, "_");
        ctorName = oss.str();
    }
    std::cout << "    static " << ctorName << "(";
    print_params(methodDefSig, params, asyncMode != async_mode::NONE);

    std::cout << ")" << std::endl;



    auto args = get_param_types(methodDefSig, params);



    if (asyncMode != async_mode::NONE) pnames.push_back("cb");

    std::string nsr = nsRoot(fullname);

    std::cout << "    {" << std::endl;
    std::cout << "        var " + nsr + " = runtime.global(\"" + nsr + "\");" << std::endl;
    std::cout << "        var factory = winrt.activationFactory( \"" << resultType << "\");" << std::endl;
    std::cout << "        var iface = " << fullname << "( factory );" << std::endl;
    std::cout << "        var result = iface." << methodDef.Name() << "( " << join(pnames, ", ") << " );" << std::endl;
    std::cout << "        iface.release();" << std::endl;
    std::cout << "        factory.release();" << std::endl;
    std::cout << "        return result;" << std::endl;
    std::cout << "    }" << std::endl;

}

inline void print_static_method(const TypeDef& typeDef, const MethodDef& methodDef, int vtable_start_index)
{
    //print_attrs(methodDef);
    // print_flags(methodDef);
 //    if(vtable_start_index!=-1)
   //      std::cout << "    @VtableIndex(" << vtable_start_index << ")" << std::endl;
    auto sig = methodDef.Signature();
    auto params = methodDef.ParamList();
    auto name = methodDef.Name();
    if (skip(sig, params))
    {
        return;
    }
    print_signature(name, sig, params);

    std::string impl = get_impl(typeDef, methodDef);

    std::cout << "    {" << std::endl;
    std::cout << "        // " << impl << std::endl;
    std::cout << "    }" << std::endl;
}


inline void print_class_method(std::string name, const TypeDef& typeDef, const MethodDef& methodDef, const std::string& defIface)
{
    std::string impl = get_impl(typeDef, methodDef);
    if (impl.empty()) return;

    if (is_generic(impl))
    {
        generic_instance gi = generic_instance::parse(impl);

        generic_instances.insert(gi);
    }
    bool isDefault = false;
    if (impl == defIface) isDefault = true;


    // print_flags(methodDef);
 //    if(vtable_start_index!=-1)
   //      std::cout << "    @VtableIndex(" << vtable_start_index << ")" << std::endl;
    auto sig = methodDef.Signature();
    auto params = methodDef.ParamList();
    // auto name = methodDef.Name();
    auto attrs = get_attrs(methodDef);
    if (attrs.count("OverloadAttribute"))
    {
        name = attrs["OverloadAttribute"].front().front();
    }
    if (skip(sig, params))
    {
        return;
    }

    auto paramTypes = get_param_types(sig, params);
    auto paramNames = get_param_names(sig, params);

   // print_attrs(methodDef);
    if (!paramNames.empty())
    {
        std::cout << "    //@Parameters(\"" << join(paramTypes, "\", \"") << "\")" << std::endl;
    }

    bool isVoid = false;
    if (!sig.ReturnType()) isVoid = true;

    std::string retType = get_retType(methodDef);
    async_mode asyncMode = is_async(retType);
    if (asyncMode != async_mode::NONE) isVoid = true;

    print_signature(name, sig, params, false);// asyncMode != async_mode::NONE);

    std::string ns{ typeDef.TypeNamespace() };
    auto nsr = nsRoot(ns);

    std::cout << "    {" << std::endl;
    std::cout << "        var " + nsr + " = runtime.global(\"" + nsr + "\");" << std::endl;
    std::cout << "        var iface = " << ungen(impl) << "( this.comPtr );" << std::endl;
    make_promise_cb(asyncMode);
    if (isVoid)
    {
        std::cout << "        ";
    }
    else
    {
        std::cout << "        var _r = ";
    }
    std::cout << "iface." << name << "( " << join(paramNames, ", ");
    if (asyncMode != async_mode::NONE)
    {
        if (!paramNames.empty()) std::cout << ", ";
        std::cout << "_cb ";
    }
    
    std::cout << " );" << std::endl;
    std::cout << "        iface.release();" << std::endl;
    if (asyncMode == async_mode::NONE)
    {
        if (!isVoid)
        {
            std::cout << "        return _r;" << std::endl;
        }
    }
    else
    {
        std::cout << "        return _promise;" << std::endl;
    }
    
    std::cout << "    }" << std::endl << std::endl;
}


inline void print_static_ctor(std::string typeName, const TypeDef& typeDef, const MethodDef& methodDef, int vtable_start_index)
{
   // print_attrs(methodDef);
    // print_flags(methodDef);
 //    if(vtable_start_index!=-1)
   //      std::cout << "    @VtableIndex(" << vtable_start_index << ")" << std::endl;

    std::string fullname = fullName(typeDef.TypeNamespace(), typeDef.TypeName());
    std::string flatname = flatten(ungen(fullname));
    auto sig = methodDef.Signature();
    auto params = methodDef.ParamList();
    auto name = methodDef.Name();
    print_ctor_signature(typeName, fullname, vtable_start_index, methodDef, sig, params);

    //std::string impl = get_impl(typeDef, methodDef);

}


inline void print_iface_method(std::string fullname, const MethodDef& methodDef, int vtable_start_index)
{
    auto sig = methodDef.Signature();
    auto params = methodDef.ParamList();
    auto name = methodDef.Name();

    if (skip(sig, params)) return;


    if (name == "add_Closed")
    {
        int x = 1;
    }
    auto attrs = get_attrs(methodDef);
    if (attrs.count("OverloadAttribute"))
    {
        name = attrs["OverloadAttribute"].front().front();
    }

    // print_flags(methodDef);
    if (vtable_start_index != -1)
        std::cout << "    //@VtableIndex(" << vtable_start_index << ")" << std::endl;

    std::string retType = "void";
    auto rt = methodDef.Signature().ReturnType();
    if (rt)
    {
        auto retTypeSig = rt.Type();
        ParamAttributes attr{};
        std::pair<ParamAttributes, std::string> p{ attr, "" };
        retType = print_param_type(retTypeSig, p);
    }
    async_mode asyncMode = is_async(retType);
    std::cout << "    //@Async(" << (int)asyncMode << ")" << std::endl;

    print_signature(name, sig, params, asyncMode != async_mode::NONE);
    std::cout << "    {" << std::endl;

    std::string nsr = nsRoot(fullname);
    std::cout << "        var " + nsr + " = runtime.global(\"" + nsr + "\");" << std::endl;

    std::vector<std::string> paramTypes = get_param_types(sig, params);
    int n = 1;
    auto paramNames = get_param_names(sig, params);
    std::vector<std::string> pnames;
    for (auto& paramType : paramTypes)
    {
        std::string pname = "p";
        pname += std::to_string(n);
        pnames.push_back(pname);

        if (is_delegate(paramType))
        {
            std::cout << "        var p" << n << " = nil; " << std::endl;
            std::cout << "        if( typeof(" << paramNames[n - 1] << ") == \"comptr\" ) { " << std::endl;
            std::cout << "            p" << n << " = " << paramNames[n - 1] << ";" << std::endl;
            std::cout << "        } else {" << std::endl;
            std::cout << "            p" << n << " = " << ungen(paramType) << ".Create( " << paramNames[n - 1] << "); " << std::endl;
            std::cout << "        }" << std::endl;
        }
        else if (is_enum(paramType))
        {
            std::cout << "        var p" << n << " = " << paramNames[n - 1] << ";" << std::endl;
        }
        else if (is_iface(paramType) || is_class(paramType))
        {
            std::cout << "        var p" << n << " = " << ungen(paramType) << "( " << paramNames[n - 1] << "); " << std::endl;
        }
        else
        {
            std::cout << "        var p" << n << " = " << paramNames[n - 1] << ";" << std::endl;
        }
        n++;
    }
    if (retType != "void" && asyncMode == async_mode::NONE)
    {
        if (is_enum(retType))
        {
            std::cout << "        var _r = ";
        }
        else if (is_iface(retType) || is_class(retType))
        {
            std::cout << "        var _r = " << ungen(retType) << "(" << std::endl << "            ";
        }
        else
        {
            std::cout << "        var _r = ";
        }
    }
    else
    {
        std::cout << "        ";
    }
    std::cout << "this.comPtr." << asyncModeOperation[asyncMode];
    std::cout << "( " << vtable_start_index;

    std::string asyncReturnType = asyncMode == async_mode::OPERATION || asyncMode == async_mode::OPERATION_WITH_PROGRESS ? get_first_gen_arg(retType) : "";

    if (asyncMode == async_mode::NONE || asyncMode == async_mode::ACTION)
    {
        std::string rtype = retType;

        size_t pos = rtype.find(".");
        if (pos != std::string::npos)
        {
            auto td = theCache.find(rtype);
            if (td)
            {
                auto cat = get_category(td);
                if (cat == category::enum_type)
                {
                    rtype = "UInt32";
                }
            }
        }
        std::cout << ", \"" << rtype << "\", ";
    }
    else
    {
        std::string rtype = asyncReturnType;
        if (is_enum(rtype))
        {
            rtype = "UInt32";
        }
        std::cout << ", \"" << rtype << "\", ";
    }

    if (asyncMode != async_mode::NONE)
    {
        std::cout << ungen(retType) << ".handler_iid, ";
    }

    if (paramTypes.empty())
    {
        std::cout << "[], ";
    }
    else
    {
        std::vector<std::string> pt;
        for (auto& p : paramTypes)
        {
            if (is_enum(p))
            {
                pt.push_back("UInt32");
            }
            else
            {
                pt.push_back(p);
            }
        }
        std::cout << "[\"" << join(pt, "\", \"") << "\"], ";
    }
    std::cout << "[" << join(pnames, ", ") << "] ";
    if (asyncMode != async_mode::NONE)
    {
        //if (!pnames.empty()) 
        std::cout << ", ";

        if (is_iface(asyncReturnType) || is_class(asyncReturnType))
        {
            std::cout << "fun(status,value) { if(status == Windows.Foundation.AsyncStatus.Completed ) {";
            std::cout << "cb(status, ";
            std::cout << ungen(asyncReturnType);
            std::cout << "(value)); ";
            std::cout << "} else { cb( status, value ); } ";
            std::cout << "} ";
        }
        else
        {
            std::cout << "cb ";
        }
    }

    if ((is_iface(retType) || is_class(retType)) && asyncMode == async_mode::NONE)
    {
        std::cout << ")" << std::endl << "        );" << std::endl;
    }
    else
    {
        std::cout << ");" << std::endl;
    }
    if (retType != "void" && asyncMode == async_mode::NONE)
    {
        std::cout << "        return _r;" << std::endl;
    }
    std::cout << "    }" << std::endl;
    std::cout << std::endl;

}


inline void print_gen_iface_method(std::string fullname, const MethodDef& methodDef, int vtable_start_index, std::vector<std::string> genArgs)
{
    auto sig = methodDef.Signature();
    auto params = methodDef.ParamList();
    auto name = methodDef.Name();

    if (skip(sig, params)) return;

//    print_attrs(methodDef);
    auto attrs = get_attrs(methodDef);
    if (attrs.count("OverloadAttribute"))
    {
        name = attrs["OverloadAttribute"].front().front();
    }
    // print_flags(methodDef);
    if (vtable_start_index != -1)
        std::cout << "    //@VtableIndex(" << vtable_start_index << ")" << std::endl;

    std::string retType = "void";
    auto rt = methodDef.Signature().ReturnType();
    if (rt)
    {
        auto retTypeSig = rt.Type();

        ParamAttributes attr{};
        std::pair<ParamAttributes, std::string> p{ attr, "" };
        retType = print_param_type(retTypeSig, p);
        retType = ungen_arg(retType, genArgs);
    }
    async_mode asyncMode = is_async(retType);
    print_signature(name, sig, params, asyncMode != async_mode::NONE);
    std::cout << "    {" << std::endl;

    std::string nsr = nsRoot(fullname);
    std::cout << "        var " + nsr + " = runtime.global(\"" + nsr + "\");" << std::endl;

    std::vector<std::string> paramTypes = get_param_types(sig, params);
    int n = 1;
    auto paramNames = get_param_names(sig, params);
    std::vector<std::string> pnames;
    for (auto& paramType : paramTypes)
    {
        std::string pname = "p";
        pname += std::to_string(n);
        pnames.push_back(pname);
        paramType = ungen_arg(paramType, genArgs);
        if (paramType == "Windows.Foundation.Collections.MapChangedEventHandler`2<String,Object>")
        {
            int x = 1;
        }
        if (is_delegate(paramType))
        {
            std::cout << "        var p" << n << " = nil; " << std::endl;
            std::cout << "        if( typeof(" << paramNames[n - 1] << ") == \"comptr\" ) { " << std::endl;
            std::cout << "            p" << n << " = " << paramNames[n - 1] << ";" << std::endl;
            std::cout << "        } else {" << std::endl;
            std::cout << "            p" << n << " = " << ungen(paramType) << ".Create( " << paramNames[n - 1] << "); " << std::endl;
            std::cout << "        }" << std::endl;
        }
        else if (is_iface(paramType) || is_class(paramType))
        {
            std::cout << "        var p" << n << " = " << ungen(paramType) << "( " << paramNames[n - 1] << "); " << std::endl;
        }
        else
        {
            std::cout << "        var p" << n << " = " << paramNames[n - 1] << ";" << std::endl;
        }
        n++;
    }
    if (retType != "void" && asyncMode == async_mode::NONE)
    {
        if (is_iface(retType) || is_class(retType))
        {
            std::cout << "        var _r = " << ungen(retType) << "(" << std::endl << "            ";
        }
        else
        {
            std::cout << "        var _r = ";
        }
    }
    else
    {
        std::cout << "        ";
    }
    std::cout << "this.comPtr." << asyncModeOperation[asyncMode];
    std::cout << "(" << vtable_start_index;
    if (asyncMode == async_mode::NONE || asyncMode == async_mode::ACTION)
    {
        if (is_enum(retType))
        {
            std::cout << ", \"UInt32\", ";
        }
        else
        {
            std::cout << ", \"" << retType << "\", ";
        }
    }
    else
    {
        std::cout << ", \"" << get_first_gen_arg(retType) << "\", ";
    }
    if (asyncMode != async_mode::NONE)
    {
        std::cout << ungen(retType) << ".handler_iid, " << std::endl;
    }

    if (paramTypes.empty())
    {
        std::cout << "[], ";
    }
    else
    {
        std::cout << "[\"" << join(paramTypes, "\", \"") << "\"], ";
    }
    std::cout << "[" << join(pnames, ", ") << "] ";
    if (asyncMode != async_mode::NONE)
    {
        if (!pnames.empty()) std::cout << ", ";
        std::cout << "cb ";
    }
    std::cout << ")";

    if (is_iface(retType) || is_class(retType))
    {
        std::cout << std::endl << "        )";
    }
    std::cout << ";" << std::endl;
    if (retType != "void" && asyncMode == async_mode::NONE)
    {
        std::cout << "        return _r;" << std::endl;
    }
    std::cout << "    }" << std::endl << std::endl;
}



inline void write_iface_methods(TypeDef& typeDef, int vtable_start_index)
{
    auto impls = typeDef.get_database().MethodImpl;

    std::string fullname = fullName(typeDef.TypeNamespace(), typeDef.TypeName());

    int n = 0;
    for (auto it : typeDef.MethodList())
    {
        print_iface_method(fullname, it, vtable_start_index + n);
        if (vtable_start_index != -1)
            n++;
    }
}

inline void write_gen_iface_methods(TypeDef& typeDef, int vtable_start_index, std::vector<std::string> genArgs)
{
    auto impls = typeDef.get_database().MethodImpl;

    auto fullname = fullName(typeDef.TypeNamespace(), typeDef.TypeName());
    int n = 0;
    for (auto it : typeDef.MethodList())
    {
        print_gen_iface_method(fullname, it, vtable_start_index + n, genArgs);
        if (vtable_start_index != -1)
            n++;
    }
}


void write_iface(TypeDef& typeDef)
{
    int genArgs = gen_args(typeDef);
    if (genArgs) return;


    int vtable_start_index = 6;
    std::string fullname = fullName(typeDef.TypeNamespace(), typeDef.TypeName());
    std::string flatname = flatten(ungen(fullname));

    if (typesSeen.count(fullname)) return;
    typesSeen.insert(fullname);

    if (fullname == "Windows.Foundation.IAsyncOperation`1")
    {
        int x = 1;
    }

    std::cout << "//@Interface(\"" << fullname << "\")" << std::endl;

    GUID guid = get_guid(typeDef);
    std::cout << "//@IID(\"" << guid_2_string(guid) << "\")" << std::endl;

    std::vector<std::string> gargs;
    if (genArgs)
    {
        std::string t = "T";
        for (int i = 0; i < genArgs; i++)
        {
            std::string n = t + std::to_string(i);
            gargs.push_back(n);
        }
        std::cout << "//@GenerigArgs(\"" << join(gargs, "\",\"") << "\")" << std::endl;
    }

    std::string base = "";
    auto ext = typeDef.Extends();
    if (ext)
    {
        auto extends = find(ext);
        if (extends)
        {
            base = fullName(extends.TypeNamespace(), extends.TypeName());
        }
    }

    if (!base.empty())
    {
        std::cout << "//@Extends(\"" << base << "\")" << std::endl;
    }

    std::string nsr = nsRoot(fullname);

    std::cout << "class " << flatname << " : WinRtInterface " << std::endl << "{" << std::endl;

    std::cout << "    " << flatname << "( comPtr )" << std::endl;
    std::cout << "    {" << std::endl;
    std::cout << "        var " + nsr + " = runtime.global(\"" + nsr + "\");" << std::endl;
    std::cout << "        this.comPtr = comPtr.queryInterface( \"" << guid_2_string(guid) << "\");" << std::endl;
    std::cout << "    }" << std::endl << std::endl;

    write_iface_methods(typeDef, vtable_start_index);
    std::cout << std::endl;
    std::cout << "}" << std::endl;
    std::cout << flatname << ".iid = \"" << guid_2_string(guid) << "\";" << std::endl;

    if (fullname.starts_with("Windows.Foundation.IAsyncAction"))
    {
        GUID handler_guid = get_handler_guid(fullname);
        std::cout << flatname << ".handler_iid = \"" << guid_2_string(handler_guid) << "\";" << std::endl << std::endl;
        //return;
    }
    std::cout << ungen(fullname) << " = " << flatname << ";" << std::endl;

    std::cout << std::endl;
    std::cout << std::endl;

}


inline void write_delegate(TypeDef& typeDef)
{
    int genArgs = gen_args(typeDef);
    if (genArgs) return;

    int vtable_start_index = 4;
    std::string fullname = fullName(typeDef.TypeNamespace(), typeDef.TypeName());
    std::string flatname = flatten(ungen(fullname));
    //GUID handler_guid = get_handler_guid(fullname);
    if (fullname == "Windows.Foundation.IAsyncOperation`1")
    {
        int x = 1;
    }

    if (typesSeen.count(fullname)) return;
    typesSeen.insert(fullname);

    std::cout << "//@Delegate(\"" << fullname << "\")" << std::endl;

    GUID guid = get_guid(typeDef);
    std::cout << "//@IID(\"" << guid_2_string(guid) << "\")" << std::endl;

    std::vector<std::string> gargs;
    if (genArgs)
    {
        std::string t = "T";
        for (int i = 0; i < genArgs; i++)
        {
            std::string n = t + std::to_string(i);
            gargs.push_back(n);
        }
        std::cout << "//@GenerigArgs(\"" << join(gargs, "\",\"") << "\")" << std::endl;
    }

    std::string base = "";
    auto ext = typeDef.Extends();
    if (ext)
    {
        auto extends = find(ext);
        if (extends)
        {
            base = fullName(extends.TypeNamespace(), extends.TypeName());
        }
    }

    if (!base.empty())
    {
        std::cout << "//@Extends(\"" << base << "\")" << std::endl;
    }
    std::cout << "class " << flatname << std::endl << "{" << std::endl;

    for (auto m : typeDef.MethodList())
    {
        std::string n(m.Name());
        if (n == ".ctor") continue;
        auto sig = m.Signature();
        auto params = m.ParamList();
        auto paramTypes = get_param_types(sig, params);
        std::cout << "    static Create( cb ) " << std::endl;
        std::cout << "    {" << std::endl;

        if (paramTypes.size() == 2 && (is_iface(paramTypes[1]) || is_class(paramTypes[1])))
        {
            std::cout << "        var callback = fun( s, arg ) {" << std::endl;
            std::cout << "            cb( s, " << ungen(paramTypes[1]) << "(arg) );" << std::endl;
            std::cout << "        };" << std::endl;
        }
        else if (paramTypes.size() == 1 && (is_iface(paramTypes[0]) || is_class(paramTypes[0])))
        {
            std::cout << "        var callback = fun( arg ) {" << std::endl;
            std::cout << "            cb( " << ungen(paramTypes[0]) << "(arg) );" << std::endl;
            std::cout << "        };" << std::endl;
        }
        else
        {
            std::cout << "        var callback = cb;" << std::endl;
        }

        std::cout << "        return winrt.Delegate(";
        if (paramTypes.empty())
        {
            std::cout << "[], ";
        }
        else
        {
            std::cout << "[\"" << join(paramTypes, "\", \"") << "\"], ";
        }
        std::cout << "\"" << guid_2_string(guid) << "\", callback );" << std::endl;
        std::cout << "    }" << std::endl;
        std::cout << "}" << std::endl;
    }
    /*
        std::cout << "    " << flatname << "( comPtr )" << std::endl;
        std::cout << "    {" << std::endl;
        std::cout << "        this.comPtr = comPtr.queryInterface(compPtr, \"" << guid_2_string(guid) << "\");" << std::endl;
        std::cout << "    }" << std::endl << std::endl;
    */
    // write_iface_methods(typeDef, vtable_start_index);

 //    std::cout << "}" << std::endl;
    std::cout << flatname << ".iid = \"" << guid_2_string(guid) << "\";" << std::endl;
    std::cout << ungen(fullname) << " = " << flatname << ";" << std::endl;

    if (fullname.starts_with("Windows.Foundation.IAsync"))
    {
        GUID handler_guid = get_handler_guid(fullname);
        std::cout << flatname << ".handler_iid = \"" << guid_2_string(handler_guid) << "\";" << std::endl << std::endl;
        return;
    }

    std::cout << std::endl;
    std::cout << std::endl;

}

void write_gen_iface(TypeDef& typeDef, GUID guid, std::vector<std::string> args);

inline void write_generic_delegate(TypeDef& typeDef, const std::string& guid, std::vector<std::string>& args)
{
    int genArgs = gen_args(typeDef);
    if (!genArgs) return;

    std::vector<std::string> gargs;
    if (genArgs)
    {
        std::string t = "T";
        for (int i = 0; i < genArgs; i++)
        {
            std::string n = t + std::to_string(i);
            gargs.push_back(n);
        }
        //std::cout << "//@GenerigArgs(\"" << join(gargs, "\",\"") << "\")" << std::endl;
    }

    int vtable_start_index = 4;
    std::string fullname = fullName(typeDef.TypeNamespace(), typeDef.TypeName());
    std::string shortname = fullname;
    fullname += "<";
    fullname += join(args, ",");
    fullname += ">";

    if (typesSeen.count(fullname)) return;
    typesSeen.insert(fullname);

    std::string flatname = flatten(ungen(fullname));

    if (fullname == "Windows.Foundation.IAsyncOperation`1")
    {
        int x = 1;
    }

    std::cout << "//@Delegate(\"" << fullname << "\")" << std::endl;

    std::cout << "//@IID(\"" << guid << "\")" << std::endl;



    std::string base = "";
    auto ext = typeDef.Extends();
    if (ext)
    {
        auto extends = find(ext);
        if (extends)
        {
            base = fullName(extends.TypeNamespace(), extends.TypeName());
        }
    }

    if (!base.empty())
    {
        std::cout << "//@Extends(\"" << base << "\")" << std::endl;
    }
    std::cout << "class " << flatname << "{" << std::endl;

    std::string argType;
    for (auto m : typeDef.MethodList())
    {
        std::string n(m.Name());
        if (n == ".ctor") continue;
        auto sig = m.Signature();
        auto params = m.ParamList();
        auto paramTypes = get_param_types(sig, params);
        argType = paramTypes[1];

        std::vector<std::string> v;
        for (size_t i = 0; i < paramTypes.size(); i++)
        {
            auto it = paramTypes[i];
            if (it.size() > 2 && it[0] != 'T')
            {
                auto s = it;
                for (int j = 0; j < args.size(); j++)
                {
                    std::string k = "T";
                    k += std::to_string(j);
                    replace(s, k, args[j]);
                }
                v.push_back(s);
            }
            else
            {
                if (it.size() >= 2)
                {
                    int i = atoi(it.substr(1).c_str());

                    if (i < args.size())
                    {
                        v.push_back(args[i]);
                    }
                    else
                    {
                        v.push_back("Object");
                    }
                }
                else
                {
                    v.push_back("Object");
                }
            }
        }

        std::cout << "    static Create( cb ) " << std::endl;
        std::cout << "    {" << std::endl;
        if (v.size() == 2 && ( is_iface(v[1]) || is_class(v[1]) ) )
        {
            std::cout << "        var callback = fun( s, arg ) {" << std::endl;
            std::cout << "            cb( s, " << ungen(v[1]) << "(arg) );" << std::endl;
            std::cout << "        };" << std::endl;
        }
        else if (v.size() == 1 && (is_iface(v[0]) || is_class(v[0])))
        {
            std::cout << "        var callback = fun( arg ) {" << std::endl;
            std::cout << "            cb( " << ungen(v[0]) << "(arg) );" << std::endl;
            std::cout << "        };" << std::endl;
        }
        else
        {
            std::cout << "        var callback = cb;" << std::endl;
        }
        std::cout << "        return winrt.Delegate(";
        if (paramTypes.empty())
        {
            std::cout << "[], ";
        }
        else
        {
            std::cout << "[\"" << join(v, "\", \"") << "\"], ";
        }
        std::cout << "\"" << guid << "\", callback );" << std::endl;
        std::cout << "    }" << std::endl;

    }
    std::cout << "}" << std::endl;

    /*
        std::cout << "    " << flatname << "( comPtr )" << std::endl;
        std::cout << "    {" << std::endl;
        std::cout << "        this.comPtr = comPtr.queryInterface(compPtr, \"" << guid_2_string(guid) << "\");" << std::endl;
        std::cout << "    }" << std::endl << std::endl;

        var uri = Windows.Foundation.Uri.CreateUri("http://oha7.org/");
print uri;
print uri.get_DisplayUri();
print uri.get_SchemeName();

    */
    // write_iface_methods(typeDef, vtable_start_index);

 //    std::cout << "}" << std::endl;
    std::cout << flatname << ".iid = \"" << guid << "\";" << std::endl;
    std::cout << ungen(fullname) << " = " << flatname << ";" << std::endl;

    if (fullname.starts_with("Windows.Foundation.IAsync"))
    {
        GUID handler_guid = get_handler_guid(shortname);
        std::cout << flatname << ".handler_iid = \"" << guid_2_string(handler_guid) << "\";" << std::endl << std::endl;
        return;
    }

    std::cout << std::endl;
    std::cout << std::endl;

    size_t pos = argType.find("`");
    if (pos != std::string::npos)
    {
        auto t = argType.substr(0, pos + 2);
        auto typeDef = theCache.find(t);
        if (typeDef)
        {
            std::string gt = t;
            gt += "<";
            gt += args[0];
            gt += ">";
            GUID guid = get_generic(gt);
            std::vector<std::string> v{ args[0] };

            write_gen_iface(typeDef, guid, v);
        }
    }
}

inline void write_gen_iface(TypeDef& typeDef, GUID guid, std::vector<std::string> args)
{
    int vtable_start_index = 6;
    std::string fullname = fullName(typeDef.TypeNamespace(), typeDef.TypeName());
    std::string genType = fullname;
    fullname += "<";
    fullname += join(args, ",");
    fullname += ">";
    std::string flatname = flatten(ungen(fullname));

    if (typesSeen.count(fullname)) return;
    typesSeen.insert(fullname);

    std::cout << "//@Interface(\"" << fullname << "\")" << std::endl;

    std::cout << "//@IID(\"" << guid_2_string(guid) << "\")" << std::endl;

    std::string base = "";
    auto ext = typeDef.Extends();
    if (ext)
    {
        auto extends = find(ext);
        if (extends)
        {
            base = fullName(extends.TypeNamespace(), extends.TypeName());
        }
    }

    if (!base.empty())
    {
        std::cout << "//@Extends(\"" << base << "\")" << std::endl;
    }
    std::cout << "class " << flatname;

    if (genType.starts_with("Windows.Foundation.IAsyncAction") || genType.starts_with("Windows.Foundation.IAsyncOperation"))
    {
        std::cout << " : WinRtInterface {}" << std::endl;
        std::cout << flatname << ".iid = \"" << guid_2_string(guid) << "\";" << std::endl;
        std::string arg = get_first_gen_arg(fullname);
        std::string handler = get_callback(genType, fullname);
        /*
        std::string handler = "Windows.Foundation.AsyncOperationCompletedHandler`1<";
        handler += arg + ">";
        */
        GUID hg = get_generic(handler);
        //GUID handler_guid = get_generic_handler_guid(fullname);
        std::cout << flatname << ".handler_iid = \"" << guid_2_string(hg) << "\";" << std::endl << std::endl;
        std::cout << ungen(genType) << "_" << flatten(ungen(join(args, ",")));
        std::cout << "_ = " << flatname << ";" << std::endl;
        return;
    }

    std::cout << " : WinRtInterface " << std::endl << "{" << std::endl;
    std::cout << "    " << flatname << "( comPtr )" << std::endl;
    std::cout << "    {" << std::endl;
    std::cout << "        this.comPtr = comPtr.queryInterface( \"" << guid_2_string(guid) << "\");" << std::endl;
    std::cout << "    }" << std::endl << std::endl;

    write_gen_iface_methods(typeDef, vtable_start_index, args);
    std::cout << std::endl;

    std::cout << "}" << std::endl;

    std::cout << flatname << ".iid = \"" << guid_2_string(guid) << "\";" << std::endl;
    std::cout << ungen(genType) << "_" << flatten(ungen(join(args, ","))) << "_";

    std::cout << " = " << flatname << ";" << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;

}


inline void write_static_method(const TypeDef& classTypeDef, const std::string& fullname, const std::string& staticIface, const MethodDef& methodDef)
{
    std::string retType = "void";
    auto rt = methodDef.Signature().ReturnType();
    if (rt)
    {
        auto retTypeSig = rt.Type();

        ParamAttributes attr{};
        std::pair<ParamAttributes, std::string> p{ attr, "" };
        retType = print_param_type(retTypeSig, p);
        //retType = ungen_arg(retType, genArgs);
    }
    auto sig = methodDef.Signature();
    auto params = methodDef.ParamList();
    if (skip(sig, params))
    {
        return;
    }
    async_mode asyncMode = is_async(retType);
    std::cout << "    //@Async(" << (int)asyncMode << ")" << std::endl;
    std::cout << "    //@Returns(\"" << retType << "\")" << std::endl;

    print_param_types(sig, params);
    auto paramNames = get_param_names(sig, params);
    std::string name(methodDef.Name());
    auto attrs = get_attrs(methodDef);
    if (attrs.count("OverloadAttribute"))
    {
        name = attrs["OverloadAttribute"].front().front();
    }

    std::cout << "    static " << name << "( " << join(paramNames, ", ");
    /*
    if (asyncMode != async_mode::NONE)
    {
        if (!paramNames.empty()) std::cout << ", ";
        std::cout << "cb ";
    }
    */
    std::cout << ")" << std::endl;
    std::cout << "    {" << std::endl;

    std::string nsr = nsRoot(fullname);

    std::cout << "        var " + nsr + " = runtime.global(\"" + nsr + "\");" << std::endl;
    std::cout << "        var factory = winrt.activationFactory( \"" << fullname << "\" );" << std::endl;
    std::cout << "        var iface = " << ungen(staticIface) << "( factory );" << std::endl;

    make_promise_cb(asyncMode);

    if (retType == "void" || asyncMode != async_mode::NONE)
    {
        std::cout << "        ";
    }
    else
    {
        std::cout << "        var _r = ";
    }
    std::cout << "iface." << name << "( " << join(paramNames, ", ");
    if (asyncMode != async_mode::NONE)
    {
        if (!paramNames.empty()) std::cout << ", ";
        std::cout << "_cb";
    }
    std::cout << ");" << std::endl;
    std::cout << "        iface.release();" << std::endl;
    std::cout << "        factory.release();" << std::endl;
    if (retType != "void" && asyncMode == async_mode::NONE)
    {
        std::cout << "        return _r;" << std::endl;
    }
    else if (retType != "void" && asyncMode != async_mode::NONE)
    {
        std::cout << "        return _promise;" << std::endl;
    }
    std::cout << "    }" << std::endl << std::endl;
}

inline void write_static_methods(const TypeDef& classTypeDef, const std::string& fullname, const std::string& staticIface)
{

    auto staticTypeDef = theCache.find(staticIface);
    if (!staticTypeDef) return;

    for (auto m : staticTypeDef.MethodList())
    {
        write_static_method(classTypeDef, fullname, staticIface, m);
        std::cout << std::endl;
    }

}

inline void write_class_hierarchy(std::set<std::string>& seen, TypeDef& typeDef, std::string defIface)
{
    for (auto methodDef : typeDef.MethodList())
    {
        std::string name(methodDef.Name());
        while (seen.count(name))
        {
            name.append("_");
        }

        seen.insert(name);

        print_class_method(name, typeDef, methodDef, defIface);
    }
    std::cout << std::endl;

    auto ext = typeDef.Extends();
    if (ext)
    {
        auto extends = find(ext);
        if (extends)
        {
            std::cout << "// ext: " << fullName(extends.TypeNamespace(), extends.TypeName()) << std::endl;
            write_class_hierarchy(seen, extends, defIface);
        }
    }
    std::cout << std::endl;
}

inline void write_class(TypeDef& typeDef)
{
    std::string fullname = fullName(typeDef.TypeNamespace(), typeDef.TypeName());
    std::string flatname = flatten(ungen(fullname));

    if (typesSeen.count(fullname)) return;
    typesSeen.insert(fullname);

    std::cout << "//@RuntimeClass(\"" << fullname << "\")" << std::endl;

    auto attr = get_attrs(typeDef);

//    print_attrs(typeDef);

    if (attr.count("ContentPropertyAttribute"))
    {
        auto v = attr["ContentPropertyAttribute"];
        for (auto& i : v)
        {
            for (auto& ii : i)
            {
                std::cout << "@ContentProperty(\"" << ii << "\")" << std::endl;

            }
        }
    }

    std::cout << "//@Implements(\"";
    std::vector<std::string> ifaces;
    std::string defIface;

    for (auto impl : typeDef.InterfaceImpl())
    {
        if (impl)
        {
            //print_attrs(impl);
            if (impl.Interface())
            {
                if (impl.Interface().type() == TypeDefOrRef::TypeRef || impl.Interface().type() == TypeDefOrRef::TypeDef)
                {
                    auto iface = find(impl.Interface());
                    if (iface)
                    {
                        std::string fn = fullName(iface.TypeNamespace(), iface.TypeName());
                        bool def = is_default(impl);
                        if (def) defIface = fn;
                        ifaces.push_back(fn);
                    }
                    else
                    {
                        ifaces.push_back("+++Unknown+++");
                    }
                }
            }
        }
    }
    std::cout << join(ifaces, "\",\"") << "\")" << std::endl;
    std::cout << "//@DefaultInterface(\"" << defIface << "\")" << std::endl;


    std::cout << "@Proxy(\"WinRtProxy\")" << std::endl;

    std::string base;// = "WinRtInterface";
    auto ext = typeDef.Extends();
    if (ext)
    {
        auto extends = find(ext);
        if (extends)
        {
            base = fullName(extends.TypeNamespace(), extends.TypeName());
        }
    }
    if (!base.empty())
    {
        std::cout << "@Extends(\"" << base << "\")" << std::endl;
    }


    std::string nsr = nsRoot(fullname);

    std::cout << "class " << flatname << " : WinRtInterface" << std::endl << "{" << std::endl;

    std::cout << "    " << flatname << "( comPtr )" << std::endl << "    {" << std::endl;
    if (defIface.empty())
    {
        std::cout << "    " << "    this.comPtr = comPtr;" << std::endl;
    }
    else
    {
        std::cout << "        var " + nsr + " = runtime.global(\"" + nsr + "\");" << std::endl;
        std::cout << "    " << "    this.comPtr = comPtr.queryInterface( " << ungen(defIface) << ".iid );" << std::endl;
    }
    std::cout << "    }" << std::endl << std::endl;
    //write_methods(typeDef, -1);

    std::vector<std::string> statics = get_statics(typeDef);
    for (auto& s : statics)
    {
        std::cout << "    // s " << s << std::endl;
        write_static_methods(typeDef, fullname, s);
    }

    // Constructors

    std::cout << "    static Create()" << std::endl;
    std::cout << "    {" << std::endl;
    std::cout << "        return " << fullname << "(" << std::endl;
    std::cout << "            winrt.activate(\"" << fullname << "\")" << std::endl;
    std::cout << "        );" << std::endl;
    std::cout << "    }" << std::endl;

    std::vector<std::string> factories = get_factories(typeDef);
    for (auto& f : factories)
    {
        if (f.empty())
        {
            //
        }
        else
        {
            auto factory = theCache.find(f);
            if (factory)
            {
                auto methods = factory.MethodList();
                int n = 6;
                for (auto&& m : methods)
                {
                    print_static_ctor(fullname, factory, m, n);
                    n++;
                }
            }
            std::cout << "    // f " << f << std::endl;
        }
    }

    std::set<std::string> seen;

    //write_class_hierarchy(seen, typeDef, defIface);


    for (auto methodDef : typeDef.MethodList())
    {
        std::string name(methodDef.Name());
        while (seen.count(name))
        {
            name.append("_");
        }

        seen.insert(name);

        if (name == "GetAt" && fullname == "Windows.Web.Http.Headers.HttpLanguageHeaderValueCollection")
        {
            int x = 1;
        }
        print_class_method(name, typeDef, methodDef, defIface);
    }

    std::cout << std::endl;

    std::cout << "}" << std::endl;

    std::cout << ungen(fullname) << " = " << flatname << ";" << std::endl;
    std::cout << std::endl;
}

inline void print_namespace(const std::string& ns, bool pop = true)
{
    auto elems = split(ns, ".");

    if (elems.empty()) return;
    if (pop)
    {
        elems.pop_back();
        if (elems.empty()) return;
    }

    auto it = elems.begin();

    //    std::cout << "if( global(\"" << *it << "\") == nil ) { global(\"" << *it << "\", {}); }" << std::endl;
    //    std::cout << "var " << *it << " = global(\"" << *it << "\");" << std::endl;

    std::string n = *it;
    it++;

    for (it; it != elems.end(); it++)
    {
        std::cout << "if ( " << n << "." << *it << " == nil ) { " << n << "." << *it << " = {}; }" << std::endl;
        std::vector<std::string> v{ n, *it };
        n = join(v, ".");
    }
}

inline void write_namespace(const std::string& line, const cache::namespace_members& ns)
{
    // build namespace
    auto elems = split(line, ".");

    if (elems.empty()) return;

    auto it = elems.begin();

    std::cout << "if( runtime.global(\"" << *it << "\") == nil ) { runtime.global(\"" << *it << "\", {}); }" << std::endl;
    std::cout << "var " << *it << " = runtime.global(\"" << *it << "\");" << std::endl;

    print_namespace(line, false);
    /*    std::string n = *it;
        it++;
        for (it; it != elems.end(); it++)
        {
            std::cout << n << "." << *it << " = {};" << std::endl;
            std::vector<std::string> v{ n, *it };
            n = join( v, ".");
        }
    */
    // enums

    for (auto en : ns.enums)
    {
        write_enum(en);
    }

    for (auto s : ns.structs)
    {
        write_struct(s);
    }

    for (auto iface : ns.interfaces)
    {
        write_iface(iface);
    }

    for (auto clazz : ns.classes)
    {
        write_class(clazz);
    }

    for (auto delegat : ns.delegates)
    {
        write_delegate(delegat);
    }

}

