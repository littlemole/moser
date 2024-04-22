//#include "pch.h"
#include "common.h"
#include "vm.h"
#include "gc.h"

#include "must.h"

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include "win32/uni.h"
#include <roapi.h>
#include <windows.foundation.h>
#include "win32/xaml.h"
#endif


static void runFile(VM& vm, const char* path) 
{
    InterpretResult result = vm.execute(path);

    if (result == InterpretResult::INTERPRET_COMPILE_ERROR) exit(65);
    if (result == InterpretResult::INTERPRET_RUNTIME_ERROR) exit(70);
}

static void repl(VM& vm) 
{
    char line[1024];
    for (;;) 
    {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        vm.interpret(line);
    }
}

static void compileFile(VM& vm, const char* path)
{
    InterpretResult result = vm.compile(path,true);

    if (result == InterpretResult::INTERPRET_COMPILE_ERROR) exit(65);
    if (result == InterpretResult::INTERPRET_RUNTIME_ERROR) exit(70);
}

static void debugFile(VM& vm, const char* path) 
{
    vm.debug(path);
}


std::string get_env_var(std::string const& key) 
{
    char* val;
    val = getenv(key.c_str());
    std::string retval = "";
    if (val != nullptr ) 
    {
        retval = val;
    }
    return retval;
}



int main(int argc, char** argv)
{
    VM vm;
    std::string libpath = path_to_self_directory("lib");
    vm.include_path.push_back(libpath);

#ifdef _WIN32
    libpath += "\\win";
#else
    libpath += "/linux";
    vm.include_path.push_back(libpath);
    libpath = "/usr/local/lib/moser/lib";
    vm.include_path.push_back(libpath);
    libpath += "/linux";
#endif
    vm.include_path.push_back(libpath);

    libpath = get_env_var("MOCLIB");
    if (!libpath.empty())
    {
        vm.include_path.push_back(libpath);
    }
    
    for(int i = 0; i < argc; i++)
    {
        vm.cliArgs.push_back(argv[i]);
    }
    if (argc == 1) 
    {
        repl(vm);
    } 
    else if (argc == 2) 
    {
        runFile(vm,argv[1]);
    }
    else if (argc == 3) 
    {
        std::string cmd = argv[1];
        if(cmd == "-c")
        {
            compileFile(vm, argv[2]);
        }
        else if(cmd =="-d")
        {
            debugFile(vm, argv[2]);
        }
        else
        {
            runFile(vm,argv[1]);
        }
    }     
    else if (argc < 2)
    {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }
    else 
    {
        runFile(vm, argv[1]);
    }
    return 0;
}
