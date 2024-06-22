
#pragma once

#include <cstdint>
#include <vector>

namespace script
{
#define OPCODE(name) OP_##name,

	enum OpCodes
	{
#include "OpCodes.h"
	};

#undef OPCODE 

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