
#include "stdafx.h"
#include "Lexer.h"
#include <sstream>

using namespace std;

namespace Lexer
{
	quex::GeneratedLexer* CreateLexer(string& filePath)
	{
		return new quex::GeneratedLexer(filePath);
	}

	quex::GeneratedLexer* CreateLexer(istream* stream)
	{
		return new quex::GeneratedLexer(stream);
	}

	void Analyze(string input, vector<quex::Token>& tokenList)
	{
		istringstream inputStream(input);
		quex::GeneratedLexer lexer(&inputStream);
		quex::Token* token = nullptr;

		do
		{
			lexer.receive(&token);
			if (token != nullptr)
			{
				if (token->type_id() == TKN_FAILURE)
				{
					throw UnexpectedTokenException(token->get_string());
				}
				else if (token->type_id() == TKN_UNEXPECTED_CTRL_CHAR)
				{
					throw InvalidControlCharacterException(token->get_string());
				}
			}
			tokenList.push_back(*token);
		} while (token->type_id() != TKN_TERMINATION); 
	}
}