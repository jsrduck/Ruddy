#include "stdafx.h"
#include <Grammar.h>
#include <Lexer.h>
#include "Parser.h"
#include <sstream>

int yyparse(quex::GeneratedLexer*, Ast::GlobalStatements**);

int yylex(YYSTYPE *yylval, quex::GeneratedLexer *qlex)
{
	quex::Token* token;
	qlex->receive(&token);
	switch (token->type_id())
	{
		case TKN_CONSTANT_STRING:
		case TKN_IDENTIFIER:
		case TKN_CONSTANT_INT:
		case TKN_CONSTANT_FLOAT:
		case TKN_CONSTANT_CHAR:
			yylval->id = new std::string((const char *)token->get_text().c_str());
			break;
		default:
			break;
	}

	return (int)token->type_id();
}

void yyerror(quex::GeneratedLexer *qlex, Ast::GlobalStatements**, const std::string& m)
{
	std::stringstream str;
	str << "Parsing error at " << qlex->line_number()
		<< ":" << qlex->column_number() << " : " << m;
	throw std::exception(str.str().c_str());
}

Ast::GlobalStatements* Parser::Parse(std::istream* stream)
{
	auto lexer = Lexer::CreateLexer(stream);
	return Parse(lexer);
}

Ast::GlobalStatements* Parser::Parse(std::string& fileName)
{
	auto lexer = Lexer::CreateLexer(fileName);
	return Parse(lexer);
}

Ast::GlobalStatements* Parser::Parse(quex::GeneratedLexer* qlex)
{
	Ast::GlobalStatements* globalStatements;
	int ret = yyparse(qlex, &globalStatements);
	if (ret != 0)
		throw std::exception("Failure to parse");
	return globalStatements;
}