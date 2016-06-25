#include "stdafx.h"
#include "TypeInfo.h"
#include "Statements.h"
#include "Classes.h"
#include "Primitives.h"

#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>
#include <llvm\IR\Verifier.h>

#include <assert.h>
using namespace std;

namespace Ast {

	std::shared_ptr<TypeInfo> Reference::Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	{
		_symbol = symbolTable->Lookup(_id);
		if (_symbol == nullptr)
		{
			// This id isn't defined in the symbol table yet
			throw SymbolNotDefinedException(_id);
		}
		if (!_symbol->IsVariableBinding())
		{
			// This symbol exists, but it's not a variable so it can't be used as an expression
			throw SymbolWrongTypeException(_id);
		}
		return _symbol->GetTypeInfo();
	}

	std::shared_ptr<TypeInfo> ExpressionList::Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	{
		return std::make_shared<CompositeTypeInfo>(_left->Evaluate(symbolTable), std::make_shared<CompositeTypeInfo>(_right->Evaluate(symbolTable), nullptr));
	}

	std::shared_ptr<TypeInfo> NewExpression::Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto symbol = symbolTable->Lookup(_className);
		if (symbol == nullptr)
		{
			// This id isn't defined in the symbol table yet
			throw SymbolNotDefinedException(_className);
		}
		if (!symbol->IsClassBinding())
		{
			// This symbol exists, but it's not the name of a class
			throw SymbolWrongTypeException(symbol->GetFullyQualifiedName());
		}
		return symbol->GetTypeInfo();
	}

	std::shared_ptr<TypeInfo> FunctionCall::Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto symbol = symbolTable->Lookup(_name);
		if (symbol == nullptr)
		{
			// This id isn't defined in the symbol table yet
			throw SymbolNotDefinedException(_name);
		}
		if (!symbol->IsFunctionBinding())
		{
			// This symbol exists, but it's not the name of a function
			throw SymbolWrongTypeException(symbol->GetFullyQualifiedName());
		}
		auto functionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(symbol->GetTypeInfo());
		if (functionTypeInfo == nullptr)
			throw UnexpectedException();
		return functionTypeInfo->OutputArgsType();
	}

	std::shared_ptr<TypeInfo> DebugPrintStatement::Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	{
		_expressionTypeInfo = _expression->Evaluate(symbolTable);
		return nullptr;
	}

	void LineStatements::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_statement->TypeCheck(symbolTable, builder, context, module);
		if (_next != nullptr)
			_next->TypeCheck(symbolTable, builder, context, module);
	}

	void IfStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		bool doCodeGen = builder != nullptr;
		// Type check the condition
		auto conditionType = _condition->Evaluate(symbolTable);
		if (!BoolTypeInfo::Get()->IsImplicitlyAssignableFrom(conditionType, symbolTable))
		{
			throw TypeMismatchException(BoolTypeInfo::Get(), conditionType);
		}

		// CodeGen
		auto ifContBlock = doCodeGen ? llvm::BasicBlock::Create(*context) : nullptr; // The label after the if/else statement is over
		auto func = doCodeGen ? builder->GetInsertBlock()->getParent() : nullptr;
		auto elseBlock = _elseStatement != nullptr && doCodeGen ? llvm::BasicBlock::Create(*context) : nullptr;
		if (doCodeGen)
		{
			auto cond = _condition->CodeGen(symbolTable, builder, context, module, BoolTypeInfo::Get());
			auto result = builder->CreateICmpNE(cond, builder->getInt1(0));

			auto ifBlock = llvm::BasicBlock::Create(*context, "", func);

			if (elseBlock != nullptr)
				builder->CreateCondBr(cond, ifBlock, elseBlock);
			else
				builder->CreateCondBr(cond, ifBlock, ifContBlock);

			builder->SetInsertPoint(ifBlock);
		}

		symbolTable->Enter();
		_statement->TypeCheck(symbolTable, builder, context, module);
		if (doCodeGen)
			builder->CreateBr(ifContBlock);
		symbolTable->Exit();

		if (_elseStatement != nullptr)
		{
			if (doCodeGen)
			{
				// CodeGen
				func->getBasicBlockList().push_back(elseBlock);
				builder->SetInsertPoint(elseBlock);
			}
			symbolTable->Enter();
			_elseStatement->TypeCheck(symbolTable, builder, context, module);
			if (doCodeGen)
				builder->CreateBr(ifContBlock);
			symbolTable->Exit();
		}

		if (doCodeGen)
		{
			func->getBasicBlockList().push_back(ifContBlock);
			builder->SetInsertPoint(ifContBlock);
		}
	}

	void WhileStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO: More static analysis. Infinite loops? Unreachable code?
		symbolTable->Enter();
		symbolTable->BindLoop();
		auto conditionType = _condition->Evaluate(symbolTable);
		if (!BoolTypeInfo::Get()->IsImplicitlyAssignableFrom(conditionType, symbolTable))
		{
			throw TypeMismatchException(BoolTypeInfo::Get(), conditionType);
		}

		bool doCodeGen = builder != nullptr;
		auto func = doCodeGen ? builder->GetInsertBlock()->getParent() : nullptr;
		auto conditionBlock = doCodeGen ? llvm::BasicBlock::Create(*context, "", func) : nullptr;
		if (doCodeGen)
		{
			builder->CreateBr(conditionBlock);
			builder->SetInsertPoint(conditionBlock);
			auto loopBlock = llvm::BasicBlock::Create(*context);
			builder->CreateCondBr(_condition->CodeGen(symbolTable, builder, context, module, BoolTypeInfo::Get()), loopBlock, symbolTable->GetCurrentLoop()->GetEndOfScopeBlock(context));
			func->getBasicBlockList().push_back(loopBlock);
			builder->SetInsertPoint(loopBlock);
		}
		_statement->TypeCheck(symbolTable, builder, context, module);
		if (doCodeGen)
		{
			builder->CreateBr(conditionBlock);

			auto fallthrough = symbolTable->GetCurrentLoop()->GetEndOfScopeBlock(context);
			func->getBasicBlockList().push_back(fallthrough);
			builder->SetInsertPoint(fallthrough);
		}
		symbolTable->Exit();
	}

	void BreakStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		auto loopSymbol = symbolTable->GetCurrentLoop();
		if (loopSymbol == nullptr)
		{
			throw BreakInWrongPlaceException();
		}
		if (builder != nullptr)
		{
			builder->CreateBr(loopSymbol->GetEndOfScopeBlock(context));
		}
	}

	std::shared_ptr<TypeInfo> AssignFromReference::Resolve(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto symbol = symbolTable->Lookup(_ref);
		if (symbol == nullptr)
		{
			throw SymbolNotDefinedException(_ref);
		}
		if (!symbol->IsClassMemberBinding() && !symbol->IsVariableBinding())
		{
			throw SymbolWrongTypeException(_ref);
		}
		return symbol->GetTypeInfo();
	}

	std::shared_ptr<TypeInfo> DeclareVariable::Resolve(std::shared_ptr<SymbolTable> symbolTable)
	{
		if (_typeInfo->NeedsResolution())
		{
			auto symbol = symbolTable->Lookup(_typeInfo->Name());
			if (symbol == nullptr)
				throw SymbolNotDefinedException(_typeInfo->Name());
			if (!symbol->IsClassBinding())
				throw SymbolWrongTypeException(symbol->GetFullyQualifiedName());
			_typeInfo = symbol->GetTypeInfo();
		}
		return _typeInfo;
	}

	void DeclareVariable::Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs)
	{
		if (!_typeInfo->IsImplicitlyAssignableFrom(rhs, symbolTable))
		{
			throw TypeMismatchException(_typeInfo, rhs);
		}
		// TODO: On auto type, assigning right hand type of constant won't work, 
		// you need to assign the type to the "best fit" for that constant
		// TODO: This should be fixed now, test with a unit test
		symbolTable->BindVariable(_name,_typeInfo->IsAutoType() ? rhs : _typeInfo);
	}

	void Assignment::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_lhsTypeInfo = _lhs->Resolve(symbolTable);

		_rhsTypeInfo = _rhs->Evaluate(symbolTable);
		if (!_lhsTypeInfo->IsImplicitlyAssignableFrom(_rhsTypeInfo, symbolTable))
		{
			throw TypeMismatchException(_lhsTypeInfo, _rhsTypeInfo);
		}
		_lhs->Bind(symbolTable, _rhsTypeInfo);
		if (builder != nullptr)
			_lhs->CodeGen(_rhs, symbolTable, builder, context, module);
	}

	void ReturnStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_returnType = _idList->Evaluate(symbolTable);
		auto functionSymbol = symbolTable->GetCurrentFunction();
		if (symbolTable->GetCurrentFunction() == nullptr)
		{
			throw ReturnStatementMustBeDeclaredInFunctionScopeException();
		}
		auto functionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(functionSymbol->GetTypeInfo());
		if (functionTypeInfo == nullptr)
			throw UnexpectedException();
		if (!functionTypeInfo->OutputArgsType()->IsImplicitlyAssignableFrom(_returnType, symbolTable))
			throw TypeMismatchException(functionTypeInfo->OutputArgsType(), _returnType);
		if (builder != nullptr)
		{
			throw UnexpectedException(); // TODO: Function support (besides main)
		}
	}

	void GlobalStatements::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_stmt->TypeCheck(symbolTable, builder, context, module);
		if (_next != nullptr)
			_next->TypeCheck(symbolTable, builder, context, module);
	}

	void NamespaceDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		symbolTable->Enter();
		symbolTable->BindNamespace(_name);
		_stmts->TypeCheck(symbolTable, builder, context, module);
		symbolTable->Exit();
	}

	void ScopedStatements::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		symbolTable->Enter();
		_statements->TypeCheck(symbolTable, builder, context, module);
		symbolTable->Exit();
	}

	void ExpressionAsStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_expr->Evaluate(symbolTable);
		if (builder != nullptr)
			_expr->CodeGen(symbolTable, builder, context, module);
	}

	void ClassDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO: CodeGen support for classes
		symbolTable->Enter();
		symbolTable->BindClass(_name, dynamic_pointer_cast<ClassDeclaration>(shared_from_this()));
		if (_list != nullptr)
			_list->TypeCheck(symbolTable, builder, context, module);
		symbolTable->Exit();
	}

	void ClassMemberDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO: Check if type is even visible
		if (_defaultValue != nullptr && !_typeInfo->IsImplicitlyAssignableFrom(_defaultValue->Evaluate(symbolTable), symbolTable))
		{
			throw TypeMismatchException(_defaultValue->Evaluate(symbolTable), _typeInfo);
		}
		else if (_defaultValue == nullptr && !_typeInfo->IsLegalTypeForAssignment(symbolTable))
		{
			// We need to make sure that this is a legal type for assignment
			throw SymbolWrongTypeException(_typeInfo->Name());
		}

		symbolTable->BindMemberVariable(_name, dynamic_pointer_cast<ClassMemberDeclaration>(shared_from_this()));
		// TODO: CodeGen support
	}

	// TODO: Static analysis to make sure all code paths return a value (for non void return types)
	// TODO: Static analysis to make sure there is no "unreachable" code (maybe that should be on "linestatements?"
	void FunctionDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		symbolTable->Enter();
		symbolTable->BindFunction(_name, dynamic_pointer_cast<FunctionDeclaration>(shared_from_this()));

		// TODO: Actual support for function codegen
		bool doCodeGen = builder != nullptr;
		auto ft = doCodeGen ? llvm::FunctionType::get(llvm::Type::getVoidTy(*context), false) : nullptr;
		auto function = doCodeGen ? llvm::Function::Create(ft, llvm::Function::ExternalLinkage, _name, module) : nullptr;
		if (doCodeGen)
		{
			auto bb = llvm::BasicBlock::Create(*context, "", function);
			builder->SetInsertPoint(bb);
		}

		auto argumentList = _inputArgs;
		while (argumentList != nullptr)
		{
			symbolTable->BindVariable(argumentList->_argument->_name, argumentList->_argument->_typeInfo);
			argumentList = argumentList->_next;
		}

		if (_body != nullptr)
			_body->TypeCheck(symbolTable, builder, context, module);

		if (doCodeGen)
		{
			builder->CreateRetVoid();
			llvm::verifyFunction(*function);
		}
		symbolTable->Exit();
	}

	void ConstructorDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO: C'tor CodeGen support
		if (builder != nullptr)
			throw UnexpectedException();

		symbolTable->Enter();
		symbolTable->BindFunction(_name, dynamic_pointer_cast<FunctionDeclaration>(shared_from_this()));
		auto argumentList = _inputArgs;
		while (argumentList != nullptr)
		{
			symbolTable->BindVariable(argumentList->_argument->_name, argumentList->_argument->_typeInfo);
			argumentList = argumentList->_next;
		}

		if (_body != nullptr)
			_body->TypeCheck(symbolTable, builder, context, module);
		symbolTable->Exit();
	}

	void DestructorDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// TODO: D'tor CodeGen support
		if (builder != nullptr)
			throw UnexpectedException();

		symbolTable->Enter();
		symbolTable->BindFunction("~" + _name, dynamic_pointer_cast<FunctionDeclaration>(shared_from_this()));
		if (_body != nullptr)
			_body->TypeCheck(symbolTable, builder, context, module);
		symbolTable->Exit();
	}

	void ClassStatementList::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_statement->TypeCheck(symbolTable, builder, context, module);
		if (_next != nullptr)
			_next->TypeCheck(symbolTable, builder, context, module);
	}
}