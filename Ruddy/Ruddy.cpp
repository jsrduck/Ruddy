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
#include <llvm\IRReader\IRReader.h>
#include <llvm\IR\Verifier.h>
#include <llvm\Support\raw_ostream.h>
#include <llvm\Support\FileSystem.h>
#include <llvm\Support\SourceMgr.h>
#include <llvm\Pass.h>

#include "ArchiveSerialization.h"

#include <boost\filesystem\path.hpp>
#include <boost\algorithm\string.hpp>

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

char* runtime_shim =

"declare void @_os_printf(i16*, ...) \"gc-leaf-function\"\r\n"
"declare i8 addrspace(0)* @_heap_alloc(i32)\r\n"
"declare void @runtime_safepoint_poll()\r\n"
"define void @gc.safepoint_poll()\r\n"
"{\r\n"
"	call void @runtime_safepoint_poll()\r\n"
"	ret void\r\n"
"}\r\n"
"attributes #1 = { \"gc-leaf-function\" nounwind readnone }\r\n";

std::unique_ptr<llvm::Module> AddExternOsFunctions(llvm::LLVMContext& context, llvm::IRBuilder<>& builder)
{
	llvm::SMDiagnostic err;
	// Using the file is handy for debugging, but we want to ship with the text inline
	//std::unique_ptr<llvm::Module> irModule = llvm::parseIRFile("runtime_shim.ll", err, context);
	auto memBuf = llvm::MemoryBuffer::getMemBuffer(runtime_shim);
	std::unique_ptr<llvm::Module> irModule = llvm::parseIR(memBuf->getMemBufferRef(), err, context);
	if (!irModule)
	{
		std::string what;
		llvm::raw_string_ostream os(what);
		err.print("error loading IR for runtime shim", os);
		std::cerr << what;
	}
	return irModule;
}

int compileModule(const char * InputFilename, const char * OutputFilename, llvm::LLVMContext &Context, llvm::IRBuilder<>& builder);
int LLC(std::string &outputIRFileName, std::string& outputObjFileName, llvm::LLVMContext& context, llvm::IRBuilder<>& builder)
{
	return compileModule(outputIRFileName.c_str(), outputObjFileName.c_str(), context, builder);
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

int RunLinker(std::vector<std::string>& objFileNames, std::string& outputName, std::vector<std::shared_ptr<Ruddy::Rib>>& ribs)
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

	std::wstringstream linkCmdPath(cmdExePath);
	linkCmdPath<< L" /c \"";
	linkCmdPath << installDir;
	linkCmdPath << L"\\VC\\Auxiliary\\Build\\vcvarsall.bat\" amd64 & link";
	for (auto& objFileName : objFileNames)
	{
		linkCmdPath << L" ";
		linkCmdPath << std::wstring(objFileName.begin(), objFileName.end());
	}
	// todo: make this platform/flavor independent. It's ok for now.
	linkCmdPath << L" ..\\x64\\Debug\\WindowsImpl.lib ..\\x64\\Debug\\Runtime.lib /subsystem:console /OUT:";
	linkCmdPath << std::wstring(outputName.begin(), outputName.end());

	return ExecuteExe(cmdExePath, linkCmdPath.str());
}

