
#include "Chunk.h"
#include "Value.h"
#include "Memory.h"

namespace script
{
	Chunk::~Chunk()
	{
	}

	void Chunk::WriteByte(uint8_t byte)
	{
		code.push_back(byte);
	}

	size_t Chunk::WriteConstant(Value value)
	{

		size_t idx = AddConstant(value);


		WriteByte(OP_CONSTANT);
		WriteByte((uint8_t)idx);

		return idx;
	}

	size_t Chunk::AddConstant(Value value)
	{
		constants.push_back(value);

		size_t idx = constants.size() - 1;

		if (idx > 255 && idx <= 65536)
		{
			WriteByte(OP_CONSTANT_LONG);

			uint8_t higher = uint8_t(idx >> 8);
			uint8_t lower = uint8_t(idx & 0xFF);

			WriteByte(higher);
			WriteByte(lower);

			return idx;
		}
		else if (idx > 65536)
		{
			assert(false && "Constant numbers have hit the max per chunk");
		}

		return idx;
	}

	void Chunk::Dissassemble(const char* name)
	{
		printf("===== Chunk: %s ===== \n", name);
		for (size_t offset = 0; offset < code.size();)
		{
			auto simpleInstruction = [&](const char* name)
				{
					printf("%s \n", name);
					offset += 1;
				};

			auto constantInstruction = [&](const char* name)
				{
					uint8_t constant = code[offset + 1];
					printf("%s %d -> ", name, constant);
					constants[constant].Print();
					printf("\n");
					offset += 2;
				};

			auto constantInstructionLong = [&](const char* name)
				{
					uint8_t higher = code[offset + 1];
					uint8_t lower = code[offset + 2];

					uint16_t constant = higher << 8;
					constant = constant | lower;

					printf("%s %d -> ", name, constant);
					constants[constant].Print();
					printf("\n");
					offset += 3;
				};


			auto byteInstruction = [&](const char* name)
				{
					uint8_t higher = code[offset + 1];


					printf("%s %d\n", name, higher);
					offset += 3;
				};

			auto byteInstructionLong = [&](const char* name)
				{
					uint8_t higher = code[offset + 1];
					uint8_t lower = code[offset + 2];

					uint16_t constant = higher << 8;
					constant = constant | lower;

					printf("%s %d\n", name, constant);
					offset += 2;
				};


			uint8_t instruction = code[offset];
			switch (instruction)
			{
			case OP_RETURN:
				simpleInstruction("OP_RETURN");
				break;
			case OP_CONSTANT:
				constantInstruction("OP_CONSTANT");
				break;
			case OP_CONSTANT_LONG:
				constantInstructionLong("OP_CONSTANT_LONG");
				break;
			case OP_NEGATE:
				simpleInstruction("OP_NEGATE");
				break;
			case OP_ADD:
				simpleInstruction("OP_ADD");
				break;
			case OP_SUBTRACT:
				simpleInstruction("OP_SUBTRACT");
				break;
			case OP_MULTIPLY:
				simpleInstruction("OP_MULTIPLY");
				break;
			case OP_DIVIDE:
				simpleInstruction("OP_DIVIDE");
				break;
			case OP_TRUE:
				simpleInstruction("OP_TRUE");
				break;
			case OP_FALSE:
				simpleInstruction("OP_FALSE");
				break;
			case OP_NIL:
				simpleInstruction("OP_NIL");
				break;
			case OP_NOT:
				simpleInstruction("OP_NOT");
				break;
			case OP_EQUAL:
				simpleInstruction("OP_EQUAL");
				break;
			case OP_LESS:
				simpleInstruction("OP_LESS");
				break;
			case OP_GREATER:
				simpleInstruction("OP_GREATER");
				break;
			case OP_POP:
				simpleInstruction("OP_POP");
				break;
			case OP_DEFINE_GLOBAL:
				constantInstructionLong("OP_DEFINE_GLOBAL");
				break;
			case OP_GET_GLOBAL:
				constantInstructionLong("OP_GET_GLOBAL");
				break;
			case OP_SET_GLOBAL:
				constantInstructionLong("OP_SET_GLOBAL");
				break;
			case OP_EXPORT_GLOBAL:
				constantInstructionLong("OP_EXPORT_GLOBAL");
				break;
			case OP_GET_LOCAL:
				byteInstructionLong("OP_GET_LOCAL");
				break;
			case OP_SET_LOCAL:
				byteInstructionLong("OP_SET_LOCAL");
				break;
			case OP_JUMP_IF_FALSE:
				byteInstructionLong("OP_JUMP_IF_FALSE");
				break;
			case OP_JUMP:
				byteInstructionLong("OP_JUMP");
				break;
			case OP_LOOP:
				byteInstructionLong("OP_LOOP");
				break;
			case OP_CALL:
				byteInstruction("OP_CALL");
				break;
			case OP_POWER:
				simpleInstruction("OP_POWER");
				break;
			case OP_MODULO:
				simpleInstruction("OP_MODULO");
				break;
			case OP_CREATE_LIST:
				byteInstructionLong("OP_CREATE_LIST");
				break;
			case OP_SUBSCRIPT_READ:
				simpleInstruction("OP_SUBSCRIPT_READ");
				break;
			case OP_SUBSCRIPT_WRITE:
				simpleInstruction("OP_SUBSCRIPT_WRITE");
				break;
			case OP_SLICE_ARRAY:
				simpleInstruction("OP_SLICE_ARRAY");
				break;
			default:
				printf("Unknown OpCode -> %d\n", instruction);
				offset++;
			}

		}
	}

