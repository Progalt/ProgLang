
#include "Lexer.h"

namespace script
{


	static bool isAlpha(char c) {
		return (c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			c == '_';
	}

	static bool isDigit(char c) {
		return c >= '0' && c <= '9';
	}
	
	void Lexer::Tokenise(const std::string& src)
	{
		m_Source = src;

		char current = m_Source[0];
		while (current != '\0')
		{

			switch (current)
			{
			case '+':
				EmitToken(TK_PLUS, "+");
				break;
			case '-':
				EmitToken(TK_MINUS, "-");
				break;
			case '*':
				if (Peek() == '*')
				{
					EmitToken(TK_POW, "**");
					Advance();
				}
				else
					EmitToken(TK_STAR, "*");
				break;
			case '/':
				if (Peek() == '/')
				{
					// We want to skip until end of line

					while (Advance() != '\n') {}
				}
				else
					EmitToken(TK_SLASH, "/");
				break;
			case '%':
				EmitToken(TK_MODULO, "%");
				break;
			case '=':
				if (Peek() == '=')
				{
					EmitToken(TK_EQUALS, "==");
					Advance();
				}
				else if (Peek() == '>')
				{
					EmitToken(TK_EQUALS_ARROW, "=>");
					Advance();
				}
				else
					EmitToken(TK_ASSIGN, "=");
				break;
			case ':':
				EmitToken(TK_COLON, ":");
				break;
			case ';':
				EmitToken(TK_SEMICOLON, ";");
				break;

			case '(':
				EmitToken(TK_OPEN_BRACE, "(");
				break;

			case ')':
				EmitToken(TK_CLOSE_BRACE, ")");
				break;
			case '{':
				EmitToken(TK_OPEN_CURLY, "{");
				break;

			case '}':
				EmitToken(TK_CLOSE_CURLY, "}");
				break;
			case '[':
				EmitToken(TK_OPEN_SQUARE, "[");
				break;

			case ']':
				EmitToken(TK_CLOSE_SQUARE, "]");
				break;

			case ',':
				EmitToken(TK_COMMA, ",");
				break;
			case '.':
				EmitToken(TK_DOT, ".");
				break;
			case '>':
				if (Peek() == '=')
				{
					EmitToken(TK_GREATER_EQUALS, ">=");
					Advance();
				}
				else
					EmitToken(TK_GREATER_THAN, ">");
				break;
			case '<':
				if (Peek() == '=')
				{
					EmitToken(TK_LESS_EQUALS, "<=");
					Advance();
				}
				else
					EmitToken(TK_LESS_THAN, "<");
				break;
			case '!':
				if (Peek() == '=')
				{
					EmitToken(TK_BANG_EQUALS, "!=");
					Advance();
				}
				else
					EmitToken(TK_BANG, "!");
				break;
			case '"':
			{
				std::string str = "";
				current = Advance();
				str += "\"";

				while (current != '"')
				{
					str += current;

					current = Advance();
				}
				str += "\"";
				EmitToken(TK_STRING, str);
				break;
			}
			case '\n':
				m_CurrentLine++;
				m_CurrentIndex = 0;
				break;
			}

			if (isAlpha(current))
			{
				std::string iden = "";

				while (isAlpha(current) || isDigit(current))
				{


					iden += current;

					current = Advance();
				}

				
				if (iden == "if")
					EmitToken(TK_IF, iden);
				else if (iden == "else")
					EmitToken(TK_ELSE, iden);
				else if (iden == "and")
					EmitToken(TK_AND, iden);
				else if (iden == "or")
					EmitToken(TK_OR, iden);
				else if (iden == "is")
					EmitToken(TK_IS, iden);
				else if (iden == "as")
					EmitToken(TK_AS, iden);
				else if (iden == "while")
					EmitToken(TK_WHILE, iden);
				else if (iden == "for")
					EmitToken(TK_FOR, iden);
				else if (iden == "nil")
					EmitToken(TK_NIL, iden);
				else if (iden == "true")
					EmitToken(TK_TRUE, iden);
				else if (iden == "false")
					EmitToken(TK_FALSE, iden);
				else if (iden == "func")
					EmitToken(TK_FUNCTION, iden);
				else if (iden == "return")
					EmitToken(TK_RETURN, iden);
				else if (iden == "enum")
					EmitToken(TK_ENUM, iden);
				else if (iden == "var")
					EmitToken(TK_VAR, iden);
				else if (iden == "export")
					EmitToken(TK_EXPORT, iden);
				else if (iden == "strict")
					EmitToken(TK_STRICT, iden);
				else if (iden == "class")
					EmitToken(TK_CLASS, iden);
				else if (iden == "self")
					EmitToken(TK_SELF, iden);
				else if (iden == "throw")
					EmitToken(TK_THROW, iden);
				else if (iden == "namespace")
					EmitToken(TK_NAMESPACE, iden);
				else if (iden == "import")
					EmitToken(TK_IMPORT, iden);
				else 
					EmitToken(TK_IDENTIFIER, iden);

				if (m_CurrentOffset != m_Source.size() - 1)
				{
					m_CurrentOffset--;
					m_CurrentIndex--;
				}
			}

			if (isDigit(current))
			{
				// This is really hacky
				std::string num = "";

				while (isDigit(current) || current == '.')
				{

					num += current;

					current = Advance();
				}

				EmitToken(TK_NUMBER, num);
				if (m_CurrentOffset != m_Source.size() - 1)
				{
					m_CurrentOffset--;
					m_CurrentIndex--;
				}
			}

			current = Advance();
		}

		EmitToken(TK_EOF, "");
	}

	void Lexer::EmitToken(TokenType type, const std::string& value)
	{
		m_Tokens.push_back({ type, value, (int)m_CurrentLine, (int)m_CurrentIndex });
	}

	char Lexer::Advance()
	{
		if (m_CurrentOffset + 1 < m_Source.size())
		{
			m_CurrentOffset++;
			m_CurrentIndex++;

			return m_Source[m_CurrentOffset];
		}

		return '\0';
	}

	char Lexer::Peek(size_t offset)
	{
		if (m_CurrentOffset + offset < m_Source.size())
		{
			return m_Source[m_CurrentOffset + offset];
		}

		return '\0';
	}
}