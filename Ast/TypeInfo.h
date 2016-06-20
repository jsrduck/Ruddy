#pragma once
#include <memory>
#include <string>
#include "Node.h"
#include "TypeExceptions.h"

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
		virtual std::shared_ptr<TypeInfo> EvaluateOperation(Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr);
		virtual bool SupportsOperator(Operation* operation) = 0;
		virtual bool IsAutoType() { return false; }
		virtual bool NeedsResolution() { return false; }

	/*	virtual void CodegenAdd(AddOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenSubtract(SubtractOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenMultiply(MultiplyOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenDivide(DivideOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenRemainder(RemainderOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenGTOE(GreaterThanOrEqualOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenLTOE(LessThanOrEqualOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenGT(GreaterThanOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenLT(LessThanOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenET(EqualToOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenNET(NotEqualToOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenLogicalAnd(LogicalAndOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenLogicalOr(LogicalOrOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenBitwiseAnd(BitwiseAndOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenBitwiseOr(BitwiseOrOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenBitwiseXor(BitwiseXorOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenSHL(BitwiseShiftLeftOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
		virtual void CodegenSHR(BitwiseShiftRightOperation* operation, Object* lhs, Object* rhs) { throw OperationNotDefinedException(operation->OperatorString()); }
	*/
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

		// Operator logic
		virtual std::shared_ptr<TypeInfo> EvaluateOperation(Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override { throw NotSupportedByAutoTypeException(); }
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

		std::shared_ptr<TypeInfo> InputArgsType() { return _inputArgs; }
		std::shared_ptr<TypeInfo> OutputArgsType() { return _outputArgs; }
		std::shared_ptr<TypeInfo> _inputArgs;
		std::shared_ptr<TypeInfo> _outputArgs;
		std::string _name;
	};

	class ClassDeclaration;
	class ClassTypeInfo : public TypeInfo
	{
	public:
		ClassTypeInfo(std::shared_ptr<ClassDeclaration> classDeclaration);

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return true;
		}

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override
		{
			return _name;
		}

		virtual std::shared_ptr<TypeInfo> EvaluateOperation(Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override;

		virtual bool SupportsOperator(Operation* operation) override;

	private:
		std::string _name;
	};

	class Reference;
	class UnresolvedClassTypeInfo : public TypeInfo
	{
	public:
		UnresolvedClassTypeInfo(Reference* name);

		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override;

		virtual const std::string& Name() override
		{
			return _name;
		}

		virtual std::shared_ptr<TypeInfo> EvaluateOperation(Operation* operation, std::shared_ptr<TypeInfo> rhs = nullptr, std::shared_ptr<SymbolTable> symbolTable = nullptr) override;

		virtual bool SupportsOperator(Operation* operation) override;
		
		virtual bool NeedsResolution() override
		{
			return true;
		}

	private:
		std::string _name;
	};

}