#include "stdafx.h"
#include "CppUnitTest.h"

#include <Primitives.h>
#include <Classes.h>
#include <Operations.h>

#include "CodeGenUtils.h"
#include <Utils.h>
#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Ast;
using namespace ParserTests;
using namespace std;

static llvm::LLVMContext TheContext;

namespace CodeGenTests
{		
	TEST_CLASS(PrimitivesOfSameType)
	{
	public:
		TEST_METHOD(Int32AddCodegen)
		{
			llvm::IRBuilder<> builder(TheContext); 
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<AddOperation>(new IntegerConstant("1"), new IntegerConstant("1"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 2);
		}

		TEST_METHOD(Int32SubCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<SubtractOperation>(new IntegerConstant("1"), new IntegerConstant("1"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 0);
		}

		TEST_METHOD(Int32MulCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<MultiplyOperation>(new IntegerConstant("5"), new IntegerConstant("6"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 30);
		}

		TEST_METHOD(Int32DivCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<DivideOperation>(new IntegerConstant("100"), new IntegerConstant("5"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 20);
		}

		TEST_METHOD(Int32RemCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<RemainderOperation>(new IntegerConstant("11"), new IntegerConstant("3"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 2);
		}

		TEST_METHOD(Int32GTECodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<GreaterThanOrEqualOperation>(new IntegerConstant("5"), new IntegerConstant("5"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(!!((llvm::ConstantInt*)val)->getValue());

			expr = std::make_shared<GreaterThanOrEqualOperation>(new IntegerConstant("5"), new IntegerConstant("4"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(!!((llvm::ConstantInt*)val)->getValue());

			expr = std::make_shared<GreaterThanOrEqualOperation>(new IntegerConstant("5"), new IntegerConstant("6"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsFalse(!!((llvm::ConstantInt*)val)->getValue());
		}

		TEST_METHOD(Int32LTECodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<LessThanOrEqualOperation>(new IntegerConstant("5"), new IntegerConstant("5"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(!!((llvm::ConstantInt*)val)->getValue());

			expr = std::make_shared<LessThanOrEqualOperation>(new IntegerConstant("5"), new IntegerConstant("4"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsFalse(!!((llvm::ConstantInt*)val)->getValue());

			expr = std::make_shared<LessThanOrEqualOperation>(new IntegerConstant("5"), new IntegerConstant("6"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(!!((llvm::ConstantInt*)val)->getValue());
		}

		TEST_METHOD(Int32GTCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<GreaterThanOperation>(new IntegerConstant("5"), new IntegerConstant("5"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsFalse(!!((llvm::ConstantInt*)val)->getValue());

			expr = std::make_shared<GreaterThanOperation>(new IntegerConstant("5"), new IntegerConstant("4"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(!!((llvm::ConstantInt*)val)->getValue());

			expr = std::make_shared<GreaterThanOperation>(new IntegerConstant("5"), new IntegerConstant("6"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsFalse(!!((llvm::ConstantInt*)val)->getValue());
		}

		TEST_METHOD(Int32LTCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<LessThanOperation>(new IntegerConstant("5"), new IntegerConstant("5"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsFalse(!!((llvm::ConstantInt*)val)->getValue());

			expr = std::make_shared<LessThanOperation>(new IntegerConstant("5"), new IntegerConstant("4"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsFalse(!!((llvm::ConstantInt*)val)->getValue());

			expr = std::make_shared<LessThanOperation>(new IntegerConstant("5"), new IntegerConstant("6"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(!!((llvm::ConstantInt*)val)->getValue());
		}

		TEST_METHOD(Int32EqCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<EqualToOperation>(new IntegerConstant("5"), new IntegerConstant("5"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(!!((llvm::ConstantInt*)val)->getValue());

			expr = std::make_shared<EqualToOperation>(new IntegerConstant("5"), new IntegerConstant("4"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsFalse(!!((llvm::ConstantInt*)val)->getValue());
		}

		TEST_METHOD(Int32NeqCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<NotEqualToOperation>(new IntegerConstant("5"), new IntegerConstant("5"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsFalse(!!((llvm::ConstantInt*)val)->getValue());

			expr = std::make_shared<NotEqualToOperation>(new IntegerConstant("5"), new IntegerConstant("4"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 1, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(!!((llvm::ConstantInt*)val)->getValue());
		}

		TEST_METHOD(Int32BitAndCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<BitwiseAndOperation>(new IntegerConstant("0x1010"), new IntegerConstant("0x1101"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 0x1000);
		}

		TEST_METHOD(Int32BitOrCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<BitwiseOrOperation>(new IntegerConstant("0x1010"), new IntegerConstant("0x1100"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 0x1110);
		}

		TEST_METHOD(Int32BitXorCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<BitwiseXorOperation>(new IntegerConstant("0x1010"), new IntegerConstant("0x1100"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 0x0110);
		}

		TEST_METHOD(Int32BitShlCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<BitwiseShiftLeftOperation>(new IntegerConstant("0xF"), new IntegerConstant("2"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 60);

			expr = std::make_shared<BitwiseShiftLeftOperation>(new IntegerConstant("1", true /*negate*/), new IntegerConstant("2"));
			expr->Evaluate(table);
			val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == llvm::APInt(32, -4, true));
		}

		TEST_METHOD(Int32BitShrCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<BitwiseShiftRightOperation>(new IntegerConstant("0xF"), new IntegerConstant("2"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 3);
		}

		TEST_METHOD(Int32PostIncrCodegen)
		{
			auto tree = ParseTree("class A { static fun B() { int i = 1; i++; } }");
			TestTreeCodegen(tree.get(), TheContext);
		}

		TEST_METHOD(Int32PostDecCodegen)
		{
			auto tree = ParseTree("class A { static fun B() { int i = 1; i--; } }");
			TestTreeCodegen(tree.get(), TheContext);
		}

		TEST_METHOD(Int32PreIncrCodegen)
		{
			auto tree = ParseTree("class A { static fun B() { int i = 1; ++i; } }");
			TestTreeCodegen(tree.get(), TheContext);
		}

		TEST_METHOD(Int32PreDecCodegen)
		{
			auto tree = ParseTree("class A { static fun B() { int i = 1; --i; } }");
			TestTreeCodegen(tree.get(), TheContext);
		}

		TEST_METHOD(Int32ComplCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<ComplementOperation>(new IntegerConstant("0x0F0F0F0F"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 32, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 4042322160);
		}

		TEST_METHOD(Int64AddCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<AddOperation>(new IntegerConstant("2147483649"), new IntegerConstant("2147483649"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int) 64, ((llvm::ConstantInt*)val)->getBitWidth());
		}

		TEST_METHOD(Float64AddCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			auto expr = std::make_shared<AddOperation>(new FloatingConstant("1.5"), new FloatingConstant("1.5"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isDoubleTy());
		}
	};

	TEST_CLASS(PrimitivesWithImplicitlyCastTypes)
	{
		TEST_METHOD(Int32AndInt64AddCodegen)
		{
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();

			auto expr = std::make_shared<AddOperation>(new IntegerConstant("1"), new IntegerConstant("2147483649"));
			expr->Evaluate(table);
			auto val = expr->CodeGen(&builder, &TheContext, module);
			Assert::IsTrue(val->getType()->isIntegerTy());
			Assert::AreEqual((unsigned int)64, ((llvm::ConstantInt*)val)->getBitWidth());
			Assert::IsTrue(((llvm::ConstantInt*)val)->getValue() == 2147483650);
		}
	};
}