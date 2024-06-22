
#include "web.h"
#include "../VM.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace script
{
    void parseJson(json& j, ObjDictionary* d)
    {
        for (auto& [key, value] : j.items())
        {
            if (value.is_number() || value.is_number_float())
            {
                double dnum = (double)value.get<double>();

                d->map[Value(memoryManager.AllocateString(key)).Hash()] = Value(dnum);
            }
            else if (value.is_string())
            {
                std::string str = value.get<std::string>();

                d->map[Value(memoryManager.AllocateString(key)).Hash()] = Value(memoryManager.AllocateString(str));
            }
            else if (value.is_boolean())
            {
                bool b = value.get<bool>();

                d->map[Value(memoryManager.AllocateString(key)).Hash()] = Value(b);
            }
            else if (value.is_object())
            {
                json obj = value.get<json>();

                ObjDictionary* newDict = AllocateDictionary();

                parseJson(obj, newDict);

                d->map[Value(memoryManager.AllocateString(key)).Hash()] = newDict; 
            }
            else if (value.is_array())
            {
               assert(false && "not implemented");
            }
            else 
            {
                d->map[Value(memoryManager.AllocateString(key)).Hash()] = Value();
            }
        }
    }

    auto jsonParseNative = [](int argc, Value* args) 
    {

        assert(argc == 1);

        // We expect one arg a string
        // This is the source

        ObjString* str = (ObjString*)args[0].ToObject();

        ObjDictionary* dict = AllocateDictionary();

        json data = json::parse(str->str);

        parseJson(data, dict);

        return Value(dict);
    };

    void LoadJsonModule(VM* vm, ObjModule* mdl)
    {
        if (mdl == nullptr)
        {
            vm->AddNativeFunction("parse", jsonParseNative, 1);


        }

        mdl->AddNativeFunction("parse", jsonParseNative, 1);
    }

    void LoadHttpModule(VM* vm, ObjModule* mdl)
    {

    }
}