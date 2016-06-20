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
	TEST_CLASS(LoopTests)
	{
	public:
		TEST_METHOD(WhileLoopPasses)
		{
			auto tree = ParseTree("class A { fun B() { while (true) { break; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(WhileLoopWithReferencePasses)
		{
			auto tree = ParseTree("class A { fun B() { int i = 0; bool b = true; while (b) { i++; b = false; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(WhileLoopWithWrongTypeFails)
		{
			auto tree = ParseTree("class A { fun B() { int i = 0; while (i) { break; } } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(BreakOutsideOfLoopFails)
		{
			auto tree = ParseTree("class A { fun B() { break; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<BreakInWrongPlaceException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(NestedLoopsSucceed)
		{
			auto tree = ParseTree("class A { fun B() { while (true) { bool b = true; while (b) { b = false; } break; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(VariablesInsideLoopFallOutOfScope)
		{
			auto tree = ParseTree("class A { fun B() { while (true) { bool b = true; while (b) { int c = 0; b = false; } c = 1; break; } } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotDefinedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}
	};
}