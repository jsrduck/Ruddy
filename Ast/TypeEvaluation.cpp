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
		auto rhs = _right->Evaluate(symbolTable);
		auto rhsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(rhs);
		if (rhsComposite == nullptr)
			rhsComposite = std::make_shared<CompositeTypeInfo>(rhs, nullptr);
		return std::make_shared<CompositeTypeInfo>(_left->Evaluate(symbolTable), rhsComposite);
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
		bool ifContBlockReachable = false;
		auto func = doCodeGen ? builder->GetInsertBlock()->getParent() : nullptr;
		auto elseBlock = _elseStatement != nullptr && doCodeGen ? llvm::BasicBlock::Create(*context) : nullptr;
		if (doCodeGen)
		{
			auto cond = _condition->CodeGen(symbolTable, builder, context, module, BoolTypeInfo::Get());
			auto result = builder->CreateICmpNE(cond, builder->getInt1(0));

			auto ifBlock = llvm::BasicBlock::Create(*context, "", func);

			if (elseBlock != nullptr)
			{
				builder->CreateCondBr(cond, ifBlock, elseBlock);
			}
			else
			{
				builder->CreateCondBr(cond, ifBlock, ifContBlock);
				ifContBlockReachable = true;
			}

			builder->SetInsertPoint(ifBlock);
		}

		symbolTable->Enter();
		_statement->TypeCheck(symbolTable, builder, context, module);
		if (doCodeGen)
		{
			// Add a branch to the continue block (after the if/else)
			// but not if there's already a terminator, ie a return stmt.
			auto ifBlock = builder->GetInsertBlock();
			if (ifBlock->empty() || !ifBlock->back().isTerminator())
			{
				builder->CreateBr(ifContBlock);
				ifContBlockReachable = true;
			}
		}
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
			{
				// Add a branch to the continue block (after the if/else)
				// but not if there's already a terminator, ie a return stmt.
				elseBlock = builder->GetInsertBlock();
				if (elseBlock->empty() || !elseBlock->back().isTerminator())
				{
					builder->CreateBr(ifContBlock);
					ifContBlockReachable = true;
				}
			}
			symbolTable->Exit();
		}

		if (doCodeGen && ifContBlockReachable)
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

	std::shared_ptr<TypeInfo> AssignFromReference::Resolve(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhsTypeInfo, std::shared_ptr<Expression> rhsExpr)
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

	std::shared_ptr<TypeInfo> DeclareVariable::Resolve(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhsTypeInfo, std::shared_ptr<Expression> rhsExpr)
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
		if (_typeInfo->IsAutoType())
		{
			// Need to resolve based on the rhs
			if (rhsTypeInfo->IsConstant())
			{
				auto rhsAsConstant = std::dynamic_pointer_cast<ConstantExpression>(rhsExpr);
				if (rhsAsConstant == nullptr)
					throw UnexpectedException();
				_typeInfo = rhsAsConstant->BestFitTypeInfo();
			}
		}
		return _typeInfo;
	}

	void DeclareVariable::Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs)
	{
		if (!_typeInfo->IsImplicitlyAssignableFrom(rhs, symbolTable))
		{
			throw TypeMismatchException(_typeInfo, rhs);
		}
		// TODO: Unit test cases where rhs is constant
		symbolTable->BindVariable(_name, _typeInfo->IsAutoType() ? rhs : _typeInfo);
	}

	void Assignment::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		_rhsTypeInfo = _rhs->Evaluate(symbolTable);
		_lhsTypeInfo = _lhs->Resolve(symbolTable, _rhsTypeInfo, _rhs);

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
		if (functionSymbol == nullptr)
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
			auto outputTypeAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(functionTypeInfo->OutputArgsType());
			if (outputTypeAsComposite != nullptr)
			{
				// Return the 1st parameter, treat the rest as out parameters
				auto exprList = std::dynamic_pointer_cast<ExpressionList>(_idList);

				auto function = (llvm::Function*)functionSymbol->GetIRValue();

				// Get the number of input arguments
				int inputArgCount = 0;
				if (functionTypeInfo->InputArgsType() != nullptr)
				{
					auto inputArgsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(functionTypeInfo->InputArgsType());
					if (inputArgsComposite == nullptr)
					{
						inputArgCount++;
					}
					else
					{
						auto inputArg = inputArgsComposite;
						while (inputArg != nullptr && inputArgsComposite->_thisType != nullptr)
						{
							inputArgCount++;
							inputArg = inputArg->_next;
						}
					}
				}
				auto currExprList = exprList;
				auto currOutputType = outputTypeAsComposite;
				int i = 0;
				for (auto &arg : function->args())
				{
					if (i++ < inputArgCount)
						continue;

					// Now we've gotten to output args
					// TODO: Passing fun with same output type???
					currOutputType = currOutputType->_next;
					auto currExpr = currExprList->_right;
					if (currExpr != nullptr)
					{
						currExprList = std::dynamic_pointer_cast<ExpressionList>(currExpr);
						if (currExprList == nullptr)
						{
							// Just finish up last parameter
							builder->CreateStore(currExpr->CodeGen(symbolTable, builder, context, module, currOutputType->_thisType), &arg/*ptr (deref)*/);
							break;
						}
						else
						{
							builder->CreateStore(currExprList->_left->CodeGen(symbolTable, builder, context, module, currOutputType->_thisType), &arg /*ptr (deref)*/);
						}
					}
					else
					{
						throw UnexpectedException();
					}
				}

				// Ret has to go last
				builder->CreateRet(exprList ? 
					exprList->_left->CodeGen(symbolTable, builder, context, module, outputTypeAsComposite->_thisType) 
					: _idList->CodeGen(symbolTable, builder, context, module, outputTypeAsComposite)); // TODO: Will this work for return functions with same type?
			}
			else
			{
				auto val = _idList->CodeGen(symbolTable, builder, context, module, _returnType);
				builder->CreateRet(val);
			}
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
		auto binding = symbolTable->BindFunction(_name, dynamic_pointer_cast<FunctionDeclaration>(shared_from_this()));

		auto argumentList = _inputArgs;
		std::vector<std::shared_ptr<SymbolTable::SymbolBinding>> argBindings;
		while (argumentList != nullptr)
		{
			auto varBinding = symbolTable->BindVariable(argumentList->_argument->_name, argumentList->_argument->_typeInfo);
			argBindings.push_back(varBinding);
			argumentList = argumentList->_next;
		}

		bool doCodeGen = builder != nullptr;

		llvm::FunctionType* ft = nullptr;
		llvm::Function* function = nullptr;
		if (doCodeGen)
		{
			// Parameter type
			std::vector<llvm::Type*> argTypes;
			if (_inputArgs != nullptr)
				_inputArgs->AddIRTypesToVector(argTypes, context);

			// Return type
			llvm::Type* retType = llvm::Type::getVoidTy(*context);
			if (_returnArgs != nullptr && _returnArgs->_argument != nullptr)
			{
				retType = _returnArgs->_argument->_typeInfo->GetIRType(context);
			}
			if (_returnArgs != nullptr && _returnArgs->_next != nullptr)
			{
				// LLVM doesn't support multiple return types, so we'll treat them as out
				// paramaters
				_returnArgs->_next->AddIRTypesToVector(argTypes, context, true /*asOutput*/);
			}

			ft = llvm::FunctionType::get(retType, argTypes, false /*isVarArg*/);
			function = llvm::Function::Create(ft, llvm::Function::ExternalLinkage /*TODO*/, _name, module);
			binding->BindIRValue(function);
			auto bb = llvm::BasicBlock::Create(*context, "", function);
			builder->SetInsertPoint(bb);

			// Now store the args
			auto currArg = _inputArgs;
			int i = 0;
			for (auto &arg : function->args())
			{
				arg.setName(currArg->_argument->_name);
				if (i >= argBindings.size())
				{
					// We've reached output args, we don't need to bind them.
					currArg = currArg->_next;
					i++;
					continue;
				}
				auto binding = argBindings[i++];
				// Create an allocation for the arg
				auto alloc = binding->CreateAllocationInstance(currArg->_argument->_name, builder, context);
				builder->CreateStore(&arg, alloc);
				binding->BindIRValue(alloc);
				currArg = currArg->_next;
				if (currArg == nullptr && _returnArgs != nullptr && _returnArgs->_next != nullptr)
					currArg = _returnArgs->_next;
			}
		}

		if (_body != nullptr)
			_body->TypeCheck(symbolTable, builder, context, module);

		if (doCodeGen)
		{
			if (_returnArgs == nullptr || _returnArgs->_argument == nullptr)
				builder->CreateRetVoid(); // What if user added return statement to the end of the block already?
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