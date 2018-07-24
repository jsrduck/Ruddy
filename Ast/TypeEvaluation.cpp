#include "stdafx.h"
#include "TypeInfo.h"
#include "Statements.h"
#include "Classes.h"
#include "Primitives.h"
#include <assert.h>
using namespace std;

namespace Ast {
	void Statement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		FileLocationContext locationContext(_location);
		TypeCheckInternal(symbolTable, pass);
	}

	void Ast::ClassStatement::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		_classBinding = symbolTable->GetCurrentClass();
		Statement::TypeCheck(symbolTable, pass);
	}

	std::shared_ptr<TypeInfo> Expression::Evaluate(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList)
	{
		FileLocationContext context(_location);
		_typeInfo = EvaluateInternal(symbolTable, inInitializerList);
		return _typeInfo;
	}

	std::shared_ptr<TypeInfo> Reference::EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList)
	{
		_symbolBinding = symbolTable->Lookup(_id, inInitializerList);
		if (_symbolBinding == nullptr)
		{
			// This id isn't defined in the symbol table yet
			throw SymbolNotDefinedException(_id);
		}
		if (!(_symbolBinding->IsVariableBinding() || _symbolBinding->IsClassMemberBinding()))
		{
			// This symbol exists, but it's not a variable so it can't be used as an expression
			throw SymbolWrongTypeException(_id);
		}
		return _symbolBinding->GetTypeInfo();
	}

	std::shared_ptr<TypeInfo> ExpressionList::EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList)
	{
		auto rhs = _right->Evaluate(symbolTable, inInitializerList);
		auto rhsComposite = rhs->IsComposite() ? std::dynamic_pointer_cast<CompositeTypeInfo>(rhs) : std::make_shared<CompositeTypeInfo>(rhs, nullptr);
		auto lhs = _left->Evaluate(symbolTable, inInitializerList);
		if (lhs->IsComposite())
		{
			auto lhsAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(lhs);
			auto typeInfoToReturn = CompositeTypeInfo::Clone(lhsAsComposite);
			auto typeInfoIter = typeInfoToReturn;
			while (typeInfoIter->_next != nullptr)
			{
				typeInfoIter = typeInfoIter->_next;
			}
			typeInfoIter->_next = rhsComposite;
			return typeInfoToReturn;
		}
		else
		{
			return std::make_shared<CompositeTypeInfo>(_left->Evaluate(symbolTable, inInitializerList), rhsComposite);
		}
	}

	std::shared_ptr<TypeInfo> NewExpression::EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList)
	{
		auto symbol = symbolTable->Lookup(_className, inInitializerList);
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

		// Check the c'tors for type match, find the right one
		auto argsExpression = _expression ? _expression->Evaluate(symbolTable) : nullptr;
		auto classBinding = dynamic_pointer_cast<SymbolTable::ClassBinding>(symbol);
		for (auto constructorSymbol : classBinding->_ctors)
		{
			auto functionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(constructorSymbol->GetTypeInfo());
			if (functionTypeInfo == nullptr)
				throw UnexpectedException();
			if ((argsExpression == nullptr && functionTypeInfo->InputArgsType() != nullptr) ||
				(argsExpression != nullptr && functionTypeInfo->InputArgsType() == nullptr))
			{
				continue;
			}
			else if (functionTypeInfo->InputArgsType() != nullptr &&
				!functionTypeInfo->InputArgsType()->IsImplicitlyAssignableFrom(argsExpression, symbolTable))
			{
				continue;
			}
			else
			{
				// Found it!
				return std::make_shared<ClassTypeInfo>(symbol->GetTypeInfo(), false /*valueType*/);
			}
		}
		throw NoMatchingFunctionSignatureFoundException(argsExpression);
	}

	std::shared_ptr<TypeInfo> StackConstructionExpression::EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList)
	{
		_symbolBinding = symbolTable->Lookup(_className, inInitializerList);
		if (_symbolBinding == nullptr)
		{
			// This id isn't defined in the symbol table yet
			throw SymbolNotDefinedException(_className);
		}
		if (!_symbolBinding->IsClassBinding())
		{
			// This symbol exists, but it's not the name of a class
			throw SymbolWrongTypeException(_symbolBinding->GetFullyQualifiedName());
		}

		// Check the c'tors for type match, find the right one
		auto argsExpressionTypeInfo = _argumentExpression ? _argumentExpression->Evaluate(symbolTable, inInitializerList) : nullptr;
		auto classBinding = dynamic_pointer_cast<SymbolTable::ClassBinding>(_symbolBinding);
		for (auto constructorSymbol : classBinding->_ctors)
		{
			auto functionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(constructorSymbol->GetTypeInfo());
			if (functionTypeInfo == nullptr)
				throw UnexpectedException();
			if ((argsExpressionTypeInfo == nullptr && functionTypeInfo->InputArgsType() != nullptr) ||
				(argsExpressionTypeInfo != nullptr && functionTypeInfo->InputArgsType() == nullptr))
			{
				continue;
			}
			else if (functionTypeInfo->InputArgsType() != nullptr &&
				!functionTypeInfo->InputArgsType()->IsImplicitlyAssignableFrom(argsExpressionTypeInfo, symbolTable))
			{
				continue;
			}
			else
			{
				// Found it!
				auto classTypeInfo = std::make_shared<ClassTypeInfo>(_symbolBinding->GetTypeInfo(), true /*valueType*/);

				if (_varBinding == nullptr)
				{
					// Bind the variable we're assigning it to
					_varBinding = symbolTable->BindVariable(_varName, classTypeInfo);
				}

				_ctorCall = std::make_shared<FunctionCall>(functionTypeInfo, _argumentExpression, constructorSymbol, _varBinding, _location);
				_ctorCall->Evaluate(symbolTable, inInitializerList /*????*/);

				return _varBinding->GetTypeInfo();
			}
		}

		throw NoMatchingFunctionSignatureFoundException(argsExpressionTypeInfo);
	}

	std::shared_ptr<TypeInfo> FunctionCall::EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList)
	{
		if (_functionTypeInfo == nullptr)
		{
			_symbolBinding = symbolTable->Lookup(_name, inInitializerList);
			if (_symbolBinding == nullptr)
			{
				// This id isn't defined in the symbol table yet
				throw SymbolNotDefinedException(_name);
			}
			else if (!_symbolBinding->IsFunctionBinding())
			{
				// This symbol exists, but it's not the name of a function
				throw SymbolWrongTypeException(_symbolBinding->GetFullyQualifiedName());
			}

			auto functionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(_symbolBinding->GetTypeInfo());
			if (functionTypeInfo == nullptr)
				throw UnexpectedException();
			_functionTypeInfo = functionTypeInfo;

			if (!_functionTypeInfo->_mods->IsStatic())
			{
				auto functionInstanceBinding = std::dynamic_pointer_cast<Ast::SymbolTable::FunctionInstanceBinding>(_symbolBinding);
				_varBinding = functionInstanceBinding->_reference;
			}
		}

		//_classBinding = symbolTable->GetCurrentClass();
		_argsExprTypeInfo = _expression ? _expression->Evaluate(symbolTable) : nullptr;
		if ((_argsExprTypeInfo == nullptr && _functionTypeInfo->InputArgsType() != nullptr) ||
			(_argsExprTypeInfo != nullptr && _functionTypeInfo->InputArgsType() == nullptr))
		{
			throw TypeMismatchException(_functionTypeInfo->InputArgsType(), _argsExprTypeInfo);
		}
		else if (_functionTypeInfo->InputArgsType() != nullptr &&
				!_functionTypeInfo->InputArgsType()->IsImplicitlyAssignableFrom(_argsExprTypeInfo, symbolTable))
		{
			throw TypeMismatchException(_functionTypeInfo->InputArgsType(), _argsExprTypeInfo);
		}
		return _functionTypeInfo->OutputArgsType();
	}

	std::shared_ptr<TypeInfo> DebugPrintStatement::EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList)
	{
		_expressionTypeInfo = _expression->Evaluate(symbolTable, inInitializerList);
		return nullptr;
	}

	void LineStatements::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		_statement->TypeCheck(symbolTable, pass);
		if (_next != nullptr)
			_next->TypeCheck(symbolTable, pass);
	}

	void IfStatement::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		assert(pass == METHOD_BODIES);
		// Type check the condition
		auto conditionType = _condition->Evaluate(symbolTable);
		if (!BoolTypeInfo::Get()->IsImplicitlyAssignableFrom(conditionType, symbolTable))
		{
			throw TypeMismatchException(BoolTypeInfo::Get(), conditionType);
		}

		symbolTable->Enter();
		_statement->TypeCheck(symbolTable, pass);
		_ifStatementEndScopeDtors = symbolTable->Exit();
		for (auto& dtor : _ifStatementEndScopeDtors)
			dtor->Evaluate(symbolTable);

		if (_elseStatement != nullptr)
		{
			symbolTable->Enter();
			_elseStatement->TypeCheck(symbolTable, pass);
			_elseStatementEndScopeDtors = symbolTable->Exit();
			for (auto& dtor : _elseStatementEndScopeDtors)
				dtor->Evaluate(symbolTable);
		}
	}

	void WhileStatement::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		assert(pass == METHOD_BODIES);
		// TODO: More static analysis. Infinite loops? Unreachable code?
		symbolTable->Enter();
		_currentLoopBinding = symbolTable->BindLoop();
		auto conditionType = _condition->Evaluate(symbolTable);
		if (!BoolTypeInfo::Get()->IsImplicitlyAssignableFrom(conditionType, symbolTable))
		{
			throw TypeMismatchException(BoolTypeInfo::Get(), conditionType);
		}

		_statement->TypeCheck(symbolTable, pass);
		_endScopeDtors = symbolTable->Exit();
		for (auto& dtor : _endScopeDtors)
			dtor->Evaluate(symbolTable);
	}

	void BreakStatement::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		assert(pass == METHOD_BODIES);
		_currentLoopBinding = symbolTable->GetCurrentLoop();
		_endScopeDtors = symbolTable->BreakFromCurrentLoop();
		if (_currentLoopBinding == nullptr)
		{
			throw BreakInWrongPlaceException();
		}
	}

	std::shared_ptr<TypeInfo> AssignFrom::Resolve(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhsTypeInfo, std::shared_ptr<Expression> rhsExpr)
	{
		FileLocationContext locationContext(_location);
		auto rhsTypeInfoAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(rhsTypeInfo);
		auto thisRhs = rhsTypeInfoAsComposite != nullptr ? rhsTypeInfoAsComposite->_thisType : rhsTypeInfo;
		auto rhsExprAsList = std::dynamic_pointer_cast<ExpressionList>(rhsExpr);
		auto thisRhsExpr = rhsExprAsList != nullptr ? rhsExprAsList->_left : rhsExpr;
		_thisType = _thisOne->Resolve(symbolTable, thisRhs, thisRhsExpr);
		if (_next == nullptr)
			return _thisType;
		if (rhsTypeInfoAsComposite == nullptr)
			throw UnexpectedException();
		_nextType = _next->Resolve(symbolTable, rhsTypeInfoAsComposite->_next, rhsExprAsList ? rhsExprAsList->_right : rhsExpr);
		auto nextAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(_nextType);
		if (nextAsComposite == nullptr)
			nextAsComposite = std::make_shared<CompositeTypeInfo>(_nextType);
		return std::make_shared<CompositeTypeInfo>(_thisType, nextAsComposite);
	}

	void AssignFrom::Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs)
	{
		FileLocationContext locationContext(_location);
		if (_next == nullptr)
		{
			auto rhsAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(rhs);
			if (rhsAsComposite != nullptr)
			{
				// It better be a single type
				if (rhsAsComposite->_next != nullptr)
					throw TypeMismatchException(_thisType, rhs);
				_thisOne->Bind(symbolTable, rhsAsComposite->_thisType);
			}
			else
			{
				_thisOne->Bind(symbolTable, rhs);
			}
		}
		else
		{
			auto rhsAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(rhs);
			if (rhsAsComposite == nullptr)
			{
				auto nextAsComp = std::dynamic_pointer_cast<CompositeTypeInfo>(_nextType);
				if (nextAsComp == nullptr)
					nextAsComp = std::make_shared<CompositeTypeInfo>(_nextType);
				throw TypeMismatchException(std::make_shared<CompositeTypeInfo>(_thisType, nextAsComp), rhs);
			}
			_thisOne->Bind(symbolTable, rhsAsComposite->_thisType);
			_next->Bind(symbolTable, rhsAsComposite->_next);
		}
	}

	std::shared_ptr<TypeInfo> AssignFromReference::Resolve(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhsTypeInfo, std::shared_ptr<Expression> rhsExpr)
	{
		FileLocationContext locationContext(_location);
		_symbolBinding = symbolTable->Lookup(_ref);
		if (_symbolBinding == nullptr)
		{
			throw SymbolNotDefinedException(_ref);
		}
		if (!_symbolBinding->IsClassMemberBinding() && !_symbolBinding->IsVariableBinding())
		{
			throw SymbolWrongTypeException(_ref);
		}

		return _symbolBinding->GetTypeInfo();
	}

	std::shared_ptr<TypeInfo> DeclareVariable::Resolve(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhsTypeInfo, std::shared_ptr<Expression> rhsExpr)
	{
		FileLocationContext locationContext(_location);
		if (_typeInfo->NeedsResolution())
		{
			auto symbol = symbolTable->Lookup(_typeInfo->Name());
			if (symbol == nullptr)
				throw SymbolNotDefinedException(_typeInfo->Name());
			if (!symbol->IsClassBinding())
				throw SymbolWrongTypeException(symbol->GetFullyQualifiedName());
			auto asClassType = std::dynamic_pointer_cast<BaseClassTypeInfo>(_typeInfo);
			if (asClassType == nullptr)
			{
				throw UnexpectedException();
			}
			_typeInfo = std::make_shared<ClassTypeInfo>(symbol->GetTypeInfo(), asClassType->IsValueType());
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
		FileLocationContext locationContext(_location);
		if (!_typeInfo->IsImplicitlyAssignableFrom(rhs, symbolTable))
		{
			throw TypeMismatchException(_typeInfo, rhs);
		}
		// TODO: Unit test cases where rhs is constant
		_symbolBinding = symbolTable->BindVariable(_name, _typeInfo->IsAutoType() ? rhs : _typeInfo);
	}

	void Assignment::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		assert(pass == METHOD_BODIES);
		_rhsTypeInfo = _rhs->Evaluate(symbolTable, _inInitializerList);
		_lhsTypeInfo = _lhs->Resolve(symbolTable, _rhsTypeInfo, _rhs);

		if (!_lhsTypeInfo->IsImplicitlyAssignableFrom(_rhsTypeInfo, symbolTable))
		{
			throw TypeMismatchException(_lhsTypeInfo, _rhsTypeInfo);
		}
		_lhs->Bind(symbolTable, _rhsTypeInfo);
	}

	void ReturnStatement::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		assert(pass == METHOD_BODIES);
		// TODO: Catch unreachable code after return;
		_returnType = _idList->Evaluate(symbolTable);
		_functionBinding = symbolTable->GetCurrentFunction();
		if (_functionBinding == nullptr)
		{
			throw ReturnStatementMustBeDeclaredInFunctionScopeException();
		}
		auto functionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(_functionBinding->GetTypeInfo());
		if (functionTypeInfo == nullptr)
			throw UnexpectedException();
		if (!functionTypeInfo->OutputArgsType()->IsImplicitlyAssignableFrom(_returnType, symbolTable))
			throw TypeMismatchException(functionTypeInfo->OutputArgsType(), _returnType);
		_endScopeDtors = symbolTable->ReturnFromCurrentFunction();
	}

	void GlobalStatements::TypeCheck(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		if (pass == TYPE_CHECK_ALL)
		{
			for (auto currPass = static_cast<int>(TYPE_CHECK_START); currPass <= static_cast<int>(TYPE_CHECK_END); currPass++)
				GlobalStatement::TypeCheck(symbolTable, static_cast<TypeCheckPass>(currPass));
		}
		else
		{
			GlobalStatement::TypeCheck(symbolTable, pass);
		}
	}

	void GlobalStatements::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		_stmt->TypeCheck(symbolTable, pass);
		if (_next != nullptr)
			_next->TypeCheck(symbolTable, pass);
	}

	void NamespaceDeclaration::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		symbolTable->Enter();
		symbolTable->BindNamespace(_name, pass);
		_stmts->TypeCheck(symbolTable, pass);
		symbolTable->Exit();
	}

	void ScopedStatements::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		assert(pass == METHOD_BODIES);
		symbolTable->Enter();
		if (_statements != nullptr)
			_statements->TypeCheck(symbolTable, pass);
		_endScopeDtors = symbolTable->Exit();
		for (auto& dtor : _endScopeDtors)
			dtor->Evaluate(symbolTable);
	}

	void ExpressionAsStatement::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		assert(pass == METHOD_BODIES);
		_expr->Evaluate(symbolTable);
	}

	void ClassDeclaration::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		symbolTable->Enter();
		_classBinding = symbolTable->BindClass(_name, dynamic_pointer_cast<ClassDeclaration>(shared_from_this()), pass);

		if (pass >= METHOD_DECLARATIONS)
		{
			if (_list != nullptr)
				_list->TypeCheck(symbolTable, pass);

			if (pass == METHOD_DECLARATIONS)
			{
				// If no constructors are defined, define the default c'tor for them
				if (_classBinding->_ctors.size() == 0)
				{
					auto ctor = std::make_shared<ConstructorDeclaration>(Visibility::PUBLIC, _name, nullptr, nullptr, nullptr, FileLocationContext::CurrentLocation());
					ctor->TypeCheck(symbolTable, pass);
					// Add it to the beginning of the statement list so next phase of type checking finds it
					_list = std::make_shared<ClassStatementList>(ctor, _list, FileLocationContext::CurrentLocation());
				}

				// If the d'tor isn't defined, define an implicit one
				if (_classBinding->_dtor == nullptr)
				{
					auto dtor = std::make_shared<DestructorDeclaration>(_name, nullptr, FileLocationContext::CurrentLocation());
					dtor->TypeCheck(symbolTable, pass);
					// Add it to the beginning of the statement list so next phase of type checking finds it
					_list = std::make_shared<ClassStatementList>(dtor, _list, FileLocationContext::CurrentLocation());
				}
			}
		}

		symbolTable->Exit();
	}

	void ClassMemberDeclaration::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		if (pass >= CLASS_VARIABLES)
		{
			if (pass == CLASS_VARIABLES)
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
			}

			auto binding = symbolTable->BindMemberVariable(_name, dynamic_pointer_cast<ClassMemberDeclaration>(shared_from_this()), pass);
		}
	}

	void Initializer::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		assert(pass == METHOD_BODIES);
		// Find the member variable being constructed
		if (!_stackAssignment)
		{
			auto symbol = symbolTable->Lookup(_name);
			if (symbol == nullptr)
			{
				throw SymbolNotDefinedException(_name);
			}
			else if (!symbol->IsClassMemberBinding())
			{
				throw SymbolWrongTypeException(_name);
			}
			_memberBinding = std::dynamic_pointer_cast<Ast::SymbolTable::MemberInstanceBinding>(symbol);
			if (_memberBinding == nullptr)
				throw UnexpectedException();

			auto typeInfo = _memberBinding->GetTypeInfo();
			if (!typeInfo->IsClassType())
			{
				throw ExpectedValueTypeException(_memberBinding->GetName());
			}
			auto asClassType = std::dynamic_pointer_cast<BaseClassTypeInfo>(typeInfo);
			if (asClassType == nullptr)
			{
				throw UnexpectedException();
			}
			if (!asClassType->IsValueType())
			{
				throw ExpectedValueTypeException(_name);
			}
			_stackAssignment = std::make_shared<StackConstructionExpression>(_memberBinding, _expr, FileLocationContext::CurrentLocation());
			symbolTable->BindInitializer(_name, _stackAssignment);
		}
		_stackAssignment->Evaluate(symbolTable, true /*inInitializerList*/);
	}

	void InitializerStatement::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		assert(pass == METHOD_BODIES);
		auto list = _list;
		while (list != nullptr)
		{
			// Find the member variable being constructed
			list->_thisInitializer->TypeCheck(symbolTable, pass);
			list = list->_next;
		}
	}

	// TODO: Static analysis to make sure all code paths return a value (for non void return types)
	// TODO: Static analysis to make sure there is no "unreachable" code (maybe that should be on "linestatements?"
	void FunctionDeclaration::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		if (pass == METHOD_DECLARATIONS || pass >= METHOD_BODIES)
		{
			symbolTable->Enter();
			_functionBinding = symbolTable->BindFunction(Name(), dynamic_pointer_cast<FunctionDeclaration>(shared_from_this()), pass);

			if (pass == METHOD_BODIES)
			{
				TypeCheckArgumentList(_functionBinding, symbolTable);

				if (_body != nullptr)
					_body->TypeCheck(symbolTable, pass);
			}

			_endScopeDtors = symbolTable->Exit();

			if (pass == METHOD_BODIES && _body != nullptr)
			{
				for (auto& dtor : _endScopeDtors)
					dtor->Evaluate(symbolTable);
			}
		}
	}
	
	void FunctionDeclaration::TypeCheckArgumentList(std::shared_ptr<Ast::SymbolTable::FunctionBinding> binding, std::shared_ptr<SymbolTable> symbolTable)
	{
		// If this is a method, bind the "this" variable
		if (!_mods->IsStatic())
		{
			_thisPtrBinding = symbolTable->BindVariable("this", _classBinding->GetTypeInfo());
		}

		auto argumentList = _inputArgs;
		while (argumentList != nullptr)
		{
			auto varBinding = symbolTable->BindVariable(argumentList->_argument->_name, argumentList->_argument->_typeInfo);
			if (argumentList->_argument->_typeInfo->NeedsResolution())
				std::dynamic_pointer_cast<UnresolvedClassTypeInfo>(argumentList->_argument->_typeInfo)->EnsureResolved(symbolTable);
			_argBindings.push_back(varBinding);
			argumentList = argumentList->_next;
		}
	}

	void ConstructorDeclaration::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		if (pass == METHOD_DECLARATIONS || pass >= METHOD_BODIES)
		{
			symbolTable->Enter();
			auto ctorBinding = symbolTable->BindConstructor(dynamic_pointer_cast<FunctionDeclaration>(shared_from_this()), pass);

			if (pass == METHOD_DECLARATIONS)
			{
				_functionBinding = ctorBinding;
			}

			if (pass == METHOD_BODIES)
			{
				TypeCheckArgumentList(ctorBinding, symbolTable);
			}

			if (pass == METHOD_BODIES)
			{
				// Once we enter the c'tor scope, the initializers are the first thing to get done
				if (_initializerStatement != nullptr)
					_initializerStatement->TypeCheck(symbolTable, pass);

				// Are there any uninitialized value-types? Go ahead and initialize them, if we can
				auto classBinding = symbolTable->GetCurrentClass();
				int index = 0;
				for (auto& memberBinding : classBinding->_members)
				{
					auto memberType = memberBinding->GetTypeInfo();
					if (memberType->IsClassType())
					{
						auto asClassType = std::dynamic_pointer_cast<BaseClassTypeInfo>(memberType);
						if (asClassType == nullptr)
						{
							throw UnexpectedException();
						}
						if (asClassType->IsValueType() &&
							ctorBinding->_initializers.count(memberBinding->_memberDeclaration->_name) == 0)
						{
							// Try to find a default c'tor and add it
							auto initer = std::make_shared<Initializer>(memberBinding->_memberDeclaration->_name, nullptr /*no args*/, FileLocationContext::CurrentLocation());
							try
							{
								initer->TypeCheck(symbolTable, pass);
								// Add it to the initializer statement so it gets code gen'd
								if (_initializerStatement == nullptr)
									_initializerStatement = std::make_shared<InitializerStatement>(nullptr, FileLocationContext::CurrentLocation());
								_initializerStatement->_list = std::make_shared<InitializerList>(initer, _initializerStatement->_list);
							}
							catch (NoMatchingFunctionSignatureFoundException&)
							{
								throw ValueTypeMustBeInitializedException(memberBinding->_memberDeclaration->_name);
							}
						}
					}
				}

				// Now do the body of the c'tor
				if (_body != nullptr)
					_body->TypeCheck(symbolTable, pass);
			}

			_endScopeDtors = symbolTable->Exit();
		}
	}

	void DestructorDeclaration::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		if (pass == METHOD_DECLARATIONS || pass >= METHOD_BODIES)
		{
			FunctionDeclaration::TypeCheckInternal(symbolTable, pass);

			if (pass == METHOD_DECLARATIONS)
				_classBinding->_dtor = std::dynamic_pointer_cast<DestructorDeclaration>(shared_from_this()); // TODO: This should be done through AddDestructorBinding method

			if (pass == METHOD_BODIES)
			{
				// Also add a call to the d'tor of each stack-allocated member variable to the d'tor of this type
				// We destroy variables in the reverse order that they are declared
				for (auto& iter = _classBinding->_members.rbegin(); iter != _classBinding->_members.rend(); ++iter)
				{
					auto memberBinding = *iter;
					auto typeInfo = memberBinding->GetTypeInfo();
					if (typeInfo->IsClassType())
					{
						auto asClassTypeInfo = std::dynamic_pointer_cast<BaseClassTypeInfo>(typeInfo);
						if (asClassTypeInfo == nullptr)
							throw UnexpectedException();
						if (asClassTypeInfo->IsValueType())
						{
							auto memberClassBinding = std::dynamic_pointer_cast<Ast::SymbolTable::ClassBinding>(symbolTable->Lookup(asClassTypeInfo->FullyQualifiedName(symbolTable)));
							if (memberClassBinding == nullptr)
								throw UnexpectedException();
							auto instanceBinding = std::make_shared<Ast::SymbolTable::MemberInstanceBinding>(memberBinding, _thisPtrBinding);
							AppendDtor(memberClassBinding->_dtor->CreateCall(instanceBinding, FileLocationContext::CurrentLocation()));
						}
					}
				}
			}
		}
	}

	void DestructorDeclaration::AppendDtor(std::shared_ptr<FunctionCall> dtor)
	{
		if (dtor != nullptr)
			_endScopeDtors.push_back(dtor);
	}

	void ClassStatementList::TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass)
	{
		_statement->TypeCheck(symbolTable, pass);
		if (_next != nullptr)
			_next->TypeCheck(symbolTable, pass);
	}
}