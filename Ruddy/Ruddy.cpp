// Ruddy.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Grammar.h>
#include <Statements.h>
#include <Parser.h>

#include <string>
#include <memory>
#include <sstream>
#include <iostream>
#include <fstream>

#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>
#include <llvm\Support\raw_ostream.h>
#include <llvm\Support\FileSystem.h>

inline std::unique_ptr<Ast::GlobalStatements> ParseTree(std::string code)
{
	std::istringstream inputStream(code);
	return std::unique_ptr<Ast::GlobalStatements>(Parser::Parse(&inputStream));
}

// Useful for getting bison grammar logging in Console
void TestGrammar()
{
	try
	{
		auto tree = ParseTree("class A { fun B() { let c = new D(); } }");
	}
	catch (...)
	{

	}
}

void AddExternOsFunctions(llvm::Module* module)
{
	// printf
	std::vector<llvm::Type*> printfArgTypes;
	printfArgTypes.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
	auto printfFunctionType = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), printfArgTypes, true /*isVarArg*/);
	auto printfFunction = llvm::Function::Create(printfFunctionType, llvm::Function::ExternalLinkage, "_os_printf", module);
}

// Ruddy.exe produces the LLVM intermediate code representation. A batch file can be used as a full end-to-end compiler
int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 1)
	{
		std::cout << "Must pass in a file to compile";
		return -1;
	}

	std::ifstream inputFile;
	inputFile.open(argv[1], std::ios::in);
	auto tree = std::unique_ptr<Ast::GlobalStatements>(Parser::Parse(&inputFile));

	// Type check
	auto symbolTable = std::make_shared<Ast::SymbolTable>();
	tree->TypeCheck(symbolTable);

	// Code generation
	llvm::IRBuilder<> builder(llvm::getGlobalContext());
	auto module = new llvm::Module("Module", llvm::getGlobalContext());
	// Generate extern statements for os code. In the future, this will be
	// replaced by import statements so we're not generating everything
	AddExternOsFunctions(module);
	// Generate code from the input
	tree->CodeGen(symbolTable, &builder, &llvm::getGlobalContext(), module);
	
	// Now save it to a file
	std::wstring fileName = argv[1];
	auto outputFileName = fileName.substr(0, fileName.find_last_of('.'));
	outputFileName.append(L".ll");
	std::string outputFileNameAsString = std::string(outputFileName.begin(), outputFileName.end());

	std::error_code errInfo;
	auto stream = std::make_unique<llvm::raw_fd_ostream>(outputFileNameAsString, errInfo, llvm::sys::fs::OpenFlags::F_RW);
	module->print(*stream.get(), nullptr);
	stream->close();
	return 0;
}