	void DisassembleInstruction(Chunk* chunk, size_t offset)
	{
		auto simpleInstruction = [&](const char* name)
			{
				printf("%s \n", name);
				offset += 1;
			};

		auto constantInstruction = [&](const char* name)
			{
				uint8_t constant = chunk->code[offset + 1];
				printf("%s %d -> ", name, constant);
				chunk->constants[constant].Print();
				printf("\n");
				offset += 2;
			};

		auto constantInstructionLong = [&](const char* name)
			{
				uint8_t higher = chunk->code[offset + 1];
				uint8_t lower = chunk->code[offset + 2];

				uint16_t constant = higher << 8;
				constant = constant | lower;

				printf("%s %d -> ", name, constant);
				chunk->constants[constant].Print();
				printf("\n");
				//offset += 2;
			};


		auto byteInstruction = [&](const char* name)
			{
				uint8_t higher = chunk->code[offset + 1];


				printf("%s %d\n", name, higher);
				//offset += 3;
			};

		auto byteInstructionLong = [&](const char* name)
			{
				uint8_t higher = chunk->code[offset + 1];
				uint8_t lower = chunk->code[offset + 2];

				uint16_t constant = higher << 8;
				constant = constant | lower;

				printf("%s %d\n", name, constant);
				//offset += 2;
			};

		auto invokeInstruction = [&](const char* name)
			{
				uint8_t higher = chunk->code[offset + 1];
				uint8_t lower = chunk->code[offset + 2];
				uint8_t argCount = chunk->code[offset + 3];

				uint16_t constant = higher << 8;
				constant = constant | lower;

				printf("%s %d -> args: %d\n", name, constant, argCount);
				//offset += 3;
			};

		

		uint8_t instruction = chunk->code[offset];
		
		switch (instruction)
		{
		case OP_RETURN:
			simpleInstruction("OP_RETURN");
			break;
		case OP_CONSTANT:
			constantInstruction("OP_CONSTANT");
			break;
		case OP_CONSTANT_LONG:
			constantInstructionLong("OP_CONSTANT_LONG");
			break;
		case OP_NEGATE:
			simpleInstruction("OP_NEGATE");
			break;
		case OP_ADD:
			simpleInstruction("OP_ADD");
			break;
		case OP_SUBTRACT:
			simpleInstruction("OP_SUBTRACT");
			break;
		case OP_MULTIPLY:
			simpleInstruction("OP_MULTIPLY");
			break;
		case OP_DIVIDE:
			simpleInstruction("OP_DIVIDE");
			break;
		case OP_TRUE:
			simpleInstruction("OP_TRUE");
			break;
		case OP_FALSE:
			simpleInstruction("OP_FALSE");
			break;
		case OP_NIL:
			simpleInstruction("OP_NIL");
			break;
		case OP_NOT:
			simpleInstruction("OP_NOT");
			break;
		case OP_EQUAL:
			simpleInstruction("OP_EQUAL");
			break;
		case OP_LESS:
			simpleInstruction("OP_LESS");
			break;
		case OP_GREATER:
			simpleInstruction("OP_GREATER");
			break;
		case OP_POP:
			simpleInstruction("OP_POP");
			break;
		case OP_DEFINE_GLOBAL:
			constantInstructionLong("OP_DEFINE_GLOBAL");
			break;
		case OP_GET_GLOBAL:
			constantInstructionLong("OP_GET_GLOBAL");
			break;
		case OP_SET_GLOBAL:
			constantInstructionLong("OP_SET_GLOBAL");
			break;
		case OP_EXPORT_GLOBAL:
			constantInstructionLong("OP_EXPORT_GLOBAL");
			break;
		case OP_GET_LOCAL:
			byteInstructionLong("OP_GET_LOCAL");
			break;
		case OP_SET_LOCAL:
			byteInstructionLong("OP_SET_LOCAL");
			break;
		case OP_JUMP_IF_FALSE:
			byteInstructionLong("OP_JUMP_IF_FALSE");
			break;
		case OP_JUMP:
			byteInstructionLong("OP_JUMP");
			break;
		case OP_LOOP:
			byteInstructionLong("OP_LOOP");
			break;
		case OP_CALL:
			byteInstruction("OP_CALL");
			break;
		case OP_POWER:
			simpleInstruction("OP_POWER");
			break;
		case OP_MODULO:
			simpleInstruction("OP_MODULO");
			break;
		case OP_CREATE_LIST:
			byteInstructionLong("OP_CREATE_LIST");
			break;
		case OP_SUBSCRIPT_READ:
			simpleInstruction("OP_SUBSCRIPT_READ");
			break;
		case OP_SUBSCRIPT_WRITE:
			simpleInstruction("OP_SUBSCRIPT_WRITE");
			break;
		case OP_SLICE_ARRAY:
			simpleInstruction("OP_SLICE_ARRAY");
			break;
		case OP_CLASS:
			byteInstructionLong("OP_CLASS");
			break;
		case OP_SET_PROPERTY:
			byteInstructionLong("OP_SET_PROPERTY");
			break;
		case OP_GET_PROPERTY:
			byteInstructionLong("OP_GET_PROPERTY");
			break;
		case OP_METHOD:
			constantInstructionLong("OP_METHOD");
			break;
		case OP_STRING_INTERP:
			byteInstruction("OP_STRING_INTERP");
			break;
		case OP_IMPORT_MODULE:
			//byteInstruction("OP_IMPORT_MODULE");
			printf("OP_IMPORT_MODULE\n");
			break;
		case OP_IMPORT_MODULE_AS:
			//byteInstruction("OP_IMPORT_MODULE");
			printf("OP_IMPORT_MODULE_AS\n");
			break;
		case OP_ITER:
			printf("OP_ITER\n");
			break;
		case OP_CREATE_RANGE:
			printf("OP_CREATE_RANGE\n");
			break;
		default:
			printf("Unknown OpCode -> %d\n", instruction);
			offset++;
		}

	}
}