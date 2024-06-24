
#include "VM.h"
#include "Memory.h"
#include "Compiler.h"

#include <fstream>
#include <sstream>

#include "Libraries/std.h"

namespace script
{


    VM::VM(const VMCreateInfo& createInfo) : m_CurrentGlobal(&m_GlobalVariables)
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

        CallFrame* frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
        m_CurrentGlobal = &m_GlobalVariables;

        // Store the ptr in a value to avoid the ptr->ptr
        // With how frequent this gets access its important
        uint8_t* ip = frame->ip;

        Stack* stack = &m_CurrentFiber->stack;

        // Does this have any benefit? 
       Value* constantTable = frame->function->chunk.constants.data();

        // Welcome to define hell

#define READ_BYTE() (*ip++)
#define READ_SHORT() \
    (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (constantTable[READ_BYTE()])
#define READ_CONSTANT_LONG() (constantTable[READ_SHORT()])

        // Stack functions
#define PUSH(val) (*(stack->m_Top++) = (val))
#define POP() (*(--stack->m_Top))
#define PEEK(off) (stack->m_Top[-1 - off])

#define BINARY_OP(op) \
    do { \
      Value b = POP(); \
      Value a = POP(); \
      PUSH(a op b); \
    } while (false)

       auto handleEvent = [&](Event evnt)
           {
               switch (evnt.type)
               {
               case EVENT_PUSH_FIBER:
               {
                   ObjFiber* newFiber = evnt.fiber;

                   newFiber->caller = m_CurrentFiber;

                   m_CurrentFiber = newFiber;

                   frame->ip = ip;
                   frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
                   ip = frame->ip;
                   constantTable = frame->function->chunk.constants.data();
                   stack = &m_CurrentFiber->stack;
                   break;
               }
               case EVENT_POP_FIBER:
               {
                   if (m_CurrentFiber == nullptr || m_CurrentFiber->caller == nullptr)
                       break;
                   
                   m_CurrentFiber = m_CurrentFiber->caller;

                   frame->ip = ip;
                   frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
                   ip = frame->ip;
                   constantTable = frame->function->chunk.constants.data();
                   stack = &m_CurrentFiber->stack;

                   break;
               }
               case EVENT_TRIGGER_GC:
               {

                   CollectGarbage(); 
                   memoryManager.m_NextGC = memoryManager.m_BytesAllocated * GC_HEAP_GROW_FACTOR; 
                   
                   break;
               }
              
               }
           };

       auto eventLoop = [&]() {
           eventManager.TranslateMessages();

           while(!eventManager.IsEmpty())
               handleEvent(eventManager.Pop());
       };

        uint8_t instruction = 0;

        // Define the stack trace
//#define DEBUG_STACK_TRACE
#ifdef DEBUG_STACK_TRACE
        auto stackTrace = [&]() {

            if (m_CurrentFiber == nullptr)
                return;

            printf("          ");
            for (Value* slot = m_CurrentFiber->stack.m_Stack; slot < m_CurrentFiber->stack.m_Top; slot++) {
                printf("[ ");
                slot->Print();
                printf(" ]");
            }
            printf("\n");

            DisassembleInstruction(&frame->function->chunk, (int)(ip - frame->function->chunk.code.data()));
            };


#define STACK_TRACE() stackTrace()
#else 
#define STACK_TRACE()
#endif


        // This references how wren does computed gotos 
        // https://github.com/wren-lang/wren/blob/main/src/vm/wren_vm.c

        // Computer goto is not supported on MSVC++ 
        // Clang does support it though 
#if defined(_MSC_VER) && !defined(__clang__)
    #define COMPUTED_GOTO 0
#else
    #define COMPUTED_GOTO 1
#endif

        // Computed goto uses dispatch tables to jump
        // Its a decent speed up over regular switches
#if COMPUTED_GOTO 

        static void* dispatchTable[] = {
    #define OPCODE(name) &&code_##name,
    #include "OpCodes.h"
    #undef OPCODE
        };

#define CASE_CODE(name) code_##name


#define DISPATCH()  \
do { \
        STACK_TRACE(); \
        eventLoop(); \
        goto *dispatchTable[instruction = READ_BYTE()]; \
} while (false)

#define INTERPRET_LOOP DISPATCH();

#else

#define CASE_CODE(name) case OP_##name

#define INTERPRET_LOOP  \
        loop:  \
            STACK_TRACE(); \
            eventLoop(); \
            switch(instruction = READ_BYTE())

#define DISPATCH() goto loop

    
#endif
	
        // Begin the VM interpret loop

        INTERPRET_LOOP
        {
        CASE_CODE(CONSTANT): 
        {
            PUSH(READ_CONSTANT());

            DISPATCH();
        }
        CASE_CODE(CONSTANT_LONG):
        {
            PUSH(READ_CONSTANT_LONG());

            DISPATCH();
        }
        CASE_CODE(NEGATE): {
            // TODO: Make this better
            Value vl = POP();
            vl.MakeNumber(-vl.ToNumber());
            PUSH(vl);
            DISPATCH();
        }
        CASE_CODE(NIL):
            PUSH(Value());
            DISPATCH();
        CASE_CODE(TRUE):
            PUSH(Value(true));
            DISPATCH();
        CASE_CODE(FALSE):
            PUSH(Value(false));
            DISPATCH();
        CASE_CODE(NOT):
        {
            Value v = POP();
            v.MakeBool(!v.AsBool());
            PUSH(v);

            DISPATCH();
        }
        CASE_CODE(EQUAL):
        {
            Value a = POP();
            Value b = POP();

            PUSH(Value(a == b));

            DISPATCH();
        }
        CASE_CODE(LESS):
        {
            Value a = POP();
            Value b = POP();

            PUSH(Value(a > b));

            DISPATCH();
        }
        CASE_CODE(GREATER):
        {
            Value a = POP();
            Value b = POP();

            PUSH(Value(a < b));

            DISPATCH();
        }
        CASE_CODE(ADD):
        {
            Value b = POP();
            Value a = POP();

            if (a.IsObject())
            {
                if (a.IsObjType(OBJ_ARRAY))
                {
                    assert(false);
                    DISPATCH();
                }
                else if (a.IsObjType(OBJ_STRING) && b.IsObjType(OBJ_STRING))
                {

                    ObjString* str1 = (ObjString*)a.ToObject();
                    ObjString* str2 = (ObjString*)b.ToObject();

                    PUSH(Value(str1->AppendNew(str2)));

                    DISPATCH();
                }
            }

            double anum = a.ToNumber();
            double bnum = b.ToNumber();

            a.MakeNumber(anum + bnum);

            PUSH(a);

            DISPATCH();
        }
        CASE_CODE(SUBTRACT): 
        {

            double bnum = POP().ToNumber();
            double anum = POP().ToNumber();

            PUSH(Value(anum - bnum));

            DISPATCH();
        }
        CASE_CODE(MULTIPLY): BINARY_OP(*); DISPATCH();
        CASE_CODE(DIVIDE):   BINARY_OP(/); DISPATCH();
        CASE_CODE(POWER): 
        {
            Value b = POP();
            Value a = POP();

            double power = std::pow(a.ToNumber(), b.ToNumber());

            a.MakeNumber(power);
            PUSH(a);

            DISPATCH();
        }
        CASE_CODE(MODULO):
        {
            Value b = POP();
            Value a = POP();

            double remainder = std::remainder(a.ToNumber(), b.ToNumber());
            PUSH(Value(remainder));

            DISPATCH();
        }
        CASE_CODE(POP): 
        {
            POP(); 
            DISPATCH();
        }
        CASE_CODE(DEFINE_GLOBAL):
        {
            // uint16_t constant = READ_SHORT();

            ObjString* name = (ObjString*)READ_CONSTANT_LONG().ToObject();

            m_CurrentGlobal->operator[](name->str) = POP();

            DISPATCH();
        }

        CASE_CODE(EXPORT_GLOBAL):
        {
            // Export is the same as define global 
            // But we add it to a list of exported variables so its easier to get later. 

            // uint16_t constant = READ_SHORT();

            ObjString* name = (ObjString*)READ_CONSTANT_LONG().ToObject();

            m_CurrentGlobal->operator[](name->str) = POP();

            m_ExportedVariables.push_back(name->str);

            DISPATCH();
        }
        CASE_CODE(CREATE_LIST):
        {
            {
                uint16_t size = READ_SHORT();

                std::vector<Value> value(size);

                for (uint16_t i = 0; i < size; i++)
                {
                    Value val = POP();

                    value[size - i - 1] = val;
                }

                PUSH(Value(AllocateArray(value)));
            }
            DISPATCH();
        }
        CASE_CODE(CREATE_RANGE):
        {
            Value end = POP();
            Value start = POP();

            ObjRange* range = CreateRange();
            range->from = start.ToNumber();
            range->to = end.ToNumber();

            PUSH(Value(range));

            DISPATCH();
        }
        CASE_CODE(SUBSCRIPT_READ):
        {
            Value idx = POP();
            Value arr = POP();

            if (arr.IsObjType(OBJ_DICTIONARY))
            {
                ObjDictionary* dict = (ObjDictionary*)arr.ToObject();
                {
                    auto it = dict->map.find(idx.Hash());
                    if (it != dict->map.end())
                    {
                        PUSH(it->second);
                    }
                    else
                    {
                        Error("Dictionary does not have key");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }

                DISPATCH();

            }

            if (!idx.IsNumber())
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
                    PUSH(obj->values[idxInt]);
                else
                {
                    uint64_t absIdx = (-idxInt);//% obj->values.size();



                    PUSH(obj->values[obj->size - absIdx]);
                }
            }
            else
            {
                assert("Implement string indexing");


            }

            DISPATCH();
        }
        CASE_CODE(SUBSCRIPT_WRITE):
        {
            Value item = POP();
            Value idx = POP();
            Value arr = POP();

            if (arr.IsObjType(OBJ_DICTIONARY))
            {
                ObjDictionary* dict = (ObjDictionary*)arr.ToObject();

                dict->map[idx.Hash()] = item;
                    

                DISPATCH();
            }

            if (!idx.IsNumber())
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

                if (idxInt < (int64_t)obj->size)
                {
                    obj->values[idxInt] = item;

                    PUSH(item);
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

            DISPATCH();
        }
        CASE_CODE(SLICE_ARRAY):
        {
            {
                Value idx2 = POP();
                Value idx = POP();
                Value arr = POP();

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

                PUSH(Value(AllocateArray(value)));
            }
            DISPATCH();
        }
        CASE_CODE(GET_GLOBAL):
        {

            // uint16_t constant = READ_SHORT();

            ObjString* name = (ObjString*)READ_CONSTANT_LONG().ToObject();

            {
                auto it = m_CurrentGlobal->find(name->str);

                if (it == m_CurrentGlobal->end())
                {
                    Error("cannot get global variable: \"" + std::string(name->str) + "\" as it does not exist.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                PUSH(it->second);
            }


            DISPATCH();
        } 
        CASE_CODE(SET_GLOBAL):
        {
            // uint16_t constant = READ_SHORT();

            ObjString* name = (ObjString*)READ_CONSTANT_LONG().ToObject();

            {
                auto it = m_CurrentGlobal->find(name->str);
                if (it == m_CurrentGlobal->end())
                {
                    Error("cannot set global variable: " + std::string(name->str) + " as it does not exist.");
                    return INTERPRET_RUNTIME_ERROR;
                }


                it->second = PEEK(0);
            }

            DISPATCH();
        }
        CASE_CODE(GET_LOCAL):
        {
            uint16_t constant = READ_SHORT();

            PUSH(frame->slots[constant]);

            DISPATCH();
        }
        CASE_CODE(SET_LOCAL):
        {
            uint16_t constant = READ_SHORT();

            frame->slots[constant] = PEEK(0);


            DISPATCH();
        }
        CASE_CODE(JUMP_IF_FALSE):
        {
            uint16_t offset = READ_SHORT();

            if (PEEK(0).IsBool())
            {
                if (PEEK(0).AsBool() == false)
                {
                    ip += offset;
                }
            }
            else if (PEEK(0).IsNil())
            {
                ip += offset;
            }

            DISPATCH();
        }
        CASE_CODE(JUMP):
        {
            uint16_t offset = READ_SHORT();
            ip += offset;
            DISPATCH();
        }
        CASE_CODE(LOOP):
        {
            uint16_t offset = READ_SHORT();
            ip -= offset;

            DISPATCH();
        }
        CASE_CODE(CALL):
        {
            int argCount = READ_BYTE();
            if (!CallValue(PEEK(argCount), argCount))
            {
                Error("Could not call function");
                return INTERPRET_RUNTIME_ERROR;
            }

            frame->ip = ip;
            frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
            ip = frame->ip;
            constantTable = frame->function->chunk.constants.data();
            stack = &m_CurrentFiber->stack;
               

            DISPATCH();
        }
        CASE_CODE(CLASS):
        {
            //uint16_t constant = READ_SHORT();

            ObjString* name = (ObjString*)READ_CONSTANT_LONG().ToObject();


            PUSH(Value(NewClass(name->str)));

            DISPATCH();
        }
        CASE_CODE(GET_PROPERTY):
        {

            // Any object could have methods
            //uint16_t constant = READ_SHORT();

            ObjString* name = (ObjString*)READ_CONSTANT_LONG().ToObject();

            // TODO: Make this better
            // Its quite slow 
            {
                Value stackObj = PEEK(0);
                Object* obj = (Object*)stackObj.ToObject();
                auto it = obj->methods.find(name->str);
                if (it != obj->methods.end())
                {
                    // Check if its a class or module
                    // We just push these onto the stack instead of binding to methods
                    if (it->second.IsObjType(OBJ_CLASS) || stackObj.IsObjType(OBJ_MODULE))
                    {
                        POP();
                        PUSH(it->second);
                    }
                    else
                    {
                        ObjBoundMethod* bound = nullptr;

                        if (it->second.IsObjType(OBJ_FUNCTION))
                            bound = NewBoundMethod(stackObj, (ObjFunction*)it->second.ToObject());
                        else
                            bound = NewNativeBoundMethod(stackObj, (ObjNative*)it->second.ToObject());

                        POP();
                        PUSH(Value(bound));
                    }

                }
                else
                {

                    // Only instances can have fields
                    ObjInstance* instance = (ObjInstance*)obj;


                    auto itfield = instance->fields.find(name->str);
                    if (itfield == instance->fields.end())
                    {
                        Error("Field does not exist in instance: " + std::string(name->str));
                        return INTERPRET_RUNTIME_ERROR;
                    }

                    Value value = itfield->second;
                    POP();
                    PUSH(value);
                }
            }
            
            DISPATCH();
        }
        CASE_CODE(SET_PROPERTY):
        {
            if (!PEEK(1).IsObjType(OBJ_INSTANCE))
            {
                Error("Set field: Only instances of classes can have fields");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjInstance* instance = (ObjInstance*)PEEK(1).ToObject();

            //uint16_t constant = READ_SHORT();

            ObjString* name = (ObjString*)READ_CONSTANT_LONG().ToObject();
                
            instance->fields[name->str] = PEEK(0);

            Value value = POP();
            POP();
            PUSH(value);

            DISPATCH();
        }
        CASE_CODE(METHOD):
        {

            ObjString* name = (ObjString*)READ_CONSTANT_LONG().ToObject();
            DefineMethod(name->str);
            DISPATCH();
        }
        CASE_CODE(THROW):
        {
            // TODO: Throwing
                

            DISPATCH();
        }
        CASE_CODE(STRING_INTERP):
        {
            {
                uint8_t args = READ_BYTE();

                std::string output = "";

                for (uint8_t i = 0; i < args; i++)
                {
                    Value val = POP();

                    // TODO: This could probably be improved 

                    // String inputs are flipped to where they should be on the output so always prepend the string 

                    if (val.IsNumber())
                        output = std::to_string(val.ToNumber()) + output;
                    else if (val.IsBool())
                        output = (val.AsBool() ? "true" : "false") + output;
                    else if (val.IsObject())
                    {
                        Object* obj = val.ToObject();

                        // TODO: More object types
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

                PUSH(Value(memoryManager.AllocateString(output)));
            }
            DISPATCH();
        }
        CASE_CODE(ITER):
        {
            // Iteration using in

            Value* value = stack->m_Top - 3;
            Value seq = *(stack->m_Top - 2);
            Value* iterator = stack->m_Top - 1;

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
                    ip += jumpOffset;

                //*iterator = Value((double)(idx + 1));
                *value = arr->values[idx];
                iterator->MakeNumber((double)(idx + 1));
                break;
            }
            case OBJ_RANGE:
            {
                ObjRange* range = (ObjRange*)obj;

                if (it < range->from)
                    it = range->from;

                if (it >= range->to)
                    ip += jumpOffset;

                *value = it; 
                iterator->MakeNumber(it + range->step);

                break;
            }
            default:
                Error("Object does not support iteration.");

                return INTERPRET_RUNTIME_ERROR;
            }

            DISPATCH();
        }
        CASE_CODE(AWAIT):
        {
            
            Value v = POP();

            if (m_CurrentFiber->caller != nullptr)
            {
                ObjFiber* callee = m_CurrentFiber;

                // We want to go back to the caller
                // And setup a callback to notify what to do and to return to this

                Timer::GetInstance().StartTimer((uint64_t)v.ToNumber(), [callee] {
                    // printf("Timer done");

                    Event evnt{};
                    evnt.type = EVENT_PUSH_FIBER;
                    evnt.fiber = callee;

                    eventManager.Push(evnt);
                });

                m_CurrentFiber->stack = *stack;
                m_CurrentFiber = m_CurrentFiber->caller;
                stack = &m_CurrentFiber->stack;



                frame->ip = ip;
                frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
                ip = frame->ip;
                constantTable = frame->function->chunk.constants.data();
            }

            DISPATCH();
        }
        CASE_CODE(IMPORT_MODULE):
        {
            // uint16_t moduleNameConstant = READ_SHORT();

            ObjString* moduleName = (ObjString*)READ_CONSTANT_LONG().ToObject();

            ObjFiber* fiber = ImportModule(moduleName->str);

            if (fiber)
            {
                // We have a fiber now let's run it
                // Just make it be the current fiber we execute 
                m_CurrentFiber->stack = *stack;
                m_CurrentFiber = fiber;
                stack = &m_CurrentFiber->stack;
                frame->ip = ip;
                frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
                ip = frame->ip;
                constantTable = frame->function->chunk.constants.data();
            }

            DISPATCH();
        }
        CASE_CODE(IMPORT_MODULE_AS):
        {
            //uint16_t moduleNameConstant = READ_SHORT();
            //uint16_t asNameConstant = READ_SHORT();

            ObjString* moduleName = (ObjString*)READ_CONSTANT_LONG().ToObject();
            ObjString* asName = (ObjString*)READ_CONSTANT_LONG().ToObject();

            ObjFiber* fiber = ImportModule(moduleName->str, asName->str);

            if (fiber)
            {
                // We have a fiber now let's run it
                // Just make it be the current fiber we execute 
                m_CurrentFiber->stack = *stack;
                m_CurrentFiber = fiber;
                stack = &m_CurrentFiber->stack;
                frame->ip = ip;
                frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
                ip = frame->ip;
                constantTable = frame->function->chunk.constants.data();
            }

            DISPATCH();
        }
        CASE_CODE(RETURN): {

               

            Value result = POP();
            m_CurrentFiber->framesCount--;
            if (m_CurrentFiber->framesCount == 0)
            {
                POP();

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

                    m_CurrentFiber->stack = *stack;
                    m_CurrentFiber = m_CurrentFiber->caller;
                    stack = &m_CurrentFiber->stack;
                    // NOTE: This might break
                    // PUSH(result);

                    frame->ip = ip;
                    frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
                    ip = frame->ip;
                    constantTable = frame->function->chunk.constants.data();
                    DISPATCH();
                }
                else
                {
                    m_CurrentFiber = nullptr;
                    while (Timer::GetInstance().GetActiveTimerCount() > 0)
                    {
                        // Translate messages from the OS 
                        eventManager.TranslateMessages();

                        // If we aren't empty on events dispatch and handle them
                        if (!eventManager.IsEmpty())
                        {
                            DISPATCH();
                        }
                    }
                    return INTERPRET_ALL_GOOD;
                }
            }

                

            m_CurrentFiber->stack.m_Top = frame->slots;
            PUSH(result);
            frame->ip = ip;
            frame = &m_CurrentFiber->frames[m_CurrentFiber->framesCount - 1];
            ip = frame->ip;
            constantTable = frame->function->chunk.constants.data();
            stack = &m_CurrentFiber->stack;
               
            DISPATCH();
        }

        }



        return INTERPRET_ALL_GOOD;

        // Undef everything
#undef BINARY_OP 
#undef READ_CONSTANT
#undef READ_BYTE
#undef READ_CONSTANT_LONG
#undef POP
#undef PUSH
#undef DISPATCH
#undef CASE_CODE
#undef PEEK
#undef INTERPRET_LOOP
	}

    bool VM::CallValue(Value value, int argCount)
    {

        switch(value.GetObjectType())
        {
        case OBJ_FUNCTION:
        {
            if (!((ObjFunction*)value.ToObject())->async)
                return Call((ObjFunction*)value.ToObject(), argCount);
            else
            {
                // The function is an async function

                // Create a new fiber
                ObjFiber* fiber = CreateFiber((ObjFunction*)value.ToObject());

                // Set the caller to the current fiber
                fiber->caller = m_CurrentFiber;

                m_CurrentFiber = fiber;

                return true;
            }

            break;
        }
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
        default:

            break;
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


        frame->slots = m_CurrentFiber->stack.m_Top - argCount - 1;
        return true; 
    }

    ObjFiber* VM::ImportModule(const std::string& name, const std::string& asName)
    {
       
        std::string importName = asName.empty() ? name : asName;

        // Default modules 
        if (name == "std:io" || name == "std:maths" || name == "std:filesystem" || name == "std:json" || name == "std:time")
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
                else if (name == "std:time")
                    LoadStdTime(this, mdl);
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
            else if (name == "std:time")
                LoadStdTime(this, nullptr);
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

    void VM::MarkStringTable(ankerl::unordered_dense::map<std::string, ObjString*> strs)
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

    void VM::MarkTable(ankerl::unordered_dense::map<std::string, Value> table)
    {
        for (auto& i : table)
        {
            MarkValue(i.second);
        }
    }

    void VM::MarkTable(ankerl::unordered_dense::map<uint64_t, Value> table)
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

            /*mdl = mdl->caller;
            while (mdl != nullptr)
            {
                MarkObject(mdl);

                mdl = mdl->caller;
            }*/

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