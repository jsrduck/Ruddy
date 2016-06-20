#include "stdafx.h"
#include "TypeInfo.h"
#include "Statements.h"
#include "Classes.h"
#include "Primitives.h"

#include <assert.h>
using namespace std;

namespace Ast {

	std::shared_ptr<TypeInfo> Reference::Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto symbol = symbolTable->Lookup(_id);
		if (symbol == nullptr)
		{
			// This id isn't defined in the symbol table yet
			throw SymbolNotDefinedException(_id);
		}
		if (!symbol->IsVariableBinding())
		{
			// This symbol exists, but it's not a variable so it can't be used as an expression
			throw SymbolWrongTypeException(_id);
		}
		return symbol->GetTypeInfo();
	}

	std::shared_ptr<TypeInfo> ExpressionList::Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	{
		return std::make_shared<CompositeTypeInfo>(_left->Evaluate(symbolTable), std::make_shared<CompositeTypeInfo>(_right->Evaluate(symbolTable), nullptr));
	}

	std::shared_ptr<TypeInfo> NewExpression::Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto symbol = symbolTable->Lookup(_id->Id());
		if (symbol == nullptr)
		{
			// This id isn't defined in the symbol table yet
			throw SymbolNotDefinedException(_id->Id());
		}
		if (!symbol->IsClassBinding())
		{
			// This symbol exists, but it's not the name of a class
			throw SymbolWrongTypeException(_id->Id());
		}
		return symbol->GetTypeInfo();
	}

	std::shared_ptr<TypeInfo> FunctionCall::Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto symbol = symbolTable->Lookup(_id->Id());
		if (symbol == nullptr)
		{
			// This id isn't defined in the symbol table yet
			throw SymbolNotDefinedException(_id->Id());
		}
		if (!symbol->IsFunctionBinding())
		{
			// This symbol exists, but it's not the name of a function
			throw SymbolWrongTypeException(_id->Id());
		}
		auto functionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(symbol->GetTypeInfo());
		if (functionTypeInfo == nullptr)
			throw UnexpectedException();
		return functionTypeInfo->OutputArgsType();
	}

	std::shared_ptr<TypeInfo> DebugPrintStatement::Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	{
		return nullptr;
	}

	void LineStatements::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		_statement->TypeCheck(symbolTable);
		if (_next != nullptr)
			_next->TypeCheck(symbolTable);
	}

	void IfStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto conditionType = _condition->Evaluate(symbolTable);
		if (!BoolTypeInfo::Get()->IsImplicitlyAssignableFrom(conditionType, symbolTable))
		{
			throw TypeMismatchException(BoolTypeInfo::Get(), conditionType);
		}
		symbolTable->Enter();
		_statement->TypeCheck(symbolTable);
		symbolTable->Exit();
		if (_elseStatement != nullptr)
		{
			symbolTable->Enter();
			_elseStatement->TypeCheck(symbolTable);
			symbolTable->Exit();
		}
	}

	void WhileStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		// TODO: More static analysis. Infinite loops? Unreachable code?
		symbolTable->Enter();
		symbolTable->BindLoop();
		auto conditionType = _condition->Evaluate(symbolTable);
		if (!BoolTypeInfo::Get()->IsImplicitlyAssignableFrom(conditionType, symbolTable))
		{
			throw TypeMismatchException(BoolTypeInfo::Get(), conditionType);
		}
		_statement->TypeCheck(symbolTable);
		symbolTable->Exit();
	}

	void BreakStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		if (symbolTable->GetCurrentLoop() == nullptr)
		{
			throw BreakInWrongPlaceException();
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
		symbolTable->BindVariable(_name,_typeInfo->IsAutoType() ? rhs : _typeInfo);
	}

	void Assignment::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto typeInfo = _lhs->Resolve(symbolTable);

		auto rhsType = _rhs->Evaluate(symbolTable);
		if (!typeInfo->IsImplicitlyAssignableFrom(rhsType, symbolTable))
		{
			throw TypeMismatchException(typeInfo, rhsType);
		}
		_lhs->Bind(symbolTable, rhsType);
	}

	void ReturnStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto returnType = _idList->Evaluate(symbolTable);
		auto functionSymbol = symbolTable->GetCurrentFunction();
		if (symbolTable->GetCurrentFunction() == nullptr)
		{
			throw ReturnStatementMustBeDeclaredInFunctionScopeException();
		}
		auto functionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(functionSymbol->GetTypeInfo());
		if (functionTypeInfo == nullptr)
			throw UnexpectedException();
		if (!functionTypeInfo->OutputArgsType()->IsImplicitlyAssignableFrom(returnType, symbolTable))
			throw TypeMismatchException(functionTypeInfo->OutputArgsType(), returnType);
	}

	void GlobalStatements::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		_stmt->TypeCheck(symbolTable);
		if (_next != nullptr)
			_next->TypeCheck(symbolTable);
	}

	void NamespaceDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		symbolTable->Enter();
		symbolTable->BindNamespace(_name);
		_stmts->TypeCheck(symbolTable);
		symbolTable->Exit();
	}

	void ScopedStatements::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		symbolTable->Enter();
		_statements->TypeCheck(symbolTable);
		symbolTable->Exit();
	}

	void ExpressionAsStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		_expr->Evaluate(symbolTable);
	}

	void ClassDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		symbolTable->Enter();
		symbolTable->BindClass(_name, dynamic_pointer_cast<ClassDeclaration>(shared_from_this()));
		if (_list != nullptr)
			_list->TypeCheck(symbolTable);
		symbolTable->Exit();
	}

	void ClassMemberDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
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
	}

	// TODO: Static analysis to make sure all code paths return a value (for non void return types)
	// TODO: Static analysis to make sure there is no "unreachable" code (maybe that should be on "linestatements?"
	void FunctionDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		symbolTable->Enter();
		symbolTable->BindFunction(_name, dynamic_pointer_cast<FunctionDeclaration>(shared_from_this()));
		auto argumentList = _inputArgs;
		while (argumentList != nullptr)
		{
			symbolTable->BindVariable(argumentList->_argument->_name, argumentList->_argument->_typeInfo);
			argumentList = argumentList->_next;
		}

		if (_body != nullptr)
			_body->TypeCheck(symbolTable);
		symbolTable->Exit();
	}

	void ConstructorDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		symbolTable->Enter();
		symbolTable->BindFunction(_name, dynamic_pointer_cast<FunctionDeclaration>(shared_from_this()));
		auto argumentList = _inputArgs;
		while (argumentList != nullptr)
		{
			symbolTable->BindVariable(argumentList->_argument->_name, argumentList->_argument->_typeInfo);
			argumentList = argumentList->_next;
		}

		if (_body != nullptr)
			_body->TypeCheck(symbolTable);
		symbolTable->Exit();
	}

	void DestructorDeclaration::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		symbolTable->Enter();
		symbolTable->BindFunction("~" + _name, dynamic_pointer_cast<FunctionDeclaration>(shared_from_this()));
		if (_body != nullptr)
			_body->TypeCheck(symbolTable);
		symbolTable->Exit();
	}

	void ClassStatementList::TypeCheck(std::shared_ptr<SymbolTable> symbolTable)
	{
		_statement->TypeCheck(symbolTable);
		if (_next != nullptr)
			_next->TypeCheck(symbolTable);
	}
}