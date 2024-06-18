
#pragma once

#include "Object.h"
#include "Value.h"

#include <fstream>
#include <sstream>

namespace script
{

	// This interface determines how the VM can access the hardware
	class IOInterface
	{
	public:

		virtual std::string ReadFile(const std::string& filepath) = 0;

		virtual void Print(const std::string& str) = 0;

	private:
	};

	class DefaultIOInterface : public IOInterface
	{
	public:

		std::string ReadFile(const std::string& filepath) override
		{
			std::ifstream t(filepath);

			if (!t.is_open())
			{
				return "";
			}

			std::stringstream buffer;
			buffer << t.rdbuf();

			return buffer.str();
		}
		
		void Print(const std::string& str) override
		{
			// Just a simple call to printf 
			printf(str.c_str());
		}
	};

	// This class interface helps bind native classes to VM
	class ClassInterface
	{
	public:
		
		// Add a constructor to the class
		// This called on class instance creation
		void AddConstructor(NativeFunc func, int arity)
		{
			assert(m_Ptr);

			m_Ptr->methods["construct"] = Value(NewNativeFunction(func, arity));
		}

		void AddMethod(const std::string& name, NativeFunc func, int arity)
		{
			assert(m_Ptr);

			m_Ptr->methods[name] = Value(NewNativeFunction(func, arity)); 
		}

	private:

		friend class VM;

		ObjClass* m_Ptr; 
	};

}