#include "stdafx.h"
#include "CppUnitTest.h"

#include <Primitives.h>
#include <Classes.h>
#include <Operations.h>

#include <Utils.h>
#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Ast;
using namespace ParserTests;
using namespace std;

namespace CodeGenTests
{		
	TEST_CLASS(PrimitivesOfSameType)
	{
	public:
		TEST_METHOD(Int32AddCodegen)
		{
			llvm::LLVMContext context;
			llvm::IRBuilder<> builder(context); 
			auto module = new llvm::Module("Module", context);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<AddOperation>(new IntegerConstant("1"), new IntegerConstant("1"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(table , &builder, &context, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
		}

		TEST_METHOD(Int64AddCodegen)
		{
			llvm::LLVMContext context;
			llvm::IRBuilder<> builder(context);
			auto module = new llvm::Module("Module", context);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<AddOperation>(new IntegerConstant("2147483649"), new IntegerConstant("2147483649"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(table, &builder, &context, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 64, ((llvm::ConstantInt*)val)->getBitWidth());
		}

		TEST_METHOD(Float64AddCodegen)
		{
			llvm::LLVMContext context;
			llvm::IRBuilder<> builder(context);
			auto module = new llvm::Module("Module", context);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<AddOperation>(new FloatingConstant("1.5"), new FloatingConstant("1.5"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(table, &builder, &context, module);
			Assert::IsTrue(val->getType()->isDoubleTy());
		}
	};

	TEST_CLASS(PrimitivesWithImplicitlyCastTypes)
	{
		TEST_METHOD(Int32AndInt64AddCodegen)
		{
			llvm::LLVMContext context;
			llvm::IRBuilder<> builder(context);
			auto module = new llvm::Module("Module", context);
			auto table = std::make_shared<SymbolTable>();

			auto expr = std::make_shared<AddOperation>(new IntegerConstant("1"), new IntegerConstant("2147483649"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(table, &builder, &context, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int)64, ((llvm::ConstantInt*)val)->getBitWidth());
		}
	};
}