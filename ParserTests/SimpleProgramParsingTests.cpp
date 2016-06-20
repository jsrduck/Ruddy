#include "stdafx.h"
#include "CppUnitTest.h"
#include <Statements.h>
#include <Classes.h>
#include "Utils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace Parser;
using namespace Ast;

namespace ParserTests
{		
	TEST_CLASS(SimpleProgramParsingTests)
	{
	public:
		TEST_METHOD(NamespaceAndClass)
		{
			auto tree = ParseTree("namespace A { class B { } }");
			Assert::IsNotNull(tree->_stmt.get());
			NamespaceDeclaration* nmsp = dynamic_cast<NamespaceDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(nmsp);
			Assert::AreEqual("A", nmsp->_name.c_str());
			Assert::IsNotNull(nmsp->_stmts.get());
			Assert::IsNull(nmsp->_stmts->_next.get());
			auto clss = dynamic_cast<ClassDeclaration*>(nmsp->_stmts->_stmt.get());
			Assert::IsNotNull(clss);
			Assert::AreEqual((int)Visibility::PUBLIC, (int)clss->_visibility);
			Assert::AreEqual("B", clss->_name.c_str());
			Assert::IsNull(clss->_list.get());
		}

		TEST_METHOD(GlobalClasses)
		{
			auto tree = ParseTree("class A{} class B{}");
			Assert::IsNotNull(tree->_stmt.get());
			auto clssA = dynamic_cast<ClassDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(clssA);
			Assert::IsNotNull(tree->_next.get());
			auto clssB = dynamic_cast<ClassDeclaration*>(tree->_next->_stmt.get());
			Assert::IsNotNull(clssB);
			Assert::IsNull(tree->_next->_next.get());
			Assert::IsNull(clssA->_list.get());
			Assert::AreEqual("A", clssA->_name.c_str());
			Assert::AreEqual("B", clssB->_name.c_str());
		}

		TEST_METHOD(MultiNamespaces)
		{
			auto tree = ParseTree("namespace A { class B {} } namespace C { class B {} }");
			Assert::IsNotNull(tree->_stmt.get());
			auto nmspA = dynamic_cast<NamespaceDeclaration*>(tree->_stmt.get());
			Assert::IsNotNull(nmspA);
			auto nmspC = dynamic_cast<NamespaceDeclaration*>(tree->_next->_stmt.get());
			Assert::IsNotNull(nmspC);

			auto clssAB = dynamic_cast<ClassDeclaration*>(nmspA->_stmts->_stmt.get());
			Assert::IsNotNull(clssAB);
			auto clssCB = dynamic_cast<ClassDeclaration*>(nmspC->_stmts->_stmt.get());
			Assert::IsNotNull(clssCB);
		}
	};
}