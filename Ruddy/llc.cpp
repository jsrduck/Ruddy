//===-- llc.cpp - Implement the LLVM Native Code Generator ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the llc code generator driver. It provides a convenient
// command-line interface for generating native assembly-language code
// or C code, given LLVM bitcode.
//
//===----------------------------------------------------------------------===//

#include "stdafx.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/LinkAllAsmWriterComponents.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/CodeGen/MIRParser/MIRParser.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <llvm\IR\IRBuilder.h>
#include <memory>

#include "RewriteStatepointsForGCRuddy.h"
using namespace llvm;

struct LLCDiagnosticHandler : public DiagnosticHandler
{
	bool *HasError;
	LLCDiagnosticHandler(bool *HasErrorPtr) : HasError(HasErrorPtr)
	{
	}
	bool handleDiagnostics(const DiagnosticInfo &DI) override
	{
		if (DI.getSeverity() == DS_Error)
			*HasError = true;

		if (auto *Remark = dyn_cast<DiagnosticInfoOptimizationBase>(&DI))
			if (!Remark->isEnabled())
				return true;

		DiagnosticPrinterRawOStream DP(errs());
		errs() << LLVMContext::getDiagnosticMessagePrefix(DI.getSeverity()) << ": ";
		DI.print(DP);
		errs() << "\n";
		return true;
	}
};

int RunGCPass(llvm::Module& module)
{
	auto registry = PassRegistry::getPassRegistry();
	initializeCore(*registry);
	initializeTransformUtils(*registry);
	initializeScalarOpts(*registry);
	initializeRewriteStatepointsForGC_RuddyLegacyPassPass(*registry);
	legacy::PassManager PM;

	//todo: NEED gc.safepoint_poll method to add this
	auto safepoints = llvm::createPlaceSafepointsPass();
	PM.add(safepoints);

	// This requires us to differentiate a gc-managed address space
	// from the stack one. Unfortunately, this has huge implications
	// on what we can do as far as the "this" pointer is concerned.
	// For now, we're going to ignore compaction in the GC anyway,
	// and forget lowering for now. In the future, we'll have to use
	// one of these techniques, probably:
	// 1. Have two versions of every method (ugh)
	// 2. Just let the mutator analyze stack allocated data and eat the 
	// cost of the exra work.
	// At the moment we let it go just so we can have a stackmap
	auto gcPass = llvm::createRewriteStatepointsForGC_RuddyLegacyPass();
	PM.add(gcPass);
	if (!PM.run(module))
	{
		errs() << "Failed to run gc pass on module ";
		return -1;
	}
	return 0;
}

int compileModule(const char * InputFilename, const char * OutputFilename, LLVMContext &Context, llvm::IRBuilder<>& builder)
{
	InitializeAllTargetInfos();
	InitializeAllTargets();
	InitializeAllTargetMCs();
	InitializeAllAsmParsers();
	InitializeAllAsmPrinters();

	// Load the module to be compiled...
	SMDiagnostic Err;
	std::unique_ptr<Module> M;
	std::unique_ptr<MIRParser> MIR;
	Triple TheTriple;

	M = parseIRFile(InputFilename, Err, Context);
	if (!M)
	{
		Err.print("llc", errs());
		return 1;
	}

	// Verify module immediately to catch problems before doInitialization() is
	// called on any passes.
	if (verifyModule(*M, &errs()))
	{
		errs() << "llc" << ": " << InputFilename
			<< ": error: input module is broken!\n";
		return 1;
	}

	TheTriple = Triple(M->getTargetTriple());

	auto TargetTriple = "x86_64-pc-windows-msvc"; //sys::getDefaultTargetTriple();
	if (TheTriple.getTriple().empty())
		TheTriple.setTriple(TargetTriple);

	// Get the target specific parser.
	std::string Error;
	const Target *TheTarget = TargetRegistry::lookupTarget("", TheTriple,
		Error);
	if (!TheTarget)
	{
		errs() << "llc" << ": " << Error;
		return 1;
	}

	CodeGenOpt::Level OLvl = CodeGenOpt::Default;
	// TODO: Allow multiple optimization levels
	/*switch (OptLevel)
	{
		default:
			errs() << argv[0] << ": invalid optimization level.\n";
			return 1;
		case ' ': break;
		case '0': OLvl = CodeGenOpt::None; break;
		case '1': OLvl = CodeGenOpt::Less; break;
		case '2': OLvl = CodeGenOpt::Default; break;
		case '3': OLvl = CodeGenOpt::Aggressive; break;
	}*/

	auto CPU = "generic";
	auto Features = "";
	TargetOptions opt;
	auto RM = Optional<Reloc::Model>();

	assert(TheTarget && "Could not allocate target machine!");
	assert(M && "Should have exited if we didn't have a module!");

	auto TargetMachine = TheTarget->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

	// Figure out where we are going to send the output.
	std::error_code EC;
	raw_fd_ostream dest(OutputFilename, EC, sys::fs::F_None);

	// Build up all of the passes that we want to do to the module.
	legacy::PassManager PM;
	auto FileType = TargetMachine::CGFT_ObjectFile;

	// Add an appropriate TargetLibraryInfo pass for the module's triple.
	TargetLibraryInfoImpl TLII(Triple(M->getTargetTriple()));

	PM.add(new TargetLibraryInfoWrapperPass(TLII));

	// Add the target data from the target machine, if it exists, or the module.
	auto dataLayout = TargetMachine->createDataLayout();
	auto asStr = dataLayout.getStringRepresentation();
	M->setDataLayout(dataLayout);
	//auto funType = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(Context), false /*isVarArg*/);
	////auto fun = llvm::Function::Create(funType, llvm::GlobalValue::LinkageTypes::ExternalLinkage, "GetStackMapLocation", M.get());
	//auto bb = llvm::BasicBlock::Create(Context, "", fun);
	//builder.SetInsertPoint(bb);
	//auto global = M->getGlobalVariable("__LLVM_STACKMAPS");
	//builder.CreateLoad()
	//builder.CreateCall(rtSafepointPollFunction);
	//builder.CreateRetVoid();
	//llvm::verifyFunction(*safepointPollFunction);

	{
		if (TargetMachine->addPassesToEmitFile(PM, dest, FileType))
		{
			errs() << "TheTargetMachine can't emit a file of this type";
			return 1;
		}

		if (!PM.run(*M))
		{
			errs() << "Failed to run object compiler on module " << InputFilename;
			return -1;
		}
		dest.flush();
	}

	return 0;
}