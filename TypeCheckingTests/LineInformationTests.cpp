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

TEST_CLASS(LineInformationTests)
{
public:
	TEST_METHOD(LineInfo_UnrecognizedType)
	{
		auto tree = ParseTree(
			"class A"						"\n"
			"{"								"\n"
				"fun B()"					"\n"
				"{"							"\n"
					"let x = new Y();"	"\n"
				"}"							"\n"
			"}");
		auto table = std::make_shared<SymbolTable>();
		try
		{
			tree->TypeCheck(table);
		}
		catch (SymbolNotDefinedException& e)
		{
			Assert::AreEqual(5, e.Location().LineNumber);
			Assert::AreEqual(13, e.Location().ColumnNumber);
		}
	}

	TEST_METHOD(LineInfo_MismatchedTypes)
	{
		auto tree = ParseTree(
			"class A"						"\n"
			"{"								"\n"
				"fun B()"					"\n"
				"{"							"\n"
					"int x = 1.2;"			"\n"
				"}"							"\n"
			"}");
		auto table = std::make_shared<SymbolTable>();
		try
		{
			tree->TypeCheck(table);
		}
		catch (TypeMismatchException& e)
		{
			Assert::AreEqual(5, e.Location().LineNumber);
			Assert::AreEqual(1, e.Location().ColumnNumber);
		}
	}
	TEST_METHOD(LineInfo_UnsafeContext)
	{
		auto tree = ParseTree(
			"class A"						"\n"
			"{"								"\n"
				"unsafe int _b[10];"		"\n"
				"fun B()"					"\n"
				"{"							"\n"
					"_b[0] = 1;"			"\n"
				"}"							"\n"
			"}");
		auto table = std::make_shared<SymbolTable>();
		try
		{
			tree->TypeCheck(table);
		}
		catch (CannotReferenceUnsafeMemberFromSafeContextException& e)
		{
			Assert::AreEqual(6, e.Location().LineNumber);
			Assert::AreEqual(1, e.Location().ColumnNumber);
		}
	}

	TEST_METHOD(LineInfo_WrongType)
	{
		auto tree = ParseTree(
			"class A { }"					"\n"
			"class B"						"\n"
			"{"								"\n"
				"fun C()"					"\n"
				"{"							"\n"
					"let b = A;"			"\n"
				"}"							"\n"
			"}");
		auto table = std::make_shared<SymbolTable>();
		try
		{
			tree->TypeCheck(table);
		}
		catch (SymbolWrongTypeException& e)
		{
			Assert::AreEqual(6, e.Location().LineNumber);
			Assert::AreEqual(9, e.Location().ColumnNumber);
		}
	}

	TEST_METHOD(LineInfo_OperationNotDefined)
	{
		auto tree = ParseTree(
			"class A { }"					"\n"
			"class B"						"\n"
			"{"								"\n"
				"fun C()"					"\n"
				"{"							"\n"
					"let a = new A();"	"\n"
					"a++;"					"\n"
				"}"							"\n"
			"}");
		auto table = std::make_shared<SymbolTable>();
		try
		{
			tree->TypeCheck(table);
		}
		catch (OperationNotDefinedException& e)
		{
			Assert::AreEqual(7, e.Location().LineNumber);
			Assert::AreEqual(2, e.Location().ColumnNumber);
		}
	}

	TEST_METHOD(LineInfo_Accessibility)
	{
		auto tree = ParseTree(
			"class A"						"\n"
			"{"								"\n"
				"private int i;"			"\n"
			"}"								"\n"
			"class B"						"\n"
			"{"								"\n"
				"fun C()"					"\n"
				"{"							"\n"
					"let a = new A();"	"\n"
					"a.i = 0;"				"\n"
				"}"							"\n"
			"}");
		auto table = std::make_shared<SymbolTable>();
		try
		{
			tree->TypeCheck(table);
		}
		catch (SymbolNotAccessableException& e)
		{
			Assert::AreEqual(10, e.Location().LineNumber);
			Assert::AreEqual(1, e.Location().ColumnNumber);
		}
	}

};

}