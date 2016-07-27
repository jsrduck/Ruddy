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

namespace TypeCheckingTests 
{
	TEST_CLASS(AssignmentTypeCheckingTests)
	{
	public:
		TEST_METHOD(StrongTypeAssignFromNewPasses)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A a = new A(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(AutoTypeAssignFromNewPasses)
		{
			auto tree = ParseTree("class A {} class B { fun C() { let a = new A(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(StrongTypeAssignFromSameTypedVariablePasses)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A a = new A(); A b = a; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(AutoTypeAssignFromVariablePasses)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A a = new A(); let b = a; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(StrongTypeAssignFromWrongTypedVariableFails)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A a = new A(); B b = a; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(AssignToWrongTypeFails)
		{
			auto tree = ParseTree("class A {} class D {} class B { fun C() { D d = new A(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(AssignToNonExistingTypeFails)
		{
			auto tree = ParseTree("class A {} class B { fun C() { D d = new A(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotDefinedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(AssignVariableToClassNameFails)
		{
			auto tree = ParseTree("class A {} class B { fun C() { let b = A; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolWrongTypeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(DeclareVariableWithNonClassNameFails)
		{
			auto tree = ParseTree("class A { fun B() { } fun C() { B b = A; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolWrongTypeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(MemberVariableAssignFromLiteralPasses)
		{
			auto tree = ParseTree("class A {} class B { int _a = 0; }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(MemberVariableAssignFromNewPasses)
		{
			auto tree = ParseTree("class A {} class B { A _a; fun C() { _a = new A(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(MemberVariableLiteralAssignFromWrongTypeFails)
		{
			auto tree = ParseTree("class A {} class B { int _a = true; }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(MemberVariableAssignFromWrongTypeFails)
		{
			auto tree = ParseTree("class A {} class D {} class B { A _a; fun C() { _a = new D(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(DeclareMemberVariableWithNonClassTypeFails)
		{
			auto tree = ParseTree("class A {} class B { fun C() { } C _c; }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolWrongTypeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}
	};

	TEST_CLASS(NamespaceTests)
	{
		TEST_METHOD(ReferenceToClassInSameNamespaceSucceeds)
		{
			auto tree = ParseTree("namespace N { class A { } class B { A _a; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceToClassInAnotherNamespaceFails)
		{
			auto tree = ParseTree("namespace N { class A { } } namespace O { class B { A _a; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotDefinedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(ReferenceToClassInParentNamespaceSucceeds)
		{
			auto tree = ParseTree("namespace N { class A { } namespace O { class B { A _a; } } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceToMemberInAnotherClassWithoutClassQualifierFails)
		{
			auto tree = ParseTree("class A { int i; } class B { fun C() { i = 0; }} ");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotDefinedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}
	};

	TEST_CLASS(VisibilityTests)
	{
		TEST_METHOD(ReferenceToAnyVisibilityInSameClassSucceeds)
		{
			auto tree = ParseTree("class A { public int i; protected bool j; private int k; fun Foo() { i = 0; j = true; k = 1; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceToPublicMemberInAnotherClassSucceeds)
		{
			auto tree = ParseTree("class A { public int i; } class B { fun C() { A a = new A(); a.i = 0; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceToPublicStaticMemberInAnotherClassSucceeds)
		{
			auto tree = ParseTree("class A { public static int i; } class B { fun C() { A.i = 0; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceToMemberSeveralMembersDeepSucceeds)
		{
			auto tree = ParseTree("namespace N { class A { public int i; } } class B { N.A _a; } class C { fun D() { B b = new B(); b._a.i = 1; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceToPrivateMemberInAnotherClassFails)
		{
			auto tree = ParseTree("class A { private int i; } class B { fun C() { A a = new A(); a.i = 0; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotAccessableException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(ReferenceToPrivateStaticMemberInAnotherClassFails)
		{
			auto tree = ParseTree("class A { private static int i; } class B { fun C() { A.i = 0; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotAccessableException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(ReferenceToProtectedMemberInAnotherClassFails)
		{
			auto tree = ParseTree("class A { protected int i; } class B { fun C() { A a = new A(); a.i = 0; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotAccessableException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(ReferenceToProtectedStaticMemberInAnotherClassFails)
		{
			auto tree = ParseTree("class A { protected static int i; } class B { fun C() { A.i = 0; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotAccessableException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}
	};

	TEST_CLASS(FunctionTests)
	{
		TEST_METHOD(BasicFunctionCallSucceeds)
		{
			auto tree = ParseTree("class A { fun B() { } fun C() { B(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallSimpleReturnTypeSucceeds)
		{
			auto tree = ParseTree("class A { fun(int x) B() { return 0; } fun C() { int x = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallClassReturnTypeSucceds)
		{
			auto tree = ParseTree("class D { } class A { fun(D d) B() { return new D(); } fun C() { D d = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallMultiReturnTypeSucceeds)
		{
			auto tree = ParseTree("class A { fun(int i, char j) B() { return 0,'a'; } fun C() { int k, char l = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallExtraMultiReturnTypeSucceeds)
		{
			auto tree = ParseTree("class A { fun(int i, char j) B() { return 0,'a'; } fun C() { char m = 'x'; int k, m = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallMultiReturnTypeIsValidArgument)
		{
			auto tree = ParseTree("class A { fun(int i, char j) B { return 0,'a'; } fun C(int i, char j) { } fun D() { C(B()); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}
	};
}