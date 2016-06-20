// Ruddy.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Grammar.h>
#include <Statements.h>
#include <Parser.h>

#include <string>
#include <memory>
#include <sstream>

inline std::unique_ptr<Ast::GlobalStatements> ParseTree(std::string code)
{
	std::istringstream inputStream(code);
	return std::unique_ptr<Ast::GlobalStatements>(Parser::Parse(&inputStream));
}

int _tmain(int argc, _TCHAR* argv[])
{
	// Use for testing for now
	//try
	//{
	//	auto tree = ParseTree("class A { fun B() { let c = new D(); } }");
	//}
	//catch (...)
	//{

	//}
	return 0;
}

