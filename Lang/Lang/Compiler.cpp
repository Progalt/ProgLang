
#include "Compiler.h"
#include "Memory.h"
#include "String.h"

namespace script
{
	static ParseRule rules[TOKEN_COUNT];

	Parser parser;

	// Forward declare parser functions 
	// This is written in a more C way 
	// But only because these used to exist in the compiler and I moved them out
	// And to make life easier I didn't move them into a parser class 
	void Advance();
	void Consume(TokenType type, const std::string& msg);
	bool Check(TokenType type);
	bool Match(TokenType type);
	bool Peek(size_t offset, TokenType type);
	Token GetNextToken();

	void ErrorAt(Token tk, const std::string& msg)
	{
		if (parser.panicMode)
			return;

		printf("[line: %d, Column: %d] -> \"%s\" -> %s\n", tk.line, tk.index, tk.value.c_str(), msg.c_str());
		parser.hadError = true;
		parser.panicMode = true;

	}

	void Error(const std::string& msg)
	{
		if (parser.panicMode)
			return;

		printf("Compile Error: %s", msg.c_str());

		parser.hadError = true;
		parser.panicMode = true;
	}

	void Synchronise()
	{
		parser.panicMode = false;

		while (parser.current.type != TK_EOF) {
			if (parser.previous.type == TK_SEMICOLON) return;
			switch (parser.current.type) {
			case TK_FUNCTION:
			case TK_FOR:
			case TK_IF:
			case TK_WHILE:
			case TK_RETURN:
				return;

			default:
				; // Do nothing.
			}

			Advance();
		}
	}

	void Advance()
	{
		if (parser.current.type == TK_EOF)
			return;

		parser.previous = parser.current;

		for (;;)
		{
			parser.current = GetNextToken();

			if (parser.current.type != TK_ERROR) break;

			ErrorAt(parser.current, "");
		}
	}

	void Consume(TokenType type, const std::string& msg)
	{
		if (parser.current.type == type) {
			Advance();
			return;
		}

		ErrorAt(parser.current, msg);
	}

	bool Check(TokenType type)
	{
		return (parser.current.type == type);
	}

	bool Match(TokenType type)
	{
		if (!Check(type)) return false;

		Advance();
		return true;
	}

	bool Peek(size_t offset, TokenType type)
	{
		if (offset > parser.tokens.size())
			return false;

		return (parser.tokens[parser.tokenOffset + offset].type == type);
	}

	Token GetNextToken()
	{

		Token tk = parser.tokens[parser.tokenOffset];
		parser.tokenOffset++;
		return tk;
	}
	
	ObjFunction* CompileScript(const std::string& source)
	{
		// Init the parser
		parser = Parser();

		// Create the lexer and stuff
		Lexer lexer;
		Compiler compiler;
		
		// idk why i added some trailing whitespace
		// But i'm scared to remove it now
		std::string src = source + " ";

		lexer.Tokenise(src);

		parser.tokens = lexer.GetTokens();

		ObjFunction* func = compiler.Compile(TYPE_SCRIPT);

		return func;
	}

	Compiler::~Compiler()
	{
		if (m_Locals)
			delete[] m_Locals;
	}

	ObjFunction* Compiler::Compile(FunctionType type)
	{
	
		InitCompiler(nullptr, type);

		Advance();

		//Expression();

		while (!Match(TK_EOF))
		{
			Declaration();
		}

		

		if (parser.hadError)
			return nullptr;

		return EndCompiler();
	}

	ObjFunction* Compiler::EndCompiler()
	{
		// Emit a return at the end

		EmitReturn();

		return m_Function; 
	}

	void Compiler::InitCompiler(Compiler* enclosing, FunctionType type)
	{
		SetRules();
		m_Enclosing = enclosing;

		//m_Chunk = chunk;

		m_Function = nullptr;

		m_FunctionType = type;

		m_Function = NewFunction();

		// If its not a script we want to assign a name to the function
		if (type != TYPE_SCRIPT)
		{
			m_Function->name = memoryManager.AllocateString(parser.previous.value);
		}

		m_Locals = new Local[UINT8_MAX];

		Local* local = &m_Locals[m_LocalCount++];
		local->depth = 0;
		local->name.value = "";

		if (type != TYPE_FUNCTION)
		{
			local->name.value = "self";
		}

	}

