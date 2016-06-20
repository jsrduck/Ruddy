#pragma once
#include "CppUnitTest.h"

#include <vector>
#include <Lexer.h>
#include <string>

namespace LexerUnitTests
{
	inline void AssertAreEqualTokenTypes(std::vector<QUEX_TYPE_TOKEN_ID> expected, std::vector<quex::Token> actual)
	{
		// Ignore the termination token
		Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(expected.size(), actual.size() - 1);
		for (size_t i = 0; i < expected.size() - 1; i++)
		{
			Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(expected[i], actual[i].type_id());
		}
	}

	inline void AssertAreEqualValues(std::vector<std::string> expected, std::vector<quex::Token> actual)
	{
		// Ignore the termination token
		Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(expected.size(), actual.size() - 1);
		for (size_t i = 0; i < expected.size() - 1; i++)
		{
			Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(expected[i].compare((char*)actual[i].get_text().c_str()) == 0);
		}
	}
}