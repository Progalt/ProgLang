
#pragma once
#include <string>

#include "Chunk.h"
#include <functional>


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

		virtual const std::string ToString() { return "nil"; }

		// Any object can contain methods
		std::unordered_map<std::string, Value> methods; 

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

		const std::string ToString() override;
	};



	class ObjArray : public Object
	{
	public:

		void Delete() override;

		Value* values;
		size_t size;

		void PushBack(Value value);

		void Append(ObjArray* arr);

	private:
	};

	ObjArray* AllocateArray(const std::vector<Value>& a);

	class ObjRange : public Object
	{
	public:

		double from;
		double to;
		double step;

	};

	ObjRange* CreateRange();

	class ObjDictionary : public Object
	{
	public:

		// TODO: Make this a non C++ object

		// We take the uint64_t hash from a value for this
		std::unordered_map<uint64_t, Value> map;

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
	};

	using NativeFunc = std::function<Value( int argCount, Value* args)>;

	class ObjNative : public Object
	{
	public:

		int arity = 0;
		NativeFunc function;

	private:
	};

	ObjFunction* NewFunction();

	inline ObjNative* NewNativeFunction(NativeFunc func, int arity)
	{
		ObjNative* native = new ObjNative;
		native->function = func;
		native->arity = arity;
		native->type = OBJ_NATIVE;
		return native;
	}

	class ObjClass : public Object
	{
	public:

		ObjString* name;


	private:
	};

	ObjClass* NewClass(const std::string& name);

	class ObjInstance : public Object
	{
	public:

		void Delete() override;

		ObjClass* klass;
		std::unordered_map<std::string, Value> fields;

	};

	ObjInstance* NewInstance(ObjClass* klass);


	class ObjModule : public Object
	{
	public:

		ObjString* name;

		ObjModule* caller = nullptr;

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
		FIBER_ROOT, 
	};




}