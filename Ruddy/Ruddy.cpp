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

using namespace std;

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

int llc(int argc, char **argv);
int LLC(std::string &outputFileName, llvm::LLVMContext& context)
{
	//-filetype = obj
	char* args[3];

	char* llcName = "llc";
	std::string arg1 = "-filetype=obj";
	char arg1Buff[_MAX_PATH];
	if (strncpy_s(arg1Buff, _countof(arg1Buff), arg1.c_str(), arg1.size()+1) != 0)
	{
		return -11;
	}

	char arg2Buff[_MAX_PATH];
	if (strncpy_s(arg2Buff, _countof(arg2Buff), outputFileName.c_str(), outputFileName.size() + 1) != 0)
	{
		return -12;
	}

	args[0] = llcName;
	args[1] = arg1Buff;
	args[2] = arg2Buff;

	return llc(3, args);
}

class CoInitializer
{
public:
	CoInitializer()
	{
		hr = ::CoInitialize(NULL);
		if (FAILED(hr))
		{
			throw std::exception("failed to initialize COM");
		}
	}

	~CoInitializer()
	{
		if (SUCCEEDED(hr))
		{
			::CoUninitialize();
		}
	}

private:
	HRESULT hr;
};


// Use smart pointers (without ATL) to release objects when they fall out of scope.
_COM_SMARTPTR_TYPEDEF(ISetupInstance, __uuidof(ISetupInstance));
_COM_SMARTPTR_TYPEDEF(ISetupInstance2, __uuidof(ISetupInstance2));
_COM_SMARTPTR_TYPEDEF(IEnumSetupInstances, __uuidof(IEnumSetupInstances));
_COM_SMARTPTR_TYPEDEF(ISetupConfiguration, __uuidof(ISetupConfiguration));
_COM_SMARTPTR_TYPEDEF(ISetupConfiguration2, __uuidof(ISetupConfiguration2));
_COM_SMARTPTR_TYPEDEF(ISetupHelper, __uuidof(ISetupHelper));
_COM_SMARTPTR_TYPEDEF(ISetupPackageReference, __uuidof(ISetupPackageReference));
_COM_SMARTPTR_TYPEDEF(ISetupPropertyStore, __uuidof(ISetupPropertyStore));
_COM_SMARTPTR_TYPEDEF(ISetupInstanceCatalog, __uuidof(ISetupInstanceCatalog));
int GetCurrentVSInstance(BSTR &installPath)
{
	CoInitializer init;
	ISetupConfigurationPtr query;

	auto hr = query.CreateInstance(__uuidof(SetupConfiguration));
	if (REGDB_E_CLASSNOTREG == hr)
	{
		std::cout << "Could not find instance of VS on this machine, cannot link files." << endl;
		return -7;
	}
	else if (FAILED(hr))
	{
		std::cout << "Unexpected error: could not find instance of VS on this machine, cannot link files." << endl;
		return -8;
	}

	ISetupConfiguration2Ptr query2(query);
	IEnumSetupInstancesPtr e;

	hr = query2->EnumAllInstances(&e);
	if (FAILED(hr))
	{
		std::cout << "Unexpected error: could not find instance of VS on this machine, cannot link files." << endl;
		return -9;
	}

	ISetupHelperPtr helper(query);

	ISetupInstance* pInstances[1] = {};
	hr = e->Next(1, pInstances, NULL);
	if (FAILED(hr))
	{
		std::cout << "Unexpected error: could not find instance of VS on this machine, cannot link files." << endl;
		return -10;
	}
	while (S_OK == hr)
	{
		// Wrap instance without AddRef'ing.
		ISetupInstancePtr instance = ISetupInstancePtr(pInstances[0], false);
		instance->GetInstallationPath(&installPath);
		hr = e->Next(1, pInstances, NULL);
	}
	return 0;
}

int ExecuteExe(std::wstring process, std::wstring& args)
{
	wchar_t argsBuff[2047];
	if (wcsncpy_s(argsBuff, args.c_str(), _countof(argsBuff)) != 0)
	{
		return -6;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	ZeroMemory(&pi, sizeof(pi));
	if (!!::CreateProcess(process.c_str(), argsBuff, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD exitCode;
		if (!!::GetExitCodeProcess(pi.hProcess, &exitCode))
		{
			if (exitCode != 0)
			{
				return exitCode;
			}
		}
		else
		{
			return -4;
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else
	{
		return -5;
	}

	return 0;
}

int RunLinker(std::string& objFileName, std::string& outputName)
{
	// Get current working directory
	wchar_t workingDir[_MAX_PATH];

	if (!_wgetcwd(workingDir, _countof(workingDir)))
	{
		std::cerr << "Failed to get current working dir" << endl;
		return errno;
	}

	wchar_t comSpecBuffer[_MAX_PATH];
	size_t reqCount;
	if (_wgetenv_s(&reqCount, comSpecBuffer, L"ComSpec") != 0)
	{
		return -2;
	}
	std::wstring cmdExePath(comSpecBuffer);

	// Set VS env variables
	if (_wputenv_s(L"VSCMD_START_DIR", workingDir) != 0)
	{
		return -3;
	}

	// Get VS install dir
	BSTR installDir;
	int retval = GetCurrentVSInstance(installDir);
	if (retval != 0)
		return retval;

	std::wstring linkCmdPath(cmdExePath);
	linkCmdPath.append(L" /c \"");
	linkCmdPath.append(installDir);
	linkCmdPath.append(L"\\VC\\Auxiliary\\Build\\vcvarsall.bat\" x86 & link ");
	linkCmdPath.append(objFileName.begin(), objFileName.end());
	linkCmdPath.append(L" ..\\WindowsImpl\\Debug\\IO.obj ..\\WindowsImpl\\Debug\\stdafx.obj /subsystem:console /OUT:");
	linkCmdPath.append(outputName.begin(), outputName.end());

	return ExecuteExe(cmdExePath, linkCmdPath);
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
		llvm::LLVMContext TheContext {};
		llvm::IRBuilder<> builder(TheContext);
		llvm::Module module("Module", TheContext);
		// Generate extern statements for os code. In the future, this will be
		// replaced by import statements so we're not generating everything
		AddExternOsFunctions(&module, TheContext);
		// Generate code from the input
		masterList->TypeCheck(symbolTable);

		masterList->CodeGen(&builder, &TheContext, &module);

		// Now save it to a file
		std::string fileName = argv[1];
		auto outputFileName = fileName.substr(0, fileName.find_last_of('.'));
		outputFileName.append(".ll");

		std::error_code errInfo;
		auto stream = std::make_unique<llvm::raw_fd_ostream>(outputFileName, errInfo, llvm::sys::fs::OpenFlags::F_RW);
		module.print(*stream.get(), nullptr);
		stream->close();

		auto retVal = LLC(outputFileName, TheContext);
		if (retVal != 0)
			return retVal;

		auto objFileName = fileName.substr(0, fileName.find_last_of('.'));
		auto exeFileName = objFileName;
		objFileName.append(".obj");
		exeFileName.append(".exe");
		retVal = RunLinker(objFileName, exeFileName);
		if (retVal != 0)
		{
			std::cerr << "Failed to execute link.exe, cannot produce link files: " << GetLastError() << endl;
			return retVal;
		}
	}
	catch (Ast::Exception& e)
	{
		std::cerr << e.Message() << endl;
		return -1;
	}
	catch (std::exception& e)
	{
		std::cerr << "Unhandled exception, bad Ruddy, bad: " << e.what() << endl;
		return -1;
	}
	
	return 0;
}

