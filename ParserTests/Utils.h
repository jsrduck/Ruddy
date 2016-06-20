#pragma once
#include <string>
#include <memory>
#include <Statements.h>
#include <Parser.h>

namespace ParserTests
{
	inline std::unique_ptr<Ast::GlobalStatements> ParseTree(std::string code)
	{
		std::istringstream inputStream(code);
		return std::unique_ptr<Ast::GlobalStatements>(Parser::Parse(&inputStream));
	}
}