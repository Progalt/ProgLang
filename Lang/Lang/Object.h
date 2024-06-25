
#pragma once
#include <string>

#include "Chunk.h"
#include <functional>
#include "Vendor/unordered_dense.h"

namespace script
{


	enum ObjectType
	{
		OBJ_NONE,

		OBJ_STRING,

		OBJ_FUNCTION,

		OBJ_NATIVE,

		OBJ_ARRAY,

		OBJ_CLASS, 

		OBJ_INSTANCE, 

		OBJ_BOUND_METHOD, 

		OBJ_DICTIONARY, 

		OBJ_MODULE, 

		OBJ_FIBER,

		OBJ_USER_DATA, 

		OBJ_RANGE, 
	};

	

	class Object
	{
	public:

		ObjectType type = OBJ_NONE;

		virtual void Delete() {} 

		virtual std::string ToString() { return "nil"; }


		bool isMarked = false;

		void Mark()
		{
			isMarked = true;

		}
	};


	class ObjString : public Object
	{
	public:

		// std::string str = "";

		void Delete() override;

		char* str = nullptr;
		size_t length = 0;

		// Apends to the current string object
		void Append(ObjString* str2);

		// Appends to a new string object that gets allocated
		// This is more what you want for a + operator
		ObjString* AppendNew(ObjString* str2);

		std::string ToString() override { return "string"; }
	};



	class ObjArray : public Object
	{
	public:

		void Delete() override;

		Value* values = nullptr;
		size_t size = 0;

		void PushBack(Value value);

		void Append(ObjArray* arr);

		std::string ToString() override { return "list"; }

	private:
	};

	ObjArray* AllocateArray(const std::vector<Value>& a);

	class ObjRange : public Object
	{
	public:

		double from = 0.0;
		double to = 0.0;
		double step = 1.0;

		std::string ToString() override { return "range"; }

	};

	ObjRange* CreateRange();

	class ObjDictionary : public Object
	{
	public:

		// TODO: Make this a non C++ object

		// We take the uint64_t hash from a value for this
		ankerl::unordered_dense::map<uint64_t, Value> map;

		std::string ToString() override { return "dictionary"; }

	private:
	};

	ObjDictionary* AllocateDictionary();

	enum FunctionType
	{
		TYPE_FUNCTION,
		TYPE_METHOD,
		TYPE_INITIALIZER, 
		TYPE_SCRIPT
		
	};
	 
	class ObjFunction : public Object
	{
	public:

		Chunk chunk;
		int arity = 0;
		ObjString* name;

		bool async = false;

		std::string ToString() override { return "function"; }
	};

	using NativeFunc = std::function<Value( int argCount, Value* args)>;

	class ObjNative : public Object
	{
	public:

		int arity = 0;
		NativeFunc function;

		std::string ToString() override { return "native function"; }

	private:
	};

	ObjFunction* NewFunction();

	inline ObjNative* NewNativeFunction(NativeFunc func, int arity)
	{
		ObjNative* native = new ObjNative();
		native->function = func;
		native->arity = arity;
		native->type = OBJ_NATIVE;
		return native;
	}

	class ObjClass : public Object
	{
	public:

		ObjString* name;
		ankerl::unordered_dense::map<std::string, Value> methods;

		std::string ToString() override { return std::string(name->str); }

	private:
	};

	ObjClass* NewClass(const std::string& name);

	class ObjInstance : public Object
	{
	public:

		void Delete() override;

		ObjClass* klass;
		ankerl::unordered_dense::map<std::string, Value> fields;

		// We just return the base class to string
		std::string ToString() override { return klass->ToString(); }

	};

	ObjInstance* NewInstance(ObjClass* klass);


	class ObjModule : public Object
	{
	public:

		ObjString* name;

		ObjModule* caller = nullptr;

		ankerl::unordered_dense::map<std::string, Value> methods;

		void AddNativeFunction(const std::string& name, NativeFunc func, int arity);
	};

	ObjModule* CreateModule(ObjString* name);

	class ObjUserData : public Object
	{
	public:

		void Delete() override; 

		void* userData;
		std::function<void(void*)> destructor;
	};

	ObjUserData* CreateUserData();

	struct CallFrame
	{
		ObjFunction* function;
		uint8_t* ip;
		Value* slots;
	};

	enum FiberState
	{
		FIBER_UNKNOWN,

		FIBER_ROOT, 

		// Being run as an async function
		FIBER_ASYNC,
	};





}