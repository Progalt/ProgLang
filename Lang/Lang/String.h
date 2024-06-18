
#include <string>
#include <vector>

namespace script
{
	enum StringTokenType
	{
		STRTK_NONE, 
		STRTK_SUBSTRING,
		STRTK_VARIABLE,
		STRTK_EXPRESSION, 
	};

	struct StringToken
	{
		StringTokenType type = STRTK_NONE;
		size_t start = 0, end = 0;
		std::string str;
	};

	class StringScanner
	{
	public:

		StringScanner(const std::string& str);

		const std::vector<StringToken>& GetTokens() const { return m_Tokens; }

	private:

		void Tokenise();

		char* m_StringPtr;
		size_t m_StringLength = 0;
		size_t m_CurrentOffset = 0;
		size_t m_PrevTokenEndOffset = 0;

		std::vector<StringToken> m_Tokens;

		char Advance();

		char Peek(size_t offset = 1);

		void EmitToken(StringTokenType type, const std::string& str);
	};
}