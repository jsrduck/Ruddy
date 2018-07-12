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

namespace ParserTests {

TEST_CLASS(ExpressionTests)
{
public:
	TEST_METHOD(DebugPrintStatementTest)
	{
		// Remove this test when we're done with this guy
		auto tree = ParseTree("class A { fun B() { print(\"Hello\"); } }"); auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
		Assert::IsNotNull(clssA);
		auto funB = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
		Assert::IsNotNull(funB);
		auto stmts = dynamic_cast<LineStatements*>(funB->_body.get());
		Assert::IsNotNull(stmts);
		auto exprStmt = dynamic_cast<ExpressionAsStatement*>(stmts->_statement.get());
		Assert::IsNotNull(exprStmt);
		auto printStmt = dynamic_cast<DebugPrintStatement*>(exprStmt->_expr.get());
		Assert::IsNotNull(printStmt);
	}

	TEST_METHOD(AssignToNewStatement)
	{
		auto tree = ParseTree("class A { fun B() { let c = new D(); } }");
		auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
		Assert::IsNotNull(clssA);
		auto funB = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
		Assert::IsNotNull(funB);
		auto stmts = dynamic_cast<LineStatements*>(funB->_body.get());
		Assert::IsNotNull(stmts);
		auto cDeclaration = dynamic_cast<Assignment*>(stmts->_statement.get());
		Assert::IsNotNull(cDeclaration);
		auto newExpression = dynamic_cast<NewExpression*>(cDeclaration->_rhs.get());
		Assert::IsNotNull(newExpression);
		Assert::AreEqual("D", newExpression->_className.c_str());
	}

	TEST_METHOD(WhileStatementTest)
	{
		auto tree = ParseTree("class A { fun B() { while (true) { break; } } }");
		auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
		Assert::IsNotNull(clssA);
		auto funB = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
		Assert::IsNotNull(funB);
		auto stmts = dynamic_cast<LineStatements*>(funB->_body.get());
		Assert::IsNotNull(stmts);
		auto whileStmt = dynamic_cast<WhileStatement*>(stmts->_statement.get());
		Assert::IsNotNull(whileStmt);
	}

	TEST_METHOD(CastStatement)
	{
		auto tree = ParseTree("class A { fun B() { int i = (int)1.0; } }");
		auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
		Assert::IsNotNull(clssA);
		auto funB = dynamic_cast<FunctionDeclaration*>(clssA->_list->_statement.get());
		Assert::IsNotNull(funB);
		auto stmts = dynamic_cast<LineStatements*>(funB->_body.get());
		Assert::IsNotNull(stmts);
		auto assgn = dynamic_cast<Assignment*>(stmts->_statement.get());
		Assert::IsNotNull(assgn);
		auto castOp = dynamic_cast<CastOperation*>(assgn->_rhs.get());
		Assert::IsNotNull(castOp);
	}

	TEST_METHOD(StackAllocateStatement)
	{
		auto tree = ParseTree("class A { fun B() { C& c(); } }");
		auto clssA = std::dynamic_pointer_cast<ClassDeclaration>(tree->_stmt);
		Assert::IsNotNull(clssA.get());
		auto list = SkipCtorsAndDtors(clssA);
		auto funB = std::dynamic_pointer_cast<FunctionDeclaration>(list->_statement);
		Assert::IsNotNull(funB.get());
		auto stmts = std::dynamic_pointer_cast<LineStatements>(funB->_body);
		Assert::IsNotNull(stmts.get());
		auto exprStatement = std::dynamic_pointer_cast<ExpressionAsStatement>(stmts->_statement);
		Assert::IsNotNull(exprStatement.get());
		auto stackAllocation = std::dynamic_pointer_cast<StackConstructionExpression>(exprStatement->_expr);
		Assert::IsNotNull(stackAllocation.get());
		Assert::AreEqual("c", stackAllocation->_varName.c_str());
		Assert::AreEqual("C", stackAllocation->_className.c_str());
	}

	TEST_METHOD(ConstructorWithInitializerList)
	{
		auto tree = ParseTree("class A { B& _b; A() : _b(0) {} }");
		auto clssA = std::dynamic_pointer_cast<ClassDeclaration>(tree->_stmt);
		Assert::IsNotNull(clssA.get());
		auto memberB = std::dynamic_pointer_cast<ClassMemberDeclaration>(clssA->_list->_statement);
		auto ctor = std::dynamic_pointer_cast<ConstructorDeclaration>(clssA->_list->_next->_statement);
		Assert::IsNotNull(ctor->_initializerStatement.get());
		Assert::AreEqual("_b", ctor->_initializerStatement->_list->_thisInitializer->_name.c_str());
	}
};

}