	void Compiler::SetRules()
	{
		// TODO: These parse rules get reassigned every time we compile anything...
		// Move this to a first time kind of thing and only do it once 

		auto GroupingFunc = [&](bool canAssign) {
			this->Grouping(canAssign);
			};

		auto UnaryFunc = [&](bool canAssign) {
			this->Unary(canAssign);
			};

		auto BinaryFunc = [&](bool canAssign) {
			this->Binary(canAssign);
			};

		auto NumberFunc = [&](bool canAssign) {
			this->Number(canAssign);
			};

		auto LiteralFunc = [&](bool canAssign) {
			this->Literal(canAssign);
			};

		auto StringFunc = [&](bool canAssign) {
			this->String(canAssign);
			};

		auto VariableFunc = [&](bool canAssign) {
			this->Variable(canAssign);
			};

		auto AndFunc = [&](bool canAssign) {
			this->And(canAssign);
			};

		auto OrFunc = [&](bool canAssign) {
			this->Or(canAssign);
			};

		auto CallFunc = [&](bool canAssign) {
			this->Call(canAssign);
			};

		auto ArrayFunc = [&](bool canAssign) {
			this->Array(canAssign);
		};

		auto ArrayAccess = [&](bool canAssign) {
			this->ArrayAccess(canAssign);
		};

		auto DotFunc = [&](bool canAssign) {
			this->Dot(canAssign);
		};

		auto SelfFunc = [&](bool canAssign) {
			this->Self(canAssign);
		};

		auto IncrementFunc = [&](bool canAssign) {
			//this->Increment(canAssign);
		};

		auto RangeFunc = [&](bool canAssign)
			{
				this->Range(canAssign);
			};

		rules[TK_OPEN_BRACE] = { GroupingFunc, CallFunc,   PREC_CALL },
			rules[TK_CLOSE_BRACE] = { NULL,     NULL,   PREC_NONE },
			rules[TK_OPEN_CURLY] = { NULL,     NULL,   PREC_NONE },
			rules[TK_CLOSE_CURLY] = { NULL,     NULL,   PREC_NONE },
			rules[TK_OPEN_SQUARE] = { ArrayFunc, ArrayAccess,  PREC_INDEX },
			rules[TK_CLOSE_SQUARE] = { NULL,     NULL,   PREC_NONE },
			rules[TK_COMMA] = { NULL,     NULL,   PREC_NONE },
			rules[TK_DOT] = { NULL,     DotFunc,   PREC_CALL },
			rules[TK_MINUS] = { UnaryFunc,    BinaryFunc, PREC_TERM },
			rules[TK_PLUS] = { NULL,     BinaryFunc, PREC_TERM },
			rules[TK_SLASH] = { NULL,     BinaryFunc, PREC_FACTOR },
			rules[TK_STAR] = { NULL,     BinaryFunc, PREC_FACTOR },
			rules[TK_BANG] = { UnaryFunc,     NULL,   PREC_NONE },
			rules[TK_BANG_EQUALS] = { NULL,     BinaryFunc,   PREC_EQUALITY },
			rules[TK_ASSIGN] = { NULL,     NULL,   PREC_NONE },
			rules[TK_EQUALS] = { NULL,     BinaryFunc,   PREC_EQUALITY },
			rules[TK_GREATER_THAN] = { NULL,     BinaryFunc,   PREC_COMPARISON },
			rules[TK_GREATER_EQUALS] = { NULL,     BinaryFunc,   PREC_COMPARISON },
			rules[TK_LESS_THAN] = { NULL,     BinaryFunc,   PREC_COMPARISON },
			rules[TK_LESS_EQUALS] = { NULL,     BinaryFunc,   PREC_COMPARISON },
			rules[TK_IDENTIFIER] = { VariableFunc,     NULL,   PREC_NONE },
			rules[TK_STRING] = { StringFunc,     NULL,   PREC_NONE },
			rules[TK_NUMBER] = { NumberFunc,   NULL,   PREC_NONE },
			rules[TK_AND] = { NULL,     AndFunc,   PREC_AND },
			rules[TK_ELSE] = { NULL,     NULL,   PREC_NONE },
			rules[TK_FALSE] = { LiteralFunc,     NULL,   PREC_NONE },
			rules[TK_FOR] = { NULL,     NULL,   PREC_NONE },
			rules[TK_FUNCTION] = { NULL,     NULL,   PREC_NONE },
			rules[TK_IF] = { NULL,     NULL,   PREC_NONE },
			rules[TK_NIL] = { LiteralFunc,     NULL,   PREC_NONE },
			rules[TK_OR] = { NULL,     OrFunc,   PREC_OR },
			rules[TK_RETURN] = { NULL,     NULL,   PREC_NONE },
			rules[TK_TRUE] = { LiteralFunc,     NULL,   PREC_NONE },
			rules[TK_WHILE] = { NULL,     NULL,   PREC_NONE },
			rules[TK_ERROR] = { NULL,     NULL,   PREC_NONE },
			rules[TK_EOF] = { NULL,     NULL,   PREC_NONE };
			rules[TK_SELF] = { SelfFunc,     NULL,   PREC_NONE };
			rules[TK_POW] = { NULL,     BinaryFunc,   PREC_POWER };
			rules[TK_MODULO] = { NULL,     BinaryFunc,   PREC_FACTOR };
			rules[TK_COLON] = { NULL,     NULL,   PREC_NONE };
			rules[TK_PLUS_PLUS] = { NULL,     IncrementFunc,   PREC_CALL };
			rules[TK_DOT_DOT] = { NULL,     BinaryFunc,   PREC_POWER };


	}

