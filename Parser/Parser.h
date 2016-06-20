#pragma once
#include <Statements.h>

namespace quex
{
	class GeneratedLexer;
}

namespace Parser
{
	Ast::GlobalStatements* Parse(std::istream* stream);
	Ast::GlobalStatements* Parse(std::string& fileName);
	Ast::GlobalStatements* Parse(quex::GeneratedLexer* qlex);
}