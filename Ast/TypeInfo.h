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

#define GC_MANAGED_HEAP_ADDRESS_SPACE 0

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
		virtual bool IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(Operation* operation, std::shared_ptr<TypeInfo>& implicitCastTypeOut) { return false; }
		virtual bool IsAutoType() { return false; }
		virtual bool NeedsResolution() { return false; }
		virtual llvm::Value* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) = 0;
		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) = 0;
		virtual bool IsPrimitiveType() { return false; }
		virtual bool IsConstant() { return false; }
		virtual bool IsInteger() { return false; }
		virtual bool IsFloatingPoint() { return false; }
		virtual bool IsComposite() { return false; }
		virtual bool IsClassType() { return false; }
		virtual bool IsNativeArrayType() { return false; }
		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) = 0;
		virtual std::string FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable = nullptr) { throw UnexpectedException(); }

		virtual int CreateCast(std::shared_ptr<TypeInfo> castTo) { throw UnexpectedException(); }

		virtual llvm::Value* GetDefaultValue(llvm::LLVMContext* context) { throw UnexpectedException(); }

		virtual std::string SerializedName(std::shared_ptr<SymbolTable> symbolTable) { throw UnexpectedException(); }

		virtual void AddIRTypesToVector(std::vector<llvm::Type*>& inputVector, llvm::LLVMContext* context, bool asOutput = false);
	};

	class AutoTypeInfo : public TypeInfo
	{
	public:
		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override { return true; }

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override { return true; }

		virtual const std::string& Name() override
		{
			return _name;
		}

		virtual llvm::Value* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override
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
		CompositeTypeInfo(std::shared_ptr<TypeInfo> thisType, std::shared_ptr<CompositeTypeInfo> next = nullptr);

		CompositeTypeInfo(std::shared_ptr<ArgumentList> argumentList);

		static std::shared_ptr<CompositeTypeInfo> Clone(std::shared_ptr<CompositeTypeInfo> from);

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual std::string SerializedName(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override;

		virtual bool SupportsOperator(Operation* operation) override;

		virtual llvm::Value* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override;

		virtual void AddIRTypesToVector(std::vector<llvm::Type*>& inputVector, llvm::LLVMContext* context, bool asOutput = false) override; // TODO

		virtual bool IsComposite() override;

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override;

		std::shared_ptr<TypeInfo> _thisType;
		std::shared_ptr<CompositeTypeInfo> _next;
		std::string _name;
	};

	class FunctionDeclaration;
	class FunctionTypeInfo : public TypeInfo
	{
	public:
		FunctionTypeInfo(const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, std::shared_ptr<Modifier> mods);
		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override;
		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;
		virtual const std::string& Name() override;

		// Operator logic
		virtual bool SupportsOperator(Operation* operation); // For now, we don't support operators on functions.

		virtual llvm::Value* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override;

		virtual bool IsMethod();
		virtual bool IsOverloadTypeInfo();

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override;

		std::shared_ptr<TypeInfo> InputArgsType() { return _inputArgs; }
		std::shared_ptr<TypeInfo> OutputArgsType() { return _outputArgs; }
		std::shared_ptr<TypeInfo> _inputArgs;
		std::shared_ptr<TypeInfo> _outputArgs;
		std::string _name;
		std::shared_ptr<Ast::Modifier> _mods;
	};

	class OverloadedFunctionTypeInfo : public FunctionTypeInfo
	{
	public:
		OverloadedFunctionTypeInfo(const std::string& name);
		virtual bool IsMethod() override;
		virtual bool IsOverloadTypeInfo() override;
		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override;
	};

	class BaseClassTypeInfo : public TypeInfo
	{
	public:
		bool IsClassType() override;

		virtual std::shared_ptr<TypeInfo> GetClassDeclarationTypeInfo(std::shared_ptr<SymbolTable> symbolTable) = 0;

		virtual bool IsValueType() = 0;

		virtual llvm::Value* GetDefaultValue(llvm::LLVMContext* context) override;
	};

	// Represents the TypeInfo for a class's declaration, which is equally valid for value or reference instantiations.
	// This TypeInfo is used for basic type compatibility without regards to value or reference types. It's not a legal
	// assignment type, and thus expressions may not evaluate to this kind of TypeInfo (they return the ClassTypeInfo instead)
	class ClassDeclaration;
	class ClassDeclarationTypeInfo : public TypeInfo
	{
	public:
		ClassDeclarationTypeInfo(const std::string& name, const std::string& fullyQualifiedName);

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override;

		virtual std::string FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable = nullptr);

		virtual std::shared_ptr<TypeInfo> EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override;

		virtual bool SupportsOperator(Operation* operation) override;

		virtual llvm::Value* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override;

		void BindType(llvm::Type* type);

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override;

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
		ClassTypeInfo(std::shared_ptr<TypeInfo> classDeclTypeInfo, bool valueType);

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override;

		virtual std::shared_ptr<TypeInfo> EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override;

		virtual bool SupportsOperator(Operation* operation) override;

		virtual llvm::Value* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override;

		bool IsValueType() override;

		std::shared_ptr<TypeInfo> GetClassDeclarationTypeInfo(std::shared_ptr<SymbolTable> /*symbolTable*/) override;

		virtual llvm::Value* GetDefaultValue(llvm::LLVMContext* context) override;

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override;

		virtual std::string FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable = nullptr) override;

		virtual std::string SerializedName(std::shared_ptr<SymbolTable> symbolTable) override;

	private:
		std::shared_ptr<TypeInfo> _classDeclTypeInfo;
		bool _valueType;
	};

	class Reference;
	class UnresolvedClassTypeInfo : public BaseClassTypeInfo
	{
	public:
		UnresolvedClassTypeInfo(const std::string& name, bool valueType);

		UnresolvedClassTypeInfo(const std::string& name);

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override;

		virtual std::string FullyQualifiedName(std::shared_ptr<SymbolTable> symbolTable = nullptr) override;

		virtual std::shared_ptr<TypeInfo> EvaluateOperation(std::shared_ptr<TypeInfo>& implicitCastTypeOut, Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override;

		virtual bool SupportsOperator(Operation* operation) override;
		
		virtual bool NeedsResolution() override;

		virtual llvm::Value* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override;

		void EnsureResolved(std::shared_ptr<SymbolTable> symbolTable); 
		
		bool IsValueType() override;

		std::shared_ptr<TypeInfo> GetClassDeclarationTypeInfo(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override;

		virtual std::string SerializedName(std::shared_ptr<SymbolTable> symbolTable) override;

	private:
		std::string _name;
		bool _valueType;
		std::shared_ptr<ClassTypeInfo> _resolvedType;
	};

	class IntegerConstant;
	class UnsafeArrayTypeInfo : public TypeInfo
	{
	public:
		UnsafeArrayTypeInfo(std::shared_ptr<TypeInfo> elementTypeInfo, std::shared_ptr<Ast::IntegerConstant> rank);

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual bool SupportsOperator(Operation* operation) override;

		virtual const std::string& Name() override;

		virtual llvm::Value* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput = false) override;

		virtual bool IsSameType(std::shared_ptr<TypeInfo> other) override;
		
		virtual bool IsNativeArrayType() override { return true; }

		std::shared_ptr<TypeInfo> GetElementTypeInfo() { return _elementTypeInfo; }

	private:
		std::shared_ptr<TypeInfo> _elementTypeInfo;
		std::shared_ptr<IntegerConstant> _rank;
		llvm::Type* _type;
		std::string _name;
	};

	// Used in the grammar, as a step towards resolving towards an actual type.
	// This is mostly necessary because the grammar demands we pass around raw pointers
	// and our primitive types are managed by shared pointers
	class TypeSpecifier
	{
	public:
		TypeSpecifier();

		TypeSpecifier(const std::string& name, bool valueType);

		TypeSpecifier(std::shared_ptr<TypeInfo> knownType);

		std::shared_ptr<TypeInfo> GetTypeInfo();

	private:
		std::shared_ptr<TypeInfo> _resolvedType;
	};
}