
#pragma once

#include <cstdint>
#include <vector>

namespace script
{
	enum OpCodes
	{
		OP_RETURN,
		OP_CONSTANT,
		OP_CONSTANT_LONG,

		OP_NEGATE,
		OP_ADD,
		OP_SUBTRACT,
		OP_MULTIPLY, 
		OP_DIVIDE, 
		OP_POWER,
		OP_MODULO, 

		OP_FALSE,
		OP_TRUE,
		OP_NIL, 

		OP_NOT, 
		OP_INCREMENT, 
		OP_DECREMENT, 

		OP_EQUAL, 
		OP_GREATER,
		OP_LESS,

		OP_POP,
		OP_DEFINE_GLOBAL, 
		OP_GET_GLOBAL,
		OP_EXPORT_GLOBAL, 

		OP_SET_GLOBAL, 

		OP_GET_LOCAL,
		OP_SET_LOCAL, 

		OP_JUMP_IF_FALSE, 
		OP_JUMP,

		OP_LOOP, 

		OP_CALL, 

		OP_CREATE_LIST, 
		OP_SUBSCRIPT_READ, 
		OP_SUBSCRIPT_WRITE,

		OP_SLICE_ARRAY,

		OP_CLASS,

		OP_GET_PROPERTY,
		OP_SET_PROPERTY, 

		OP_METHOD, 

		OP_THROW,  

		OP_INVOKE, 

		OP_STRING_INTERP,

		OP_IMPORT_MODULE, 
		OP_IMPORT_MODULE_AS,

		OP_ITER, 

		OP_CREATE_RANGE, 
	};

	class Value;
	class Object;

	

	struct Chunk
	{
		std::vector<uint8_t> code;
		std::vector<Value> constants;

		~Chunk();


		void WriteByte(uint8_t byte);

		size_t WriteConstant(Value value);

		size_t AddConstant(Value value);

		void Dissassemble(const char* name);
	};

	void DisassembleInstruction(Chunk* chunk, size_t offset);
}