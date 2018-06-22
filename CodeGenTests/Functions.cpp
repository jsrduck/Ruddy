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

static llvm::LLVMContext TheContext;

namespace CodeGenTests {
	// Most of these tests just test against regressions that introduce exceptions in the codegen layer. Actual verification of behavior 
	// can't really be tested in a unit test. That will be tested later by actually running the compiler and verifying the output.
	TEST_CLASS(StaticFunctions)
	{
	public:
		TEST_METHOD(SimpleStaticFunctionCodegen)
		{
			auto tree = ParseTree("class A { static fun B() { let i = 0; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(SimpleStaticFunctionCallCodegen)
		{
			auto tree = ParseTree("class A { static fun B() { let i = 0; } static fun main() { B(); } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(VoidRetOneArgStaticFunctionCodegen)
		{
			auto tree = ParseTree("class A { static fun B(int j) { let i = j; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(VoidRetOneArgStaticFunctionCallCodegen)
		{
			auto tree = ParseTree("class A { static fun B(int j) { let i = j; } static fun main() { B(1); } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(OneRetOneArgStaticFunctionCodegen)
		{
			auto tree = ParseTree("class A { static fun(int k) B(int j) { return j+1; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(OneRetOneArgStaticFunctionCallCodegen)
		{
			auto tree = ParseTree("class A { static fun(int k) B(int j) { return j+1; } static fun main() { int n = B(1); } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(OneRetMultiArgStaticFunctionCodegen)
		{
			auto tree = ParseTree("class A { static fun(int k) B(int j, float l) { let m = l; return j+1; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(OneRetMultiArgStaticFunctionCallCodegen)
		{
			auto tree = ParseTree("class A { static fun(int k) B(int j, float l) { let m = l; return j+1; } static fun main() { int n = B(1,0.5); } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(MultiRetMultiArgStaticFunctionCodegen)
		{
			auto tree = ParseTree("class A { static fun(int k, float m) B(int j, float l) { return j+1, l+0.5; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(MultiRetMultiArgStaticFunctionCallCodegen)
		{
			auto tree = ParseTree("class A { static fun(int k, float64 m) B(int j, float64 l) { return j+1, l+0.5; } static fun main() { int n, float64 o = B(1,0.5); } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}
	};

	TEST_CLASS(ControlFlowCodegen)
	{
		TEST_METHOD(VariableDeclarationCodegen)
		{
			auto tree = ParseTree("class A { static fun(int j) B(int i) { int k = i; return k; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(VariableAssignmentCodegen)
		{
			auto tree = ParseTree("class A { static fun(int j) B(int i) { int k = i; k = 0; return k; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(ArgAssignmentCodegen)
		{
			auto tree = ParseTree("class A { static fun(int j) B(int i) { i = 0; return i; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(IfStatementCodegen)
		{
			auto tree = ParseTree("class A { static fun(bool isPositive) B(int i) { if (i>=0) return true; return false; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(IfElseStatementCodegen)
		{
			auto tree = ParseTree("class A { static fun(bool isPositive) B(int i) { if (i>=0) return true; else return false; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(IfElseIfElseStatementCodegen)
		{
			auto tree = ParseTree("class A { static fun(int j) B(int i) { if (i>0) return 1; else if (i == 0) return 0; else return -1; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(WhileStatementCodegen)
		{
			auto tree = ParseTree("class A { static fun(uint j) B(uint i) { uint j = 0; while (i > 0) { j = j+i--; } return j; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(WhileAndBreakStatementCodegen)
		{
			auto tree = ParseTree("class A { static fun(uint j) B(uint i) { uint j = 0; while (i > 0) { j = j+i--; if (j == 10) { break; } } return j; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}

		TEST_METHOD(ScopeCodegen)
		{
			auto tree = ParseTree("class A { static fun(int j) B(int i) { { int j = i; } return i; } }");
			llvm::IRBuilder<> builder(TheContext);
			auto module = new llvm::Module("Module", TheContext);
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table, &builder, &TheContext, module);
		}
	};
}