	void Compiler::MarkInitialised()
	{
		if (m_ScopeDepth == 0) return;

		m_Locals[m_LocalCount - 1].depth = m_ScopeDepth;
	}

	void Compiler::EmitByte(uint8_t byte)
	{
		GetCurrentChunk()->WriteByte(byte);
	}

	void Compiler::EmitReturn()
	{
		if (m_FunctionType == TYPE_INITIALIZER)
		{
			EmitByte(OP_GET_LOCAL);
			EmitBytes(0, 0);
		}
		else
		{
			EmitByte(OP_NIL);
		}
		EmitByte(OP_RETURN);
	}

	void Compiler::EmitBytes(uint8_t byte1, uint8_t byte2)
	{
		EmitByte(byte1);
		EmitByte(byte2);
	}

	void Compiler::EmitConstant(Value value)
	{
		GetCurrentChunk()->WriteConstant(value);
	}


	void Compiler::Expression()
	{
		ParsePrecedence(PREC_ASSIGNMENT);
	}

	void Compiler::Grouping(bool canAssign)
	{
		Expression();
		Consume(TK_CLOSE_BRACE, "Expected ')' after expression");
	}

	void Compiler::Unary(bool canAssign)
	{
		TokenType operatorType = parser.previous.type;

		Expression();

		switch (operatorType) {
		case TK_MINUS: EmitByte(OP_NEGATE); break;
		case TK_BANG: EmitByte(OP_NOT); break;
		default: return; // Unreachable.
		}
	}

	void Compiler::Binary(bool canAssign)
	{
		TokenType operatorType = parser.previous.type;

		ParseRule* rule = GetRule(operatorType);
		ParsePrecedence((Precedence)(rule->precedence + 1));

		switch (operatorType) {
		case TK_PLUS:          EmitByte(OP_ADD); break;
		case TK_MINUS:         EmitByte(OP_SUBTRACT); break;
		case TK_STAR:          EmitByte(OP_MULTIPLY); break;
		case TK_SLASH:         EmitByte(OP_DIVIDE); break;
		case TK_POW:			EmitByte(OP_POWER); break;
		case TK_MODULO:			EmitByte(OP_MODULO); break;

		case TK_DOT_DOT: EmitByte(OP_CREATE_RANGE); break;

		case TK_BANG_EQUALS:    EmitBytes(OP_EQUAL, OP_NOT); break;
		case TK_EQUALS:   EmitByte(OP_EQUAL); break;
		case TK_GREATER_THAN:       EmitByte(OP_GREATER); break;
		case TK_GREATER_EQUALS: EmitBytes(OP_LESS, OP_NOT); break;
		case TK_LESS_THAN:          EmitByte(OP_LESS); break;
		case TK_LESS_EQUALS:    EmitBytes(OP_GREATER, OP_NOT); break;

		default: return; // Unreachable.
		}
	}

	void Compiler::Literal(bool canAssign)
	{
		switch (parser.previous.type) {
		case TK_FALSE: EmitByte(OP_FALSE); break;
		case TK_NIL: EmitByte(OP_NIL); break;
		case TK_TRUE: EmitByte(OP_TRUE); break;
		default: return; // Unreachable.
		}
	}

