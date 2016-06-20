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
		Assert::AreEqual("D", newExpression->_id->Id().c_str());
	}
};

}