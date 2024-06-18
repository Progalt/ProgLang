
#pragma once
#include <vector>
#include "Value.h"
#include "Chunk.h"
#include "Lexer.h"
#include <functional>

namespace script
{

	enum Precedence
	{
		PREC_NONE,
		PREC_ASSIGNMENT,  // =
		PREC_OR,          // or
		PREC_AND,         // and
		PREC_EQUALITY,    // == !=
		PREC_COMPARISON,  // < > <= >=
		PREC_TERM,        // + -
		PREC_FACTOR,      // * /
		PREC_POWER, 
		PREC_UNARY,       // ! -
		PREC_CALL,        // . ()
		PREC_INDEX, 
		PREC_PRIMARY
	};


	struct ParseRule
	{
		std::function<void(bool)> prefix;
		std::function<void(bool)> infix;
		Precedence precedence;
	};

	struct Local
	{
		Token name;
		uint32_t depth;
	};

	struct Parser
	{
		std::vector<Token> tokens;
		size_t tokenOffset;

		Token previous;
		Token current;

		bool hadError = false;
		bool panicMode = false;
	};

	// This function takes an input source string, compiles it and outputs a ObjFunction pointer
	// containing the compiled bytecode 
	ObjFunction* CompileScript(const std::string& source);

	class Compiler
	{
	public:

		~Compiler();

		void InitCompiler(Compiler* enclosing, FunctionType type);

		ObjFunction* EndCompiler();

		ObjFunction* Compile(FunctionType type);

	private:

		void SetRules();

		Chunk* GetCurrentChunk()
		{
			return &m_Function->chunk;
		}


		Compiler* m_Enclosing;

		ObjFunction* m_Function;
		FunctionType m_FunctionType;

		uint32_t m_LocalCount = 0;
		uint32_t m_ScopeDepth = 0;
		Local* m_Locals;

		void MarkInitialised();

		// Byte code function 

		void EmitByte(uint8_t byte);
		void EmitReturn();
		void EmitBytes(uint8_t byte1, uint8_t byte2);
		void EmitConstant(Value value);

		// Parsing

		void Expression();
		void Number(bool canAssign);
		void Grouping(bool canAssign);
		void Unary(bool canAssign);
		void Binary(bool canAssign);
		void Literal(bool canAssign);
		void String(bool canAssign);
		void Statement();
		void Declaration();
		void VariableDeclaration(bool exportVar);
		void Variable(bool canAssign);
		void IfStatement();
		void WhileStatement();
		void ForStatement();
		void Call(bool canAssign);
		void FunctionDeclaration();
		void ReturnStatement();
		void Array(bool canAssign);
		void ArrayAccess(bool canAssign);
		void ClassDeclaration();
		void Dot(bool canAssign);
		void Self(bool canAssign);
		void ImportStatement();

		void Function(FunctionType type);

		void Method();

		uint8_t ArgumentList();

		void And(bool canAssign);
		void Or(bool canAssign);

		int EmitJump(uint8_t instruction);
		void EmitLoop(int loopStart);

		void PatchJump(int offset);

		void Block();

		void BeginScope();
		void EndScope();

		void DeclareVariable();

		void AddLocal(Token name);

		int ResolveLocal(Token* name);

		void NamedVariable(Token name, bool canAssign);

		void ExpressionStatement();

		void DefineVariable(uint16_t global, bool exportVar);

		size_t ParseVariable(const std::string& message);

		ParseRule* GetRule(TokenType type);

		void ParsePrecedence(Precedence precedence);
	};
}