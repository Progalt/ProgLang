
#pragma once
#include <unordered_map>
#include "Value.h"

// #define VM_DEBUG_ALLOCS

#define GC_HEAP_GROW_FACTOR 2

namespace script
{
	void* Allocate(void* ptr, size_t oldSize, size_t newSize);

	class MemoryManager
	{
	public:

		~MemoryManager()
		{
			for (auto i : m_Allocations)
			{
				FreeObject(i);
			}
		}

		ObjString* AllocateString(const std::string& str);

		ObjString* AllocateString(ObjString* str);


		template<typename _Ty>
		_Ty* AllocateObject()
		{
			_Ty* ptr = (_Ty*)Allocate(nullptr, 0, sizeof(_Ty));
#ifdef VM_DEBUG_ALLOCS
			// printf("%p Allocated of size %zu bytes of type: %s\n", (void*)ptr, sizeof(_Ty), typeid(_Ty).name());
#endif

			ptr = new(ptr) _Ty;

			m_Allocations.push_back(ptr);

			return ptr;
		}

		template<typename _Ty> 
		void FreeObject(_Ty* obj)
		{
			((Object*)obj)->Delete();

			// delete(obj);
#ifdef VM_DEBUG_ALLOCS
			// printf("%p Freed of type: %d\n", (void*)obj, obj->type);
#endif

			Allocate(obj, sizeof(_Ty), 0);
		}


		size_t m_BytesAllocated = 0;
		size_t m_NextGC = 1024 * 512;

		std::vector<Object*> m_Allocations;
		std::unordered_map<std::string, ObjString*> m_Strings;

		bool shouldCollectGarbage = false;

	};

	extern MemoryManager memoryManager;
}