#include "stdafx.h"
#include "CppUnitTest.h"
#include <Statements.h>
#include <Classes.h>
#include <Primitives.h>
#include <Operations.h>
#include "Utils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace Parser;
using namespace Ast;

namespace ParserTests
{
	TEST_CLASS(ClassTests)
	{
	public:
		
		TEST_METHOD(ClassVisibility)
		{
			auto tree = ParseTree("class A {} public class B{} private class C{}");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			auto clssB = dynamic_cast<ClassDeclaration*>(tree->_next->_stmt.get());
			auto clssC = dynamic_cast<ClassDeclaration*>(tree->_next->_next->_stmt.get());

			Assert::IsNotNull(clssA);
			Assert::IsNotNull(clssB);
			Assert::IsNotNull(clssC);

			Assert::AreEqual("A", clssA->_name.c_str());
			Assert::AreEqual("B", clssB->_name.c_str());
			Assert::AreEqual("C", clssC->_name.c_str());

			Assert::AreEqual((int)Visibility::PUBLIC, (int)clssA->_visibility);
			Assert::AreEqual((int)Visibility::PUBLIC, (int)clssB->_visibility);
			Assert::AreEqual((int)Visibility::PRIVATE, (int)clssC->_visibility);
		}

		TEST_METHOD(ClassCtor)
		{
			auto tree = ParseTree("class A { A(){} }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto ctor = dynamic_cast<ConstructorDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(ctor);
			Assert::AreEqual("A", ctor->_name.c_str());
		}

		TEST_METHOD(ClassDtor)
		{
			auto tree = ParseTree("class A { ~A(){} }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto dtor = dynamic_cast<DestructorDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(dtor);
			Assert::AreEqual("A", dtor->_name.c_str());
		}

		TEST_METHOD(ClassPrivateCtor)
		{
			auto tree = ParseTree("class A { private A() {} }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto ctor = dynamic_cast<ConstructorDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(ctor);
			Assert::AreEqual((int)ctor->_visibility, (int)Visibility::PRIVATE);
		}

		TEST_METHOD(ClassProtectedCtor)
		{
			auto tree = ParseTree("class A { protected A() {} }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto ctor = dynamic_cast<ConstructorDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(ctor);
			Assert::AreEqual((int)ctor->_visibility, (int)Visibility::PROTECTED);
		}

		TEST_METHOD(ClassCtorWithArg)
		{
			auto tree = ParseTree("class A { A(int a){} }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto ctor = dynamic_cast<ConstructorDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(ctor);
			Assert::IsNotNull(ctor->_inputArgs.get());
			Assert::IsNotNull(ctor->_inputArgs->_argument.get());
			Assert::IsTrue(ctor->_inputArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual("a", ctor->_inputArgs->_argument->_name.c_str());
		}

		TEST_METHOD(ClassCtorWithArgsAndBody)
		{
			auto tree = ParseTree("class A { A(int a, char B){ a++; b--; } }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto ctor = dynamic_cast<ConstructorDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(ctor);
			Assert::IsNotNull(ctor->_inputArgs.get());
			Assert::IsNotNull(ctor->_inputArgs->_argument.get());
			Assert::IsTrue(ctor->_inputArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::IsNotNull(ctor->_inputArgs->_next.get());
			Assert::IsNotNull(ctor->_inputArgs->_next->_argument.get());
			Assert::IsTrue(ctor->_inputArgs->_next->_argument->_typeInfo->Equals(CharTypeInfo::Get()));
			Assert::AreEqual("B", ctor->_inputArgs->_next->_argument->_name.c_str());
		}

		TEST_METHOD(ClassWithLiteralMemberDeclarations)
		{
			auto tree = ParseTree("class A { int i; char j; }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto memberVarI = dynamic_cast<ClassMemberDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(memberVarI);
			Assert::IsNull(memberVarI->_defaultValue.get());
			Assert::AreEqual("i", memberVarI->_name.c_str());
			Assert::IsTrue(memberVarI->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual((int) Visibility::PUBLIC, (int) memberVarI->_visibility);

			auto memberVarJ = dynamic_cast<ClassMemberDeclaration*>(clssA->_list->_next->_statement.get());
			Assert::IsNotNull(memberVarJ);
			Assert::IsNull(memberVarJ->_defaultValue.get());
			Assert::AreEqual("j", memberVarJ->_name.c_str());
			Assert::IsTrue(memberVarJ->_typeInfo->Equals(CharTypeInfo::Get()));
			Assert::AreEqual((int) Visibility::PUBLIC, (int) memberVarJ->_visibility);
		}

		TEST_METHOD(ClassWithLiteralMemberDeclarationsAndDefaultValues)
		{
			auto tree = ParseTree("class A { int i = 0; bool j = true; }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto memberVarI = dynamic_cast<ClassMemberDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(memberVarI);
			Assert::IsTrue(dynamic_pointer_cast<IntegerConstant>(memberVarI->_defaultValue) != nullptr);
			Assert::AreEqual(0, dynamic_pointer_cast<IntegerConstant>(memberVarI->_defaultValue)->AsInt32());
			Assert::AreEqual("i", memberVarI->_name.c_str());
			Assert::IsTrue(memberVarI->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual((int) Visibility::PUBLIC, (int) memberVarI->_visibility);

			auto memberVarJ = dynamic_cast<ClassMemberDeclaration*>(clssA->_list->_next->_statement.get());
			Assert::IsNotNull(memberVarJ);
			Assert::IsNotNull(dynamic_cast<BoolConstant*>(memberVarJ->_defaultValue.get()));
			Assert::AreEqual(true, dynamic_cast<BoolConstant*>(memberVarJ->_defaultValue.get())->Value());
			Assert::AreEqual("j", memberVarJ->_name.c_str());
			Assert::IsTrue(memberVarJ->_typeInfo->Equals(BoolTypeInfo::Get()));
			Assert::AreEqual((int) Visibility::PUBLIC, (int) memberVarJ->_visibility);
		}

		TEST_METHOD(ClassWithClassMemberDeclarations)
		{
			auto tree = ParseTree("class A { B b; }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto memberVarB = dynamic_cast<ClassMemberDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(memberVarB);
			Assert::IsNull(memberVarB->_defaultValue.get());
			Assert::AreEqual((int) Visibility::PUBLIC, (int) memberVarB->_visibility);
			Assert::AreEqual("b", memberVarB->_name.c_str());
			Assert::IsNotNull(dynamic_cast<UnresolvedClassTypeInfo*>(memberVarB->_typeInfo.get()));
		}

		TEST_METHOD(ClassWithVaryingVisibilityMemberDeclarations)
		{
			auto tree = ParseTree("class A { public B b; private int i; protected bool j = false; }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);

			auto memberVarB = dynamic_cast<ClassMemberDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(memberVarB);
			Assert::IsNull(memberVarB->_defaultValue.get());
			Assert::AreEqual((int) Visibility::PUBLIC, (int) memberVarB->_visibility);
			Assert::AreEqual("b", memberVarB->_name.c_str());
			Assert::IsNotNull(dynamic_cast<UnresolvedClassTypeInfo*>(memberVarB->_typeInfo.get()));

			auto memberVarI = dynamic_cast<ClassMemberDeclaration*>(clssA->_list->_next->_statement.get());
			Assert::IsNotNull(memberVarI);
			Assert::IsNull(memberVarI->_defaultValue.get());
			Assert::AreEqual("i", memberVarI->_name.c_str());
			Assert::IsTrue(memberVarI->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual((int) Visibility::PRIVATE, (int) memberVarI->_visibility);

			auto memberVarJ = dynamic_cast<ClassMemberDeclaration*>(clssA->_list->_next->_next->_statement.get());
			Assert::IsNotNull(memberVarJ);
			Assert::IsNotNull(dynamic_cast<BoolConstant*>(memberVarJ->_defaultValue.get()));
			Assert::AreEqual(false, dynamic_cast<BoolConstant*>(memberVarJ->_defaultValue.get())->Value());
			Assert::AreEqual("j", memberVarJ->_name.c_str());
			Assert::IsTrue(memberVarJ->_typeInfo->Equals(BoolTypeInfo::Get()));
			Assert::AreEqual((int) Visibility::PROTECTED, (int) memberVarJ->_visibility);
		}

		TEST_METHOD(ClassWithSimpleFunction)
		{
			auto tree = ParseTree("class A { fun B() {} }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto bDeclaration = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(bDeclaration);
			Assert::AreEqual("B", bDeclaration->_name.c_str());
			Assert::AreEqual((int)Visibility::PUBLIC, (int)bDeclaration->_visibility);
			Assert::IsTrue(bDeclaration->_returnArgs == nullptr);
		}

		TEST_METHOD(ClassWithSimpleFunctionVoid)
		{
			auto tree = ParseTree("class A { fun() B() {} }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto bDeclaration = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(bDeclaration);
			Assert::AreEqual("B", bDeclaration->_name.c_str());
			Assert::AreEqual((int) Visibility::PUBLIC, (int) bDeclaration->_visibility);
			Assert::IsTrue(bDeclaration->_returnArgs == nullptr);
		}

		TEST_METHOD(ClassWithVoidFunctionWithArg)
		{
			auto tree = ParseTree("class A { fun B(int a) {} }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto bDeclaration = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(bDeclaration);
			Assert::AreEqual("a", bDeclaration->_inputArgs->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_inputArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
		}

		TEST_METHOD(ClassWithVoidFunctionWithArgsAndBody)
		{
			auto tree = ParseTree("class A { fun B(int a, byte b) { a++; b--; } }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto bDeclaration = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(bDeclaration);
			Assert::AreEqual("a", bDeclaration->_inputArgs->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_inputArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual("b", bDeclaration->_inputArgs->_next->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_inputArgs->_next->_argument->_typeInfo->Equals(ByteTypeInfo::Get()));

			auto stmts = dynamic_cast<LineStatements*>(bDeclaration->_body.get());
			Assert::IsNotNull(dynamic_cast<ExpressionAsStatement*>(stmts->_statement.get()));
			Assert::IsNotNull(dynamic_cast<ExpressionAsStatement*>(stmts->_next->_statement.get()));
		}

		TEST_METHOD(ClassWithFunctionWithReturnType)
		{
			auto tree = ParseTree("class A { fun(int a) B() { return 0; } }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto bDeclaration = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(bDeclaration);
			Assert::AreEqual("a", bDeclaration->_returnArgs->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_returnArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));

			auto stmts = dynamic_cast<LineStatements*>(bDeclaration->_body.get());
			Assert::IsNotNull(dynamic_cast<ReturnStatement*>(stmts->_statement.get()));
		}

		TEST_METHOD(ClassWithFunctionWithMultiReturnType)
		{
			auto tree = ParseTree("class A { fun(int a, int b) B() { return 0,1; } }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto bDeclaration = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(bDeclaration);
			Assert::AreEqual("a", bDeclaration->_returnArgs->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_returnArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual("b", bDeclaration->_returnArgs->_next->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_returnArgs->_next->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));

			auto stmts = dynamic_cast<LineStatements*>(bDeclaration->_body.get());
			Assert::IsNotNull(dynamic_cast<ReturnStatement*>(stmts->_statement.get()));
		}

		TEST_METHOD(ClassWithFunctionWithMultiArgAndReturnTypes)
		{
			auto tree = ParseTree("class A { fun(int a, int b) B(int c, int d) { return c,d; } }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto bDeclaration = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(bDeclaration);

			Assert::AreEqual("c", bDeclaration->_inputArgs->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_inputArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual("d", bDeclaration->_inputArgs->_next->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_inputArgs->_next->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));

			Assert::AreEqual("a", bDeclaration->_returnArgs->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_returnArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual("b", bDeclaration->_returnArgs->_next->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_returnArgs->_next->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));

			auto stmts = dynamic_cast<LineStatements*>(bDeclaration->_body.get());
			Assert::IsNotNull(dynamic_cast<ReturnStatement*>(stmts->_statement.get()));
		}

		TEST_METHOD(ClassWithFunctionWithMultiArgAndExprReturnTypes)
		{
			auto tree = ParseTree("class A { fun(int a, int b) B(int c, int d) { return c+d,d*2; } }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto bDeclaration = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(bDeclaration);

			Assert::AreEqual("c", bDeclaration->_inputArgs->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_inputArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual("d", bDeclaration->_inputArgs->_next->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_inputArgs->_next->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));

			Assert::AreEqual("a", bDeclaration->_returnArgs->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_returnArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual("b", bDeclaration->_returnArgs->_next->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_returnArgs->_next->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));

			auto stmts = dynamic_cast<LineStatements*>(bDeclaration->_body.get());
			Assert::IsNotNull(dynamic_cast<ReturnStatement*>(stmts->_statement.get()));
		}

		TEST_METHOD(ClassWithFunctionWithTriReturnType)
		{
			auto tree = ParseTree("class A { fun(int a, int b, byte c) B(int d, int e) { return c+d,d*2,c%d; } }");
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			auto bDeclaration = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
			Assert::IsNotNull(bDeclaration);

			Assert::AreEqual("d", bDeclaration->_inputArgs->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_inputArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual("e", bDeclaration->_inputArgs->_next->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_inputArgs->_next->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));

			Assert::AreEqual("a", bDeclaration->_returnArgs->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_returnArgs->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual("b", bDeclaration->_returnArgs->_next->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_returnArgs->_next->_argument->_typeInfo->Equals(Int32TypeInfo::Get()));
			Assert::AreEqual("c", bDeclaration->_returnArgs->_next->_next->_argument->_name.c_str());
			Assert::IsTrue(bDeclaration->_returnArgs->_next->_next->_argument->_typeInfo->Equals(ByteTypeInfo::Get()));

			auto stmts = dynamic_cast<LineStatements*>(bDeclaration->_body.get());
			Assert::IsNotNull(dynamic_cast<ReturnStatement*>(stmts->_statement.get()));
		}
	};
}