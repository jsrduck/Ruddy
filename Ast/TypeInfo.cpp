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
	ClassDeclarationTypeInfo::ClassDeclarationTypeInfo(const std::string & name, const std::string & fullyQualifiedName) :
		_name(name), _fullyQualifiedName(fullyQualifiedName), _type(nullptr)
	{
	}

	bool ClassDeclarationTypeInfo::IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable)
	{
		return false;
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

		if (other->IsPrimitiveType())
		{
			return false;
		}

		throw UnexpectedException();
	}

	const std::string & ClassDeclarationTypeInfo::Name()
	{
		return _name;
	}

	std::string ClassDeclarationTypeInfo::FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable)
	{
		return _fullyQualifiedName;
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

	llvm::Value * ClassDeclarationTypeInfo::CreateAllocation(const std::string & name, llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		// TODO
		throw UnexpectedException();
	}

	llvm::Type * ClassDeclarationTypeInfo::GetIRType(llvm::LLVMContext * context, bool asOutput)
	{
		if (_type == nullptr)
			throw UnexpectedException();
		return _type;
	}

	void ClassDeclarationTypeInfo::BindType(llvm::Type * type)
	{
		_type = type;
	}

	bool ClassDeclarationTypeInfo::IsSameType(std::shared_ptr<TypeInfo> other)
	{
		auto otherAsClass = std::dynamic_pointer_cast<BaseClassTypeInfo>(other);
		if (otherAsClass == nullptr)
			return false;
		return FullyQualifiedName().compare(otherAsClass->FullyQualifiedName()) == 0;
	}

	/* ClassTypeInfo */

	ClassTypeInfo::ClassTypeInfo(std::shared_ptr<TypeInfo> classDeclTypeInfo, bool valueType) :
		_classDeclTypeInfo(classDeclTypeInfo), _valueType(valueType)
	{
	}

	bool ClassTypeInfo::IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable)
	{
		return true;
	}

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

	const std::string & ClassTypeInfo::Name()
	{
		return _classDeclTypeInfo->Name();
	}

	std::shared_ptr<TypeInfo> ClassTypeInfo::EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation * operation, std::shared_ptr<TypeInfo> rhs, std::shared_ptr<SymbolTable> symbolTable)
	{
		return _classDeclTypeInfo->EvaluateOperation(implicitCastTypeOut, operation, rhs, symbolTable);
	}

	llvm::Value* ClassTypeInfo::GetDefaultValue(llvm::LLVMContext* context)
	{
		if (IsValueType())
			throw UnexpectedException();
		return llvm::ConstantPointerNull::get(this->GetIRType(context)->getPointerTo(GC_MANAGED_HEAP_ADDRESS_SPACE));
	}

	bool ClassTypeInfo::IsSameType(std::shared_ptr<TypeInfo> other)
	{
		return _classDeclTypeInfo->IsSameType(other);
	}

	std::string ClassTypeInfo::FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable)
	{
		return std::dynamic_pointer_cast<ClassDeclarationTypeInfo>(_classDeclTypeInfo)->FullyQualifiedName(symbolTable);
	}

	std::string ClassTypeInfo::SerializedName(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto result = FullyQualifiedName(symbolTable);
		if (IsValueType())
			result = result + "&";
		return result;
	}
	bool ClassTypeInfo::SupportsOperator(Operation * operation)
	{
		return _classDeclTypeInfo->SupportsOperator(operation);
	}

	llvm::Type * ClassTypeInfo::GetIRType(llvm::LLVMContext * context, bool asOutput)
	{
		auto structIRType = _classDeclTypeInfo->GetIRType(context, asOutput);
		if (IsValueType())
			return structIRType;
		else
			return structIRType->getPointerTo(GC_MANAGED_HEAP_ADDRESS_SPACE);
	}

	bool ClassTypeInfo::IsValueType()
	{
		return _valueType;
	}

	std::shared_ptr<TypeInfo> ClassTypeInfo::GetClassDeclarationTypeInfo(std::shared_ptr<SymbolTable>)
	{
		return _classDeclTypeInfo;
	}


	/* UnresolvedClassTypeInfo */

	UnresolvedClassTypeInfo::UnresolvedClassTypeInfo(const std::string & name, bool valueType) : _name(name), _valueType(valueType)
	{
	}

	UnresolvedClassTypeInfo::UnresolvedClassTypeInfo(const std::string & name)
	{
		_valueType = name.at(name.size() - 1) == '&';
		if (_valueType)
		{
			_name = name.substr(0, name.size() - 1);
		}
		else
		{
			_name = name;
		}
	}

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

	const std::string & UnresolvedClassTypeInfo::Name()
	{
		return _name;
	}

	std::string UnresolvedClassTypeInfo::FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable)
	{
		if (NeedsResolution())
			if (symbolTable != nullptr)
				EnsureResolved(symbolTable);
			else
				throw UnexpectedException();
		return _resolvedType->FullyQualifiedName();
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

	bool UnresolvedClassTypeInfo::NeedsResolution()
	{
		return true;
	}

	llvm::Value * UnresolvedClassTypeInfo::CreateAllocation(const std::string & name, llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		if (_resolvedType == nullptr)
			throw UnexpectedException();
		return _resolvedType->CreateAllocation(name, builder, context, module);
	}

	llvm::Type * UnresolvedClassTypeInfo::GetIRType(llvm::LLVMContext * context, bool asOutput)
	{
		if (_resolvedType == nullptr)
			throw UnexpectedException();
		return _resolvedType->GetIRType(context, asOutput);
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

	bool UnresolvedClassTypeInfo::IsValueType()
	{
		return _valueType;
	}

	std::shared_ptr<TypeInfo> UnresolvedClassTypeInfo::GetClassDeclarationTypeInfo(std::shared_ptr<SymbolTable> symbolTable)
	{
		EnsureResolved(symbolTable);
		return _resolvedType->GetClassDeclarationTypeInfo(symbolTable);
	}

	bool UnresolvedClassTypeInfo::IsSameType(std::shared_ptr<TypeInfo> other)
	{
		return _resolvedType->IsSameType(other);
	}

	std::string UnresolvedClassTypeInfo::SerializedName(std::shared_ptr<SymbolTable> symbolTable)
	{
		if (NeedsResolution())
			if (symbolTable != nullptr)
				EnsureResolved(symbolTable);
			else
				throw UnexpectedException();
		return _resolvedType->SerializedName(symbolTable);
	}

	/* CompositeTypeInfo */
	CompositeTypeInfo::CompositeTypeInfo(std::shared_ptr<TypeInfo> thisType, std::shared_ptr<CompositeTypeInfo> next) : _thisType(thisType), _next(next)
	{
		_name = thisType->Name();
		if (_next != nullptr)
		{
			_name.append(",");
			_name.append(_next->Name());
		}
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

	std::shared_ptr<CompositeTypeInfo> CompositeTypeInfo::Clone(std::shared_ptr<CompositeTypeInfo> from)
	{
		std::shared_ptr<CompositeTypeInfo> retVal = nullptr;
		auto current = retVal;
		while (from != nullptr)
		{
			auto newGuy = std::make_shared<CompositeTypeInfo>(from->_thisType);
			if (current != nullptr)
			{
				current->_next = newGuy;
			}
			current = newGuy;
			if (retVal == nullptr)
			{
				retVal = current;
			}
			from = from->_next;
		}
		return retVal;
	}

	bool CompositeTypeInfo::IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable)
	{
		return _thisType->IsLegalTypeForAssignment(symbolTable) && _next->IsLegalTypeForAssignment(symbolTable);
	}

	bool CompositeTypeInfo::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable)
	{
		auto otherArgList = std::dynamic_pointer_cast<CompositeTypeInfo>(other);
		if (otherArgList == nullptr)
		{
			// This has to be a single argument
			return _next == nullptr && _thisType->IsImplicitlyAssignableFrom(other, symbolTable);
		}
		if (_next == nullptr)
		{
			// The other one had better be a single argument
			return otherArgList->_next == nullptr && _thisType->IsImplicitlyAssignableFrom(otherArgList->_thisType, symbolTable);
		}
		// Both have more to look at
		return _thisType->IsImplicitlyAssignableFrom(otherArgList->_thisType, symbolTable) &&
			_next->IsImplicitlyAssignableFrom(otherArgList->_next, symbolTable);
	}

	std::string CompositeTypeInfo::SerializedName(std::shared_ptr<SymbolTable> symbolTable)
	{
		if (_next == nullptr)
		{
			return _thisType->SerializedName(symbolTable);
		}
		else
		{
			return  _thisType->SerializedName(symbolTable) + "," + _next->SerializedName(symbolTable);
		}
	}

	const std::string & CompositeTypeInfo::Name()
	{
		return _name;
	}

	bool CompositeTypeInfo::SupportsOperator(Operation * operation)
	{
		// TODO overloading
		return false;
	}

	llvm::Value * CompositeTypeInfo::CreateAllocation(const std::string & name, llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		// TODO
		throw UnexpectedException();
	}

	llvm::Type * CompositeTypeInfo::GetIRType(llvm::LLVMContext * context, bool asOutput)
	{
		// TODO
		throw UnexpectedException();
	}

	void Ast::TypeInfo::AddIRTypesToVector(std::vector<llvm::Type*>& inputVector, llvm::LLVMContext * context, bool asOutput)
	{
		auto irType = GetIRType(context, asOutput);
		inputVector.push_back(irType);
	}

	void CompositeTypeInfo::AddIRTypesToVector(std::vector<llvm::Type*>& inputVector, llvm::LLVMContext * context, bool asOutput)
	{
		if (_thisType != nullptr)
			_thisType->AddIRTypesToVector(inputVector, context, asOutput);
		if (_next != nullptr)
			_next->AddIRTypesToVector(inputVector, context, asOutput);
	}

	bool CompositeTypeInfo::IsComposite()
	{
		return true;
	}

	bool CompositeTypeInfo::IsSameType(std::shared_ptr<TypeInfo> other)
	{
		auto otherAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(other);
		if (otherAsComposite != nullptr)
		{
			return _thisType->IsSameType(otherAsComposite->_thisType) && _next->IsSameType(otherAsComposite->_next);
		}
		else
		{
			return _next == nullptr && _thisType->IsSameType(other);
		}
	}

	/* FunctionTypeInfo */
	FunctionTypeInfo::FunctionTypeInfo(const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, std::shared_ptr<Modifier> mods) :
		_name(name), _inputArgs(inputArgs), _outputArgs(outputArgs), _mods(mods)
	{
	}

	bool FunctionTypeInfo::IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable)
	{
		return false;
	}

	bool FunctionTypeInfo::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable)
	{
		return false; // We don't support assigning functions as variables yet. This will come later.
	}

	const std::string& FunctionTypeInfo::Name()
	{
		return _name;
	}

	// Operator logic

	bool FunctionTypeInfo::SupportsOperator(Operation * operation)
	{
		return false;
	}
	llvm::Value * FunctionTypeInfo::CreateAllocation(const std::string & name, llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		// TODO
		throw UnexpectedException();
	}
	llvm::Type * FunctionTypeInfo::GetIRType(llvm::LLVMContext * context, bool asOutput)
	{
		// TODO
		throw UnexpectedException();
	}
	bool FunctionTypeInfo::IsMethod()
	{
		return !_mods->IsStatic();
	}
	bool FunctionTypeInfo::IsSameType(std::shared_ptr<TypeInfo> other)
	{
		auto otherAsFunction = std::dynamic_pointer_cast<FunctionTypeInfo>(other);
		if (otherAsFunction == nullptr)
			return false;

		// TODO: Is it enough for a function to have a matching signature to be the same type?
		return InputArgsType()->IsSameType(otherAsFunction->InputArgsType()) && OutputArgsType()->IsSameType(otherAsFunction->OutputArgsType());
	}

	/* BaseClassTypeInfo */
	bool BaseClassTypeInfo::IsClassType()
	{
		return true;
	}

	llvm::Value * BaseClassTypeInfo::GetDefaultValue(llvm::LLVMContext * context)
	{
		auto type = GetIRType(context);
		if (!type->isPointerTy())
			throw UnexpectedException();
		return llvm::ConstantPointerNull::get(reinterpret_cast<llvm::PointerType*>(type));
	}

	/* TypeSpecifier */
	TypeSpecifier::TypeSpecifier() : _resolvedType(new AutoTypeInfo())
	{
	}
	TypeSpecifier::TypeSpecifier(const std::string & name, bool valueType) : _resolvedType(new UnresolvedClassTypeInfo(name, valueType))
	{
	}
	TypeSpecifier::TypeSpecifier(std::shared_ptr<TypeInfo> knownType) : _resolvedType(knownType)
	{
	}
	std::shared_ptr<TypeInfo> TypeSpecifier::GetTypeInfo()
	{
		return _resolvedType;
	}
}