	void Compiler::String(bool canAssign)
	{
		// We want to see if we can interpolate this string
		// If we do we want to add the OpCodes for that 

		std::string str = parser.previous.value.substr(1, parser.previous.value.length() - 2);
		StringScanner scanner(str);

		std::vector<StringToken> tokens = scanner.GetTokens();

		if (tokens.size() == 1)
		{
			// If its a size of 1 its just a plain ol' basic string
			EmitConstant(Value(memoryManager.AllocateString(str)));
		}
		else
		{
			// We have some values to insert
			// The string is broken up
			// So add the new strings to the constant table and then push everything onto the stack
			if (tokens.size() >= UINT8_MAX)
			{
				ErrorAt(parser.current, "String has too many interpolated values");
			}

			for (size_t i = 0; i < tokens.size(); i++)
			{
				StringToken& tk = tokens[i];
				switch (tk.type)
				{
				case STRTK_SUBSTRING:
				{
					EmitConstant(Value(memoryManager.AllocateString(tk.str)));
					break;
				}
				case STRTK_VARIABLE:
				{
					Token token = parser.current;
					token.type = TK_IDENTIFIER;
					token.value = tk.str;
					NamedVariable(token, false);
					break;
				}
				}
			}

			EmitBytes(OP_STRING_INTERP, (uint8_t)tokens.size());
		}
	}

	void Compiler::Array(bool canAssign)
	{
		uint16_t size = 0;
		// Check if we have any elements
		if (!Check(TK_CLOSE_SQUARE))
		{
			do {
				Expression();

				size++;
			} while (Match(TK_COMMA));
		}

		Consume(TK_CLOSE_SQUARE, "Expected ']' at end of list");

		EmitByte(OP_CREATE_LIST);
		EmitBytes((size >> 8) & 0xFF, size & 0xFF);
	}

	void Compiler::ArrayAccess(bool canAssign)
	{

		Expression();

		bool slice = false;
		if (Match(TK_COLON))
		{
			// We have a slice
			

			// Grab the end
			Expression();

			slice = true;
		}

		Consume(TK_CLOSE_SQUARE, "Expected ']' at end of list");

		if (canAssign && Match(TK_ASSIGN))
		{
			if (slice)
				ErrorAt(parser.current, "Cannot assign to an array that is being sliced");

			Expression();

			EmitByte(OP_SUBSCRIPT_WRITE);
		}
		else
		{
			EmitByte(slice ? OP_SLICE_ARRAY : OP_SUBSCRIPT_READ);
		}

		
	}

	void Compiler::Statement()
	{
		if (Match(TK_IMPORT))
		{
			ImportStatement();
		}
		else if (Match(TK_CLASS))
		{
			ClassDeclaration();
		}
		else if (Match(TK_FUNCTION))
		{
			FunctionDeclaration();
		}
		else if (Match(TK_VAR))
		{
			VariableDeclaration(false);
		}
		else if (Match(TK_EXPORT))
		{
			if (Match(TK_VAR))
			{
				VariableDeclaration(true);
			}
		}
		else if (Match(TK_OPEN_CURLY))
		{
			BeginScope();
			Block();
			EndScope();
		}
		else if (Match(TK_IF))
		{
			IfStatement();
		}
		else if (Match(TK_RETURN))
		{
			ReturnStatement();
		}
		else if (Match(TK_WHILE))
		{
			WhileStatement();
		}
		else if (Match(TK_FOR))
		{
			ForStatement();
		}
		else
		{
			ExpressionStatement();
		}

	}

	void Compiler::Declaration()
	{
		Statement();

		if (parser.panicMode)
			Synchronise();
	}

	void Compiler::ExpressionStatement()
	{
		Expression();
		Consume(TK_SEMICOLON, "Expected ';' at the end of expression.");
		EmitByte(OP_POP);
	}

	void Compiler::VariableDeclaration(bool exportVar, bool expectSemicolon)
	{

		size_t global = ParseVariable("Expected a variable name.");

		if (parser.current.type == TK_ASSIGN)
		{
			Advance();
			Expression();
			
			//uint8_t upper = global >> 8;
			//uint8_t lower = (global & 0xFF);

			//EmitByte(OP_SET_GLOBAL);
			//EmitBytes(upper, lower);
		}
		else
		{
			EmitByte(OP_NIL);
		}

		if (expectSemicolon)
			Consume(TK_SEMICOLON, "Expected ';' at the end of declaration.");

		DefineVariable((uint16_t)global, exportVar);
	}

