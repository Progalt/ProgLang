
#pragma once
#include "Lexer.h"
#include <assert.h>
#include "Object.h"

// Only do NAN Boxing on release
// Better for debugging this way
#ifndef _DEBUG
#define NAN_BOXING
#endif

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)

#define TAG_NIL   1
#define TAG_FALSE 2
#define TAG_TRUE  3 

#define NIL_VAL         ((uint64_t)(QNAN | TAG_NIL))
#define FALSE_VAL       ((uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((uint64_t)(QNAN | TAG_TRUE))

#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)

#define OBJ_VAL(obj) \
    (uint64_t)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

#define AS_OBJ(value) \
    ((Object*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define IS_OBJ(value) \
    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#endif

namespace script
{
	enum ValueType
	{
		VALUE_NIL, 
		VALUE_NUMBER,
		VALUE_BOOL,
		
		VALUE_OBJ,

		VALUE_TRUE,
		VALUE_FALSE,
	};

	class VM;

	class Value
	{
	public:

#ifdef NAN_BOXING
		uint64_t value;
#else

		ValueType type;

		union
		{
			double number;
			Object* object;
		};
#endif

		Value() 
		{
			MakeNil();
		}

		Value(bool value)
		{
			MakeBool(value);
		}

		Value(double num)
		{
			MakeNumber(num);
		}

		Value(Object* obj)
		{
			MakeObject(obj);
		}

		// Helper functions to create a Value 

		void MakeNil()
		{
#ifdef NAN_BOXING

			value = NIL_VAL;

#else
			type = VALUE_NIL;
#endif
		}

		void MakeBool(bool value)
		{
#ifdef NAN_BOXING

			this->value = BOOL_VAL(value);

#else
			type = value ? VALUE_TRUE : VALUE_FALSE;
#endif
		}

		void MakeNumber(double num)
		{
#ifdef NAN_BOXING

			memcpy(&value, &num, sizeof(double));

#else
			type = VALUE_NUMBER;
			number = num;
#endif
		}

		void MakeObject(Object* obj)
		{
#ifdef NAN_BOXING

			value = OBJ_VAL(obj);

#else
			type = VALUE_OBJ;
			object = obj;
#endif
		}

		// Type Checkers

		const bool IsNil() const
		{
#ifdef NAN_BOXING

			return value == NIL_VAL;

#else
			return type == VALUE_NIL;
#endif
		}


		const bool IsBool() const
		{
#ifdef NAN_BOXING

			return (((value) | 1) == TRUE_VAL);

#else
			return type == VALUE_FALSE || type == VALUE_TRUE;
#endif
		}

		const bool IsObjType(ObjectType type) const
		{
#ifdef NAN_BOXING

			if (!IS_OBJ(value))
				return false;

			Object* obj = ToObject();

			return obj->type == type;

#else

			return (this->type == VALUE_OBJ && object->type == type);
#endif
		}

		ObjectType GetObjectType() 
		{
			
#ifdef NAN_BOXING

			if (!IS_OBJ(value))
				return OBJ_NONE;

			Object* obj = ToObject();

			return obj->type;

#else
			if (IsObject())
				return object->type;

			return OBJ_NONE; 
#endif
		}

		const bool IsObject() const 
		{
#ifdef NAN_BOXING
			return IS_OBJ(value);
#else
			return type == VALUE_OBJ;
#endif
		}

		const bool IsNumber() const
		{
#ifdef NAN_BOXING
			return (((value) & QNAN) != QNAN);
#else
			return (type == VALUE_NUMBER);
#endif

		}

		const double ToNumber() const
		{
#ifdef NAN_BOXING

			double num;
			memcpy(&num, &value, sizeof(value));

			return num;

#else
			return number;
#endif
		}

		Object* ToObject() const
		{
#ifdef NAN_BOXING

			return AS_OBJ(value);

#else 

			return object;
#endif
		}

		const bool AsBool() const
		{
#ifdef NAN_BOXING

			return ((value) == TRUE_VAL);

#else 
			return (type == VALUE_TRUE);
#endif
		}

		// Hash function
		uint64_t Hash()
		{
#ifdef NAN_BOXING
			return value;
#else

			if (IsObject())
			{
				// Object hashes are done seperately

				Object* obj = ToObject();

				if (obj->type == OBJ_STRING)
				{
					// Strings are done here for speed
					
					return std::hash<std::string>{}(std::string( ((ObjString*)obj)->str ));

				}

				// Other objects have custom hash functions typically defined by the user or through language code

				auto hashingMethod = obj->methods.find("hash");

				if (hashingMethod != obj->methods.end())
				{
					// We have a hashing method
				}

			}
			else if (IsNumber())
			{
				return std::hash<double>{}(ToNumber());
			}
			else if (IsBool())
			{
				return std::hash<bool>{}(AsBool());
			}
			
			// We can't hash a nil object
			return 0;
#endif
		}

		// Print

		void Print()
		{
#ifdef NAN_BOXING

			if (IsBool())
			{
				printf(AsBool() ? "true" : "false");
			}
			else if (IsNil())
			{
				printf("nil");
			}
			else if (IsNumber())
			{
				printf("%g", ToNumber());
			}
			else if (IS_OBJ(value))
			{
				Object* obj = ToObject();

				if (obj->type == OBJ_STRING)
					printf("%s", ((ObjString*)obj)->str);

				if (obj->type == OBJ_ARRAY)
				{
					printf("[ ");

					ObjArray* arr = ((ObjArray*)obj);
					for (size_t idx = 0 ; idx < arr->size; idx++)
					{
						arr->values[idx].Print();

						if (idx != arr->size - 1)
							printf(", ");

					}

					printf(" ]");
				}

				if (obj->type == OBJ_CLASS)
				{
					ObjClass* klass = ((ObjClass*)obj);

					printf("%s", klass->name->str);
				}

				if (obj->type == OBJ_INSTANCE)
				{
					ObjInstance* klass = ((ObjInstance*)obj);

					printf("Instance of %s", klass->klass->name->str);
				}
			}

#else 
			switch (type)
			{
			case VALUE_NIL:
				printf("nil");
				break;
			case VALUE_TRUE:
				printf("true");
				break;
			case VALUE_FALSE:
				printf("false");
				break;
			case VALUE_NUMBER:
				printf("%g", number);
				break;
			case VALUE_OBJ:

				if (IsObjType(OBJ_STRING))
					printf("%s", ((ObjString*)object)->str);

				if (IsObjType(OBJ_ARRAY))
				{
					printf("[ ");

					ObjArray* arr = ((ObjArray*)object);
					for (size_t idx = 0; idx < arr->size; idx++)
					{
						arr->values[idx].Print();

						if (idx != arr->size - 1)
							printf(", ");

					}

					printf(" ]");
				}

				if (IsObjType(OBJ_CLASS))
				{
					ObjClass* klass = ((ObjClass*)object);

					printf("%s", klass->name->str);
				}

				if (IsObjType(OBJ_INSTANCE))
				{
					ObjInstance* klass = ((ObjInstance*)object);

					printf("Instance of %s", klass->klass->name->str);
				}

				if (IsObjType(OBJ_DICTIONARY))
				{
					ObjDictionary* dict = ((ObjDictionary*)object);

					for (auto& [key, val] : dict->map)
					{
						val.Print();
					}
				}

				if (IsObjType(OBJ_FUNCTION))
				{
					ObjFunction* func = (ObjFunction*)object;
					
					printf("%s", func->name->str);
				}

				if (IsObjType(OBJ_RANGE))
				{
					ObjRange* range = (ObjRange*)object;

					printf("%g..%g", range->from, range->to);
				}

				break;
			}

#endif

		}
		

		bool operator==(const Value& rh)
		{
#ifdef NAN_BOXING
			if (IsObject() && rh.IsObject())
			{
				// TODO: String interning
				// We don't do it so we have to manually compare strings 

				Object* obj = ToObject();
				Object* obj2 = rh.ToObject();

				if (obj->type != obj2->type)
					return false;

				if (obj->type == OBJ_STRING)
					return strcmp(((ObjString*)obj)->str, ((ObjString*)obj2)->str) == 0;

				if (obj->type == OBJ_ARRAY)
				{
					ObjArray* arr = (ObjArray*)obj;
					ObjArray* arr2 = (ObjArray*)obj2;

					if (arr->size != arr2->size)
						return false;

					for (size_t i = 0; i < arr->size; i++)
						if (!(arr->values[i] == arr2->values[i]))
							return false;

					return true;
				}

				// Return false here if we reach it
				return false;
			}
			else
				return value == rh.value;
#else
			if (type != rh.type) return false;
			if (type == VALUE_NUMBER) return (number == rh.number);
			return (object == rh.object);
#endif
		}

		const std::string& ToCppString()
		{
			if (IsNumber())
			{
				return std::to_string(ToNumber());
			}

			else if (IsBool())
			{
				return AsBool() ? "true" : "false";
			}

			else if (IsNil())
			{
				return "nil";
			}

			else if (IsObject())
			{
				Object* obj = ToObject();
				return obj->ToString();
			}
		}

	};

	inline const Value operator-(Value& rh)
	{
		

		rh.MakeNumber(-rh.ToNumber());

		return rh;
	}

	inline const Value operator+(Value& lh, Value& rh)
	{
		if (lh.IsObjType(OBJ_ARRAY))
		{
			if (rh.IsObjType(OBJ_ARRAY))
			{
				ObjArray* arr1 = (ObjArray*)lh.ToObject();
				ObjArray* arr2 = (ObjArray*)rh.ToObject();

				/*for (auto& a : arr2->values)
				{
					arr1->values.push_back(a);
				}*/

				// arr1->values.insert(arr1->values.end(), arr2->values.begin(), arr2->values.end());

				/*for (size_t i = 0; i < arr2->size; i++)
				{
					arr1->PushBack(arr2->values[i]);
				}*/

				arr1->Append(arr2);

			}
			else
			{
				ObjArray* arr = (ObjArray*)lh.ToObject();
				arr->PushBack(rh);
				

			}


			return lh;
		}
		else if (lh.IsObjType(OBJ_STRING) && rh.IsObjType(OBJ_STRING))
		{
			ObjString* str1 = (ObjString*)lh.ToObject();
			ObjString* str2 = (ObjString*)rh.ToObject();


			return Value(str1->AppendNew(str2));
			

		}
		

		

		double n1 = lh.ToNumber();
		double n2 = rh.ToNumber();

		lh.MakeNumber(n1 + n2);


		return lh;
	}

	inline const Value operator-(Value& lh, Value& rh)
	{

		double n1 = lh.ToNumber();
		double n2 = rh.ToNumber();

		lh.MakeNumber(n1 - n2);

		return lh;
	}

	inline const Value operator*(Value& lh, Value& rh)
	{

		double n1 = lh.ToNumber();
		double n2 = rh.ToNumber();

		lh.MakeNumber(n1 * n2);

		return lh;
	}

	inline const Value operator/(Value& lh, Value& rh)
	{
		

		double n1 = lh.ToNumber();
		double n2 = rh.ToNumber();

		lh.MakeNumber(n1 / n2);

		return lh;
	}

	static const Value operator>(Value& lh, Value& rh)
	{
		double n1 = lh.ToNumber();
		double n2 = rh.ToNumber();

		Value ret{};
		ret.MakeBool(n1 > n2);
		return ret;
	}

	static const Value operator<(Value& lh, Value& rh)
	{
		double n1 = lh.ToNumber();
		double n2 = rh.ToNumber();

		Value ret{};
		ret.MakeBool(n1 < n2);
		return ret;
	}

	// TODO: Make this better 
	// Large refactor


#if 0 
	class Value
	{
	public:

		Value(Token tk)
		{
			Assign(tk);
		}

		static Value MakeNil()
		{
			return Value({ TK_NIL });
		}

		static Value MakeBool(bool value)
		{
			if (value)
				return Value({ TK_TRUE });

			return Value({ TK_FALSE });
		}

		static Value MakeNumber(double num)
		{
			Value v;

			v.m_Type = VALUE_NUMBER;
			v.m_Value.number = num;

			return v;
		}

		static Value MakeObject(Object* obj)
		{
			Value v;
			v.m_Type = VALUE_OBJ;
			v.m_Value.object = obj;

			return v;
		}

		Value()
		{
			m_Type = VALUE_NIL;
		}

		bool Assign(Token tk)
		{
			
			switch (tk.type)
			{
			case VALUE_OBJ:
				m_Type = VALUE_OBJ;
				break;
			case TK_NUMBER:
				m_Type = VALUE_NUMBER;
				m_Value.number = std::stod(tk.value);
				break;
			case TK_TRUE:
				m_Type = VALUE_BOOL;
				m_Value.b = true;
				break;
			case TK_FALSE:
				m_Type = VALUE_BOOL;
				m_Value.b = false;
				break;
			default:
				m_Type = VALUE_NIL;
				break;
			}

				return true;
		
			
		}

		const ValueType GetValueType() const
		{
			return m_Type;
		}

		bool CanAssign(ValueType type)
		{
			return (type == GetValueType());
		}

		const bool IsNil() const { return (m_Type == VALUE_NIL); }

		const bool IsBool() const { return (m_Type == VALUE_BOOL); }

		const bool IsObj() const { return (m_Type == VALUE_OBJ); }

		const bool IsObjType(ObjectType type) const
		{
			return IsObj() && (m_Value.object->type == type);
		}
		
		double AsNumber()
		{
			assert(m_Type == VALUE_NUMBER && "Value is not a number so cannot be returned as a number");

			return m_Value.number;
		}

		void SetNumber(double d)
		{
			assert(m_Type == VALUE_NUMBER && "Value is not a number so cannot be set as a number");

			m_Value.number = d;
		}

		bool AsBool()
		{
			assert(m_Type == VALUE_BOOL && "Value is not a bool so cannot be returned as a bool");
			return m_Value.b;
		}

		void SetBool(bool b)
		{
			assert(m_Type == VALUE_BOOL && "Value is not a bool so cannot be set as a bool");
			m_Value.b = b;
		}

		Object* AsObject()
		{
			return m_Value.object;
		}

		void Print()
		{
			switch (m_Type)
			{
			case VALUE_NUMBER:
				printf("%f", m_Value.number);
				break;
			case VALUE_BOOL:
				printf("%s", m_Value.b ? "true" : "false");
				break;
			case VALUE_OBJ:

				if (IsObjType(OBJ_STRING))
					printf("%s", ((ObjString*)m_Value.object)->str.c_str());

				if (IsObjType(OBJ_ARRAY))
				{
					printf("[ ");

					ObjArray* arr = ((ObjArray*)m_Value.object);
					size_t idx = 0;
					for (auto i : arr->values)
					{
						i.Print();

						if (idx != arr->values.size() - 1)
							printf(", ");

						idx++;
					}

					printf(" ]");
				}

				if (IsObjType(OBJ_CLASS))
				{
					ObjClass* klass = ((ObjClass*)m_Value.object);

					printf("%s", klass->name.c_str());
				}

				if (IsObjType(OBJ_INSTANCE))
				{
					ObjInstance* klass = ((ObjInstance*)m_Value.object);

					printf("Instance of %s", klass->klass->name.c_str());
				}

				break;
			default:
				printf("nil");
				break;
			}
		}

		const std::string& Type()
		{
			switch (m_Type)
			{
			case VALUE_NUMBER:
				return "Number";
			case VALUE_BOOL:
				return "Boolean";
			case VALUE_OBJ:
			{
				if (IsObjType(OBJ_STRING))
				{
					return "String";
				}
				else if (IsObjType(OBJ_ARRAY))
				{
					return "List";
				}
				else if (IsObjType(OBJ_FUNCTION))
				{
					return "Function";
				}
				else if (IsObjType(OBJ_NATIVE))
				{
					return "Native Function";
				}

				break;
			}
			}

			return "nil";
		}

		bool operator==(const Value& rh)
		{
			if (m_Type == rh.m_Type)
			{
				switch (m_Type)
				{
				case VALUE_NIL:
					return true;
					break;
				case VALUE_NUMBER:
					return (m_Value.number == rh.m_Value.number);
					break;
				case VALUE_BOOL:
					return (m_Value.b == rh.m_Value.b);
					break;
				case VALUE_OBJ:
					if (IsObjType(OBJ_STRING))
						return (((ObjString*)m_Value.object)->str == ((ObjString*)rh.m_Value.object)->str);
					break;
				}
			
			}

			return false;
		}


	private:

		union
		{
			double number =  0.0;
			bool b;

			Object* object;
		} m_Value;

		ValueType m_Type = VALUE_NIL;
	};

	inline const Value operator-(Value& rh)
	{
		assert(rh.GetValueType() == VALUE_NUMBER);

		rh.SetNumber(-rh.AsNumber());

		return rh;
	}

	inline const Value operator+(Value& lh, Value& rh)
	{
		if (lh.IsObjType(OBJ_ARRAY))
		{
			if (rh.IsObjType(OBJ_ARRAY))
			{
				ObjArray* arr1 = (ObjArray*)lh.AsObject();
				ObjArray* arr2 = (ObjArray*)rh.AsObject();

				for (auto& a : arr2->values)
				{
					arr1->values.push_back(a);
				}
			}
			else
			{
				ObjArray* arr = (ObjArray*)lh.AsObject();
				arr->values.push_back(rh);
			}


			return lh;
		}

		assert(rh.GetValueType() == VALUE_NUMBER);

		double n1 = lh.AsNumber();
		double n2 = rh.AsNumber();

		lh.SetNumber(n1 + n2);


		return lh;
	}

	inline const Value operator-(Value& lh, Value& rh)
	{
		assert(rh.GetValueType() == VALUE_NUMBER);

		double n1 = lh.AsNumber();
		double n2 = rh.AsNumber();

		lh.SetNumber(n1 - n2);

		return lh;
	}

	inline const Value operator*(Value& lh, Value& rh)
	{
		assert(rh.GetValueType() == VALUE_NUMBER);

		double n1 = lh.AsNumber();
		double n2 = rh.AsNumber();

		lh.SetNumber(n1 * n2);

		return lh;
	}

	inline const Value operator/(Value& lh, Value& rh)
	{
		assert(rh.GetValueType() == VALUE_NUMBER);

		double n1 = lh.AsNumber();
		double n2 = rh.AsNumber();

		lh.SetNumber(n1 / n2);

		return lh;
	}

	static const Value operator>(Value& lh, Value& rh)
	{
		double n1 = lh.AsNumber();
		double n2 = rh.AsNumber();

		return Value::MakeBool(n1 > n2);
	}

	static const Value operator<(Value& lh, Value& rh)
	{
		double n1 = lh.AsNumber();
		double n2 = rh.AsNumber();

		return Value::MakeBool(n1 < n2);
	}

#endif
}