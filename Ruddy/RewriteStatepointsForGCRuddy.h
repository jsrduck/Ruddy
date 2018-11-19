#pragma once 

//===- RewriteStatepointsForGCRuddy.h - ------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Adapted for use in Ruddy programming language
//
//===----------------------------------------------------------------------===//
//
// This file provides interface to "Rewrite Statepoints for GC" pass.
//
// This passe rewrites call/invoke instructions so as to make potential
// relocations performed by the garbage collector explicit in the IR.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/PassManager.h"

namespace llvm {

class DominatorTree;
class Function;
class Module;
class TargetTransformInfo;
class TargetLibraryInfo;

struct RewriteStatepointsForGC_Ruddy : public PassInfoMixin<RewriteStatepointsForGC_Ruddy>
{
	PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);

	bool runOnFunction(Function &F, DominatorTree &, TargetTransformInfo &,
		const TargetLibraryInfo &);
};

ModulePass *createRewriteStatepointsForGC_RuddyLegacyPass();
void initializeRewriteStatepointsForGC_RuddyLegacyPassPass(PassRegistry &);

} // namespace llvm

