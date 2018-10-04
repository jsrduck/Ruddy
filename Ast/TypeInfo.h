#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Node.h"
#include "Exceptions.h"

namespace llvm {
	class ConstantFolder;

	class IRBuilderDefaultInserter;

	template <typename T = ConstantFolder,
		typename Inserter = IRBuilderDefaultInserter>
	class IRBuilder;

	class Value;

	class LLVMContext;

	class Module;

	class AllocaInst;
	class BasicBlock;
	class Type;
}

namespace Ast
{
	class Operation;
	class SymbolTable;
	class TypeInfo : public Node, public std::enable_shared_from_this<TypeInfo>
	{
	public:
		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) = 0;
		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) = 0;
		virtual const std::string& Name() = 0;
		
		// Operator logic
		virtual std::shared_ptr<TypeInfo> EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr);
		virtual bool SupportsOperator(Operation* operation) = 0;
		virtual bool IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(Operation* operation, std::shared_ptr<TypeInfo>& implicitCastTypeOut)
		{
			return false;
		}
		virtual bool IsAutoType() { return false; }
		virtual bool NeedsResolution() { return false; }
		virtual llvm::AllocaInst* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) = 0;
		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) = 0;
		virtual bool IsPrimitiveType() { return false; }
		virtual bool IsConstant() { return false; }
		virtual bool IsInteger() { return false; }
		virtual bool IsFloatingPoint() { return false; }
		virtual bool IsComposite() { return false; }
		virtual bool IsClassType() { return false; }
		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) = 0;

		virtual int CreateCast(std::shared_ptr<TypeInfo> castTo)
		{
			throw UnexpectedException();
		}

		virtual llvm::Value* GetDefaultValue(llvm::LLVMContext* context)
		{
			throw UnexpectedException();
		}

		virtual std::string SerializedName(std::shared_ptr<SymbolTable> symbolTable)
		{
			throw UnexpectedException();
		}

		virtual void AddIRTypesToVector(std::vector<llvm::Type*>& inputVector, llvm::LLVMContext* context, bool asOutput = false);
	};

	class AutoTypeInfo : public TypeInfo
	{
	public:
		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return true;
		}

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override
		{
			return true;
		}

		virtual const std::string& Name() override
		{
			return _name;
		}

		virtual llvm::AllocaInst* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) override
		{
			// TODO
			throw UnexpectedException();
		}

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override
		{
			// TODO
			throw UnexpectedException();
		}

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override
		{
			return true; // Auto is always the same type!
		}

		// Operator logic
		virtual std::shared_ptr<TypeInfo> EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override { throw NotSupportedByAutoTypeException(); }
		virtual bool SupportsOperator(Operation* operation) override { throw NotSupportedByAutoTypeException(); }
		virtual bool IsAutoType() override { return true; }
		std::string _name = "auto type";
	};

	class ArgumentList;
	class TypeAndIdentifier;
	class CompositeTypeInfo : public TypeInfo
	{
	public:
		CompositeTypeInfo(std::shared_ptr<TypeInfo> thisType, std::shared_ptr<CompositeTypeInfo> next = nullptr) : _thisType(thisType), _next(next)
		{
			_name = thisType->Name();
			if (_next != nullptr)
			{
				_name.append(",");
				_name.append(_next->Name());
			}
		}

		static std::shared_ptr<CompositeTypeInfo> Clone(std::shared_ptr<CompositeTypeInfo> from)
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

		CompositeTypeInfo(std::shared_ptr<ArgumentList> argumentList);

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _thisType->IsLegalTypeForAssignment(symbolTable) && _next->IsLegalTypeForAssignment(symbolTable);
		}

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override
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

		virtual std::string SerializedName(std::shared_ptr<SymbolTable> symbolTable) override
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

		virtual const std::string& Name() override
		{
			return _name;
		}

		virtual bool SupportsOperator(Operation* operation) override
		{
			// TODO overloading
			return false;
		}

		virtual llvm::AllocaInst* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) override
		{
			// TODO
			throw UnexpectedException();
		}

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override
		{
			// TODO
			throw UnexpectedException();
		}

		virtual void AddIRTypesToVector(std::vector<llvm::Type*>& inputVector, llvm::LLVMContext* context, bool asOutput = false) override; // TODO

		virtual bool IsComposite() override
		{
			return true;
		}

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override
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

		std::shared_ptr<TypeInfo> _thisType;
		std::shared_ptr<CompositeTypeInfo> _next;
		std::string _name;
	};

	class FunctionDeclaration;
	class FunctionTypeInfo : public TypeInfo
	{
	public:
		FunctionTypeInfo(std::shared_ptr<FunctionDeclaration> functionDeclaration);
		FunctionTypeInfo(const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, std::shared_ptr<Modifier> mods);
		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return false;
		}
		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;
		virtual const std::string& Name() override;

		// Operator logic
		virtual bool SupportsOperator(Operation* operation) { return false; } // For now, we don't support operators on functions.

		virtual llvm::AllocaInst* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) override
		{
			// TODO
			throw UnexpectedException();
		}

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override
		{
			// TODO
			throw UnexpectedException();
		}

		bool IsMethod()
		{
			return !_mods->IsStatic();
		}

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override
		{
			auto otherAsFunction = std::dynamic_pointer_cast<FunctionTypeInfo>(other);
			if (otherAsFunction == nullptr)
				return false;

			// TODO: Is it enough for a function to have a matching signature to be the same type?
			return InputArgsType()->IsSameType(otherAsFunction->InputArgsType()) && OutputArgsType()->IsSameType(otherAsFunction->OutputArgsType());
		}

		std::shared_ptr<TypeInfo> InputArgsType() { return _inputArgs; }
		std::shared_ptr<TypeInfo> OutputArgsType() { return _outputArgs; }
		std::shared_ptr<TypeInfo> _inputArgs;
		std::shared_ptr<TypeInfo> _outputArgs;
		std::string _name;
		std::shared_ptr<Ast::Modifier> _mods;
	};

	class BaseClassTypeInfo : public TypeInfo
	{
	public:
		bool IsClassType() override
		{
			return true;
		}

		virtual std::shared_ptr<TypeInfo> GetClassDeclarationTypeInfo(std::shared_ptr<SymbolTable> symbolTable) = 0;

		virtual bool IsValueType() = 0;

		virtual std::string FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable = nullptr) = 0;
	};

	// Represents the TypeInfo for a class's declaration, which is equally valid for value or reference instantiations.
	// This TypeInfo is used for basic type compatibility without regards to value or reference types. It's not a legal
	// assignment type, and thus expressions may not evaluate to this kind of TypeInfo (they return the ClassTypeInfo instead)
	class ClassDeclaration;
	class ClassDeclarationTypeInfo : public TypeInfo
	{
	public:
		ClassDeclarationTypeInfo(std::shared_ptr<ClassDeclaration> classDeclaration, std::string& fullyQualifiedName);
		ClassDeclarationTypeInfo(const std::string& name, const std::string& fullyQualifiedName);

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return false;
		}

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override
		{
			return _name;
		}

		virtual std::string FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable = nullptr)
		{
			return _fullyQualifiedName;
		}

		virtual std::shared_ptr<TypeInfo> EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override;

		virtual bool SupportsOperator(Operation* operation) override;

		virtual llvm::AllocaInst* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) override
		{
			// TODO
			throw UnexpectedException();
		}

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override
		{
			if (_type == nullptr)
				throw UnexpectedException();
			return _type;
		}

		void BindType(llvm::Type* type)
		{
			_type = type;
		}

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override
		{
			auto otherAsClass = std::dynamic_pointer_cast<BaseClassTypeInfo>(other);
			if (otherAsClass == nullptr)
				return false;
			return FullyQualifiedName().compare(otherAsClass->FullyQualifiedName()) == 0;
		}

	private:
		llvm::Type* _type;
		std::string _name;
		std::string _fullyQualifiedName;
	};

	// Represents the TypeInfo of a class (not a primitive), along with value/reference type. The passed in type is
	// the ClassDeclarationTypeInfo, although this isn't enforced.
	class ClassTypeInfo : public BaseClassTypeInfo
	{
	public:
		ClassTypeInfo(std::shared_ptr<TypeInfo> classDeclTypeInfo, bool valueType) : 
			_classDeclTypeInfo(classDeclTypeInfo), _valueType(valueType)
		{
		}

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return true;
		}

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override
		{
			return _classDeclTypeInfo->Name();
		}

		virtual std::shared_ptr<TypeInfo> EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override
		{
			return _classDeclTypeInfo->EvaluateOperation(implicitCastTypeOut, operation, rhs, symbolTable);
		}

		virtual bool SupportsOperator(Operation* operation) override
		{
			return _classDeclTypeInfo->SupportsOperator(operation);
		}

		virtual llvm::AllocaInst* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) override;

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override
		{
			return _classDeclTypeInfo->GetIRType(context, asOutput);
		}

		bool IsValueType() override
		{
			return _valueType;
		}

		std::shared_ptr<TypeInfo> GetClassDeclarationTypeInfo(std::shared_ptr<SymbolTable> /*symbolTable*/) override
		{
			return _classDeclTypeInfo;
		}

		virtual llvm::Value* GetDefaultValue(llvm::LLVMContext* context) override;

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override
		{
			return _classDeclTypeInfo->IsSameType(other);
		}

		virtual std::string FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable = nullptr) override
		{
			return std::dynamic_pointer_cast<ClassDeclarationTypeInfo>(_classDeclTypeInfo)->FullyQualifiedName(symbolTable);
		}

		virtual std::string SerializedName(std::shared_ptr<SymbolTable> symbolTable) override
		{
			auto result = FullyQualifiedName(symbolTable);
			if (IsValueType())
				result = result + "&";
			return result;
		}

	private:
		std::shared_ptr<TypeInfo> _classDeclTypeInfo;
		bool _valueType;
	};

	class Reference;
	class UnresolvedClassTypeInfo : public BaseClassTypeInfo
	{
	public:
		UnresolvedClassTypeInfo(const std::string& name, bool valueType) : _name(name), _valueType(valueType)
		{
		}

		UnresolvedClassTypeInfo(const std::string& name)
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

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override
		{
			return _name;
		}

		virtual std::string FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable = nullptr) override
		{
			if (NeedsResolution())
				if (symbolTable != nullptr)
					EnsureResolved(symbolTable);
				else
					throw UnexpectedException();
			return _resolvedType->FullyQualifiedName();
		}

		virtual std::shared_ptr<TypeInfo> EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override;

		virtual bool SupportsOperator(Operation* operation) override;
		
		virtual bool NeedsResolution() override
		{
			return true;
		}

		virtual llvm::AllocaInst* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) override
		{
			if (_resolvedType == nullptr)
				throw UnexpectedException();
			return _resolvedType->CreateAllocation(name, builder, context);
		}

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override
		{
			if (_resolvedType == nullptr)
				throw UnexpectedException();
			return _resolvedType->GetIRType(context, asOutput);
		}

		void EnsureResolved(std::shared_ptr<SymbolTable> symbolTable); 
		
		bool IsValueType() override
		{
			return _valueType;
		}

		std::shared_ptr<TypeInfo> GetClassDeclarationTypeInfo(std::shared_ptr<SymbolTable> symbolTable) override
		{
			EnsureResolved(symbolTable);
			return _resolvedType->GetClassDeclarationTypeInfo(symbolTable);
		}

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override
		{
			return _resolvedType->IsSameType(other);
		}

		virtual std::string SerializedName(std::shared_ptr<SymbolTable> symbolTable) override
		{
			if (NeedsResolution())
				if (symbolTable != nullptr)
					EnsureResolved(symbolTable);
				else
					throw UnexpectedException();
			return _resolvedType->SerializedName(symbolTable);
		}

	private:
		std::string _name;
		bool _valueType;
		std::shared_ptr<ClassTypeInfo> _resolvedType;
	};

	// Used in the grammar, as a step towards resolving towards an actual type.
	// This is mostly necessary because the grammar demands we pass around raw pointers
	// and our primitive types are managed by shared pointers
	class TypeSpecifier
	{
	public:
		TypeSpecifier() : _resolvedType(new AutoTypeInfo())
		{
		}

		TypeSpecifier(const std::string& name, bool valueType) : _resolvedType(new UnresolvedClassTypeInfo(name, valueType))
		{
		}

		TypeSpecifier(std::shared_ptr<TypeInfo> knownType) : _resolvedType(knownType)
		{
		}

		std::shared_ptr<TypeInfo> GetTypeInfo()
		{
			return _resolvedType;
		}

	private:
		std::shared_ptr<TypeInfo> _resolvedType;
	};
}