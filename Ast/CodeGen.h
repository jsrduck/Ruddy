#pragma once

#include "Exceptions.h"

#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>

namespace Ast {
	class SymbolCodeGenerator
	{
	public:
		virtual void BindIRValue(llvm::Value* value) = 0;

		virtual llvm::Value* GetIRValue(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) = 0;

		virtual llvm::Value* CreateAllocationInstance(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module* module) = 0;

		virtual llvm::BasicBlock* GetEndOfScopeBlock(llvm::LLVMContext* context) = 0;
	};
}