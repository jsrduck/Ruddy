#include "stdafx.h"
#include "CppUnitTest.h"

#include "TestUtils.h"
#include <Lexer.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Lexer;
using namespace std;

namespace LexerUnitTests
{
	TEST_CLASS(StringTests)
	{
	public:
		
		TEST_METHOD(SimpleStringTest)
		{
			vector<quex::Token> tokens;
			Analyze("\"Hello\"", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_STRING }, tokens);
		}

		TEST_METHOD(StringWithNewlineTest)
		{
			vector<quex::Token> tokens;
			Analyze("\"Hello\\r\\n\"", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_STRING }, tokens);
		}

		TEST_METHOD(StringWithTabTest)
		{
			vector<quex::Token> tokens;
			Analyze("\"\\tHello\"", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_STRING }, tokens);
		}

		TEST_METHOD(StringWithBackslashTest)
		{
			vector<quex::Token> tokens;
			Analyze("\"\\\\Hello\"", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_STRING }, tokens);
		}

		TEST_METHOD(StringWithQuoteTest)
		{
			vector<quex::Token> tokens;
			Analyze("\"\\\"Hello\\\"\"", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_STRING }, tokens);
		}

		TEST_METHOD(StringWithNullTest)
		{
			vector<quex::Token> tokens;
			Analyze("\"Hello\\0\"", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_STRING }, tokens);
		}

		TEST_METHOD(StringWithUnknownControlCharThrows)
		{
			Assert::ExpectException<InvalidControlCharacterException>([]()
			{
				vector<quex::Token> tokens;
				Analyze("\"Hello\\w\"", tokens);
			});
		}

		TEST_METHOD(StringDeclaration)
		{
			vector<quex::Token> tokens;
			Analyze("String s = \"Hello\\0\";", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_STRING, TKN_IDENTIFIER, TKN_OPERATOR_ASSIGN_TO, TKN_CONSTANT_STRING, TKN_SEMICOLON }, tokens);
		}

	};
}