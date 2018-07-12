#include "stdafx.h"
#include "TypeInfo.h"
#include "Primitives.h"
#include "Operations.h"
#include "Exceptions.h"
#include "Classes.h"
#include "Statements.h"

#include <llvm\IR\Constants.h>

#include <assert.h>

using namespace std;

namespace Ast
{
	/* TypeInfo */

	std::shared_ptr<TypeInfo> TypeInfo::EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs, std::shared_ptr<SymbolTable> symbolTable)
	{
		if (operation->IsBinary())
		{
			assert(rhs);
			if (rhs == nullptr)
			{
				throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
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
						return EvaluateOperation(implicitCastTypeOut, operation, implicitCastTypeOut, symbolTable);
					}
					else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
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
						return implicitCastTypeOut->EvaluateOperation(implicitCastTypeOut, operation, rhs, symbolTable);
					}
					else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
				}
				else if (IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
				{
					return implicitCastTypeOut->EvaluateOperation(implicitCastTypeOut, operation, rhs, symbolTable);
				}
				else if (rhs->IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
				{
					return EvaluateOperation(implicitCastTypeOut, operation, implicitCastTypeOut, symbolTable);
				}
				else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
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
							return EvaluateOperation(implicitCastTypeOut, operation, implicitCastTypeOut, symbolTable);
						}
						else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
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
							return implicitCastTypeOut->EvaluateOperation(implicitCastTypeOut, operation, rhs, symbolTable);
						}
						else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
					}
					else if (rhs->IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
					{
						return EvaluateOperation(implicitCastTypeOut, operation, implicitCastTypeOut, symbolTable);
					}
					else if (IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
					{
						return implicitCastTypeOut->EvaluateOperation(implicitCastTypeOut, operation, rhs, symbolTable);
					}
					else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
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
						else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
					}
					else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
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
								return implicitCastTypeOut->EvaluateOperation(implicitCastTypeOut, operation, rhs, symbolTable);
							}
							else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
						}
						else
						{
							throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
						}
					}
					else
					{
						// Bitwise AND, OR and XOR
						if (BoolTypeInfo::Get()->IsImplicitlyAssignableFrom(shared_from_this(), symbolTable))
						{
							throw OperationNotDefinedException(operation->OperatorString(), shared_from_this()); // To avoid hard-to-find bugs
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
								return EvaluateOperation(implicitCastTypeOut, operation, implicitCastTypeOut, symbolTable);
							}
							else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
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
								return implicitCastTypeOut->EvaluateOperation(implicitCastTypeOut, operation, rhs, symbolTable);
							}
							else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
						}
						else if (rhs->IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
						{
							return EvaluateOperation(implicitCastTypeOut, operation, implicitCastTypeOut, symbolTable);
						}
						else if (IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(operation, implicitCastTypeOut))
						{
							return implicitCastTypeOut->EvaluateOperation(implicitCastTypeOut, operation, rhs, symbolTable);
						}
						else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
					}
				}
			}
		}
		else /* Unary */
		{
			assert(!rhs);
			if (rhs != nullptr)
			{
				throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
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
				else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
			}
			else if (operation->IsLogical())
			{
				if (operation->IsBoolean())
				{
					if (shared_from_this() != BoolTypeInfo::Get())
					{
						throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
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
					else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
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
					else throw OperationNotDefinedException(operation->OperatorString(), shared_from_this());
				}
			}
			else if (dynamic_cast<CastOperation*>(operation) != nullptr)
			{
				return dynamic_cast<CastOperation*>(operation)->_castTo; // TODO: What if type casting not supported?
			}
		}
		throw UnexpectedException();
	}

	
	/* ClassDeclarationTypeInfo */

	ClassDeclarationTypeInfo::ClassDeclarationTypeInfo(std::shared_ptr<ClassDeclaration> classDeclaration, std::string& fullyQualifiedName) : _name(classDeclaration->_name), _fullyQualifiedName(fullyQualifiedName), _type(nullptr)
	{
	}

	bool ClassDeclarationTypeInfo::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable)
	{
		auto otherAsUnresolvedClassType = std::dynamic_pointer_cast<UnresolvedClassTypeInfo>(other);
		if (otherAsUnresolvedClassType != nullptr)
		{
			auto otherResolvedType = symbolTable->Lookup(otherAsUnresolvedClassType->Name());
			return IsImplicitlyAssignableFrom(otherResolvedType->GetTypeInfo(), symbolTable);
		}

		// No inheritence yet. Just test that it's the same class through a pointer comparison.
		auto othserAsClassDeclType = std::dynamic_pointer_cast<ClassDeclarationTypeInfo>(other);
		if (othserAsClassDeclType != nullptr)
		{
			return othserAsClassDeclType.get() == this;
		}

		auto otherAsCompositeType = std::dynamic_pointer_cast<CompositeTypeInfo>(other);
		if (otherAsCompositeType != nullptr)
		{
			return otherAsCompositeType->_next == nullptr && IsImplicitlyAssignableFrom(otherAsCompositeType->_thisType, symbolTable);
		}
		throw UnexpectedException();
	}

	std::shared_ptr<TypeInfo> ClassDeclarationTypeInfo::EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs, std::shared_ptr<SymbolTable> symbolTable)
	{
		// This would depend on operator overloading
		return nullptr;
	}

	bool ClassDeclarationTypeInfo::SupportsOperator(Operation* operation)
	{
		// Not until operator overloading is implemented
		return false;
	}

	/* ClassTypeInfo */

	bool ClassTypeInfo::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable)
	{
		if (other->IsClassType())
		{
			auto otherAsClassType = std::dynamic_pointer_cast<BaseClassTypeInfo>(other);
			if (otherAsClassType == nullptr)
			{
				throw UnexpectedException();
			}
			return otherAsClassType->IsValueType() == _valueType &&
				_classDeclTypeInfo->IsImplicitlyAssignableFrom(otherAsClassType->GetClassDeclarationTypeInfo(symbolTable), symbolTable);
		}
		else
		{
			return _classDeclTypeInfo->IsImplicitlyAssignableFrom(other, symbolTable);
		}
	}

	llvm::Value* ClassTypeInfo::GetDefaultValue(llvm::LLVMContext* context)
	{
		return llvm::ConstantPointerNull::get(this->GetIRType(context)->getPointerTo());
	}

	/* UnresolvedClassTypeInfo */

	bool UnresolvedClassTypeInfo::IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable)
	{
		EnsureResolved(symbolTable);
		return _resolvedType->IsLegalTypeForAssignment(symbolTable);
	}

	bool UnresolvedClassTypeInfo::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable)
	{
		// Try and find the class in the current context
		EnsureResolved(symbolTable);
		return _resolvedType->IsImplicitlyAssignableFrom(other, symbolTable);
	}

	std::shared_ptr<TypeInfo> UnresolvedClassTypeInfo::EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs, std::shared_ptr<SymbolTable> symbolTable)
	{
		// This would depend on operator overloading
		EnsureResolved(symbolTable);
		return nullptr;
	}

	bool UnresolvedClassTypeInfo::SupportsOperator(Operation* operation)
	{
		// Not until operator overloading is implemented
		return false;
	}

	void UnresolvedClassTypeInfo::EnsureResolved(std::shared_ptr<SymbolTable> symbolTable)
	{
		if (_resolvedType == nullptr)
		{
			auto classSymbol = symbolTable->Lookup(_name);
			if (classSymbol == nullptr)
			{
				throw SymbolNotDefinedException(_name);
			}
			if (!classSymbol->IsClassBinding())
			{
				throw SymbolWrongTypeException(_name);
			}
			_resolvedType = std::make_shared<ClassTypeInfo>(classSymbol->GetTypeInfo(), _valueType);
		}
	}

	/* CompositeTypeInfo */

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

	/* FunctionTypeInfo */

	FunctionTypeInfo::FunctionTypeInfo(std::shared_ptr<FunctionDeclaration> functionDeclaration) : _inputArgs(nullptr), _outputArgs(nullptr), _mods(functionDeclaration->_mods)
	{
		_name = functionDeclaration->Name();

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