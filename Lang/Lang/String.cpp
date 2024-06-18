
#include "String.h"
#include <cassert>

namespace script
{
	StringScanner::StringScanner(const std::string& str)
	{
		m_StringPtr = (char*)str.data();
		m_StringLength = str.length();


		Tokenise();
	}

	void StringScanner::Tokenise()
	{

		char current = m_StringPtr[0];
		std::string substr = "";
		Advance();
		while (current != '\0')
		{
			if (current == '$')
			{
				
				// We hit a $

				// Add a token to the list

				EmitToken(STRTK_SUBSTRING, substr);
				substr = "";

				if (Peek() == '{')
				{
					assert(false);
				}
				else
				{

					// We have a variable

					
					while (current != '\0' && current != ' ')
					{
						
						current = Advance(); 

						if (current == ' ')
							break;

						substr += current;
					}

					EmitToken(STRTK_VARIABLE, substr);
					substr = "";
				}

			}

			substr += current;
			current = Advance();
		}

		EmitToken(STRTK_SUBSTRING, substr);

	}

	char StringScanner::Advance()
	{
		if (m_CurrentOffset + 1 < m_StringLength + 1)
		{
			return m_StringPtr[m_CurrentOffset++];
		}

		return '\0';
	}

	char StringScanner::Peek(size_t offset)
	{
		if (m_CurrentOffset + offset < m_StringLength + 1)
		{
			return m_StringPtr[m_CurrentOffset + offset];
		}

		return '\0';
	}

	void StringScanner::EmitToken(StringTokenType type, const std::string& str)
	{
		if (m_PrevTokenEndOffset == m_CurrentOffset)
			return;

		m_Tokens.push_back({ type, m_PrevTokenEndOffset, m_CurrentOffset, str });
		m_PrevTokenEndOffset = m_CurrentOffset;
	}
}