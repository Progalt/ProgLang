
#pragma once
#include "Value.h"

namespace script
{

	constexpr uint32_t UINT8_COUNT = UINT8_MAX + 1;

	constexpr uint32_t MaxCallFrames = 64;

	constexpr size_t StackMax = (MaxCallFrames * UINT8_COUNT);


	class Stack
	{
	public:

		Stack()
		{
			m_Stack = new Value[StackMax];
			m_Top = m_Stack;
		}

		~Stack()
		{
			delete[] m_Stack;
		}

		void Push(Value value)
		{

			*m_Top = value;

			/*printf("Stack Push: ");
			m_Top->Print();
			printf("\n");*/

			m_Top++;
		}

		Value Pop()
		{

			m_Top--;

			/*printf("Stack Pop: ");
			m_Top->Print();
			printf("\n");*/

			return *m_Top;
		}


		Value Peek(int distance)
		{
			return m_Top[-1 - distance];
		}

		Value& operator[](size_t idx)
		{
			return m_Stack[idx];
		}

		Value* m_Stack;
		Value* m_Top;


		size_t m_StackCapacity = 0;

	};
}