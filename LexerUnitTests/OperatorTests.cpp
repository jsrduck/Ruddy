#include "stdafx.h"
#include "CppUnitTest.h"

#include "TestUtils.h"
#include <Lexer.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace Lexer;

namespace LexerUnitTests
{
	TEST_CLASS(OperatorTests)
	{
	public:
		
		TEST_METHOD(PostIncrementTest)
		{
			vector<quex::Token> tokens;
			Analyze("i++", tokens);
			AssertAreEqualTokenTypes({ TKN_IDENTIFIER, TKN_ARITHMETIC_OPERATOR_INCREMENT }, tokens);
		}

		TEST_METHOD(PreIncrementTest)
		{
			vector<quex::Token> tokens;
			Analyze("++i", tokens);
			AssertAreEqualTokenTypes({ TKN_ARITHMETIC_OPERATOR_INCREMENT, TKN_IDENTIFIER }, tokens);
		}

		TEST_METHOD(PlusTest)
		{
			vector<quex::Token> tokens;
			Analyze("1+1", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_ARITHMETIC_OPERATOR_PLUS, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(MinusTest)
		{
			vector<quex::Token> tokens;
			Analyze("1-1", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_ARITHMETIC_OPERATOR_MINUS, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(TimesTest)
		{
			vector<quex::Token> tokens;
			Analyze("1*1", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_ARITHMETIC_OPERATOR_TIMES, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(DivideTest)
		{
			vector<quex::Token> tokens;
			Analyze("1/1", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_ARITHMETIC_OPERATOR_DIVIDE, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(ShiftLeftTest)
		{
			vector<quex::Token> tokens;
			Analyze("0xFF<<1", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_BITWISE_SHIFT_LEFT_OPERATOR, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(ShiftRightTest)
		{
			vector<quex::Token> tokens;
			Analyze("0xFF>>1", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_BITWISE_SHIFT_RIGHT_OPERATOR, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(RemainderTest)
		{
			vector<quex::Token> tokens;
			Analyze("10%3", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_ARITHMETIC_OPERATOR_REMAINDER, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(GreaterThanOrEqualToTest)
		{
			vector<quex::Token> tokens;
			Analyze("2>=1", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_COMPARISON_OPERATOR_GREATER_THAN_OR_EQUAL_TO, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(LessThanOrEqualToTest)
		{
			vector<quex::Token> tokens;
			Analyze("1<=2", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_COMPARISON_OPERATOR_LESS_THAN_OR_EQUAL_TO, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(GreaterThanTest)
		{
			vector<quex::Token> tokens;
			Analyze("2>1", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_COMPARISON_OPERATOR_GREATER_THAN, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(LessThanTest)
		{
			vector<quex::Token> tokens;
			Analyze("1<2", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_COMPARISON_OPERATOR_LESS_THAN, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(EqualToTest)
		{
			vector<quex::Token> tokens;
			Analyze("1==1", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_COMPARISON_OPERATOR_EQUAL_TO, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(NotEqualToTest)
		{
			vector<quex::Token> tokens;
			Analyze("1!=1", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_COMPARISON_OPERATOR_NOT_EQUAL_TO, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(LogicalAndTest)
		{
			vector<quex::Token> tokens;
			Analyze("true && true", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_BOOL_TRUE, TKN_LOGICAL_OPERATOR_AND, TKN_CONSTANT_BOOL_TRUE }, tokens);
		}

		TEST_METHOD(LogicalORTest)
		{
			vector<quex::Token> tokens;
			Analyze("true || true", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_BOOL_TRUE, TKN_LOGICAL_OPERATOR_OR, TKN_CONSTANT_BOOL_TRUE }, tokens);
		}

		TEST_METHOD(LogicalNegationTest)
		{
			vector<quex::Token> tokens;
			Analyze("!true", tokens);
			AssertAreEqualTokenTypes({ TKN_LOGICAL_OPERATOR_NEGATE, TKN_CONSTANT_BOOL_TRUE }, tokens);
		}

		TEST_METHOD(BitwiseAndTest)
		{
			vector<quex::Token> tokens;
			Analyze("0x10 & 0x01", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_BITWISE_OPERATOR_AND, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(BitwiseOrTest)
		{
			vector<quex::Token> tokens;
			Analyze("0x10 | 0x01", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_BITWISE_OPERATOR_OR, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(BitwiseXorTest)
		{
			vector<quex::Token> tokens;
			Analyze("0x10 ^ 0x01", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_BITWISE_OPERATOR_XOR, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(BitwiseComplementTest)
		{
			vector<quex::Token> tokens;
			Analyze("~0x01", tokens);
			AssertAreEqualTokenTypes({ TKN_BITWISE_OPERATOR_COMPLEMENT, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(AssignmentTest)
		{
			vector<quex::Token> tokens;
			Analyze("int i = 0", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_INT, TKN_IDENTIFIER, TKN_OPERATOR_ASSIGN_TO, TKN_CONSTANT_INT }, tokens);
		}

		TEST_METHOD(CastTest)
		{
			vector<quex::Token> tokens;
			Analyze("int i = (int)1.0", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_INT, TKN_IDENTIFIER, TKN_OPERATOR_ASSIGN_TO, TKN_PAREN_OPEN, TKN_TYPE_INT, TKN_PAREN_CLOSE, TKN_CONSTANT_FLOAT }, tokens);
		}
	};
}