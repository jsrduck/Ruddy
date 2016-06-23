#include "stdafx.h"
#include "CppUnitTest.h"
#include <Statements.h>
#include <Classes.h>
#include <Primitives.h>
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
		Assert::IsTrue(cDeclaration->_lhs->Resolve(nullptr)->IsAutoType());
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
};

}