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

    { ElementType::Void, "void"},
    { ElementType::Boolean, "bool"},
    { ElementType::Char, "short"},
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
    { ElementType::String, "wstr" },
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
    { ElementType::MVar, "unk" }
};



void handle_line(const std::string& line)
{

    for (auto ns : theCache.namespaces())
    {
        if (ns.first == line)
        {
            write_namespace(line, ns.second);
        }
    }
 
    std::cout << std::endl;
}


int main(int argc, char** argv)
{
    import_namespace("Windows.Win32");
    import_namespace("Windows.Win32.Globalisation");
    import_namespace("Windows.Win32.Foundation");

    if (argc < 2) return 0;
    std::string line = argv[1];
    handle_line(line);

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

