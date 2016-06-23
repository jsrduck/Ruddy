#include "stdafx.h"
#include "Expressions.h"
#include "Statements.h"
#include "Classes.h"

#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>
#include <llvm\IR\Verifier.h>

//static llvm::IRBuilder<> Builder(*context);

namespace Ast {

	void GlobalStatements::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_stmt->CodeGen(symbolTable, builder, context, module);
		if (_next != nullptr)
			_next->CodeGen(symbolTable, builder, context, module);
	}

	void NamespaceDeclaration::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_stmts->CodeGen(symbolTable, builder, context, module);
	}

	void LineStatements::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_statement->CodeGen(symbolTable, builder, context, module);
		if (_next != nullptr)
			_next->CodeGen(symbolTable, builder, context, module);
	}

	void ExpressionAsStatement::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_expr->CodeGen(symbolTable, builder, context, module);
	}

	void IfStatement::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO
		throw UnexpectedException();
	}

	void WhileStatement::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO
		throw UnexpectedException();
	}

	void BreakStatement::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO
		throw UnexpectedException();
	}

	void Assignment::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		throw UnexpectedException();
	}

	void ScopedStatements::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO
		throw UnexpectedException();
	}

	void ReturnStatement::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO
		throw UnexpectedException();
	}

	void ClassDeclaration::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO: Class stuff
		_list->CodeGen(symbolTable, builder, context, module);
	}

	void ClassStatementList::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_statement->CodeGen(symbolTable, builder, context, module);
		if (_next != nullptr)
			_next->CodeGen(symbolTable, builder, context, module);
	}

	void ClassMemberDeclaration::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO
		throw UnexpectedException();
	}

	void FunctionDeclaration::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO: Make this work
		auto ft = llvm::FunctionType::get(llvm::Type::getVoidTy(*context), false);
		auto function = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "main", module);
		auto bb = llvm::BasicBlock::Create(*context, "entry", function);
		builder->SetInsertPoint(bb);
		_body->CodeGen(symbolTable, builder, context, module);
		builder->CreateRetVoid();
		llvm::verifyFunction(*function);
	}

	llvm::Value* DebugPrintStatement::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		auto value = _expression->CodeGen(symbolTable, builder, context, module);
		std::vector<llvm::Value*> args;

		if (value->getType()->isIntegerTy())
		{
			// Create a string on the stack, and then print it
			auto strConst = llvm::ConstantDataArray::getString(*context, llvm::StringRef("%d"));
			auto alloc = builder->CreateAlloca(strConst->getType(), builder->getInt32(llvm::dyn_cast<llvm::ConstantDataSequential>(strConst)->getNumElements()));
			builder->CreateStore(strConst, alloc);
			std::vector<llvm::Value*> index_vector;
			index_vector.push_back(builder->getInt32(0));
			index_vector.push_back(builder->getInt32(0));
			auto valueAsPtr = builder->CreateGEP(alloc, index_vector);

			// printf("%d", val)
			args.push_back(valueAsPtr);
			args.push_back(value);
		}
		else if (value->getType()->isDoubleTy())
		{
			// Create a string on the stack, and then print it
			auto strConst = llvm::ConstantDataArray::getString(*context, llvm::StringRef("%f"));
			auto alloc = builder->CreateAlloca(strConst->getType(), builder->getInt32(llvm::dyn_cast<llvm::ConstantDataSequential>(strConst)->getNumElements()));
			builder->CreateStore(strConst, alloc);
			std::vector<llvm::Value*> index_vector;
			index_vector.push_back(builder->getInt32(0));
			index_vector.push_back(builder->getInt32(0));
			auto valueAsPtr = builder->CreateGEP(alloc, index_vector);

			// printf("%f", val)
			args.push_back(valueAsPtr);
			args.push_back(value);
		}
		else if (llvm::isa<llvm::AllocaInst>(value))
		{
			std::vector<llvm::Value*> index_vector;
			index_vector.push_back(builder->getInt32(0));
			index_vector.push_back(builder->getInt32(0));
			auto valueAsPtr = builder->CreateGEP(value, index_vector);
			args.push_back(valueAsPtr);
		}
		else if (value->getType()->isArrayTy())
		{
			// Store the string
			auto alloc = builder->CreateAlloca(value->getType(), builder->getInt32(llvm::dyn_cast<llvm::ConstantDataSequential>(value)->getNumElements()));
			builder->CreateStore(value, alloc);

			std::vector<llvm::Value*> index_vector;
			index_vector.push_back(builder->getInt32(0));
			index_vector.push_back(builder->getInt32(0));
			auto valueAsPtr = builder->CreateGEP(alloc, index_vector);
			args.push_back(valueAsPtr);
		}
		else
		{
			throw UnexpectedException();
		}
		builder->CreateCall(module->getFunction("_os_printf"), args);
		return nullptr; // ???
	}

	llvm::Value* Reference::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		throw UnexpectedException();
		//return _symbol->GetAllocationInstance();
	}

	llvm::Value* ExpressionList::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		throw UnexpectedException();
		//return _symbol->GetAllocationInstance();
	}

	llvm::Value* StringConstant::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		return llvm::ConstantDataArray::getString(*context, llvm::StringRef(_input.c_str()));
	}
}