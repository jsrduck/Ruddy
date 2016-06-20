#include "stdafx.h"
#include "CppUnitTest.h"

#include "TestUtils.h"
#include <Lexer.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Lexer;
using namespace std;

namespace LexerUnitTests
{
	TEST_CLASS(StatementTests)
	{
	public:
		
		TEST_METHOD(BasicIf)
		{
			vector<quex::Token> tokens;
			Analyze("if (true) { i++; }", tokens);
			AssertAreEqualTokenTypes({ TKN_IF, TKN_PAREN_OPEN, TKN_CONSTANT_BOOL_TRUE, 
				TKN_PAREN_CLOSE, TKN_BRACKET_OPEN, TKN_IDENTIFIER, TKN_ARITHMETIC_OPERATOR_INCREMENT,
				TKN_SEMICOLON, TKN_BRACKET_CLOSE }, tokens);
		}

		TEST_METHOD(IfElse)
		{
			vector<quex::Token> tokens;
			Analyze("if (i<1) i++; else i--;", tokens);
			AssertAreEqualTokenTypes({TKN_IF, TKN_PAREN_OPEN, TKN_IDENTIFIER, TKN_COMPARISON_OPERATOR_LESS_THAN, 
				TKN_CONSTANT_INT, TKN_PAREN_CLOSE, TKN_IDENTIFIER, TKN_ARITHMETIC_OPERATOR_INCREMENT, TKN_SEMICOLON, 
				TKN_ELSE, TKN_IDENTIFIER, TKN_ARITHMETIC_OPERATOR_DECREMENT, TKN_SEMICOLON}, tokens);
		}

		TEST_METHOD(IfElseIf)
		{
			vector<quex::Token> tokens;
			Analyze("if (i<1) { i++; } else if (i > 1) { i--; } else { i = 0; }", tokens);
			AssertAreEqualTokenTypes({ TKN_IF, TKN_PAREN_OPEN, TKN_IDENTIFIER, TKN_COMPARISON_OPERATOR_LESS_THAN,
				TKN_CONSTANT_INT, TKN_PAREN_CLOSE, TKN_BRACKET_OPEN, TKN_IDENTIFIER, TKN_ARITHMETIC_OPERATOR_INCREMENT, 
				TKN_SEMICOLON, TKN_BRACKET_CLOSE, TKN_ELSE, TKN_IF, TKN_PAREN_OPEN, TKN_IDENTIFIER, TKN_COMPARISON_OPERATOR_GREATER_THAN,
				TKN_CONSTANT_INT, TKN_PAREN_CLOSE, TKN_BRACKET_OPEN, TKN_IDENTIFIER, TKN_ARITHMETIC_OPERATOR_DECREMENT, TKN_SEMICOLON,
				TKN_BRACKET_CLOSE, TKN_ELSE, TKN_BRACKET_OPEN, TKN_IDENTIFIER, TKN_OPERATOR_ASSIGN_TO, TKN_CONSTANT_INT,
				TKN_SEMICOLON, TKN_BRACKET_CLOSE }, tokens);
		}

		TEST_METHOD(LetDeclaration)
		{
			vector<quex::Token> tokens;
			Analyze("let x = 1;", tokens);
			AssertAreEqualTokenTypes({TKN_LET, TKN_IDENTIFIER, TKN_OPERATOR_ASSIGN_TO, TKN_CONSTANT_INT, TKN_SEMICOLON}, tokens);
		}

		TEST_METHOD(InstantiateNew)
		{
			vector<quex::Token> tokens;
			Analyze("let x = new foo();", tokens);
			AssertAreEqualTokenTypes({ TKN_LET, TKN_IDENTIFIER, TKN_OPERATOR_ASSIGN_TO, TKN_NEW, 
				TKN_IDENTIFIER, TKN_PAREN_OPEN, TKN_PAREN_CLOSE, TKN_SEMICOLON }, tokens);
		}

		TEST_METHOD(WhileStatement)
		{
			vector<quex::Token> tokens;
			Analyze("while (true) { i++; }", tokens);
			AssertAreEqualTokenTypes({TKN_WHILE, TKN_PAREN_OPEN, TKN_CONSTANT_BOOL_TRUE, TKN_PAREN_CLOSE, 
				TKN_BRACKET_OPEN, TKN_IDENTIFIER, TKN_ARITHMETIC_OPERATOR_INCREMENT, TKN_SEMICOLON, TKN_BRACKET_CLOSE}, tokens);
		}

		TEST_METHOD(DoWhileStatement)
		{
			vector<quex::Token> tokens;
			Analyze("do { i++; } while (true);", tokens);
			AssertAreEqualTokenTypes({ TKN_DO, TKN_BRACKET_OPEN, TKN_IDENTIFIER, TKN_ARITHMETIC_OPERATOR_INCREMENT, 
				TKN_SEMICOLON, TKN_BRACKET_CLOSE, TKN_WHILE, TKN_PAREN_OPEN, TKN_CONSTANT_BOOL_TRUE, TKN_PAREN_CLOSE, TKN_SEMICOLON}, tokens);
		}

		TEST_METHOD(WhileWithBreak)
		{
			vector<quex::Token> tokens;
			Analyze("while (true) { break; }", tokens);
			AssertAreEqualTokenTypes({ TKN_WHILE, TKN_PAREN_OPEN, TKN_CONSTANT_BOOL_TRUE, TKN_PAREN_CLOSE,
				TKN_BRACKET_OPEN, TKN_BREAK, TKN_SEMICOLON, TKN_BRACKET_CLOSE }, tokens);
		}

		TEST_METHOD(Namespace)
		{
			vector<quex::Token> tokens;
			Analyze("namespace MyNamespace { }", tokens);
			AssertAreEqualTokenTypes({ TKN_NAMESPACE, TKN_IDENTIFIER, TKN_BRACKET_OPEN, TKN_BRACKET_CLOSE }, tokens);
		}

		TEST_METHOD(ClassDeclaration)
		{
			vector<quex::Token> tokens;
			Analyze("class Foo { public int y; private char c; }", tokens);
			AssertAreEqualTokenTypes({ TKN_CLASS, TKN_IDENTIFIER, TKN_BRACKET_OPEN, TKN_PUBLIC, TKN_TYPE_INT, TKN_IDENTIFIER,
				TKN_SEMICOLON, TKN_PRIVATE, TKN_TYPE_CHAR, TKN_IDENTIFIER, TKN_SEMICOLON, TKN_BRACKET_CLOSE }, tokens);
		}

		TEST_METHOD(FunctionDeclaration)
		{
			vector<quex::Token> tokens;
			Analyze("fun(int x, int y) Foo(int z) { return z+1, z+2; }", tokens);
			AssertAreEqualTokenTypes({ TKN_FUNCTION, TKN_PAREN_OPEN, TKN_TYPE_INT, TKN_IDENTIFIER, TKN_COMMA,
				TKN_TYPE_INT, TKN_IDENTIFIER, TKN_PAREN_CLOSE, TKN_IDENTIFIER, TKN_PAREN_OPEN, TKN_TYPE_INT,
				TKN_IDENTIFIER, TKN_PAREN_CLOSE, TKN_BRACKET_OPEN, TKN_RETURN, TKN_IDENTIFIER, TKN_ARITHMETIC_OPERATOR_PLUS,
				TKN_CONSTANT_INT, TKN_COMMA, TKN_IDENTIFIER, TKN_ARITHMETIC_OPERATOR_PLUS, TKN_CONSTANT_INT,
				TKN_SEMICOLON, TKN_BRACKET_CLOSE }, tokens);
		}

		TEST_METHOD(FunctionCalls)
		{
			vector<quex::Token> tokens;
			Analyze("foo.bar() foo.bar(1, 0.5)", tokens);
			AssertAreEqualTokenTypes({TKN_IDENTIFIER, TKN_PERIOD, TKN_IDENTIFIER, TKN_PAREN_OPEN, TKN_PAREN_CLOSE, 
				TKN_IDENTIFIER, TKN_PERIOD, TKN_IDENTIFIER, TKN_PAREN_OPEN, TKN_CONSTANT_INT, TKN_COMMA, TKN_CONSTANT_FLOAT, TKN_PAREN_CLOSE }, tokens);
		}
	};
}