
#include "Memory.h"

namespace script
{
	MemoryManager memoryManager;

	void* Allocate(void* ptr, size_t oldSize, size_t newSize)
	{
		memoryManager.m_BytesAllocated += newSize - oldSize;

		if (newSize > oldSize)
		{

			if (memoryManager.m_BytesAllocated > memoryManager.m_NextGC)
			{
				memoryManager.shouldCollectGarbage = true;
			}
		}

		if (newSize == 0 && ptr != nullptr)
		{
#ifdef VM_DEBUG_ALLOCS
			printf("%p Freed of size %zu bytes\n", (void*)ptr, oldSize);
#endif
			free(ptr);
			return nullptr;
		}

		void* result = realloc(ptr, newSize);

#ifdef VM_DEBUG_ALLOCS
		printf("%p Allocated of new size %zu bytes\n", (void*)result, newSize - oldSize);
#endif

		if (result == nullptr)
		{
			// We failed the alloc this could be fatal
			exit(1);
		}

		return result;
	}


	ObjString* MemoryManager::AllocateString(const std::string& str)
	{
		// TODO: String interning

		auto it = m_Strings.find(str);
		if (it != m_Strings.end())
		{
			//printf("Interned string returned\n");
			return it->second;
		}

		ObjString* newStr = AllocateObject<ObjString>();
		newStr->type = OBJ_STRING;

		//newStr->str = str;

		newStr->str = (char*)Allocate(nullptr, 0, sizeof(char) * str.length() + 1);
		memcpy(newStr->str, str.data(), sizeof(char) * str.length());

		newStr->str[str.length()] = 0;
		newStr->length = str.length() + 1;

		m_Strings[str] = newStr;

		return newStr;
	}

	ObjString* MemoryManager::AllocateString(ObjString* str)
	{
		auto it = m_Strings.find(std::string(str->str));
		if (it != m_Strings.end())
		{
			//printf("Interned string returned\n");
			return it->second;
		}

		ObjString* newStr = AllocateObject<ObjString>();
		newStr->type = OBJ_STRING;

		newStr->str = (char*)Allocate(nullptr, 0, sizeof(char) * str->length);
		memcpy(newStr->str, str->str, sizeof(char) * str->length);
		newStr->length = str->length;

		m_Strings[std::string(str->str)] = newStr;

		return newStr;
	}

}