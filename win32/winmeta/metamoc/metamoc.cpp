// metamoc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "metamoc.h"


cache theCache;

std::set<std::string> typesSeen;
std::set<generic_instance> generic_instances;
std::set<generic_instance> generic_delegates;


std::map<async_mode, std::string> asyncModeOperation = {
    { async_mode::NONE, "invoke"},
    { async_mode::ACTION, "asyncAction"},
    { async_mode::ACTION_WITH_PROGRESS, "asyncActionWithProgress"},
    { async_mode::OPERATION, "asyncOperation"},
    { async_mode::OPERATION_WITH_PROGRESS, "asyncOperationWithProgress"}
};


std::map<ElementType, std::string> typenames = {

    { ElementType::Void, "Void"},
    { ElementType::Boolean, "Boolean"},
    { ElementType::I1, "Int8" },
    { ElementType::U1, "UInt8" },
    { ElementType::I2, "Int16" },
    { ElementType::U2, "UInt16" },
    { ElementType::I4, "Int32" },
    { ElementType::U4, "UInt32" },
    { ElementType::I8, "Int64" },
    { ElementType::U8, "UInt64" },
    { ElementType::R4, "Single" },
    { ElementType::R8, "Double" },
    { ElementType::String, "String" },
    { ElementType::Ptr, "IntPtr" },
    { ElementType::ByRef, "ByRef" },
    { ElementType::ValueType, "Value" },
    { ElementType::Class, "Class" },
    { ElementType::Var, "Var" },
    { ElementType::Array, "Array" },
    { ElementType::GenericInst, "GenericInst" },
    { ElementType::TypedByRef, "TypedByRef" },
    { ElementType::I, "iptr" },
    { ElementType::U, "uptr" },
    { ElementType::FnPtr, "fun" },
    { ElementType::Object, "Object"}, //"IInspectable" },
    { ElementType::SZArray, "SZArray" },
    { ElementType::MVar, "MVar" }
};



void handle_line(const std::string& line)
{
    //import_namespace(line);

    //print_proxy();

    std::cout << "{" << std::endl;

    std::cout << "class WinRtInterface { " << std::endl;
    std::cout << "    queryInterface(iid) { return this.comPtr.queryInterface(iid); }" << std::endl;
    std::cout << "    release() { this.comPtr.release(); this.comPtr = nil; }" << std::endl;
    std::cout << "    valid() { return this.comPtr != nil && int(this.comPtr) != 0; }" << std::endl;
    std::cout << "}" << std::endl;
    std::cout << std::endl;



    for (auto ns : theCache.namespaces())
    {
        //std::cout << ns.first << " =?= " << line << std::endl;
        if (ns.first == line)
        {
            write_namespace(line, ns.second);
        }
    }

    for (auto& it : generic_instances)
    {
        std::vector<std::string> args;
        for (auto& p : it.params)
        {
            args.push_back(p.name);
        }

        std::string fullname = it.toString();

        auto v = split_generic(fullname);
        auto jt = v.begin();
        auto typeDef = theCache.find(*jt);

        size_t pos = fullname.find("***UNKNOWN***");
        if (pos != std::string::npos) continue;

        GUID guid = get_generic(fullname);
        std::string ns(typeDef.TypeNamespace());
        print_namespace(ns, false);
        write_gen_iface(typeDef, guid, args);
    }
    generic_instances.clear();

    for (auto& it : generic_delegates)
    {
        std::vector<std::string> args;
        for (auto& p : it.params)
        {
            args.push_back(p.name);
        }

        std::string fullname = it.toString();

        auto v = split_generic(fullname);
        auto jt = v.begin();
        auto typeDef = theCache.find(*jt);

        GUID guid = get_generic(fullname);
        std::string guidStr = guid_2_string(guid);
        std::string ns(typeDef.TypeNamespace());
        print_namespace(ns, false);
        write_generic_delegate(typeDef, guidStr, args);
    }
    generic_delegates.clear();

    std::cout << std::endl;
    std::cout << "}" << std::endl;

}

int main(int argc, char** argv)
{
    import_platform_metadata(WINMETAVER);
/*    std::cout << "// " << WINMETAVER << std::endl;
    import_namespace("Windows.Foundation");
    import_namespace("Windows.Foundation.Collections");
    import_namespace("Windows.Graphics");
    import_namespace("Windows.Storage");
    import_namespace("Windows.Storage.Streams");
    import_namespace("Windows.Web");
    import_namespace("Windows.Security");
    import_namespace("Windows.System");
    import_namespace("Windows.UI");
    import_namespace("Windows.UI.Input");
    import_namespace("Windows.UI.Xaml");
    import_namespace("Windows.UI.Xaml.Input");
    import_namespace("Windows.ApplicationModel.DataTransfer");
    import_namespace("Windows.Globalization");
    */
    import_namespace("Microsoft.Web.WebView2.Core");
    import_namespace("Microsoft.UI.Xaml");
    import_namespace("Microsoft.UI");

    import_namespace("moxaml");

    if (argc < 2) return 0;
    std::string line = argv[1];
    handle_line(line);

    //handle_winmd(line);

    while (false)
    {
        std::string line;
        std::getline(std::cin, line);
        if (line.starts_with("#"))
        {
            import_namespace(line.substr(1));
            continue;
        }
        handle_line(line);
    }

    return 0;

}

