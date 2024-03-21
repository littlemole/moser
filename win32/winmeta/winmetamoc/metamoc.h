#pragma once
#include "types.h"



void write_enum(TypeDef& typeDef)
{
    std::string ns(typeDef.TypeNamespace());
    std::string n(typeDef.TypeName());

    if (n[0] == '_') return;

    std::cout << std::endl << "Win32." << typeDef.TypeName() << " = {" << std::endl << "    ";

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
    std::string n(typeDef.TypeName());

    while ( !n.empty() && n[0] == '_')
    {
        n = n.substr(1);
    }

    if(n == "LOGFONTW")
    {
        int x = 1;
    }

    std::cout << std::endl << "Win32." << n << " = foreign.named_struct( \"";
    std::cout << ns << "." << typeDef.TypeName() << "\", [ " << std::endl << "    ";

    std::vector<std::string> fields;
    for (auto& field : typeDef.FieldList())
    {
        auto meta = fieldType(field);
        std::string n = meta.first;
        std::string typeName = meta.second;
        fields.push_back(n + ":" + typeName);
    }
    if (!fields.empty()) 
    {
        std::cout << "\"";
        std::cout << join(fields, "\", \r\n    \"");
        std::cout << "\"";
    }
    std::cout << std::endl << "]); " << std::endl << std::endl;
}



inline void write_delegate(TypeDef& typeDef)
{
    std::string fullname = fullName(typeDef.TypeNamespace(), typeDef.TypeName());
    std::string flatname = flatten(ungen(fullname));
    
    for (auto m : typeDef.MethodList())
    {
        std::string n(m.Name());
        if (n == ".ctor") continue;

        std::cout << "Win32." << typeDef.TypeName() << " = foreign.callback( ";

        std::string retType = get_retType(m);
        auto sig = m.Signature();
        auto rt = sig.ReturnType();
        std::cout << "\"";
        if (rt)
        {
            std::cout << get_win32_type(m.Signature().ReturnType().Type());
        }
        else
        {
            std::cout << "void";
        }
        std::cout << "\", [ ";

        auto params =sig.Params();
        std::vector<std::string> v;
        for (auto param : params)
        {
            std::string s = get_win32_type(param.Type());
            v.push_back(s);
        }
        if (!v.empty())
        {
            std::cout << "\"" << join(v, "\", \"") << "\"";
        }
        std::cout << "] );" << std::endl;
    }
    
}


inline void write_class(TypeDef& typeDef)
{
    auto fields = typeDef.FieldList();
    for (auto& field : fields)
    {
        if (field.Name()[0] == '_') continue;
        auto c = field.Constant();
        std::string entry;
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

        if (entry.empty())
        {
            entry = "nil";
        }
        else
        {
            char sc = entry[0];
            if (sc != '-' && !isdigit(sc))
            {
                entry = "\"" + entry + "\"";
            }
            else
            {
                bool isStr = false;
                for (auto& it : entry)
                {
                    if (!isxdigit(it) && it != '-' && it != 'x' && it != 'X')
                    {
                        isStr = true;
                        break;
                    }
                }
                if (isStr)
                {
                    entry = "\"" + entry + "\"";
                }
            }
        }


        std::cout << "Win32.";
        std::string n(field.Name());
        if (n == "WIN32" )
        {
            n = "Win32";
        }
        std::cout << n;
        std::cout << " = ";
        bool b = replace(entry, "\\", "\\\\");
        std::cout << entry << ";" << std::endl;

    }

    auto methods = typeDef.MethodList();
    for (auto& m : methods)
    {
        bool vararg = m.Signature().CallConvention() == CallingConvention::VarArg;
        std::cout << "Win32.";
        std::string n(m.Name());
        while ( !n.empty() && n[0] == '_')
        {
            n = n.substr(1);
        }
        std::cout << n;

        if (m.Name() == "EnumSystemCodePagesW")
        {
            int x = 1;
        }

        if (vararg)
        {
            std::cout << " = foreign.variadic( ";
        }
        else
        {
            std::cout << " = foreign.native( ";
        }

        std::string from = get_import(typeDef,m);
        std::cout << "\"" + from + "\", ";

        auto rt = m.Signature().ReturnType();
        if (rt)
        {        
            std::string retType = get_win32_type(rt.Type());
            std::cout << "\"" << retType << "\", ";
        } 
        else 
        {
            std::cout << "\"void\", ";
        }

        std::cout << "\"" << m.Name() << "\", [ ";

        std::vector<std::string> v;
        auto sig = m.Signature();
        
        auto params = sig.Params();
        for (auto param : params)
        {
            std::string p = get_win32_type(param.Type());
            v.push_back(p);
        }
        if (vararg)
        {
            v.push_back("...");
        }
        if (!v.empty())
        {
            std::cout << "\"" << join(v, "\", \"") << "\"";
        }
        std::cout << " ] ); " << std::endl;
    }
}



inline void write_namespace(const std::string& line, const cache::namespace_members& ns)
{
    // build namespace
    auto elems = split(line, ".");

    if (elems.empty()) return;

    auto it = elems.begin();

    std::cout << "{" << std::endl << std::endl;
    std::cout << "if( runtime.global(\"Win32\") == nil ) { runtime.global(\"Win32\", {}); }" << std::endl;
    std::cout << "var Win32 = runtime.global(\"Win32\");" << std::endl;

    // enums

    for (auto en : ns.enums)
    {
        write_enum(en);
    }
    
    for (auto s : ns.structs)
    {
        write_struct(s);
    }
    /*
    for (auto iface : ns.interfaces)
    {
        write_iface(iface);
    }
    */
    for (auto clazz : ns.classes)
    {
        write_class(clazz);
    }
    
    for (auto delegat : ns.delegates)
    {
        write_delegate(delegat);
    }
    std::cout << std::endl;
    std::cout << "}" << std::endl;

}

