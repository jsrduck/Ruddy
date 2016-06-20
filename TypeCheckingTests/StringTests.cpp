#include "stdafx.h"
#include "CppUnitTest.h"
#include <Primitives.h>
#include <Operations.h>
#include <vector>
#include <Utils.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Ast;
using namespace std;
using namespace ParserTests;

namespace TypeCheckingTests {
	TEST_CLASS(BasicStringTests)
	{
	public:
		TEST_METHOD(DebugPrintStatementTest)
		{
			// Remove when we get rid of debug print statement
			auto tree = ParseTree("class A { fun B() { print(\"Hello\"); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}
		TEST_METHOD(BasicCreateStringAutoTypeTest)
		{
			auto tree = ParseTree("class A { fun B() { let s = \"Hello\"; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(BasicCreateStringExplicitTypeTest)
		{
			auto tree = ParseTree("class A { fun B() { String s = \"Hello\"; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}
	};
}