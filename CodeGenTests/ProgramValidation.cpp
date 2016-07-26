#include "stdafx.h"
#include "CppUnitTest.h"

#include <Primitives.h>
#include <Classes.h>
#include <Operations.h>

#include <Utils.h>
#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>

#include <fstream>

#include <Windows.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Ast;
using namespace ParserTests;
using namespace std;

// These tests just validate the output of the text files produced by the rdy files in this project.
namespace CodeGenTests {
	TEST_CLASS(ProgramValidation)
	{
	public:
		BEGIN_TEST_METHOD_ATTRIBUTE(ValidatePrimitives)
			TEST_METHOD_ATTRIBUTE(L"DeploymentItem", L"ValidatePrimitives.txt")
		END_TEST_METHOD_ATTRIBUTE()
		TEST_METHOD(ValidatePrimitives)
		{
			const wchar_t * c_expected = L"yyyyyyy15-93641yyyyyyyy345534838146-13214748374921474835492147483649002147483649yyyyyy"
				L"2147483649214748365021474836512147483651214748365021474836496710886468719476768983547511848109221018991929938554387344"
				L"-99213745092121474837492147483549214748364853687091249yyyyyy21474836492147483650214748365121474836512147483650214748364"
				L"96710886432590992248156543024809744381840700271214748374921474835492147483649002147483649yyyyyy214748364921474836502147"
				L"48365121474836512147483650214748364967108864687194767685764781479948109606557381770378739710598090362238392875012105535257377506575"
				L"31.975-10.225229.4630.5154030.875yyyyyyy10.87511.87512.87512.87511.87510.8753.51282e+39-3.29282e+393.7431e+770.03232611e+37yyyyyyy"
				L"10.87511.87512.87512.87511.87510.8751811513071yyyyyyyy1661671681681666036175169892192511834125yyyyyyyyabcca488309612327-123"
				L"638126977921126yyyyyyyy\u0100\u0101\u0102\u0102\u0100152895256382126-3832192511834125yyyyyyyyabcca488309612327-123";
			std::ifstream inputStream("ValidatePrimitives.txt");
			std::stringstream ss;
			ss << inputStream.rdbuf() << '\0';
			std::wstring input((wchar_t*)ss.str().c_str());
			Assert::AreEqual(c_expected, input.c_str());
		}

		TEST_METHOD(ValidateMixedPrimitives)
		{
			const wchar_t * c_expected = L"2147483658-2147483658-214748364821474836584294967295-21474836482147483658429496729518446744073709551615"
				"-111.5111.51197d32868197d355355255-104294967396089.125429496728689.1254294967286197d32868197d355355255-42949672961844674406941458"
				"4320-8.58993e+0910197d32868197d355-858993433725589.1251844674407370955160689.12518446744073709551606197d32868197d3553552553.06254e+39"
				"inf197.5d32868.5197.5d355.5355.50197.5d32868.5197.5d355.5355.5032963ab33023255";
			std::ifstream inputStream("ValidateMixedPrimitives.txt");
			std::stringstream ss;
			ss << inputStream.rdbuf() << '\0';
			std::wstring input((wchar_t*) ss.str().c_str());
			Assert::AreEqual(c_expected, input.c_str());
		}

		BEGIN_TEST_METHOD_ATTRIBUTE(ValidateFunctions)
			TEST_METHOD_ATTRIBUTE(L"DeploymentItem", L"ValidateFunctions.txt")
		END_TEST_METHOD_ATTRIBUTE()
		TEST_METHOD(ValidateFunctions)
		{
			const wchar_t * c_expected = L"Hello371112013505005000";
			std::ifstream inputStream("ValidateFunctions.txt");
			std::stringstream ss;
			ss << inputStream.rdbuf() << '\0';
			std::wstring input((wchar_t*) ss.str().c_str());
			Assert::AreEqual(c_expected, input.c_str());
		}
	};
}