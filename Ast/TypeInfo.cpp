#include "stdafx.h"
#include "TypeInfo.h"
#include "Primitives.h"
#include "Operations.h"
#include "TypeExceptions.h"
#include "Classes.h"
#include "Statements.h"

#include <assert.h>
using namespace std;

namespace Ast
{
	TypeMismatchException::TypeMismatchException(std::shared_ptr<TypeInfo> expected, std::shared_ptr<TypeInfo> actual)
	{
		error = std::string("Type MismatchException: expected type \"");
		error.append(expected ? expected->Name() : "void");
		error.append("\" found type \"");
		error.append(actual ? actual->Name() : "void");
		error.append("\"");
	}

	TypeAlreadyExistsException::TypeAlreadyExistsException(std::shared_ptr<TypeInfo> type)
	{
		error = std::string("Type doubly defined, already exists: \"");
		error.append(type->Name());
		error.append("\"");
	}

	std::shared_ptr<TypeInfo> TypeInfo::EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs, std::shared_ptr<SymbolTable> symbolTable)
	{
		if (operation->IsBinary())
		{
			assert(rhs);
			if (rhs == nullptr)
			{
				throw OperationNotDefinedException(operation->OperatorString());
			}

			if (operation->IsArithmetic())
			{
				if (rhs->IsImplicitlyAssignableFrom(shared_from_this(), symbolTable))
				{
					if (rhs->SupportsOperator(operation))
					{
						implicitCastTypeOut = rhs;
						return rhs;
					}
					else if (rhs->IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
					{
						return implicitCastTypeOut;
					}
					else throw OperationNotDefinedException(operation->OperatorString());
				}
				else if (IsImplicitlyAssignableFrom(rhs, symbolTable))
				{
					if (SupportsOperator(operation))
					{
						implicitCastTypeOut = shared_from_this();
						return shared_from_this();
					}
					else if (IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
					{
						return implicitCastTypeOut;
					}
					else throw OperationNotDefinedException(operation->OperatorString());
				}
				else
					throw OperationNotDefinedException(operation->OperatorString());
			}
			else if (operation->IsLogical())
			{
				if (operation->IsComparison())
				{
					assert(!operation->IsBoolean());
					assert(!operation->IsBitwise());
					if (rhs->IsImplicitlyAssignableFrom(shared_from_this(), symbolTable))
					{
						if (rhs->SupportsOperator(operation))
						{
							implicitCastTypeOut = rhs;
							return BoolTypeInfo::Get();
						}
						else if (rhs->IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
						{
							return BoolTypeInfo::Get();
						}
						else throw OperationNotDefinedException(operation->OperatorString());
					}
					else if (IsImplicitlyAssignableFrom(rhs, symbolTable))
					{
						if (SupportsOperator(operation))
						{
							implicitCastTypeOut = shared_from_this();
							return BoolTypeInfo::Get();
						}
						else if (IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
						{
							return BoolTypeInfo::Get();
						}
						else throw OperationNotDefinedException(operation->OperatorString());
					}
					else
						throw OperationNotDefinedException(operation->OperatorString());
				}
				else if (operation->IsBoolean())
				{
					assert(!operation->IsBitwise());
					if (BoolTypeInfo::Get()->IsImplicitlyAssignableFrom(rhs, symbolTable) && BoolTypeInfo::Get()->IsImplicitlyAssignableFrom(shared_from_this(), symbolTable))
					{
						if (BoolTypeInfo::Get()->SupportsOperator(operation))
						{
							implicitCastTypeOut = BoolTypeInfo::Get();
							return BoolTypeInfo::Get();
						}
						else throw OperationNotDefinedException(operation->OperatorString());
					}
					else throw OperationNotDefinedException(operation->OperatorString());
				}
				else
				{
					assert(operation->IsBitwise());
					if (operation->IsShift())
					{
						// Shift operators
						if (Int32TypeInfo::Get()->IsImplicitlyAssignableFrom(rhs, symbolTable) &&
							(Int32TypeInfo::Get()->IsImplicitlyAssignableFrom(shared_from_this(), symbolTable) ||
							Int64TypeInfo::Get()->IsImplicitlyAssignableFrom(shared_from_this(), symbolTable) ||
							UInt32TypeInfo::Get()->IsImplicitlyAssignableFrom(shared_from_this(), symbolTable) ||
							UInt64TypeInfo::Get()->IsImplicitlyAssignableFrom(shared_from_this(), symbolTable)))
						{
							if (SupportsOperator(operation))
							{
								implicitCastTypeOut = shared_from_this();
								return shared_from_this();
							}
							else if (IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
							{
								return implicitCastTypeOut;
							}
							else throw OperationNotDefinedException(operation->OperatorString());
						}
						else
						{
							throw OperationNotDefinedException(operation->OperatorString());
						}
					}
					else
					{
						// Bitwise AND, OR and XOR
						if (BoolTypeInfo::Get()->IsImplicitlyAssignableFrom(shared_from_this(), symbolTable))
						{
							throw OperationNotDefinedException(operation->OperatorString()); // To avoid hard-to-find bugs
						}
						else if (rhs->IsImplicitlyAssignableFrom(shared_from_this(), symbolTable))
						{
							if (rhs->SupportsOperator(operation))
							{
								implicitCastTypeOut = rhs;
								return rhs;
							}
							else if (rhs->IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
							{
								return implicitCastTypeOut;
							}
							else throw OperationNotDefinedException(operation->OperatorString());
						}
						else if (IsImplicitlyAssignableFrom(rhs, symbolTable))
						{
							if (SupportsOperator(operation))
							{
								implicitCastTypeOut = shared_from_this();
								return shared_from_this();
							}
							else if (IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
							{
								return implicitCastTypeOut;
							}
							else throw OperationNotDefinedException(operation->OperatorString());
						}
						else throw OperationNotDefinedException(operation->OperatorString());
					}
				}
			}
		}
		else /* Unary */
		{
			assert(!rhs);
			if (rhs != nullptr)
			{
				throw OperationNotDefinedException(operation->OperatorString());
			}

			if (operation->IsArithmetic())
			{
				if (SupportsOperator(operation))
				{
					implicitCastTypeOut = shared_from_this();
					return shared_from_this();
				}
				else if (IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
				{
					return implicitCastTypeOut;
				}
				else throw OperationNotDefinedException(operation->OperatorString());
			}
			else if (operation->IsLogical())
			{
				if (operation->IsBoolean())
				{
					if (shared_from_this() != BoolTypeInfo::Get())
					{
						throw OperationNotDefinedException(operation->OperatorString());
					}
					else if (BoolTypeInfo::Get()->SupportsOperator(operation))
					{
						implicitCastTypeOut = BoolTypeInfo::Get();
						return BoolTypeInfo::Get();
					}
					else if (IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
					{
						return implicitCastTypeOut;
					}
					else throw OperationNotDefinedException(operation->OperatorString());
				}
				else
				{
					assert(operation->IsBitwise());
					if (SupportsOperator(operation))
					{
						implicitCastTypeOut = shared_from_this();
						return shared_from_this();
					}
					else if (IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
					{
						return implicitCastTypeOut;
					}
					else throw OperationNotDefinedException(operation->OperatorString());
				}
			}
		}
		throw UnexpectedException();
	}

	ClassTypeInfo::ClassTypeInfo(std::shared_ptr<ClassDeclaration> classDeclaration) : _name(classDeclaration->_name)
	{
	}

	bool ClassTypeInfo::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable)
	{
		auto otherAsUnresolvedClassType = std::dynamic_pointer_cast<UnresolvedClassTypeInfo>(other);
		if (otherAsUnresolvedClassType != nullptr)
		{
			auto otherResolvedType = symbolTable->Lookup(otherAsUnresolvedClassType->Name());
			return IsImplicitlyAssignableFrom(otherResolvedType->GetTypeInfo(), symbolTable);
		}

		// No inheritence yet. Just test that it's the same class through a pointer comparison.
		auto otherAsClassType = std::dynamic_pointer_cast<ClassTypeInfo>(other);
		if (otherAsClassType != nullptr)
		{
			return otherAsClassType.get() == this;
		}

		auto otherAsCompositeType = std::dynamic_pointer_cast<CompositeTypeInfo>(other);
		if (otherAsCompositeType != nullptr)
		{
			return otherAsCompositeType->_next == nullptr && IsImplicitlyAssignableFrom(otherAsCompositeType->_thisType, symbolTable);
		}
		throw UnexpectedException();
	}

	std::shared_ptr<TypeInfo> ClassTypeInfo::EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs, std::shared_ptr<SymbolTable> symbolTable)
	{
		// This would depend on operator overloading
		return nullptr;
	}

	bool ClassTypeInfo::SupportsOperator(Operation* operation)
	{
		// Not until operator overloading is implemented
		return false;
	}

	bool UnresolvedClassTypeInfo::IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto classSymbol = symbolTable->Lookup(Name());
		if (classSymbol == nullptr)
		{
			throw SymbolNotDefinedException(Name());
		}
		if (!classSymbol->IsClassBinding())
		{
			throw SymbolWrongTypeException(classSymbol->GetName());
		}
		return classSymbol->GetTypeInfo()->IsLegalTypeForAssignment(symbolTable);
	}

	bool UnresolvedClassTypeInfo::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable)
	{
		// Try and find the class in the current context
		auto classSymbol = symbolTable->Lookup(_name);
		if (classSymbol == nullptr)
		{
			throw SymbolNotDefinedException(_name);
		}
		if (!classSymbol->IsClassBinding())
		{
			throw SymbolWrongTypeException(_name);
		}
		auto resolvedType = classSymbol->GetTypeInfo();
		return resolvedType->IsImplicitlyAssignableFrom(other, symbolTable);
	}

	std::shared_ptr<TypeInfo> UnresolvedClassTypeInfo::EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs, std::shared_ptr<SymbolTable> symbolTable)
	{
		// This would depend on operator overloading
		return nullptr;
	}

	bool UnresolvedClassTypeInfo::SupportsOperator(Operation* operation)
	{
		// Not until operator overloading is implemented
		return false;
	}

	CompositeTypeInfo::CompositeTypeInfo(std::shared_ptr<ArgumentList> argumentList)
	{
		_thisType = argumentList->_argument->_typeInfo;
		_next = argumentList->_next ? std::make_shared<CompositeTypeInfo>(argumentList->_next) : nullptr;
		_name = _thisType->Name();
		if (_next != nullptr)
		{
			_name.append(",");
			_name.append(_next->Name());
		}
	}

	FunctionTypeInfo::FunctionTypeInfo(std::shared_ptr<FunctionDeclaration> functionDeclaration) : _inputArgs(nullptr), _outputArgs(nullptr)
	{
		_name = functionDeclaration->_name;

		if (functionDeclaration->_inputArgs != nullptr)
		{
			if (functionDeclaration->_inputArgs->_next != nullptr)
				_inputArgs = std::make_shared<CompositeTypeInfo>(functionDeclaration->_inputArgs);
			else
				_inputArgs = functionDeclaration->_inputArgs->_argument->_typeInfo;
		}

		if (functionDeclaration->_returnArgs != nullptr)
		{
			if (functionDeclaration->_returnArgs->_next != nullptr)
				_outputArgs = std::make_shared<CompositeTypeInfo>(functionDeclaration->_returnArgs);
			else
				_outputArgs = functionDeclaration->_returnArgs->_argument->_typeInfo;
		}
	}

	bool FunctionTypeInfo::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable)
	{
		return false; // We don't support assigning functions as variables yet. This will come later.
	}

	const std::string& FunctionTypeInfo::Name()
	{
		return _name;
	}
}