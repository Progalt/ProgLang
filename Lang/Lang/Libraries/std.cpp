
#include "std.h"
#include "../VM.h"

#include <cstdio>
#include <iostream>

#include <cmath>

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

    auto cosNative = [&](int argc, Value* args) {

        return Value(cos(args[0].ToNumber()));
    };

    auto sinNative = [&](int argc, Value* args) {

        return Value(sin(args[0].ToNumber()));
    };

    auto tanNative = [&](int argc, Value* args) {

        return Value(tan(args[0].ToNumber()));
    };

    auto acosNative = [&](int argc, Value* args) {

        return Value(acos(args[0].ToNumber()));
    };

    auto asinNative = [&](int argc, Value* args) {

        return Value(asin(args[0].ToNumber()));
    };

    auto atanNative = [&](int argc, Value* args) {

        return Value(atan(args[0].ToNumber()));
    };

    auto atan2Native = [&](int argc, Value* args) {

        return Value(atan2(args[0].ToNumber(), args[1].ToNumber()));
    };

    auto sinhNative = [&](int argc, Value* args) {
        return Value(sinh(args[0].ToNumber()));
    };

    auto coshNative = [&](int argc, Value* args) {
        return Value(cosh(args[0].ToNumber()));
    };

    auto tanhNative = [&](int argc, Value* args) {
        return Value(tanh(args[0].ToNumber()));
    };

    auto asinhNative = [&](int argc, Value* args) {
        return Value(asinh(args[0].ToNumber()));
    };

    auto acoshNative = [&](int argc, Value* args) {
        return Value(acosh(args[0].ToNumber()));
    };

    auto atanhNative = [&](int argc, Value* args) {
        return Value(atanh(args[0].ToNumber()));
    };

    auto logNative = [&](int argc, Value* args) {
        return Value(log(args[0].ToNumber()));
    };

    auto log2Native = [&](int argc, Value* args) {
        return Value(log2(args[0].ToNumber()));
    };

    auto log10Native = [&](int argc, Value* args) {
        return Value(log10(args[0].ToNumber()));
    };

    auto expNative = [&](int argc, Value* args) {
        return Value(exp(args[0].ToNumber()));
    };

    auto exp2Native = [&](int argc, Value* args) {
        return Value(exp2(args[0].ToNumber()));
    };

    auto expm1Native = [&](int argc, Value* args) {
        return Value(expm1(args[0].ToNumber()));
    };

    auto sqrtNative = [&](int argc, Value* args) {
        return Value(sqrt(args[0].ToNumber()));
    };

    auto cbrtNative = [&](int argc, Value* args) {
        return Value(cbrt(args[0].ToNumber()));
    };

    void LoadStdMaths(VM* vm, ObjModule* mdl)
    {
        
        if (mdl == nullptr)
        {
            vm->AddNativeFunction("cos", cosNative, 1);
            vm->AddNativeFunction("sin", sinNative, 1);
            vm->AddNativeFunction("tan", tanNative, 1);
            vm->AddNativeFunction("acos", acosNative, 1);
            vm->AddNativeFunction("asin", asinNative, 1);
            vm->AddNativeFunction("atan", atanNative, 1);
            vm->AddNativeFunction("atan2", atan2Native, 1);

            vm->AddNativeFunction("sinh", sinhNative, 1);
            vm->AddNativeFunction("cosh", coshNative, 1);
            vm->AddNativeFunction("tanh", tanhNative, 1);
            vm->AddNativeFunction("asinh", asinhNative, 1);
            vm->AddNativeFunction("acosh", acoshNative, 1);
            vm->AddNativeFunction("atanh", atanhNative, 1);

            vm->AddNativeFunction("log", logNative, 1);
            vm->AddNativeFunction("log2", log2Native, 1);
            vm->AddNativeFunction("log10", log10Native, 1);
            vm->AddNativeFunction("exp", expNative, 1);
            vm->AddNativeFunction("exp2", exp2Native, 1);
            vm->AddNativeFunction("expm1", expm1Native, 1);

            vm->AddNativeFunction("sqrt", sqrtNative, 1);
            vm->AddNativeFunction("cbrt", cbrtNative, 1);

            return;
        }

        mdl->AddNativeFunction("cos", cosNative, 1);
        mdl->AddNativeFunction("sin", sinNative, 1);
        mdl->AddNativeFunction("tan", tanNative, 1);
        mdl->AddNativeFunction("acos", acosNative, 1);
        mdl->AddNativeFunction("asin", asinNative, 1);
        mdl->AddNativeFunction("atan", atanNative, 1);
        mdl->AddNativeFunction("atan2", atan2Native, 1);

        mdl->AddNativeFunction("sinh", sinhNative, 1);
        mdl->AddNativeFunction("cosh", coshNative, 1);
        mdl->AddNativeFunction("tanh", tanhNative, 1);
        mdl->AddNativeFunction("asinh", asinhNative, 1);
        mdl->AddNativeFunction("acosh", acoshNative, 1);
        mdl->AddNativeFunction("atanh", atanhNative, 1);

        mdl->AddNativeFunction("log", logNative, 1);
        mdl->AddNativeFunction("log2", log2Native, 1);
        mdl->AddNativeFunction("log10", log10Native, 1);
        mdl->AddNativeFunction("exp", expNative, 1);
        mdl->AddNativeFunction("exp2", exp2Native, 1);
        mdl->AddNativeFunction("expm1", expm1Native, 1);

        mdl->AddNativeFunction("sqrt", sqrtNative, 1);
        mdl->AddNativeFunction("cbrt", cbrtNative, 1);
    }

    auto dictionaryFunc = [&](int argc, Value* args) {
        return script::Value(script::AllocateDictionary());
    };

    auto listFunc = [&](int argc, Value* args) {
        return script::Value(script::AllocateArray({}));
    };

    void LoadStdPrimitives(VM* vm)
    {
        vm->AddNativeFunction("Dictionary", dictionaryFunc, 0);
        vm->AddNativeFunction("List", listFunc, 0);
    }
}