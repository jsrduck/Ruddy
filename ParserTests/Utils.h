#pragma once
#include <string>
#include <memory>
#include <Statements.h>
#include <Parser.h>
#include <Classes.h>

namespace ParserTests
{
	inline std::unique_ptr<Ast::GlobalStatements> ParseTree(std::string code)
	{
		std::istringstream inputStream(code);
		return std::unique_ptr<Ast::GlobalStatements>(Parser::Parse(&inputStream));
	}

	// Assumes we have
	inline std::shared_ptr<Ast::ClassStatementList> SkipCtorsAndDtors(std::shared_ptr<Ast::ClassDeclaration> classDecl)
	{
		auto statementList = classDecl->_list;
		while (statementList != nullptr)
		{
			auto asCtor = std::dynamic_pointer_cast<Ast::ConstructorDeclaration>(statementList->_statement);
			if (asCtor != nullptr)
			{
				statementList = statementList->_next;
				continue;
			}
			auto asDtor = std::dynamic_pointer_cast<Ast::DestructorDeclaration>(statementList->_statement);
			if (asDtor != nullptr)
			{
				statementList = statementList->_next;
				continue;
			}
			break;
		}
		return statementList;
	}
}