#include "stdafx.h"
#include "CppUnitTest.h"
#include <sstream>
#include <Utils.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Ast;
using namespace ParserTests;

namespace TypeCheckingTests {
	std::string _externalCode =
		"namespace external {"
		"	class A"
		"	{"
		"	}"
		"	namespace inner {"
		"		class AB"
		"		{"
		"			public AB() {}"
		"			public AB(char c) { _c = c; }"
		"			public fun() DoStuff()"
		"			{"
		"				_c++;"
		"			}"
		"			public fun(char out) DoMoreStuff(char c)"
		"			{"
		"				_c = c;"
		"				return _c;"
		"			}"
		"			public static fun(A& b) DoStaticValueStuff(A& a)"
		"			{"
		"				return a;"
		"			}"
		"			public static fun(A b) DoStaticReferenceStuff(A a)"
		"			{"
		"				return a;"
		"			}"
		"			public fun(int i, float j) ReturnInts(int a, float b)"
		"			{"
		"				return a,b;"
		"			}"
		"			public fun(int i, float64 j) ReturnInts(int a, float b, float64 c)"
		"			{"
		"				return a,b+c;"
		"			}"
		"			public fun() AllPrimitives(int a, int64 b, uint c, uint64 d, float e, float64 f, charbyte g, char h, bool i, byte j)"
		"			{"
		"			}"
		"			private char _c;"
		"			public A MyA;"
		"		}"
		"	}"
		"}";

	TEST_CLASS(SerializedSymbolTableTests)
	{
		TEST_CLASS_INITIALIZE(Setup)
		{
			auto tree = ParseTree(_externalCode);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
			std::ostringstream ostream;
			//std::ofstream ostream { "ExportedSymbolTable.txt" };
			table->Serialize(ostream, "externalLib");
			_serializedTable = ostream.str();
		}

		std::shared_ptr<SymbolTable> GetExternalTable()
		{
			std::istringstream istream { _serializedTable };
			auto table = std::make_shared<SymbolTable>();
			table->LoadFrom(istream);
			return table;
		}

		void TypeCheckAgainstDeserializedSymbolTable(const char* codeSample) 
		{
			auto tree = ParseTree(codeSample);
			auto table = GetExternalTable();
			tree->TypeCheck(table);
		}

	public:
		TEST_METHOD(UseExternalClassDefaultCtor)
		{
			TypeCheckAgainstDeserializedSymbolTable("class B { fun C() { let a = new external.A(); } }");
		}

		TEST_METHOD(UseExternalClassDefaultCtorOnStack)
		{
			TypeCheckAgainstDeserializedSymbolTable("class B { fun C() { external.A& a(); } }");
		}

		TEST_METHOD(UseExternalClassOverloadedCtors)
		{
			TypeCheckAgainstDeserializedSymbolTable("class B { fun C() { let a = new external.inner.AB(); let b = new external.inner.AB('y'); } }");
		}

		TEST_METHOD(UseExternalClassOverloadedCtorsOnStack)
		{
			TypeCheckAgainstDeserializedSymbolTable("class B { fun C() { external.inner.AB& a(); external.inner.AB& b('x'); } }");
		}

		TEST_METHOD(UseExternalClassMethod)
		{
			TypeCheckAgainstDeserializedSymbolTable("class B { fun C(external.inner.AB a) { a.DoStuff(); char x = a.DoMoreStuff('x'); } }");
		}

		TEST_METHOD(ExternalClassMethodThrowsTypeExceptionWhenAssignmentDoesntMatch)
		{
			Assert::ExpectException<TypeMismatchException>([this]()
			{
				TypeCheckAgainstDeserializedSymbolTable("class B { fun C(external.inner.AB a) { a.DoStuff(); external.A x = a.DoMoreStuff('x'); } }");
			});
		}

		TEST_METHOD(ExternalClassMethodThrowsWhenFunctionCallSignatureWrong)
		{
			Assert::ExpectException<TypeMismatchException>([this]()
			{
				TypeCheckAgainstDeserializedSymbolTable("class B { fun C(external.inner.AB a) { a.DoStuff(); let x = a.DoMoreStuff(a); } }");
			});
		}

		TEST_METHOD(UseExternalStaticFunction)
		{
			TypeCheckAgainstDeserializedSymbolTable("class B { fun C() { external.A& a(); external.A& b = external.inner.AB.DoStaticValueStuff(a); } }");
		}

		TEST_METHOD(UseExternalStaticFunction2)
		{
			TypeCheckAgainstDeserializedSymbolTable("class B { fun C() { let a = new external.A(); let b = external.inner.AB.DoStaticReferenceStuff(a); } }");
		}

		TEST_METHOD(UseExternalClassMethodOverloaded)
		{
			TypeCheckAgainstDeserializedSymbolTable("class B { fun C(external.inner.AB a) { int b, float c = a.ReturnInts(1, 2.3); int d, float64 e = a.ReturnInts(1, 2.3, 5.6); } }");
		}

		TEST_METHOD(UsePublicExternalClassMember)
		{
			TypeCheckAgainstDeserializedSymbolTable("class B { fun(external.A retVal) C(external.inner.AB a) { let b = a.MyA; a.MyA = new external.A(); return b; } }");
		}

		TEST_METHOD(PrivateExternalClassMemberNotVisible)
		{
			Assert::ExpectException<SymbolNotDefinedException>([this]()
			{
				TypeCheckAgainstDeserializedSymbolTable("class B { fun C(external.inner.AB a) { let b = a._c; } }");
			});
		}

		TEST_METHOD(AllPrimitiveTypesInExternalCodeWork)
		{
			TypeCheckAgainstDeserializedSymbolTable("class B { fun C(external.inner.AB ab) { int a = 0; int64 b = 2147483648; uint c = 2147483648; uint64 d = 4294967296; float e = 1.1; float64 f = 3.40282e39; charbyte g = 'x'; char h = '\\u0058'; bool i = false; byte j = 0x2; ab.AllPrimitives(a,b,c,d,e,f,g,h,i,j); } }");
		}

	private:
		//static std::shared_ptr<SymbolTable> _deserializedTable;
		static std::string _serializedTable;
	};

	//std::shared_ptr<SymbolTable> SerializedSymbolTableTests::_deserializedTable;
	std::string SerializedSymbolTableTests::_serializedTable;
}