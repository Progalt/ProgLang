

namespace script
{
    class ObjModule;
    class VM;

    // loaded by calling std:json 
    void LoadJsonModule(VM* vm, ObjModule* mdl);

    // loaded by calling std:http 
    void LoadHttpModule(VM* vm, ObjModule* mdl);
}