
#include "VM.h"
#include "Memory.h"
#include "Compiler.h"

#include <fstream>
#include <sstream>

#include "Libraries/std.h"

namespace script
{

    VM::VM(const VMCreateInfo& createInfo)
    {
        if (!createInfo.ioInterface)
        {
            m_IOInterface = new DefaultIOInterface;
        }
        else 
        {
            m_IOInterface = createInfo.ioInterface;
        }

        // Load the standard stuff that the language needs
        LoadStdPrimitives(this);
    }

    VM::~VM()
    {
        delete m_IOInterface;
    }

    InterpretResult VM::Interpret(ObjFunction* function)
    {
        m_CurrentFiber = CreateFiber(function);
        m_CurrentFiber->state = FIBER_ROOT;

     

        return Run();
    }

    InterpretResult VM::Run()
	{
        //this->chunk = c;

        CallFrame* frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
        m_CurrentGlobal = &m_GlobalVariables;

        uint32_t instructionCount = 0;

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants[READ_BYTE()])
#define BINARY_OP(op) \
    do { \
      Value b = m_CurrentFiber->stack.Pop(); \
      Value a = m_CurrentFiber->stack.Pop(); \
      m_CurrentFiber->stack.Push(a op b); \
    } while (false)
		
        for (;;) 
        {

            if (memoryManager.shouldCollectGarbage) {
                CollectGarbage();
                memoryManager.shouldCollectGarbage = false;
                memoryManager.m_NextGC = memoryManager.m_BytesAllocated * GC_HEAP_GROW_FACTOR;
            }

            // DisassembleInstruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code.data()));

            uint8_t instruction = 0;
            switch (instruction = READ_BYTE()) 
            {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                m_CurrentFiber->stack.Push(constant);
                break;
            }
            case OP_CONSTANT_LONG:
            {
                uint16_t idx = READ_SHORT();

                m_CurrentFiber->stack.Push(frame->function->chunk.constants[idx]);

                break;
            }
            case OP_NEGATE: {
                // TODO: Make this better
                m_CurrentFiber->stack.Push(-m_CurrentFiber->stack.Pop());
                   break;
            }
            case OP_NIL:
                m_CurrentFiber->stack.Push(Value());
                break;
            case OP_TRUE:
                m_CurrentFiber->stack.Push(Value(true));
                break;
            case OP_FALSE:
                m_CurrentFiber->stack.Push(Value(false));
                break;
            case OP_NOT:
            {
                Value v = m_CurrentFiber->stack.Pop();
                v.MakeBool(!v.AsBool());
                m_CurrentFiber->stack.Push(v);
            }
                break;
            case OP_EQUAL:
            {
                Value a = m_CurrentFiber->stack.Pop();
                Value b = m_CurrentFiber->stack.Pop();

                m_CurrentFiber->stack.Push(Value(a == b));

                break;
            }
            case OP_LESS:
            {
                Value a = m_CurrentFiber->stack.Pop();
                Value b = m_CurrentFiber->stack.Pop();

                m_CurrentFiber->stack.Push(a > b);

                break;
            }
            case OP_GREATER:
            {
                Value a = m_CurrentFiber->stack.Pop();
                Value b = m_CurrentFiber->stack.Pop();

                m_CurrentFiber->stack.Push(a < b);

                break;
            }
            case OP_ADD:      BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE:   BINARY_OP(/ ); break;
            case OP_POWER: 
            {
                Value b = m_CurrentFiber->stack.Pop();
                Value a = m_CurrentFiber->stack.Pop();

                double power = std::pow(a.ToNumber(), b.ToNumber());

                m_CurrentFiber->stack.Push(Value(power));

                break;
            }
            case OP_INCREMENT:
            {
                Value v = m_CurrentFiber->stack.Pop();

                int num = (int)v.ToNumber();
                num++;
                v = Value((double)num);

                m_CurrentFiber->stack.Push(v);
            }
            case OP_MODULO:
            {
                Value b = m_CurrentFiber->stack.Pop();
                Value a = m_CurrentFiber->stack.Pop();

                double remainder = std::remainder(a.ToNumber(), b.ToNumber());
                m_CurrentFiber->stack.Push(Value(remainder));

                break;
            }
            case OP_POP: m_CurrentFiber->stack.Pop(); break;
            case OP_DEFINE_GLOBAL:
            {
                uint16_t constant = READ_SHORT();

                ObjString* name = (ObjString*)frame->function->chunk.constants[constant].ToObject();

                m_CurrentGlobal->operator[](name->str) = m_CurrentFiber->stack.Pop();

                break;
            }

            case OP_EXPORT_GLOBAL:
            {
                // Export is the same as define global 
                // But we add it to a list of exported variables so its easier to get later. 

                uint16_t constant = READ_SHORT();

                ObjString* name = (ObjString*)frame->function->chunk.constants[constant].ToObject();

                m_CurrentGlobal->operator[](name->str) = m_CurrentFiber->stack.Pop();

                m_ExportedVariables.push_back(name->str);

                break;
            }
            case OP_CREATE_LIST:
            {
                uint16_t size = READ_SHORT();

                std::vector<Value> value(size);

                for (uint16_t i = 0; i < size; i++)
                {
                    value[size - i - 1] = m_CurrentFiber->stack.Pop();
                }

                m_CurrentFiber->stack.Push(Value(AllocateArray(value)));

                break;
            }
            case OP_SUBSCRIPT_READ:
            {
                Value idx = m_CurrentFiber->stack.Pop();
                Value arr = m_CurrentFiber->stack.Pop();

                if (arr.IsObjType(OBJ_DICTIONARY))
                {
                    ObjDictionary* dict = (ObjDictionary*)arr.ToObject();

                    auto it = dict->map.find(idx.Hash());
                    if (it != dict->map.end())
                    {
                        m_CurrentFiber->stack.Push(it->second);
                        break;
                    }

                    Error("Dictionary does not have key");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (idx.IsNumber())
                {
                    Error("Index is not a number");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int64_t idxInt = (size_t)idx.ToNumber();

                if (!arr.IsObjType(OBJ_ARRAY))
                {
                    Error("Cannot set an array index if object is not an array");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (arr.IsObjType(OBJ_ARRAY))
                {
                    ObjArray* obj = (ObjArray*)arr.ToObject();

                    if (idxInt >= 0)
                        m_CurrentFiber->stack.Push(obj->values[idxInt]);
                    else
                    {
                        uint64_t absIdx = (-idxInt);//% obj->values.size();



                        m_CurrentFiber->stack.Push(obj->values[obj->size - absIdx]);
                    }
                }
                else
                {
                    assert("Implement string indexing");


                }

                break;
            }
            case OP_SUBSCRIPT_WRITE:
            {
                Value item = m_CurrentFiber->stack.Pop();
                Value idx = m_CurrentFiber->stack.Pop();
                Value arr = m_CurrentFiber->stack.Pop();

                if (arr.IsObjType(OBJ_DICTIONARY))
                {
                    ObjDictionary* dict = (ObjDictionary*)arr.ToObject();

                    dict->map[idx.Hash()] = item;
                    

                    break;
                }

                if (idx.IsNumber())
                {
                    Error("Index is not a number");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int64_t idxInt = (size_t)idx.ToNumber();

                if (!arr.IsObjType(OBJ_ARRAY))
                {
                    Error("Cannot set an array index if object is not an array");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (arr.IsObjType(OBJ_ARRAY))
                {
                    ObjArray* obj = (ObjArray*)arr.ToObject();

                    if (idxInt < obj->size)
                    {
                        obj->values[idxInt] = item;

                        m_CurrentFiber->stack.Push(item);
                    }
                    else
                    {
                        Error("Index out of range");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                else
                {
                    assert("Implement string indexing");
                }

                break;
            }
            case OP_SLICE_ARRAY:
            {
                Value idx2 = m_CurrentFiber->stack.Pop();
                Value idx = m_CurrentFiber->stack.Pop();
                Value arr = m_CurrentFiber->stack.Pop();

                if (!idx.IsNumber() || !idx2.IsNumber())
                {
                    Error("Attempting slice where one or both of indices are not numbers");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (!arr.IsObjType(OBJ_ARRAY))
                {
                    Error("Object is not slicable as it is not an array");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int64_t idxInt = (size_t)idx.ToNumber();
                int64_t idx2Int = (size_t)idx2.ToNumber();
                ObjArray* obj = (ObjArray*)arr.ToObject();


                std::vector<Value> value(idx2Int - idxInt);

                for (int64_t i = idxInt; i < idx2Int; i++)
                {
                    value[i - idxInt] = obj->values[i];
                }

                m_CurrentFiber->stack.Push(Value(AllocateArray(value)));

                break;
            }
            case OP_GET_GLOBAL:
            {

                uint16_t constant = READ_SHORT();

                ObjString* name = (ObjString*)frame->function->chunk.constants[constant].ToObject();
                auto it = m_CurrentGlobal->find(name->str);
                if (it != m_CurrentGlobal->end())
                {
                    Value value = it->second;
                    m_CurrentFiber->stack.Push(value);
                }
                else
                {
                    Error("cannot get global variable: \"" + std::string(name->str) + "\" as it does not exist.");
                    return INTERPRET_RUNTIME_ERROR;
                }


                break;
            }
            case OP_SET_GLOBAL:
            {
                uint16_t constant = READ_SHORT();

                ObjString* name = (ObjString*)frame->function->chunk.constants[constant].ToObject();

                auto it = m_CurrentGlobal->find(name->str);
                if (it == m_CurrentGlobal->end())
                {
                    Error("cannot set global variable: " + std::string(name->str) + " as it does not exist.");
                    return INTERPRET_RUNTIME_ERROR;
                }


                it->second = m_CurrentFiber->stack.Peek(0);

                break;
            }
            case OP_GET_LOCAL:
            {
                uint16_t constant = READ_SHORT();

                // m_CurrentFiber->stack.Push(m_CurrentFiber->stack[constant]);

                m_CurrentFiber->stack.Push(frame->slots[constant]);

                break;
            }
            case OP_SET_LOCAL:
            {
                uint16_t constant = READ_SHORT();

                //m_CurrentFiber->stack[constant] = m_CurrentFiber->stack.Peek(0);
                frame->slots[constant] = m_CurrentFiber->stack.Peek(0);


                break;
            }
            case OP_JUMP_IF_FALSE:
            {
                uint16_t offset = READ_SHORT();

                if (m_CurrentFiber->stack.Peek(0).IsBool())
                {
                    if (m_CurrentFiber->stack.Peek(0).AsBool() == false)
                    {
                        frame->ip += offset;
                    }
                }
                else if (m_CurrentFiber->stack.Peek(0).IsNil())
                {
                    frame->ip += offset;
                }

                break;
            }
            case OP_JUMP:
            {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_LOOP:
            {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;

                break;
            }
            case OP_CALL:
            {
                int argCount = READ_BYTE();
                if (!CallValue(m_CurrentFiber->stack.Peek(argCount), argCount))
                {
                    Error("Could not call function");
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];

               

                break;
            }
            case OP_CLASS:
            {
                uint16_t constant = READ_SHORT();

                ObjString* name = (ObjString*)frame->function->chunk.constants[constant].ToObject();


                m_CurrentFiber->stack.Push(Value(NewClass(name->str)));

                break;
            }
            case OP_GET_PROPERTY:
            {
                // Any object could have methods
                uint16_t constant = READ_SHORT();

                ObjString* name = (ObjString*)frame->function->chunk.constants[constant].ToObject();

                Value stackObj = m_CurrentFiber->stack.Peek(0);
                Object* obj = (Object*)stackObj.ToObject();
                auto it = obj->methods.find(name->str);
                if (it != obj->methods.end())
                {
                    // TODO: Work on making this better for modules

                    if (it->second.IsObjType(OBJ_CLASS))
                    {
                        m_CurrentFiber->stack.Pop();
                        m_CurrentFiber->stack.Push(it->second);
                        break;
                    }

                    ObjBoundMethod* bound = nullptr;

                    if (it->second.IsObjType(OBJ_FUNCTION))
                        bound = NewBoundMethod(stackObj, (ObjFunction*)it->second.ToObject());
                    else 
                        bound = NewNativeBoundMethod(stackObj, (ObjNative*)it->second.ToObject());

                    m_CurrentFiber->stack.Pop();
                    m_CurrentFiber->stack.Push(Value(bound));

                    break;
                }

                // `nly instances can have fields
                ObjInstance* instance = (ObjInstance*)obj;

              
                auto itfield = instance->fields.find(name->str);
                if (itfield != instance->fields.end())
                {
                    Value value = itfield->second;
                    m_CurrentFiber->stack.Pop();
                    m_CurrentFiber->stack.Push(value);
                    break;
                }

                Error("Field does not exist in instance: " + std::string(name->str));
                return INTERPRET_RUNTIME_ERROR;

                break;
            }
            case OP_SET_PROPERTY:
            {
                if (!m_CurrentFiber->stack.Peek(1).IsObjType(OBJ_INSTANCE))
                {
                    Error("Set field: Only instances of classes can have fields");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = (ObjInstance*)m_CurrentFiber->stack.Peek(1).ToObject();

                uint16_t constant = READ_SHORT();

                ObjString* name = (ObjString*)frame->function->chunk.constants[constant].ToObject();
                
                instance->fields[name->str] = m_CurrentFiber->stack.Peek(0);

                Value value = m_CurrentFiber->stack.Pop();
                m_CurrentFiber->stack.Pop();
                m_CurrentFiber->stack.Push(value);

                break;
            }
            case OP_METHOD:
            {
                uint16_t constant = READ_SHORT();

                ObjString* name = (ObjString*)frame->function->chunk.constants[constant].ToObject();
                DefineMethod(name->str);
                break;
            }
            case OP_THROW:
            {
                // TODO: Throwing
                

                break;
            }
            case OP_STRING_INTERP:
            {
                uint8_t args = READ_BYTE();

                std::string output = "";

                for (uint8_t i = 0; i < args; i++)
                {
                    Value val = m_CurrentFiber->stack.Pop();

                    // val.Print();

                    // TODO: This could probably be improved 

                    // String inputs are flipped to where they should be on the output so always prepend the string 

                    if (val.IsNumber())
                        output = std::to_string(val.ToNumber()) + output;
                    else if (val.IsBool())
                        output = (val.AsBool() ? "true" : "false") + output;
                    else if (val.IsObject())
                    {
                        Object* obj = val.ToObject();

                        //output = obj->ToString() + output;

                        switch (obj->type)
                        {
                        case OBJ_STRING:
                        {
                            output = std::string(((ObjString*)obj)->str) + output;
                            break;
                        }
                        }
                    }
                    else
                        output = "nil" + output; 

                }

                m_CurrentFiber->stack.Push(Value(memoryManager.AllocateString(output)));

                break;
            }
            case OP_ITER:
            {
                // This works as -4... not sure why
                // Maybe might break...
                Value* value = m_CurrentFiber->stack.m_Top - 4;
                Value seq = *(m_CurrentFiber->stack.m_Top - 2);
                Value* iterator = m_CurrentFiber->stack.m_Top - 1;

                uint16_t jumpOffset = READ_SHORT();

                double it = 0.0;
                if (!iterator->IsNil())
                    it = iterator->ToNumber();

                Object* obj = seq.ToObject();

                switch (obj->type)
                {
                case OBJ_ARRAY:
                {
                    ObjArray* arr = (ObjArray*)obj;
                    uint32_t idx = (uint32_t)trunc(it);

                    if (idx >= arr->size)
                        frame->ip += jumpOffset;

                    //*iterator = Value((double)(idx + 1));
                    *value = arr->values[idx];
                    iterator->MakeNumber((double)(idx + 1));
                    break;
                }
                default:
                    Error("Object does not support iteration.");

                    return INTERPRET_RUNTIME_ERROR;
                }

                break;
            }
            case OP_IMPORT_MODULE:
            {
                uint16_t moduleNameConstant = READ_SHORT();

                ObjString* moduleName = (ObjString*)frame->function->chunk.constants[moduleNameConstant].ToObject();

                ObjFiber* fiber = ImportModule(moduleName->str);

                if (fiber)
                {
                    // We have a fiber now let's run it
                    // Just make it be the current fiber we execute 
                    m_CurrentFiber = fiber;
                    frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
                }

                break;
            }
            case OP_IMPORT_MODULE_AS:
            {
                uint16_t moduleNameConstant = READ_SHORT();
                uint16_t asNameConstant = READ_SHORT();

                ObjString* moduleName = (ObjString*)frame->function->chunk.constants[moduleNameConstant].ToObject();
                ObjString* asName = (ObjString*)frame->function->chunk.constants[asNameConstant].ToObject();

                ObjFiber* fiber = ImportModule(moduleName->str, asName->str);

                if (fiber)
                {
                    // We have a fiber now let's run it
                    // Just make it be the current fiber we execute 
                    m_CurrentFiber = fiber;
                    frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
                }

                break;
            }
            case OP_RETURN: {

               

                Value result = m_CurrentFiber->stack.Pop();
                m_CurrentFiber->framesCount--;
                if (m_CurrentFiber->framesCount == 0)
                {
                    m_CurrentFiber->stack.Pop();

                    if (m_ExecutingModule)
                    {
                        // Add the module to the global variables
                       

                        // If we are within a module 
                        // We need to check if when we go up its another module or global
                        if (m_ExecutingModule->caller)
                        {
                            m_CurrentGlobal = &m_ExecutingModule->caller->methods;
                        }
                        else 
                            m_CurrentGlobal = &m_GlobalVariables;

                        // Add the executing module to the globals
                        m_GlobalVariables[std::string(m_ExecutingModule->name->str)] = Value(m_ExecutingModule);

                        // Traverse up the module stack
                        m_ExecutingModule = m_ExecutingModule->caller;

                    }

                    // Check if we are a fiber
                    // If we aren't this is the main script and its done
                    if (m_CurrentFiber->caller != nullptr)
                    {
                        // If the caller fiber exists we want to go back to that 

                        m_CurrentFiber = m_CurrentFiber->caller;
                        frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
                        break;
                    }
                    else 
                        return INTERPRET_ALL_GOOD;
                }

                

                m_CurrentFiber->stack.m_Top = frame->slots;
                m_CurrentFiber->stack.Push(result);
                frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
              
               
                break;
            }

            }

            instructionCount++;

        }


        return INTERPRET_ALL_GOOD;
#undef BINARY_OP 
#undef READ_CONSTANT
#undef READ_BYTE
	}

    bool VM::CallValue(Value value, int argCount)
    {

        switch(value.GetObjectType())
        {
        case OBJ_FUNCTION:
            return Call((ObjFunction*)value.ToObject(), argCount);
        case OBJ_NATIVE: 
        {
            ObjNative* native = (ObjNative*)value.ToObject();
            NativeFunc func = native->function;

            Value result = func(argCount, m_CurrentFiber->stack.m_Top - argCount);

            m_CurrentFiber->stack.m_Top -= argCount + 1; 

            m_CurrentFiber->stack.Push(result);
            return true; 
        }
        case OBJ_CLASS:
        {
            ObjClass* klass = (ObjClass*)value.ToObject();
            m_CurrentFiber->stack.m_Top[-argCount - 1] = Value(NewInstance(klass));

            auto it = klass->methods.find("construct");
            
            if (it != klass->methods.end())
            {
                return Call((ObjFunction*)it->second.ToObject(), argCount);
            }
            else if (argCount != 0)
            {
                Error("Expected 0 arguments in class constructor but got " + std::to_string(argCount));
            }

            return true; 
        }
        case OBJ_BOUND_METHOD:
        {
            ObjBoundMethod* bound = (ObjBoundMethod*)value.ToObject();
                
            m_CurrentFiber->stack.m_Top[-argCount - 1] = bound->reciever;
            if (bound->isNative)
            {
                ObjNative* native = bound->native;
                NativeFunc func = native->function;

                // For native functions we need to do something a bit more
                // Native functions here always have an argument. self 

                if (argCount != native->arity)
                {
                    // Arg counts do not match
                }

                bool moduleFunc = ((m_CurrentFiber->stack.m_Top) - argCount - 1)->IsObjType(OBJ_MODULE);
                

                Value result;
                if (!moduleFunc)
                    result = func(argCount + 1, (m_CurrentFiber->stack.m_Top) - argCount - 1);
                else 
                {
                    
                    result = func(argCount, (m_CurrentFiber->stack.m_Top) - argCount);

                }

                m_CurrentFiber->stack.m_Top -= argCount + 1;

                m_CurrentFiber->stack.Push(result);
                return true;
            }
            else
            {   
                return Call(bound->function, argCount);

            }
        }
        }

        return false; 

    }

    void VM::DefineMethod(const std::string& name)
    {
        Value method = m_CurrentFiber->stack.Peek(0);
        ObjClass* klass = (ObjClass*)m_CurrentFiber->stack.Peek(1).ToObject();
        klass->methods[name] = method;
        m_CurrentFiber->stack.Pop();
    }

    bool VM::Call(ObjFunction* function, int argCount)
    {
        if (m_CurrentFiber->framesCount == MaxCallFrames)
        {
            return false;
        }

        CallFrame* frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount++];
        frame->function = function; 
        frame->ip = function->chunk.code.data();

        // function->chunk.Dissassemble(function->name.c_str());



        frame->slots = m_CurrentFiber->stack.m_Top - argCount - 1;
        return true; 
    }

    ObjFiber* VM::ImportModule(const std::string& name, const std::string& asName)
    {
       
        std::string importName = asName.empty() ? name : asName;

        // Default modules 
        if (name == "std:io" || name == "std:maths" || name == "std:filesystem" || name == "std:json")
        {
            if (!asName.empty())
            {
                auto it = m_Modules.find(asName);

                ObjModule* mdl = nullptr;
                if (it != m_Modules.end())
                {
                    mdl = (ObjModule*)it->second.ToObject();
                }
                else 
                {
                    mdl = CreateModule(memoryManager.AllocateString(asName));

                    m_Modules[asName] = mdl;

                   
                }

                if (name == "std:io")
                    LoadStdIO(this, mdl);
                else if (name == "std:maths")
                    LoadStdMaths(this, mdl);
                else if (name == "std:filesystem")
                    LoadStdFilesystem(this, mdl);
                else if (name == "std:json")
                    LoadJsonModule(this, mdl);

                // Load it into the current global
                m_CurrentGlobal->operator[](asName) = Value(mdl);
                return nullptr;
            }

            if (name == "std:io")
                LoadStdIO(this, nullptr);
            else if (name == "std:maths")
                LoadStdMaths(this, nullptr);
            else if (name == "std:filesystem")
                LoadStdFilesystem(this, nullptr);
            else if (name == "std:json")
                LoadJsonModule(this, nullptr);


            return nullptr;
        }

        // TODO: Check if we are sandboxed

        // Check if the module is already loaded
        // TODO: This check isn't the best.. probably should fix it 
        //if (m_ExecutingModule == nullptr) 
        //{
            if (m_Modules.find(importName) != m_Modules.end())
            {
                return nullptr;
            }
        //}

        if (!asName.empty())
        {
            // Create a new module
            m_Modules[importName] = Value(CreateModule(memoryManager.AllocateString(importName.c_str())));

            ObjModule* mdl = (ObjModule*)m_Modules[importName].ToObject();

            // Set the current global to import into this scope of the module
            m_CurrentGlobal = &mdl->methods;
            mdl->caller = nullptr;

            if (m_ExecutingModule)
                mdl->caller = m_ExecutingModule;

            m_ExecutingModule = mdl;
        }
        else
        {
            // Make a nil entry
            // This means we are importing it into global scope
            m_Modules[importName] = Value();
        }
       

        std::string file = m_IOInterface->ReadFile(name + ".lang");

        if (file.empty())
        {
            
            Error("Cannot read file: " + name + ".lang");

            return nullptr; 
        }

        // Compile the module 
        ObjFunction* func = CompileScript(file);

        // Create a new fiber to run it
        ObjFiber* fiber = CreateFiber(func);
        fiber->caller = m_CurrentFiber;

        return fiber;

    }

    void VM::MarkObject(Object* obj)
    {

        if (obj->isMarked)
            return;

        obj->Mark();

        if (m_GreyStackOffset >= m_GreyStack.size())
            m_GreyStack.resize(m_GreyStackOffset + 1);

        m_GreyStack[m_GreyStackOffset] = obj;
        m_GreyStackOffset++;
    }

    void VM::MarkStringTable(std::unordered_map<std::string, ObjString*> strs)
    {
        for (auto& i : strs)
            MarkObject(i.second);
    }

    void VM::MarkRoots()
    {
        MarkObject(m_CurrentFiber);

        MarkTable(m_Modules);

        // If we are executing a module mark it
        if (m_ExecutingModule)
            MarkObject(m_ExecutingModule);

        for (Value* slot = m_CurrentFiber->stack.m_Stack; slot < m_CurrentFiber->stack.m_Top; slot++)
        {
            MarkValue(*slot);
        }

        for (int i = 0; i < m_CurrentFiber->framesCount; i++)
        {
            MarkObject(m_CurrentFiber->frames[i].function);
        }

        MarkTable(m_GlobalVariables);
        // MarkStringTable(memoryManager.m_Strings);
    }

    void VM::MarkValue(Value value)
    {
        if (value.IsObject())
        {
            MarkObject(((Object*)value.ToObject()));
        }
    }

    void VM::MarkTable(std::unordered_map<std::string, Value> table)
    {
        for (auto& i : table)
        {
            MarkValue(i.second);
        }
    }

    void VM::MarkTable(std::unordered_map<uint64_t, Value> table)
    {
        for (auto& i : table)
        {
            MarkValue(i.second);

            // If we have an embedded array or dictionary we want to loop through and mark them
            if (i.second.IsObjType(OBJ_DICTIONARY))
            {
                MarkTable(((ObjDictionary*)i.second.ToObject())->map);
            }
            else if (i.second.IsObjType(OBJ_ARRAY))
            {
                MarkArray(((ObjArray*)i.second.ToObject())->values, ((ObjArray*)i.second.ToObject())->size);
            }
        }
    }

    void VM::MarkArray(Value* arr, size_t length)
    {
        for (size_t i = 0; i < length; i++)
        {
            MarkValue(arr[i]);
        }
    }

    void VM::Error(const std::string& message)
    {
        printf("Error: %s", message.c_str());
    }

    void VM::CollectGarbage()
    {
        MarkRoots();


        TraceReferences();

        // Remove marked strings from string table

        for (auto& i : memoryManager.m_Strings)
        {
            if (!i.second->isMarked)
            {
                memoryManager.m_Strings.erase(i.first);
            }
        }

        Sweep();
    }

    void VM::TraceReferences()
    {
        while (m_GreyStackOffset > 0)
        {
            m_GreyStackOffset--;
            Object* obj = m_GreyStack[m_GreyStackOffset];

            BlackenObject(obj);
        }
    }

    void VM::BlackenObject(Object* obj)
    {
#ifdef VM_DEBUG_ALLOCS
        // printf("%p blacken \n", (void*)obj);
#endif

        // All objects can contain methods
        MarkTable(obj->methods);

        switch (obj->type)
        {
        case OBJ_STRING:
        case OBJ_NATIVE:
            break;
        case OBJ_FUNCTION:
        {

            ObjFunction* func = (ObjFunction*)obj;

            MarkArray(func->chunk.constants.data(), func->chunk.constants.size());
            MarkObject(func->name);

            break;
        }
        case OBJ_CLASS:
        {

            ObjClass* instance = (ObjClass*)obj;
            MarkObject(instance->name);

            break;
        }
        case OBJ_INSTANCE:
        {
            ObjInstance* instance = (ObjInstance*)obj;

            MarkObject(instance->klass);

            MarkTable(instance->fields);
            MarkTable(instance->methods);
            break;
        }
        case OBJ_BOUND_METHOD:
        {
            ObjBoundMethod* func = (ObjBoundMethod*)obj;
            
            if (!func->isNative)
                MarkObject(func->function);

            MarkValue(func->reciever);

            break;
        }
        case OBJ_ARRAY:
        {
            ObjArray* arr = (ObjArray*)obj;
            MarkArray(arr->values, arr->size);


            break;
        }
        case OBJ_MODULE: 
        {
            ObjModule* mdl = (ObjModule*)obj;
            MarkTable(mdl->methods);
            MarkObject(mdl->name);

            mdl = mdl->caller;
            while (mdl != nullptr)
            {
                MarkObject(mdl);

                mdl = mdl->caller;
            }

            break;
        }
        case OBJ_DICTIONARY:
        {
            ObjDictionary* dict = (ObjDictionary*)obj; 

            MarkTable(dict->map);

            break;
        }
        case OBJ_FIBER:
        {
            // Go up the caller list and mark them all 

            ObjFiber* curr = ((ObjFiber*)obj)->caller;

            while (curr != nullptr)
            {
                MarkObject(curr);

                curr = curr->caller;
            }
           

            break;
        }
        }
    }

    void VM::Sweep()
    {
        for (size_t i = 0; i < memoryManager.m_Allocations.size(); i++)
        {
            Object* obj = memoryManager.m_Allocations[i];

            if (!obj->isMarked)
            {

                memoryManager.FreeObject(obj);
                memoryManager.m_Allocations.erase(memoryManager.m_Allocations.begin() + i);
                i--;
            }
            else
            {
                obj->isMarked = false;
            }

        }
    }
}