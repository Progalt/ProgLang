#pragma once

#include <string>
#include <vector>


namespace script
{
	enum TokenType
	{
		TK_UNKNOWN,

		TK_IDENTIFIER, 

		TK_FUNCTION, TK_RETURN, 

		TK_ASSIGN, TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_BANG, 

		TK_POW, TK_MODULO, 

		TK_PLUS_PLUS, TK_MINUS_MINUS, 

		TK_COLON, TK_DOT, TK_COMMA, TK_SEMICOLON, 
		TK_EQUALS_ARROW, 
		TK_DOT_DOT, 

		TK_OPEN_BRACE, TK_CLOSE_BRACE, 
		TK_OPEN_CURLY, TK_CLOSE_CURLY, 
		TK_OPEN_SQUARE, TK_CLOSE_SQUARE, 

		TK_NUMBER, TK_STRING, TK_TRUE, TK_FALSE,  
		TK_NIL, TK_VAR, TK_EXPORT, TK_STRICT,

		TK_EQUALS, TK_BANG_EQUALS, TK_LESS_THAN, TK_GREATER_THAN, TK_LESS_EQUALS, TK_GREATER_EQUALS, 

		TK_AND, TK_OR, TK_IS, TK_AS, TK_IN, 

		TK_IMPORT, 

		TK_IF, TK_ELSE, 
		TK_WHILE, TK_FOR, 

		TK_ENUM, 
		TK_CLASS, 
		TK_SELF, 

		TK_NAMESPACE, 

		TK_ERROR, TK_THROW, 

		TK_EOF, 

		TOKEN_COUNT,
	};

	struct Token
	{
		TokenType type = TK_UNKNOWN;
		std::string value;

		// For debugging
		int line = 0, index = 0;
	};

	class Lexer
	{
	public:

		void Tokenise(const std::string& src);

		const std::vector<Token>& GetTokens()
		{
			return m_Tokens;
		}

	private:

		std::vector<Token> m_Tokens;

		std::string m_Source = "";

		uint32_t m_CurrentLine = 0;
		uint32_t m_CurrentIndex = 0;

		size_t m_CurrentOffset = 0;

		void EmitToken(TokenType type, const std::string& value);

		char Advance();

		char Peek(size_t offset = 1);
	};
}