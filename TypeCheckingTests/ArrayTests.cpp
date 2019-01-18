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
	TEST_CLASS(UnsafeArrayTests)
	{
	public:
		TEST_METHOD(UnsafeIntArrayDeclarationPasses)
		{
			auto tree = ParseTree("class A { fun B() { unsafe { int buffer[5]; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(UnsafeIntArrayTooLargeRankFails)
		{
			auto tree = ParseTree("class A { fun B() { unsafe { int buffer[0xFFFFFFFFF]; } } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<OverflowException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(UnsafeArrayAssignmentFails)
		{
			auto tree = ParseTree("class A { fun B() { unsafe { int buffer[5]; int buffer2[5]; buffer = buffer2; } } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeNotAssignableException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(UnsafeArrayReadSucceeds)
		{
			auto tree = ParseTree("class A { fun B() { unsafe { int buffer[5]; int i = buffer[0]; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(UnsafeArrayReadIntoWrongTypeFails)
		{
			auto tree = ParseTree("class A { fun B() { unsafe { int buffer[5]; bool i = buffer[0]; } } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(UnsafeArrayWriteSucceeds)
		{
			auto tree = ParseTree("class A { fun B() { unsafe { int buffer[5]; buffer[0] = 5; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(UnsafeArrayWriteUsingVariableSucceeds)
		{
			auto tree = ParseTree("class A { int _i; fun B() { unsafe { int buffer[5]; buffer[0] = _i; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(UnsafeArrayWriteWrongTypeFails)
		{
			auto tree = ParseTree("class A { fun B() { unsafe { int buffer[5]; buffer[0] = true; } } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(UnsafeArrayOfReferenceTypesSucceeds)
		{
			auto tree = ParseTree("class A { fun B() { unsafe { A buffer[5]; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(UnsafeArrayOfReferenceTypesSucceedsToRead)
		{
			auto tree = ParseTree("class A { fun B() { unsafe { A buffer[5]; let a = buffer[0]; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(UnsafeArrayOfReferenceTypesSucceedsToWrite)
		{
			auto tree = ParseTree("class A { fun B() { unsafe { A buffer[5]; buffer[0] = new A(); } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(UnsafeArrayOfReferenceTypesSucceedsToReadMember)
		{
			auto tree = ParseTree("class A { int _i; fun B() { unsafe { A buffer[5]; let i = buffer[0]._i; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(UnsafeArrayOfReferenceTypesSucceedsToWriteMember)
		{
			auto tree = ParseTree("class A { int _i; fun B() { unsafe { A buffer[5]; buffer[0]._i = 0; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(UnsafeArrayCanBeReferencedInUnsafeContext)
		{
			auto tree = ParseTree("class A { unsafe int _buffer[10]; fun B() { unsafe { _buffer[0] = 1; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(UnsafeArrayCannotBeReferencedOutsideUnsafeContext)
		{
			auto tree = ParseTree("class A { unsafe int _buffer[10]; fun B() { _buffer[0] = 1; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<CannotReferenceUnsafeMemberFromSafeContextException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(UnsafeArrayCanBeReferencedInUnsafeFunction)
		{
			auto tree = ParseTree("class A { unsafe int _buffer[10]; unsafe fun B() { _buffer[0] = 1; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}
	};
}