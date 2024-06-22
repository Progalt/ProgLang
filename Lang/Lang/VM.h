
#pragma once

#include "Chunk.h"
#include "Value.h"
#include <unordered_map>
#include <functional>
#include <tuple>
#include "Interface.h"
#include "Memory.h"
#include "Stack.h"

#include "Vendor/unordered_dense.h"

namespace script
{
	// Yes this shouldn't be here but I get compiler errors due to value otherwise
	// So yeah....
	// TODO: FIX
	class ObjBoundMethod : public Object
	{
	public:

		Value reciever;
		bool isNative = false;
	
		ObjFunction* function = nullptr;
		ObjNative* native = nullptr;
		
		
	};

	inline ObjBoundMethod* NewBoundMethod(Value reciever, ObjFunction* function)
	{
		ObjBoundMethod* instance = memoryManager.AllocateObject<ObjBoundMethod>();

		instance->type = OBJ_BOUND_METHOD;
		instance->reciever = reciever;
		instance->function = function;
		instance->isNative = false;

		return instance;
	}

	inline ObjBoundMethod* NewNativeBoundMethod(Value reciever, ObjNative* function)
	{
		ObjBoundMethod* instance = memoryManager.AllocateObject<ObjBoundMethod>();

		instance->type = OBJ_BOUND_METHOD;
		instance->reciever = reciever;
		instance->native = function;
		instance->isNative = true;

		return instance;
	}

	class ObjFiber : public Object
	{
	public:

		void Delete() override
		{
			// stack.~Stack();

			if (frames)
				delete[] frames;
		}

		Stack stack;

		CallFrame* frames = nullptr;
		size_t framesCount = 0;
		size_t framesCapacity = 0;

		ObjFiber* caller = nullptr;

		FiberState state = FIBER_UNKNOWN;
	};

	inline ObjFiber* CreateFiber(ObjFunction* func)
	{
		ObjFiber* fiber = memoryManager.AllocateObject<ObjFiber>();

		fiber->type = OBJ_FIBER;
		fiber->frames = (CallFrame*)Allocate(nullptr, 0, sizeof(CallFrame) * 64);
		fiber->framesCapacity = 64;

		Value val{};
		val.MakeObject(func);
		fiber->stack.Push(val);

		CallFrame* frame = &fiber->frames[fiber->framesCount++];
		frame->function = func;
		frame->ip = func->chunk.code.data();
		frame->slots = fiber->stack.m_Stack;

		return fiber;
	}

	enum InterpretResult
	{
		INTERPRET_ALL_GOOD, 
		INTERPRET_COMPILE_ERROR,
		INTERPRET_RUNTIME_ERROR
	};

	struct VMCreateInfo
	{
		IOInterface* ioInterface = nullptr;
		bool sandboxed = false;
	};

	class VM
	{
	public:

		VM(const VMCreateInfo& createInfo = VMCreateInfo());

		~VM();

		InterpretResult Interpret(ObjFunction* function);

		InterpretResult Run();

		void DumpGlobalVariables()
		{
			for (auto& v : m_GlobalVariables)
			{
				printf("Global: %s = ", v.first.c_str());
				v.second.Print();
				printf("\n");
			}
		}

		void DumpExportedVariables()
		{
			printf("Exported Variables: \n");
			for (auto& v : m_ExportedVariables)
			{
				printf("Exported Global: %s = ",v.c_str());
				m_GlobalVariables[v].Print();
				printf("\n");
			}
		}

		const std::vector<std::string>& GetExportedVariables()
		{
			return m_ExportedVariables;
		}

		Value* GetGlobal(const std::string& name)
		{
			if (m_GlobalVariables.find(name) != m_GlobalVariables.end())
			{
				return &m_GlobalVariables[name];
			}

			return nullptr;
		}

		
		Value& operator[](const std::string& name)
		{
			return m_GlobalVariables[name];
		}

		void AddNativeFunction(const std::string& name, NativeFunc func, int arity)
		{
			m_GlobalVariables[name] = Value(NewNativeFunction(func, arity));
		}

		ClassInterface AddNativeClass(const std::string& name)
		{
			ClassInterface classInterface{};

			m_GlobalVariables[name] = Value(NewClass(name));

			classInterface.m_Ptr = (ObjClass*)m_GlobalVariables[name].ToObject();

			return classInterface;
		}

		// Pauses the current execution of code and calls this fiber
		// Result of the fiber can be retrieved 
		void ExecuteFiber(ObjFiber* fiber);

	private:

		IOInterface* m_IOInterface = nullptr;


		void Error(const std::string& message);

		bool CallValue(Value value, int argCount);

		bool Call(ObjFunction* function, int argCount);

		bool Invoke(const std::string& name, int argCount);


		void DefineMethod(const std::string& name);

		ankerl::unordered_dense::map<std::string, Value> m_GlobalVariables;

		ankerl::unordered_dense::map<std::string, Value>* m_CurrentGlobal;

		
		std::vector<std::string> m_ExportedVariables;

		ankerl::unordered_dense::map<std::string, Value> m_Modules;

		ObjFiber* m_CurrentFiber = nullptr;
		ObjModule* m_ExecutingModule = nullptr;


		ObjFiber* ImportModule(const std::string& name, const std::string& asName = "");

		// Memory Related things

		void MarkRoots();
		void MarkObject(Object* obj);
		void MarkValue(Value value);
		void MarkTable(ankerl::unordered_dense::map<std::string, Value> table);
		void MarkTable(ankerl::unordered_dense::map<uint64_t, Value> table);
		void MarkStringTable(ankerl::unordered_dense::map<std::string, ObjString*> strs);
		void MarkArray(Value* arr, size_t length);

		void CollectGarbage();

		void TraceReferences();
		void BlackenObject(Object* obj);
		void Sweep();

		size_t m_GreyStackOffset = 0;
		std::vector<Object*> m_GreyStack;

		
	};
}