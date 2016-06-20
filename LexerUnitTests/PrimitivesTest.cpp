#include "stdafx.h"
#include "CppUnitTest.h"

#include "TestUtils.h"
#include <Lexer.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Lexer;
using namespace std;

namespace LexerUnitTests
{
	TEST_CLASS(NumberTokenTests)
	{
	public:
		
		TEST_METHOD(IntTypeTokens)
		{
			vector<quex::Token> tokens;
			Analyze("int int int", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_INT, TKN_TYPE_INT, TKN_TYPE_INT}, tokens);
		}

		TEST_METHOD(IntValueTokens)
		{
			vector<quex::Token> tokens;
			Analyze("0 1 25 137 4593", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_CONSTANT_INT, TKN_CONSTANT_INT, TKN_CONSTANT_INT, TKN_CONSTANT_INT }, tokens);
			AssertAreEqualValues({ "0", "1", "25", "137", "4593" }, tokens);
		}

		TEST_METHOD(HexValueTokens)
		{
			vector<quex::Token> tokens;
			Analyze("0x0 0x1 0xabcd 0xffffffff 0xdeadbeef 0x00000001", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_INT, TKN_CONSTANT_INT, TKN_CONSTANT_INT, TKN_CONSTANT_INT, TKN_CONSTANT_INT, TKN_CONSTANT_INT }, tokens);
			AssertAreEqualValues({ "0x0", "0x1", "0xabcd", "0xffffffff", "0xdeadbeef", "0x00000001" }, tokens);
		}

		//TEST_METHOD(InvalidHexReturnsLexError)
		//{
		//	Assert::ExpectException<Lexer::UnexpectedTokenException>([]()
		//	{
		//		vector<quex::Token> tokens;
		//		Analyze("0xffg", tokens);
		//	});
		//}

		TEST_METHOD(FloatTypeTokens)
		{
			vector<quex::Token> tokens;
			Analyze("float float float", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_FLOAT, TKN_TYPE_FLOAT, TKN_TYPE_FLOAT }, tokens);
		}

		TEST_METHOD(FloatValueTokens)
		{
			vector<quex::Token> tokens;
			Analyze("0.0 1.5 25.23 137.974 4593.1234", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_FLOAT, TKN_CONSTANT_FLOAT, TKN_CONSTANT_FLOAT, TKN_CONSTANT_FLOAT, TKN_CONSTANT_FLOAT }, tokens);
			AssertAreEqualValues({ "0.0", "1.5", "25.23", "137.974", "4593.1234" }, tokens);
		}

		TEST_METHOD(FloatValueUsingExponentsTokens)
		{
			vector<quex::Token> tokens;
			Analyze("1.0 1.0e1 .12E-2", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_FLOAT, TKN_CONSTANT_FLOAT, TKN_CONSTANT_FLOAT }, tokens);
			AssertAreEqualValues({ "1.0", "1.0e1", ".12E-2"}, tokens);
		}

		TEST_METHOD(Int64TypeTokens)
		{
			vector<quex::Token> tokens;
			Analyze("int64 int64 int64", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_INT64, TKN_TYPE_INT64, TKN_TYPE_INT64 }, tokens);
		}

		TEST_METHOD(Float64TypeTokens)
		{
			vector<quex::Token> tokens;
			Analyze("float64 float64 float64", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_FLOAT64, TKN_TYPE_FLOAT64, TKN_TYPE_FLOAT64 }, tokens);
		}

		TEST_METHOD(UintTypeTokens)
		{
			vector<quex::Token> tokens;
			Analyze("uint uint uint", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_UINT, TKN_TYPE_UINT, TKN_TYPE_UINT }, tokens);
		}

		TEST_METHOD(Uint64TypeTokens)
		{
			vector<quex::Token> tokens;
			Analyze("uint64 uint64 uint64", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_UINT64, TKN_TYPE_UINT64, TKN_TYPE_UINT64 }, tokens);
		}

		TEST_METHOD(MixedUpTokens)
		{
			vector<quex::Token> tokens;
			Analyze("int 1 float 2.0 int64 10 float64 14.5 byte 0xef uint 1.9e-1", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_INT, TKN_CONSTANT_INT, TKN_TYPE_FLOAT, TKN_CONSTANT_FLOAT, TKN_TYPE_INT64, 
				TKN_CONSTANT_INT, TKN_TYPE_FLOAT64, TKN_CONSTANT_FLOAT, TKN_TYPE_BYTE, TKN_CONSTANT_INT, TKN_TYPE_UINT, TKN_TYPE_FLOAT }, tokens);
		}
	};

	TEST_CLASS(CharTokenTests)
	{
	public:
		TEST_METHOD(CharTypeTokens)
		{
			vector<quex::Token> tokens;
			Analyze("char char char", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_CHAR, TKN_TYPE_CHAR, TKN_TYPE_CHAR }, tokens);
		}

		TEST_METHOD(CharValueTokens)
		{
			vector<quex::Token> tokens;
			Analyze("'a' 'b' 'c'", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR }, tokens);
			AssertAreEqualValues({ "'a'", "'b'", "'c'" }, tokens);
		}

		TEST_METHOD(EscapeCharValueTokens)
		{
			vector<quex::Token> tokens;
			Analyze("'\\n' '\\r' '\\t' '\\'' '\\0' '\"' '\\\\'", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR }, tokens);
			AssertAreEqualValues({ "'\\n'", "'\\r'", "'\\t'", "'\\''", "'\\0'", "'\"'", "'\\\\'" }, tokens);
		}

		TEST_METHOD(UnicodeCharValueTokens)
		{
			vector<quex::Token> tokens;
			Analyze("'\\u0123' '\\uFFFF' '\\u1f3D'", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR }, tokens);
			AssertAreEqualValues({ "'\\u0123'", "'\\uFFFF'", "'\\u1f3D'" }, tokens);
		}

		TEST_METHOD(HexCharValueTokens)
		{
			vector<quex::Token> tokens;
			Analyze("'\\x0123' '\\xFc2' '\\x2a' '\\x0'", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR, TKN_CONSTANT_CHAR }, tokens);
			AssertAreEqualValues({ "'\\x0123'", "'\\xFc2'", "'\\x2a'", "'\\x0'" }, tokens);
		}

		TEST_METHOD(CharByteTypeTokens)
		{
			vector<quex::Token> tokens;
			Analyze("charbyte charbyte charbyte", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_CHARBYTE, TKN_TYPE_CHARBYTE, TKN_TYPE_CHARBYTE }, tokens);
		}

		TEST_METHOD(ByteTypeTokens)
		{
			vector<quex::Token> tokens;
			Analyze("byte byte byte", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_BYTE, TKN_TYPE_BYTE, TKN_TYPE_BYTE }, tokens);
		}
	};

	TEST_CLASS(BoolTokenTests)
	{
	public:
		TEST_METHOD(BoolTypeTokens)
		{
			vector<quex::Token> tokens;
			Analyze("bool bool bool", tokens);
			AssertAreEqualTokenTypes({ TKN_TYPE_BOOL, TKN_TYPE_BOOL, TKN_TYPE_BOOL }, tokens);
		}

		TEST_METHOD(BoolValueTokens)
		{
			vector<quex::Token> tokens;
			Analyze("true false", tokens);
			AssertAreEqualTokenTypes({ TKN_CONSTANT_BOOL_TRUE, TKN_CONSTANT_BOOL_FALSE }, tokens);
		}
	};
}