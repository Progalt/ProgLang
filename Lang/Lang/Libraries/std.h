
#pragma once

namespace script
{
    class ObjModule;
    class VM;

    void LoadStdIO(VM* vm, ObjModule* mdl);

    void LoadStdMaths(VM* vm, ObjModule* mdl);
}