// Ruddy.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Grammar.h>
#include <Statements.h>
#include <Parser.h>
#include <Exceptions.h>
#include "cxxopts.hpp"

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

void AddExternOsFunctions(llvm::Module* module, llvm::LLVMContext& context)
{
	// printf
	std::vector<llvm::Type*> printfArgTypes;
	printfArgTypes.push_back(llvm::Type::getInt16PtrTy(context));
	auto printfFunctionType = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), printfArgTypes, true /*isVarArg*/);
	auto printfFunction = llvm::Function::Create(printfFunctionType, llvm::Function::ExternalLinkage, "_os_printf", module);
}

// Ruddy.exe produces the LLVM intermediate code representation. A batch file can be used as a full end-to-end compiler
int main(int argc, char* argv[])
{
	cxxopts::Options options(argv[0]);
	options.allow_unrecognised_options()
		.add_options()
		("positional", "Positional arguemnts", cxxopts::value<std::vector<std::string>>())
		("o,output", "output type (lib,dll,exe)", cxxopts::value<std::string>()->default_value("exe"));

	options.parse_positional({ "positional" });
	auto result = options.parse(argc, argv);

	if (!result.count("positional"))
	{
		std::cout << "Must pass in at least 1 file to compile";
		return -1;
	}

	auto& files = result["positional"].as<std::vector<std::string>>();

	try
	{
		std::shared_ptr<Ast::GlobalStatements> masterList = nullptr;
		std::shared_ptr<Ast::GlobalStatements> lastFile = nullptr;
		for (auto& file : files)
		{
			std::ifstream inputFile;
			inputFile.open(file, std::ios::in);
			auto stmtList = std::shared_ptr<Ast::GlobalStatements>(Parser::Parse(&inputFile));
			if (masterList == nullptr)
			{
				masterList = std::make_shared<Ast::GlobalStatements>(stmtList);
				lastFile = masterList;
			}
			else
			{
				lastFile->_next = std::make_shared<Ast::GlobalStatements>(stmtList);
				lastFile = lastFile->_next;
			}
		}

		// Type check
		auto symbolTable = std::make_shared<Ast::SymbolTable>();

		// Code generation
		llvm::LLVMContext* TheContext = new llvm::LLVMContext();
		llvm::IRBuilder<> builder(*TheContext);
		llvm::Module* module = new llvm::Module("Module", *TheContext);
		// Generate extern statements for os code. In the future, this will be
		// replaced by import statements so we're not generating everything
		AddExternOsFunctions(module, *TheContext);
		// Generate code from the input
		masterList->TypeCheck(symbolTable);

		masterList->CodeGen(&builder, TheContext, module);

		// Now save it to a file
		std::string fileName = argv[1];
		auto outputFileName = fileName.substr(0, fileName.find_last_of('.'));
		outputFileName.append(".ll");

		std::error_code errInfo;
		auto stream = std::make_unique<llvm::raw_fd_ostream>(outputFileName, errInfo, llvm::sys::fs::OpenFlags::F_RW);
		module->print(*stream.get(), nullptr);
		stream->close();
	}
	catch (Ast::Exception& e)
	{
		std::cout << e.Message();
		return -1;
	}
	catch (std::exception& e)
	{
		std::cout << "Unhandled exception, bad Ruddy, bad: " << e.what();
		return -1;
	}
	
	return 0;
}

