
#include "Object.h"
#include "Value.h"

#include "Memory.h"

#include "Stack.h"

namespace script
{


	void ObjString::Delete()
	{
		Allocate(str, sizeof(char) * length, 0);
		str = nullptr;
		length = 0;
	}

	const std::string ObjString::ToString()
	{
		return std::string(str);
	}

	void ObjString::Append(ObjString* str2)
	{

		// This needs implementing properly
		assert(false);
		
		// Account for null terminator on string 1. 
		// String 2 will already have one so we can just copy that across
		size_t newLength = length + str2->length - 1;

		str = (char*)Allocate(str, length * sizeof(char), newLength * sizeof(char));

		size_t offset = length - 1;
		memcpy(&str[offset], str2->str, str2->length * sizeof(char));
		length = newLength;
	}

	ObjString* ObjString::AppendNew(ObjString* str2)
	{
		//size_t newLength = length + str2->length - 1;

		//// We don't have to worry about freeing this
		//// The GC will collect it and free on its next run 
		//// That was a lot of debugging...
		//ObjString* newStr = memoryManager.AllocateObject<ObjString>();
		//newStr->type = OBJ_STRING;
		//newStr->str = (char*)Allocate(nullptr, 0, newLength * sizeof(char));
		//newStr->length = newLength;

		//memcpy(newStr->str, str, (length - 1) * sizeof(char));
		//memcpy(&newStr->str[length - 1], str2->str, str2->length * sizeof(char));

		// TODO: This probably isn't the best way of doing it
		// Granted the two strings are destroyed after leeaving this scope
		ObjString* interned = memoryManager.AllocateString(std::string(str) + std::string(str2->str));

		// memoryManager.FreeObject(newStr);

		return interned;
	}


	ObjArray* AllocateArray(const std::vector<Value>& a)
	{
		ObjArray* arr = memoryManager.AllocateObject<ObjArray>();
		arr->type = OBJ_ARRAY;

		if (a.size() > 0)
		{
			arr->values = (Value*)Allocate(nullptr, 0, sizeof(Value) * a.size());
			memcpy(arr->values, a.data(), sizeof(Value) * a.size());
			arr->size = a.size();
		}
		else
		{
			arr->values = nullptr;
			arr->size = 0;
		}
		
		

		arr->methods["length"] = Value(NewNativeFunction([=](int argCount, script::Value* args) {
			
			// our first argument should be the self value

			ObjArray* arrObj = (ObjArray*)args[0].ToObject();

			return Value((double)arrObj->size);
		}, 0));

		// TODO: These
		/*arr->methods["remove"] = Value(NewNativeFunction([=](int argCount, script::Value* args) {

			assert(argCount == 2);

			ObjArray* arrObj = (ObjArray*)args[0].ToObject();

			

			return Value();
		}, 0));

		arr->methods["insert"] = Value(NewNativeFunction([=](int argCount, script::Value* args) {

			assert(argCount == 3);

			ObjArray* arrObj = (ObjArray*)args[0].ToObject();

			arrObj->values.insert(arrObj->values.begin() + (size_t)args[1].ToNumber(), args[2]);

			return Value();
			
		}, 0));

		*/arr->methods["append"] = Value(NewNativeFunction([=](int argCount, script::Value* args) {

			assert(argCount == 2);

			ObjArray* arrObj = (ObjArray*)args[0].ToObject();

			arrObj->PushBack(args[1]);

			return Value();

			}, 0));

		
		return arr;
	}

	void ObjArray::Delete()
	{
		Allocate(this->values, size * sizeof(Value), 0);
		this->size = 0;
	}

	void ObjArray::PushBack(Value value)
	{
		size_t newSize = size + 1;
		values = (Value*)Allocate(values, sizeof(Value) * size, sizeof(Value) * newSize);
		values[size] = value;

		this->size = newSize;
	}

	void ObjArray::Append(ObjArray* arr)
	{
		size_t newSize = size + arr->size;

		values = (Value*)Allocate(values, sizeof(Value) * size, sizeof(Value) * newSize);

		memcpy(&values[size], arr->values, sizeof(Value) * arr->size);

		size = newSize;
	}

	ObjRange* CreateRange()
	{
		ObjRange* range = memoryManager.AllocateObject<ObjRange>();
		range->type = OBJ_RANGE;

		range->from = 0.0;
		range->to = 0.0;
		range->step = 1.0;

		range->methods["expand"] = Value(NewNativeFunction([&](int argc, Value* args) {
			ObjRange* range = (ObjRange*)args[0].ToObject();

			double diff = range->to - range->from;
			size_t elements = (size_t)floor(diff / range->step);

			std::vector<Value> vec(elements);

			double s = range->from;
			for (size_t i = 0; i < elements; i++)
			{
				vec[i] = s;
				s += range->step;
			}

			ObjArray* arr = AllocateArray(vec);

			return Value(arr);
		}, 0));
			
		return range;
	}

	ObjDictionary* AllocateDictionary()
	{
		ObjDictionary* dict = memoryManager.AllocateObject<ObjDictionary>();
		dict->type = OBJ_DICTIONARY;

		dict->methods["put"] = Value(NewNativeFunction([&](int argCount, Value* args) {

			// Get self
			ObjDictionary* d = (ObjDictionary*)args[0].ToObject();

			d->map[args[1].Hash()] = args[2];

			return Value();
		}, 2));

		dict->methods["get"] = Value(NewNativeFunction([&](int argCount, Value* args) {

			// Get self
			ObjDictionary* d = (ObjDictionary*)args[0].ToObject();

			auto it = d->map.find(args[1].Hash());
			if (it != d->map.end())
			{
				return it->second;
			}

			return Value();
			}, 2));

		return dict;
	}

	ObjFunction* NewFunction()
	{
		ObjFunction* func = memoryManager.AllocateObject<ObjFunction>();
		func->arity = 0;
		func->type = OBJ_FUNCTION;
		func->name = memoryManager.AllocateString("placeholder");

		return func;
	}

	ObjClass* NewClass(const std::string& name)
	{
		ObjClass* klass = memoryManager.AllocateObject<ObjClass>();

		klass->type = OBJ_CLASS;
		klass->name = memoryManager.AllocateString(name);

		return klass;
	}

	ObjInstance* NewInstance(ObjClass* klass)
	{
		ObjInstance* instance = memoryManager.AllocateObject<ObjInstance>();

		instance->type = OBJ_INSTANCE;
		instance->klass = klass;
		instance->methods = klass->methods;

		return instance;
	}

	void ObjInstance::Delete()
	{
	}

	ObjModule* CreateModule(ObjString* name)
	{
		ObjModule* mdl = memoryManager.AllocateObject<ObjModule>();

		mdl->type = OBJ_MODULE;
		mdl->name = name;
		mdl->caller = nullptr;

		return mdl;
		
	}

	void ObjModule::AddNativeFunction(const std::string& name, NativeFunc func, int arity)
	{
		methods[name] = Value(NewNativeFunction(func, arity));
	}

	void ObjUserData::Delete() {
		if (destructor)
			destructor(userData);	
	}

	ObjUserData* CreateUserData()
	{
		ObjUserData* d = memoryManager.AllocateObject<ObjUserData>();
		d->type = OBJ_USER_DATA;

		return d;
	}

}