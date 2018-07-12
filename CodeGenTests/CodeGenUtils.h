#pragma once
#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>

namespace CodeGenTests {

	inline void TestTreeCodegen(Ast::GlobalStatements* tree, llvm::LLVMContext& context)
	{
		auto table = std::make_shared<Ast::SymbolTable>();
		tree->TypeCheck(table);

		llvm::IRBuilder<> builder(context);
		auto module = new llvm::Module("Module", context);
		tree->CodeGen(&builder, &context, module);
	}

}