// Ruddy.exe produces the LLVM intermediate code representation. A batch file can be used as a full end-to-end compiler
int RunGCPass(llvm::Module& module);
int main(int argc, char* argv[])
{
	cxxopts::Options options(argv[0]);
	options.allow_unrecognised_options()
		.add_options()
		("positional", "Positional arguemnts", cxxopts::value<std::vector<std::string>>())
		("t,type", "output type (rib,dll,exe)", cxxopts::value<std::string>()->default_value("exe"))
		("l,libs", "external libraries, semicolon-delimitted", cxxopts::value<std::string>());

	options.parse_positional({ "positional" });
	auto result = options.parse(argc, argv);

	if (!result.count("positional"))
	{
		std::cout << "Must pass in at least 1 file to compile";
		return -1;
	}

	auto files = result["positional"].as<std::vector<std::string>>();

	std::vector<std::string> libs;
	if (result.count("libs"))
	{
		auto libsStr = result["libs"].as<std::string>();
		boost::split(libs, libsStr, boost::is_any_of(";"));
	}

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

		// Add any external libs first
		std::vector<std::shared_ptr<Ruddy::Rib>> ribs;

		for(auto& lib : libs)
		{
			ribs.push_back(std::make_shared<Ruddy::Rib>(lib.c_str()));
		}

		for (auto& rib : ribs)
		{
			symbolTable->AddExternalLibrary(rib->Rinc->Metadata.Name, [rib]()
			{
				return rib->Rinc->GetSymbolTable();
			});
		}

		std::string fileName = files[0];
		boost::filesystem::path filePath(fileName);
		auto basePath = filePath.parent_path();
		auto libName = filePath.filename();
		libName = libName.replace_extension();
		auto outputIRFileName = fileName.substr(0, fileName.find_last_of('.'));
		outputIRFileName.append(".ll");

		// Code generation
		llvm::LLVMContext TheContext {};
		llvm::IRBuilder<> builder(TheContext);
		// Generate extern statements for os code. In the future, this will be
		// replaced by import statements so we're not generating everything
		auto module = AddExternOsFunctions(TheContext, builder);
		if (!module)
			return -1;
		auto moduleName = libName.string();
		module->setModuleIdentifier(moduleName);
		module->setSourceFileName(filePath.filename().string());
		// Generate code from the input
		masterList->TypeCheck(symbolTable);

		masterList->CodeGen(&builder, &TheContext, module.get());

		// GC Passes
		//if (RunGCPass(*module) != 0)
		//{
		//	return -1;
		//}

		// Now save it to a file
		std::error_code errInfo;
		auto stream = std::make_unique<llvm::raw_fd_ostream>(outputIRFileName, errInfo, llvm::sys::fs::OpenFlags::F_RW);
		module->print(*stream.get(), nullptr);
		stream->close();

		auto type = result["t"].as<std::string>();
		auto rootFileName = fileName.substr(0, fileName.find_last_of('.'));
		if (type.compare("exe") == 0)
		{
			auto objFileName = rootFileName;
			auto exeFileName = rootFileName;
			objFileName.append(".obj");
			exeFileName.append(".exe");

			auto retVal = LLC(outputIRFileName, objFileName, TheContext, builder);
			if (retVal != 0)
				return retVal;

			std::vector<std::string> objFiles;
			objFiles.push_back(objFileName);

			for (auto& rib : ribs)
			{
				if (rib->WasAccessed())
				{
					// We need to compile this rib into obj code as well
					auto ribLL = rib->WriteIRTo(basePath.string());
					objFileName = ribLL.substr(0, ribLL.find_last_of('.'));
					objFileName.append(".obj");
					retVal = LLC(ribLL, objFileName, TheContext, builder);
					if (retVal != 0)
						return retVal;
					boost::filesystem::path objPath(ribLL);
					objPath.replace_extension("obj");
					objFiles.push_back(objPath.string());
				}
			}

			retVal = RunLinker(objFiles, exeFileName, ribs);
			if (retVal != 0)
			{
				std::cerr << "Failed to execute link.exe, cannot produce link files: " << GetLastError() << endl;
				return retVal;
			}
		}
		else if (type.compare("rib") == 0)
		{
			auto ribName = rootFileName;
			ribName.append(".rib");
			std::ofstream ofStream(ribName);

			std::ifstream irStream(outputIRFileName);
			
			Ruddy::LibraryMetadata metadata(libName.string(), Ruddy::VersionNumber::CurrentVersion);
			auto rinc = std::make_shared<Ruddy::Rinc>(symbolTable, metadata, basePath.string());
			Ruddy::Rib rib(rinc, outputIRFileName);
			rib.Serialize(ribName.c_str());
		}
		else
		{
			std::cerr << "Unknown type option: " << type << endl;
			return -1;
		}
	}
	catch (Ast::Exception& e)
	{
		std::cerr << e.Message() << endl;
		return -1;
	}
	catch (Ruddy::CompilerException& e)
	{
		std::cerr << e.Message() << endl;
		return -1;
	}
	catch (std::exception& e)
	{
		std::cerr << "Unhandled exception, bad Ruddy, bad: " << e.what() << endl;
		return -1;
	}
	catch (...)
	{
		std::cerr << "Unhandled exception, bad Ruddy, bad " << endl;
		return -1;
	}
	
	return 0;
}

