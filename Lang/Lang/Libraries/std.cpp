
#include "std.h"
#include "../VM.h"

#include <cstdio>
#include <iostream>

namespace script
{

    auto nativePrintLn = [&](int argCount, Value* args) 
    {
        args[0].Print();
        printf("\n");
        return Value();
    };

    auto nativePrint = [&](int argCount, Value* args) 
    {
        args[0].Print();
        return Value();
    };

    auto nativeInput = [&](int argCount, Value* args)
    {
        std::string str; 
        std::getline(std::cin, str);

        return Value(memoryManager.AllocateString(str));
    };

    void LoadStdIO(VM* vm, ObjModule* mdl)
    {
        if (mdl == nullptr)
        {
            // We load into global

            vm->AddNativeFunction("println", nativePrintLn, 1);
            vm->AddNativeFunction("print", nativePrint, 1);
            vm->AddNativeFunction("input", nativeInput, 1);

            return;   
        }

        mdl->AddNativeFunction("println", nativePrintLn, 1);
        mdl->AddNativeFunction("print", nativePrint, 1);
        mdl->AddNativeFunction("input", nativeInput, 1);

        // Load into module 
    }
}