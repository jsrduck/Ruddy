#include "stdafx.h"
#include "CppUnitTest.h"
#include <Statements.h>
#include <Classes.h>
#include "Utils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace Parser;
using namespace Ast;

namespace ParserTests {

	std::shared_ptr<ClassMemberDeclaration> GetFirstMember(GlobalStatements* tree)
	{
		auto theClass = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
		Assert::IsNotNull(theClass);
		return dynamic_pointer_cast<ClassMemberDeclaration>(theClass->_list->_statement);
	}

	std::shared_ptr<ClassMemberDeclaration> GetSecondMember(GlobalStatements* tree)
	{
		auto theClass = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
		Assert::IsNotNull(theClass);
		return dynamic_pointer_cast<ClassMemberDeclaration>(theClass->_list->_next->_statement);
	}

	TEST_CLASS(NumberPrimitives)
	{
	public:

		TEST_METHOD(ByteSizedConstantParsedAsByte)
		{
			auto tree = ParseTree("class A { byte b = 255; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual((uint8_t)255, dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsByte());
		}

		TEST_METHOD(Int32SizedConstantParsedAsInteger)
		{
			auto tree = ParseTree("class A { int b = 256; int c = 2147483647; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual(256, dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsInt32());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(c->_defaultValue) != nullptr);
			Assert::AreEqual(2147483647, dynamic_pointer_cast<IntegerConstant>(c->_defaultValue)->AsInt32());
		}

		TEST_METHOD(Int32SizedNegativeConstantParsedAsInteger)
		{
			auto tree = ParseTree("class A { int b = -1; int c = -2147483648; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual(-1, dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsInt32());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(c->_defaultValue) != nullptr);
			Assert::AreEqual(-2147483648i32, dynamic_pointer_cast<IntegerConstant>(c->_defaultValue)->AsInt32());
		}

		TEST_METHOD(UInt32SizedConstantParsedAsInteger)
		{
			auto tree = ParseTree("class A { uint b = 2147483648; uint c = 4294967295; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual((uint32_t)2147483648, dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsUInt32());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(c->_defaultValue) != nullptr);
			Assert::AreEqual((uint32_t)4294967295, dynamic_pointer_cast<IntegerConstant>(c->_defaultValue)->AsUInt32());
		}

		TEST_METHOD(Int64SizeConstantParsedAsInteger)
		{
			auto tree = ParseTree("class A { int64 b = 2147483648; int64 c = -2147483649; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::IsTrue(2147483648 == dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsInt64());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(c->_defaultValue) != nullptr);
			Assert::IsTrue(-2147483649i64 == dynamic_pointer_cast<IntegerConstant>(c->_defaultValue)->AsInt64());
		}

		TEST_METHOD(Float32SizeConstantParsedAsFloat)
		{
			auto tree = ParseTree("class A { float b = 10.375; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<FloatingConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual(10.375f, dynamic_pointer_cast<FloatingConstant>(b->_defaultValue)->AsFloat32());
		}

		TEST_METHOD(Float32SizeMinMaxConstantParsedAsFloat)
		{
			auto tree = ParseTree("class A { float b = -3.40282e38; float c = 3.40282e38; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<FloatingConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual(-3.40282e+38f, dynamic_pointer_cast<FloatingConstant>(b->_defaultValue)->AsFloat32());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<FloatingConstant>(c->_defaultValue) != nullptr);
			Assert::AreEqual(3.40282e+38f, dynamic_pointer_cast<FloatingConstant>(c->_defaultValue)->AsFloat32());
		}

		TEST_METHOD(Float32SizeTinyConstantParsedAsFloat)
		{
			auto tree = ParseTree("class A { float b = 1.17549e-38; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<FloatingConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual(1.17549e-38f, dynamic_pointer_cast<FloatingConstant>(b->_defaultValue)->AsFloat32());
		}

		TEST_METHOD(Float64SizeConstantParsedAsFloat)
		{
			auto tree = ParseTree("class A { float64 b = 3.40282e39; float64 c = -3.40282e39; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<FloatingConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual(3.40282e+39, dynamic_pointer_cast<FloatingConstant>(b->_defaultValue)->AsFloat64());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<FloatingConstant>(c->_defaultValue) != nullptr);
			Assert::AreEqual(-3.40282e+39, dynamic_pointer_cast<FloatingConstant>(c->_defaultValue)->AsFloat64());
		}

		TEST_METHOD(Float64SizeMinMaxConstantParsedAsFloat)
		{
			auto tree = ParseTree("class A { float64 b = -1.79769e308; float64 c = 1.79769e308; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<FloatingConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual(-1.79769e+308, dynamic_pointer_cast<FloatingConstant>(b->_defaultValue)->AsFloat64());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<FloatingConstant>(c->_defaultValue) != nullptr);
			Assert::AreEqual(1.79769e+308, dynamic_pointer_cast<FloatingConstant>(c->_defaultValue)->AsFloat64());
		}

		TEST_METHOD(Float64SizeTinyConstantParsedAsFloat)
		{
			auto tree = ParseTree("class A { float64 b = 1.17549e-39; float64 c = 2.22507e-308; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<FloatingConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual(1.17549e-39, dynamic_pointer_cast<FloatingConstant>(b->_defaultValue)->AsFloat64());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<FloatingConstant>(c->_defaultValue) != nullptr);
			Assert::AreEqual(2.22507e-308, dynamic_pointer_cast<FloatingConstant>(c->_defaultValue)->AsFloat64());
		}

		TEST_METHOD(ByteSizedHexConstantParsedAsByte)
		{
			auto tree = ParseTree("class A { byte b = 0xFF; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual((uint8_t) 255, dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsByte());
		}

		TEST_METHOD(Int32SizedHexConstantParsedAsInteger)
		{
			auto tree = ParseTree("class A { int b = 0x100; int c = 0x7FFFFFFF; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual(256, dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsInt32());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(c->_defaultValue) != nullptr);
			Assert::AreEqual(2147483647, dynamic_pointer_cast<IntegerConstant>(c->_defaultValue)->AsInt32());
		}

		TEST_METHOD(Int32SizedNegativeHexConstantParsedAsInteger)
		{
			auto tree = ParseTree("class A { int b = 0xFFFFFFFF; int c = 0x80000000; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual(-1, dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsInt32());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(c->_defaultValue) != nullptr);
			Assert::AreEqual(-2147483648i32, dynamic_pointer_cast<IntegerConstant>(c->_defaultValue)->AsInt32());
		}

		TEST_METHOD(UInt32SizedHexConstantParsedAsInteger)
		{
			auto tree = ParseTree("class A { uint b = 0x80000000; uint c = 0xFFFFFFFF; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::AreEqual((uint32_t) 2147483648, dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsUInt32());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(c->_defaultValue) != nullptr);
			Assert::AreEqual((uint32_t) 4294967295, dynamic_pointer_cast<IntegerConstant>(c->_defaultValue)->AsUInt32());
		}

		TEST_METHOD(Int64SizeHexConstantParsedAsInteger)
		{
			auto tree = ParseTree("class A { int64 b = 0x80000000; int64 c = 0xFFFFFFFF7FFFFFFF; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::IsTrue(2147483648 == dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsInt64());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(c->_defaultValue) != nullptr);
			Assert::IsTrue(-2147483649i64 == dynamic_pointer_cast<IntegerConstant>(c->_defaultValue)->AsInt64());
		}

		TEST_METHOD(Int64Sized8BitHexConstantParsedAsPositiveInteger)
		{
			auto tree = ParseTree("class A { int64 b = 0xFFFFFFFF; int64 c = 0x80000000; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::IsTrue(4294967295 == dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsInt64());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(c->_defaultValue) != nullptr);
			Assert::IsTrue(2147483648 == dynamic_pointer_cast<IntegerConstant>(c->_defaultValue)->AsInt64());
		}

		TEST_METHOD(Int64SizedIntConstantParsedAsPositiveInteger)
		{
			auto tree = ParseTree("class A { int64 b = -1; int64 c = -2147483648; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(b->_defaultValue) != nullptr);
			Assert::IsTrue(-1 == dynamic_pointer_cast<IntegerConstant>(b->_defaultValue)->AsInt64());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(c->_defaultValue) != nullptr);
			Assert::IsTrue(-2147483648i64 == dynamic_pointer_cast<IntegerConstant>(c->_defaultValue)->AsInt64());
		}

	};

	TEST_CLASS(CharPrimitives)
	{
		TEST_METHOD(CharLiteralParsedAsChar)
		{
			auto tree = ParseTree("class A  { char b = 'b'; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<CharConstant>(b->_defaultValue) != nullptr);
			Assert::IsTrue(L'b' == dynamic_pointer_cast<CharConstant>(b->_defaultValue)->Value());
		}

		TEST_METHOD(CharUnicodeLiteralParsedAsChar)
		{
			auto tree = ParseTree("class A  { char b = '\\u0058'; char c = '\\u00BF'; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<CharConstant>(b->_defaultValue) != nullptr);
			Assert::IsTrue(L'X' == dynamic_pointer_cast<CharConstant>(b->_defaultValue)->Value());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<CharConstant>(c->_defaultValue) != nullptr);
			Assert::IsTrue(L'¿' == dynamic_pointer_cast<CharConstant>(c->_defaultValue)->Value());
		}

		TEST_METHOD(CharHexLiteralParsedAsChar)
		{
			auto tree = ParseTree("class A  { char b = '\\x0058'; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<CharConstant>(b->_defaultValue) != nullptr);
			Assert::IsTrue(L'X' == dynamic_pointer_cast<CharConstant>(b->_defaultValue)->Value());
		}

		TEST_METHOD(CharEscapeLiteralParsedAsChar)
		{
			auto tree = ParseTree("class A  { char b = '\\''; char c = '\\n'; }");
			auto b = GetFirstMember(tree.get());
			Assert::IsTrue(b != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<CharConstant>(b->_defaultValue) != nullptr);
			Assert::IsTrue(L'\'' == dynamic_pointer_cast<CharConstant>(b->_defaultValue)->Value());
			auto c = GetSecondMember(tree.get());
			Assert::IsTrue(c != nullptr);
			Assert::IsTrue(dynamic_pointer_cast<CharConstant>(c->_defaultValue) != nullptr);
			Assert::IsTrue(L'\n' == dynamic_pointer_cast<CharConstant>(c->_defaultValue)->Value());
		}
	};
}