	void Compiler::Variable(bool canAssign)
	{
		NamedVariable(parser.previous, canAssign);
	}

	void Compiler::Block()
	{
		while (!Check(TK_CLOSE_CURLY) && !Check(TK_EOF))
		{
			Declaration();
		}

		Consume(TK_CLOSE_CURLY, "Expected '}' after block.");
	}

	void Compiler::BeginScope()
	{
		m_ScopeDepth++;
	}

	void Compiler::EndScope()
	{
		m_ScopeDepth--;

		while (m_LocalCount > 0 &&
			m_Locals[m_LocalCount - 1].depth > m_ScopeDepth)
		{
			// TODO: Emit an OP_POPN with a number of pops
			EmitByte(OP_POP);
			m_LocalCount--;
		}
	}

	void Compiler::NamedVariable(Token name, bool canAssign)
	{
		uint8_t getOp, setOp;
		int arg = ResolveLocal(&name);

		if (arg != -1) {
			getOp = OP_GET_LOCAL;
			setOp = OP_SET_LOCAL;
		}
		else {
			arg = GetCurrentChunk()->AddConstant(Value(memoryManager.AllocateString(name.value)));
			getOp = OP_GET_GLOBAL;
			setOp = OP_SET_GLOBAL;
		}

		



		if (canAssign && parser.current.type == TK_ASSIGN)
		{
			Advance();
			Expression();

			uint8_t upper = arg >> 8;
			uint8_t lower = (arg & 0xFF);

			EmitByte(setOp);
			EmitBytes(upper, lower);
		}
		else {
			EmitByte(getOp);

			uint8_t upper = arg >> 8;
			uint8_t lower = (arg & 0xFF);

			EmitBytes(upper, lower);
		}


	}

	int Compiler::ResolveLocal(Token* name)
	{
		for (int i = m_LocalCount - 1; i >= 0; i--)
		{
			Local* local = &m_Locals[i];

			// if (local->name.value == name->value)
			if (strcmp(local->name.value.c_str(), name->value.c_str()) == 0)
			{
				return i;
			}
		}

		return -1;
	}


	size_t Compiler::ParseVariable(const std::string& message)
	{
		Consume(TK_IDENTIFIER, message);

		DeclareVariable();
		if (m_ScopeDepth > 0) return 0;

		return GetCurrentChunk()->AddConstant(Value(memoryManager.AllocateString(parser.previous.value)));
	}

	void Compiler::DefineVariable(uint16_t global, bool exportVar)
	{
		if (m_ScopeDepth > 0) {

			MarkInitialised();
			return;
		}

		uint8_t defineOp = OP_DEFINE_GLOBAL;

		if (exportVar)
		{
			defineOp = OP_EXPORT_GLOBAL;
		}


		EmitByte(defineOp);

		uint8_t upper = global >> 8;
		uint8_t lower = (global & 0xFF);

		EmitBytes(upper, lower);
	}

	void Compiler::DeclareVariable()
	{
		if (m_ScopeDepth == 0) {
			return;
		}

		Token* name = &parser.previous;

		for (int i = m_LocalCount - 1; i >= 0; i--)
		{
			Local* local = &m_Locals[i];
			if (local->depth != -1 && local->depth < m_ScopeDepth)
			{
				break;
			}

			if (name->value == local->name.value)
			{
				ErrorAt(*name, "Already a variable with this name in this scope.");
			}
		}

		AddLocal(*name);
	}

	void Compiler::DeclareVariableName(const std::string& name)
	{
		if (m_ScopeDepth == 0) {
			return;
		}


		for (int i = m_LocalCount - 1; i >= 0; i--)
		{
			Local* local = &m_Locals[i];
			if (local->depth != -1 && local->depth < m_ScopeDepth)
			{
				break;
			}

			if (name == local->name.value)
			{
				Error("Already a variable with this name in this scope: " + name);
			}
		}

		AddLocal(Token{ TK_IDENTIFIER, name });
	}

