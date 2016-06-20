#pragma once

#include "TokenIds.h"
#include "GeneratedLexer"
#include <memory>
#include <vector>
#include <string>

namespace Lexer
{
	quex::GeneratedLexer* CreateLexer(std::string& filePath);
	quex::GeneratedLexer* CreateLexer(std::istream* stream);
	void Analyze(std::string input, std::vector<quex::Token>& tokenList);

	class UnexpectedTokenException : public std::exception
	{
	public:
		UnexpectedTokenException(std::string lexeme) : _lexeme(lexeme) {}
		
		virtual const char * what() const { return _lexeme.c_str(); }
		
	private:
		std::string _lexeme;
	};

	class InvalidControlCharacterException : public std::exception
	{
	public:
		InvalidControlCharacterException(std::string lexeme) : _lexeme(lexeme) {}

		virtual const char * what() const { return _lexeme.c_str(); }

	private:
		std::string _lexeme;
	};
}