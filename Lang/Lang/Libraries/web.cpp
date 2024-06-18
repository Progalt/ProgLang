
#include "web.h"
#include "../VM.h"

namespace script
{
    auto jsonParseNative = [&](int argc, Value* args) 
    {

        assert(argc == 1);

        // We expect one arg a string

        ObjString* str = (ObjString*)args[0].ToObject();

        return Value();
    };

    void LoadJsonModule(VM* vm, ObjModule* mdl)
    {
        if (mdl == nullptr)
        {
            vm->AddNativeFunction("parse", jsonParseNative, 1);


        }
    }

    void LoadHttpModule(VM* vm, ObjModule* mdl)
    {

    }
}