	void Compiler::AddLocal(Token name)
	{
		if (m_LocalCount == UINT8_MAX) {
			ErrorAt(name, "Too many local variables in function.");
			return;
		}

		Local* local = &m_Locals[m_LocalCount++];

		local->name = name;
		local->depth = m_ScopeDepth;
	}

	void Compiler::IfStatement()
	{
		Consume(TK_OPEN_BRACE, "Expected '(' after 'if'");
		Expression();
		Consume(TK_CLOSE_BRACE, "Expected ')' after if condition");

		int thenJump = EmitJump(OP_JUMP_IF_FALSE);

		EmitByte(OP_POP);

		Statement();

		int elseJump = EmitJump(OP_JUMP);

		EmitByte(OP_POP);

		PatchJump(thenJump);

		if (Match(TK_ELSE))
			Statement();

		PatchJump(elseJump);
	}

	void Compiler::WhileStatement()
	{
		int loopStart = GetCurrentChunk()->code.size();

		Consume(TK_OPEN_BRACE, "Expected '(' after 'while'");
		Expression();
		Consume(TK_CLOSE_BRACE, "Expected ')' after while condition");

		int exitJump = EmitJump(OP_JUMP_IF_FALSE);
		EmitByte(OP_POP);

		Statement();

		EmitLoop(loopStart);

		PatchJump(exitJump);
		EmitByte(OP_POP);
	}

	void Compiler::ForStatement()
	{
		BeginScope();
		Consume(TK_OPEN_BRACE, "Expected '(' after 'for',");
		
		if (Match(TK_SEMICOLON))
		{

		}
		else if (Match(TK_VAR))
		{
			if (Peek(0, TK_IN))
			{
				// We have a in statement

				// Define a new variable 
				VariableDeclaration(false, false);

				NamedVariable(parser.previous, false);
				
				EmitByte(OP_POP);

				Consume(TK_IN, "Expected an 'in'.");

				// After the in we have an expression 
				Expression();

				DeclareVariableName("__seq");

				Consume(TK_CLOSE_BRACE, "Expected ')' after for clauses.");

				// Push a nil value on for the iterator
				EmitByte(OP_NIL);

				DeclareVariableName("__itr");
				
				// We start our loop here
				int loopStart = GetCurrentChunk()->code.size();


				EmitByte(OP_ITER);

				int patch = GetCurrentChunk()->code.size();
				EmitBytes(0xFF, 0xFF);

				Statement();

				
				

				EmitLoop(loopStart, false);
				int exit = GetCurrentChunk()->code.size() - loopStart - 2;

				GetCurrentChunk()->code[patch] = (exit >> 8) & 0xFF;
				GetCurrentChunk()->code[patch + 1] = exit & 0xFF;
				
				EmitByte(OP_POP);
				EndScope();

				// Early return so we don't run any other code
				return; 
			}

			VariableDeclaration(false);
		}
		else
		{
			ExpressionStatement();
		}

		int loopStart = GetCurrentChunk()->code.size();
		
		int exitJump = -1;

		if (!Match(TK_SEMICOLON))
		{
			Expression();
			Consume(TK_SEMICOLON, "Expected ';' after loop condition");

			exitJump = EmitJump(OP_JUMP_IF_FALSE);
			EmitByte(OP_POP);
		}
		
		if (!Match(TK_CLOSE_BRACE))
		{
			int bodyJump = EmitJump(OP_JUMP);
			int incrementStart = GetCurrentChunk()->code.size();

			Expression();
			EmitByte(OP_POP);
			Consume(TK_CLOSE_BRACE, "Expected ')' after for clauses.");

			EmitLoop(loopStart);
			loopStart = incrementStart;
			PatchJump(bodyJump);
		}

		Statement();

		EmitLoop(loopStart);

		if (exitJump != -1)
		{
			PatchJump(exitJump);
			EmitByte(OP_POP);
		}

		EndScope();
	}

	void Compiler::FunctionDeclaration()
	{
		size_t global = ParseVariable("Expected Function name");

		MarkInitialised();

		Function(TYPE_FUNCTION);

		DefineVariable((uint16_t)global, false);
	}

