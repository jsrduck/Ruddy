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

		TEST_METHOD(AssignFromStackAllocatedClassPasses)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A& a(); A& b = a; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(AutoTypeAssignFromStackAllocatedVariablePasses)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A& a(); let b = a; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(StrongTypeAssignFromSameTypedStackAllocatedVariablePasses)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A& a(); A& b = a; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(AssignReferenceTypeToValueTypeFails)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A a = new A(); A& b = a; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(AssignValueTypeToReferenceTypeFails)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A& a(); A b = a; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(StrongTypeAssignStackAllocatedVariableFromWrongTypedVariableFails)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A& a(); B& b = a; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(StackAllocatedAssignToWrongTypeFails)
		{
			auto tree = ParseTree("class A {} class D {} class B { fun C() { let a = new A(); D& d = a; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(StackAllocatedAssignToNonExistingTypeFails)
		{
			auto tree = ParseTree("class B { fun C() { A& a(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotDefinedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(DeclareStackAllocatedVariableWithNonClassNameFails)
		{
			auto tree = ParseTree("class A { fun B() { } fun C() { B& b = A; } }");
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

		TEST_METHOD(FunctionCallWithArgumentsSucceeds)
		{
			auto tree = ParseTree("class A { fun C(int i, char j) { } fun D() { C(0, 'x'); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallWrongArgumentTypeFails)
		{
			auto tree = ParseTree("class A { fun C(int i, char j) { } fun D() { C(0, \"Hello\"); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(FunctionCallValueClassReturnTypeSucceds)
		{
			auto tree = ParseTree("class D {} class A { fun(D& d) B() { D& d(); return d; } fun C() { D& d = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallValueClassReturnTypeReturnReferenceTypeFails)
		{
			auto tree = ParseTree("class D {} class A { fun(D& d) B() { return new D(); } fun C() { D& d = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(FunctionCallReferenceClassReturnTypeReturnValueTypeFails)
		{
			auto tree = ParseTree("class D {} class A { fun(D d) B() { D& d(); return d; } fun C() { D d = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(FunctionCallReferenceClassReturnTypeAssignToValueTypeFails)
		{
			auto tree = ParseTree("class D {} class A { fun(D d) B() { return new D(); } fun C() { D& d = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(FunctionCallValueClassReturnTypeAssignToReferenceTypeFails)
		{
			auto tree = ParseTree("class D {} class A { fun(D& d) B() { D& d(); return d; } fun C() { D d = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(FunctionCallWithValueArgumentTypeSucceeds)
		{
			auto tree = ParseTree("class D {} class A { fun C(D& d) { D& e = d;} fun B() { D& d(); C(d); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallPassReferenceTypeToValueArgumentTypeFails)
		{
			auto tree = ParseTree("class D {} class A { fun C(D& d) {} fun B() {  C(new D()); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(FunctionCallPassValueTypeToReferenceArgumentTypeFails)
		{
			auto tree = ParseTree("class D {} class A { fun C(D d) {} fun B() {  D& d(); C(d); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
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

		TEST_METHOD(FunctionCallWrongMultiReturnTypeFails)
		{
			auto tree = ParseTree("class A { fun(int i, uint64 j) B { return 0,'a'; } fun C(int i, char j) { } fun D() { C(B()); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(FunctionCallMultiReturnTypePlusAdditionalArgumentIsValidArgument)
		{
			auto tree = ParseTree("class A { fun(int i, char j) B { return 0,'a'; } fun C(int i, char j, int k) { } fun D() { C(B(), 1); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallMultiReturnTypeAfterAdditionalArgumentIsValidArgument)
		{
			auto tree = ParseTree("class A { fun(int i, char j) B { return 0,'a'; } fun C(int i, int j, char k) { } fun D() { C(1, B()); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallMultiReturnTypeArgumentsAreValidArguments)
		{
			auto tree = ParseTree("class A { fun(int i, int j) B { return 0,1; } fun(char i, char j) C { return 'a','b'; } fun() D(int i, int j, char k, char l) { } fun E() { D(B(),C()); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallMultiReturnTypeArgumentsPlusAdditionalArgumentAreValidArguments)
		{
			auto tree = ParseTree("class A { fun(int i, int j) B { return 0,1; } fun(char i, char j) C { return 'a','b'; } fun() D(int i, int j, int m, char k, char l) { } fun E() { D(B(), 1, C()); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallMultiReturnTypeArgumentsAreMixOfValueAndReferenceTypesSucceeds)
		{
			auto tree = ParseTree("class D {} class A { fun(D& i, D j) B() { D& k(); D l = new D(); return k,l; } fun C() { D& k, D l = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(FunctionCallMultiReturnTypeArgumentsAreMixOfValueAndReferenceTypesButWrongTypesReturnedFails)
		{
			auto tree = ParseTree("class D {} class A { fun(D& i, D j) B() { D& k(); D l = new D(); return l,k; } fun C() { D& k, D l = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(FunctionCallMultiReturnTypeArgumentsAreMixOfValueAndReferenceTypesButAssignedToWrongTypesFails)
		{
			auto tree = ParseTree("class D {} class A { fun(D& i, D j) B() { D& k(); D l = new D(); return k,l; } fun C() { D k, D& l = B(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<TypeMismatchException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(StaticFunctionReferenceToNonStaticMemberVariableFails)
		{
			auto tree = ParseTree("class A { bool b; static fun(bool ret) Foo() { return b; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<NonStaticMemberReferencedFromStaticContextException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(StaticFunctionReferenceToNonStaticMemberOfLocalVariableOfSameClassSucceeds)
		{
			auto tree = ParseTree("class A { bool b; static fun(bool ret) Foo() { A a = new A(); return a.b; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(StaticFunctionReferenceToNonStaticMemberOfLocalVariableSucceeds)
		{
			auto tree = ParseTree("class A { bool b; } class B { static fun(bool ret) Foo() { A a = new A(); return a.b; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(OverloadedStaticFunctionsSupported)
		{
			auto tree = ParseTree("class A { static fun Foo(int a) { } static fun Foo(int a, int b) { } static fun Foo(bool b) { } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(OverloadedStaticFunctionCallsSupported)
		{
			auto tree = ParseTree("class A { static fun Foo(int a) { } static fun Foo(int a, int b) { } static fun Foo(bool b) { } } class B { fun Go() { A.Foo(1); A.Foo(1,2); A.Foo(true); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(OverloadedMethodsSupported)
		{
			auto tree = ParseTree("class A { fun Foo(int a) { } fun Foo(int a, int b) { } fun Foo(bool b) { } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(OverloadedMethodCallsSupported)
		{
			auto tree = ParseTree("class A { fun Foo(int a) { } fun Foo(int a, int b) { } fun Foo(bool b) { } } class B { fun Go() { A a = new A(); a.Foo(1); a.Foo(1,2); a.Foo(true); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(OverloadedFunctionsWithSameInputArgsNotSupported)
		{
			auto tree = ParseTree("class A { static fun Foo(int a) { } static fun Foo(int a, int b) { } static fun Foo(int b) { } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolAlreadyDefinedInThisScopeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(OverloadedFunctionsWithSameInputArgsButDifferentOutputArgsNotSupported)
		{
			auto tree = ParseTree("class A { static fun Foo(int a) { } static fun Foo(int a, int b) { } static fun(int ret) Foo(int b) { return b; } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolAlreadyDefinedInThisScopeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}
	};

	TEST_CLASS(ConstructorTests)
	{
		TEST_METHOD(CallToDefaultCtorFailsWhenExplicitCtorDeclared)
		{
			auto tree = ParseTree("class A { public A(int i) { } } class B { fun C() { A a = new A(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<NoMatchingFunctionSignatureFoundException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(CallToDefaultCtorSucceedsWhenMultipleCtorsDeclared)
		{
			auto tree = ParseTree("class A { public A() {} public A(int i) {} } class B { fun C() { A a = new A(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(CallToExplicitCtorSucceedsWhenMultipleCtorsDeclared)
		{
			auto tree = ParseTree("class A { public A() {} public A(int i) {} } class B { fun C() { A a = new A(0); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(CallToExplicitCtorFailsWhenNoCtorsDeclared)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A a = new A(0); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<NoMatchingFunctionSignatureFoundException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(CallToCtorThatDoesNotExistFails)
		{
			auto tree = ParseTree("class A { A(bool j) {} } class B { fun C() { A a = new A(0); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<NoMatchingFunctionSignatureFoundException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(SameCtorDeclaredTwiceFails)
		{
			auto tree = ParseTree("class A { public A(int i) { } public A(int j) { } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolAlreadyDefinedInThisScopeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(AmbiguousCtorsFails)
		{
			auto tree = ParseTree("class A { public A(int i) { } public A(int64 j) { } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolAlreadyDefinedInThisScopeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(AmbiguousCtorsFails2)
		{
			auto tree = ParseTree("class A { public A(int64 i) { } public A(int j) { } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolAlreadyDefinedInThisScopeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(SameDefaultCtorDeclaredTwiceFails)
		{
			auto tree = ParseTree("class A { public A() {} public A() {} }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolAlreadyDefinedInThisScopeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(StackAllocatedClassSucceeds)
		{
			auto tree = ParseTree("class A {} class B { fun C() { A& a(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(StackAllocatedClassNonDefaultCtorSucceeds)
		{
			auto tree = ParseTree("class A { public A(int i) {} } class B { fun C() { A& a(0); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(StackAllocatedCallToDefaultCtorFailsWhenExplicitCtorDeclared)
		{
			auto tree = ParseTree("class A { public A(int i) { } } class B { fun C() { A& a(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<NoMatchingFunctionSignatureFoundException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(StackAllocatedCallToDefaultCtorSucceedsWhenMultipleCtorsDeclared)
		{
			auto tree = ParseTree("class A { public A() {} public A(int i) {} } class B { fun C() { A& a(); } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceMemberVariableInstantiateSucceeds)
		{
			auto tree = ParseTree("class A {} class B { A& _a; B() : _a() {} }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceMemberVariableAssignFromNewFails)
		{
			auto tree = ParseTree("class A {} class B { A& _a; B() : _a(new A()) {} }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<NoMatchingFunctionSignatureFoundException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(PointerMemberVariableInstantiateFails)
		{
			auto tree = ParseTree("class A {} class B { A _a; B() : _a() {} }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<ExpectedValueTypeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(ReferenceMemberVariableConstructedWithStaticMemberOfAnotherClassSucceeds)
		{
			auto tree = ParseTree("class A { int _i; static int J; A(int i) { _i = i; } } class B { A& _a; B() : _a(A.J) {} }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceMemberVariableConstructWithInitializedVariableSucceeds)
		{
			auto tree = ParseTree("class A { int _i; A(int i) { _i = i; } } class B { A& _a; A& _b; B() : _a(0), _b(_a._i) {} }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceMemberVariableConstructWithUninitializedVariableFails)
		{
			auto tree = ParseTree("class A { int _i; A(int i) { _i = i; } } class B { A& _a; A& _b; B() : _a(_b._i), _b(0) {} }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<UninitializedVariableReferencedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(ReferenceMemberVariableConstructWithMemerOfAnotherClassWithSameNameAsUninitializedVariableSucceeds)
		{
			auto tree = ParseTree("class A { int _b; A(int i) { _b = i; } } class B { A& _a; A& _b; B(A& a) : _a(a._b), _b(0) {} }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceMemberVariableConstructWithInitializedVariableFunctionCallSucceeds)
		{
			auto tree = ParseTree("class A { int _i;  A(int i) { _i = i; } fun(int i) GetInt() { return _i; } } class B { A& _a; A& _b; B() : _a(0), _b(_a.GetInt()) {} }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceMemberVariableConstructWithUninitializedVariableFunctionCallFails)
		{
			auto tree = ParseTree("class A { int _i;  A(int i) { _i = i; } fun(int i) GetInt() { return _i; } } class B { A& _a; A& _b; B() : _a(_b.GetInt()), _b(0) {} }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<UninitializedVariableReferencedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(ReferenceMemberVariableConstructWithInitializedVariableAsPartOfExpressionSucceeds)
		{
			auto tree = ParseTree("class A { int _i; A(int i) { _i = i; } } class B { A& _a; A& _b; B() : _a(0), _b(0 + 3 * _a._i) {} }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ReferenceMemberVariableConstructWithUninitializedVariableAsPartOfExpressionFails)
		{
			auto tree = ParseTree("class A { int _i; A(int i) { _i = i; } } class B { A& _a; A& _b; B() : _a(0 + 3 * _b._i), _b(0) {} }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<UninitializedVariableReferencedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(PrimitiveTypeCanNotBeInitializedInInitializer)
		{
			auto tree = ParseTree("class B { int _a; B() : _a(1) {} }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<ExpectedValueTypeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(DeclareReferenceMemberVariableWithNonClassTypeFails)
		{
			auto tree = ParseTree("class A {} class B { fun C() { } C& _c; }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolWrongTypeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(DeclareReferenceMemberAllImplicitCtorsSucceeds)
		{
			auto tree = ParseTree("class A {} class B { A& _a; }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(DeclareReferenceMemberWithImplicitCtorInClassWithExplicitCtorSucceeds)
		{
			auto tree = ParseTree("class A {} class B { A& _a; B() {} }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(DeclareReferenceMemberWithExplicitCtorAndInstantiateItSucceeds)
		{
			auto tree = ParseTree("class A { int _i; A(int i) { _i = i; } } class B { A& _a; B() : _a(0) {} }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(DeclareReferenceMemberWithExplicitCtorInClassWithNoCtorFails)
		{
			auto tree = ParseTree("class A { int _i; A(int i) { _i = i; } } class B { A& _a; }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<ValueTypeMustBeInitializedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(DeclareReferenceMemberWithExplicitCtorInClassWithExplicitCtorThatDoesntCallItFails)
		{
			auto tree = ParseTree("class A { int _i; A(int i) { _i = i; } } class B { A& _a; B() {} }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<ValueTypeMustBeInitializedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(InitializeReferenceMemberTwiceFails)
		{
			auto tree = ParseTree("class A {} class B { A& _a; B() : _a(), _a() {} }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<CannotReinitializeMemberException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(ExplicitCallToCtorFails)
		{
			auto tree = ParseTree("class A { } class B { fun Foo() { A.A(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotDefinedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(ExplicitCallToLocalCtorFails)
		{
			auto tree = ParseTree("class A { fun Foo() { A(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolWrongTypeException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}

		TEST_METHOD(ExplicitCallToVarCtorFails)
		{
			auto tree = ParseTree("class A { fun Foo() { A a = new A(); a.A(); } }");
			auto table = std::make_shared<SymbolTable>();
			Assert::ExpectException<SymbolNotDefinedException>([this, &tree, &table]()
			{
				tree->TypeCheck(table);
			});
		}
	};

	TEST_CLASS(MultiPassTests)
	{
		TEST_METHOD(ClassHasReferenceToClassThatComesAfter)
		{
			auto tree = ParseTree("class B { A _a; } class A { } ");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ClassReferencesMemberOfClassThatComesAfter)
		{
			auto tree = ParseTree("class B { fun Foo() { let a = new A(); a.i = 1; } } class A { int i; }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(ClassReferencesMethodOfClassThatComesAfter)
		{
			auto tree = ParseTree("class B { fun Bar() { A.Foo(); } } class A { static fun Foo() { } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(MethodHasReferenceToMethodThatComesAfter)
		{
			auto tree = ParseTree("class A { fun Foo() { Bar(); } fun Bar() { } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(CtorReferencesVariablesDeclaredLater)
		{
			auto tree = ParseTree("class A { A() { i = 1; } int i; }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}

		TEST_METHOD(MethodReferencesVariablesDeclaredLater)
		{
			auto tree = ParseTree("class A { fun Foo() { i = 1; } int i; }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);
		}
	};
}