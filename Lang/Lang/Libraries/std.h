
#pragma once

#include "web.h"

namespace script
{
    class ObjModule;
    class VM;

    // Loaded by std:io
    void LoadStdIO(VM* vm, ObjModule* mdl);

    // Loaded by std:maths
    void LoadStdMaths(VM* vm, ObjModule* mdl);

    void LoadStdFilesystem(VM* vm, ObjModule* mdl);

    void LoadStdTime(VM* vm, ObjModule* mdl);

    // Always loaded 
    void LoadStdPrimitives(VM* vm);
}