	void Compiler::ClassDeclaration()
	{
		Consume(TK_IDENTIFIER, "Expected class name");

		Token className = parser.previous;

		uint16_t nameConstant = GetCurrentChunk()->AddConstant(Value(memoryManager.AllocateString(parser.previous.value)));
		DeclareVariable();


		EmitByte(OP_CLASS);
		EmitBytes((nameConstant >> 8) && 0xFF, nameConstant & 0xFF);

		DefineVariable(nameConstant, false);

		NamedVariable(className, false);
		Consume(TK_OPEN_CURLY, "Expected '{' before class body");

		while (!Check(TK_CLOSE_CURLY) && !Check(TK_EOF))
		{
			
			Method();
			
		}

		Consume(TK_CLOSE_CURLY, "Expected '}' after class body");
		EmitByte(OP_POP);
	}

	void Compiler::Function(FunctionType type)
	{
		Compiler compiler;
		compiler.InitCompiler(this, type);
		
		compiler.BeginScope();

		Consume(TK_OPEN_BRACE, "Expected '(' after function name");

		if (!Check(TK_CLOSE_BRACE))
		{
			do {

				m_Function->arity++;
				if (m_Function->arity > 255)
				{
					ErrorAt(parser.current, "Exceeded parameter limit for function");
				}

				uint16_t constant = compiler.ParseVariable("Expected parameter name");
				compiler.DefineVariable(constant, false);

			} while (Match(TK_COMMA));
		}

		Consume(TK_CLOSE_BRACE, "Expected ')' after function arguments");

		if (Match(TK_EQUALS_ARROW))
		{
			compiler.Expression();
			Consume(TK_SEMICOLON, "Expected ';' after expression.");

			compiler.EmitByte(OP_RETURN);

			ObjFunction* func = compiler.EndCompiler();

			EmitConstant(Value(func));
		}
		else
		{
			Consume(TK_OPEN_CURLY, "Expected '{' before function body");

			compiler.Block();

			ObjFunction* func = compiler.EndCompiler();

			EmitConstant(Value(func));
		}

		// After we leave and come back to this compiler we need to reset the rules because its still set for the higher scoped compiler
		SetRules();

	}

	void Compiler::ReturnStatement()
	{
		if (Match(TK_SEMICOLON))
		{
			EmitReturn();
		}
		else
		{
			if (m_FunctionType == TYPE_INITIALIZER)
			{
				Error("Can't return a value from an initializer.");
			}

			Expression();
			Consume(TK_SEMICOLON, "Expected ';' after return value.");
			EmitByte(OP_RETURN);
		}
	}

	void Compiler::Dot(bool canAssign)
	{
		Consume(TK_IDENTIFIER, "Expected Identifier before '.'.");

		uint16_t name = GetCurrentChunk()->AddConstant(Value(memoryManager.AllocateString(parser.previous.value)));

		if (canAssign && Match(TK_ASSIGN)) {
			Expression();
			EmitByte(OP_SET_PROPERTY);
			EmitBytes((name >> 8) & 0xFF, name & 0xFF);
		}
		// TODO: Get invokes working for optimisation 
		/*else if (Match(TK_OPEN_BRACE))
		{
			uint8_t argCount = ArgumentList();
			EmitByte(OP_INVOKE);
			EmitBytes((name >> 8) & 0xFF, name & 0xFF);
			EmitByte(argCount);
		}*/
		else {
			EmitByte(OP_GET_PROPERTY);
			EmitBytes((name >> 8) & 0xFF, name & 0xFF);
		}
	}

	void Compiler::Increment(bool canAssign)
	{
		TokenType type = parser.previous.type;

		if (type == TK_PLUS_PLUS)
		{
			EmitByte(OP_INCREMENT);
		}
	}



	void Compiler::Method()
	{
		FunctionType type = TYPE_METHOD;

		if (Check(TK_IDENTIFIER) && parser.current.value == "construct")
		{
			type = TYPE_INITIALIZER;
		}
		else
		{
			if (parser.current.value == "constructor")
			{
				// Might of meant construct

				ErrorAt(parser.current, "constructor is not a valid method without a func declaration. Did you mean construct?");
			}

			Consume(TK_FUNCTION, "Methods must begin with a function declaration.");
		}
		Consume(TK_IDENTIFIER, "Expected method name");

		uint16_t name = GetCurrentChunk()->AddConstant(Value(memoryManager.AllocateString(parser.previous.value)));

		
		Function(type);

		EmitByte(OP_METHOD);
		EmitBytes((name >> 8) & 0xFF, name & 0xFF);
	}

