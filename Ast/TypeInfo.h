#pragma once
#include <memory>
#include <string>
#include "Node.h"
#include "TypeExceptions.h"

namespace llvm {
	class ConstantFolder;

	template <bool preserveNames = true>
	class IRBuilderDefaultInserter;

	template<bool preserveNames = true, typename T = ConstantFolder,
		typename Inserter = IRBuilderDefaultInserter<preserveNames> >
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
		virtual int CreateCast(std::shared_ptr<TypeInfo> castTo)
		{
			throw UnexpectedException();
		}
	};

	class NotSupportedByAutoTypeException : public std::exception
	{
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

		virtual bool IsComposite() override
		{
			return true;
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

		std::shared_ptr<TypeInfo> InputArgsType() { return _inputArgs; }
		std::shared_ptr<TypeInfo> OutputArgsType() { return _outputArgs; }
		std::shared_ptr<TypeInfo> _inputArgs;
		std::shared_ptr<TypeInfo> _outputArgs;
		std::string _name;
	};

	class BaseClassTypeInfo : public TypeInfo
	{
	public:
		bool IsClassType() override
		{
			return true;
		}

		virtual std::shared_ptr<TypeInfo> ClassDeclarationTypeInfo(std::shared_ptr<SymbolTable> symbolTable) = 0;

		virtual bool IsValueType() = 0;
	};

	// Represents the TypeInfo for a class's declaration, which is equally valid for value or reference instantiations.
	// This TypeInfo is used for basic type compatibility without regards to value or reference types. It's not a legal
	// assignment type, and thus expressions may not evaluate to this kind of TypeInfo (they return the ClassTypeInfo instead)
	class ClassDeclaration;
	class ClassDeclarationTypeInfo : public TypeInfo
	{
	public:
		ClassDeclarationTypeInfo(std::shared_ptr<ClassDeclaration> classDeclaration);

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return false;
		}

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override
		{
			return _name;
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
			// TODO
			throw UnexpectedException();
		}

	private:
		std::string _name;
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

		bool IsValueType() override
		{
			return _valueType;
		}

		std::shared_ptr<TypeInfo> ClassDeclarationTypeInfo(std::shared_ptr<SymbolTable> /*symbolTable*/) override
		{
			return _classDeclTypeInfo;
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

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override
		{
			return _name;
		}

		virtual std::shared_ptr<TypeInfo> EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override;

		virtual bool SupportsOperator(Operation* operation) override;
		
		virtual bool NeedsResolution() override
		{
			return true;
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

		void EnsureResolved(std::shared_ptr<SymbolTable> symbolTable); 
		
		bool IsValueType() override
		{
			return _valueType;
		}

		std::shared_ptr<TypeInfo> ClassDeclarationTypeInfo(std::shared_ptr<SymbolTable> symbolTable) override
		{
			EnsureResolved(symbolTable);
			return _resolvedType->ClassDeclarationTypeInfo(symbolTable);
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