	void Compiler::Self(bool canAssign)
	{
		if (m_FunctionType != TYPE_METHOD && m_FunctionType != TYPE_INITIALIZER)
			ErrorAt(parser.current, "Cannot use self within non-class method");

		Variable(false);
	}

	void Compiler::ImportStatement()
	{
		// Import has already been consumed

		Consume(TK_STRING, "Expected module name string. ");

		uint16_t moduleName = GetCurrentChunk()->AddConstant(Value(memoryManager.AllocateString(parser.previous.value.substr(1, parser.previous.value.length() - 2))));
		

		if (Match(TK_AS))
		{
			// Check if there is an 'as'
			Consume(TK_IDENTIFIER, "Expected name to import module as.");

			uint16_t asName = GetCurrentChunk()->AddConstant(Value(memoryManager.AllocateString(parser.previous.value)));

			EmitByte(OP_IMPORT_MODULE_AS);
			EmitBytes((moduleName >> 8) & 0xFF, moduleName & 0xFF);
			EmitBytes((asName >> 8) & 0xFF, asName & 0xFF);
		}
		else
		{
			EmitByte(OP_IMPORT_MODULE);
			EmitBytes((moduleName >> 8) & 0xFF, moduleName & 0xFF);
		}
		
		Consume(TK_SEMICOLON, "Expected ';' after import statement.");

	}


	void Compiler::Call(bool canAssign)
	{
		uint8_t argCount = ArgumentList();
		EmitBytes(OP_CALL, argCount);
	}

	uint8_t Compiler::ArgumentList()
	{
		uint8_t argCount = 0;
		if (!Check(TK_CLOSE_BRACE))
		{
			do {
				Expression();
				argCount++;
			} while (Match(TK_COMMA));
		}
		Consume(TK_CLOSE_BRACE, "Expected ')' after function arguments");
		return argCount;
	}

	int Compiler::EmitJump(uint8_t instruction)
	{
		EmitByte(instruction);
		EmitByte(0xff);
		EmitByte(0xff);
		return GetCurrentChunk()->code.size() - 2;
	}

	void Compiler::EmitLoop(int loopStart, bool iter)
	{
		EmitByte(iter ? OP_ITER : OP_LOOP);

		int offset = GetCurrentChunk()->code.size() - loopStart + 2;

		if (offset > UINT16_MAX) 
			Error("Loop body too large");

		EmitByte((offset >> 8) & 0xff);
		EmitByte(offset & 0xff);
	}

	void Compiler::PatchJump(int offset)
	{
		int jump = GetCurrentChunk()->code.size() - offset - 2;

		if (jump > UINT16_MAX)
		{
			Error("Too much code to jump over. Fix this");
		}

		GetCurrentChunk()->code[offset] = (jump >> 8) & 0xFF;
		GetCurrentChunk()->code[offset + 1] = jump & 0xFF;
	}

	void Compiler::And(bool canAssign)
	{
		int endJump = EmitJump(OP_JUMP_IF_FALSE);

		EmitByte(OP_POP);
		ParsePrecedence(PREC_AND);

		PatchJump(endJump);
	}

	void Compiler::Or(bool canAssign)
	{
		int elseJump = EmitJump(OP_JUMP_IF_FALSE);
		int endJump = EmitJump(OP_JUMP);

		PatchJump(elseJump);
		EmitByte(OP_POP);

		ParsePrecedence(PREC_OR);

		PatchJump(endJump);
	}

	void Compiler::ParsePrecedence(Precedence precedence)
	{
		Advance();
		std::function<void(bool)> prefixRule = GetRule(parser.previous.type)->prefix;

		if (prefixRule == NULL) {
			ErrorAt(parser.previous, "Expect expression.");
			return;
		}

		bool canAssign = precedence <= PREC_ASSIGNMENT;
		prefixRule(canAssign);

		while (precedence <= GetRule(parser.current.type)->precedence) 
		{
			Advance();
			std::function<void(bool)> infixRule = GetRule(parser.previous.type)->infix;
 			if (infixRule)
				infixRule(canAssign);
		}

		if (canAssign && Match(TK_ASSIGN)) {
			ErrorAt(parser.current, "Invalid Assignment target");

		}

	}

	void Compiler::Number(bool canAssign)
	{
		Value val(std::stod(parser.previous.value));
		EmitConstant(val);
	}

	ParseRule* Compiler::GetRule(TokenType type)
	{
		return &rules[type];
	}
}