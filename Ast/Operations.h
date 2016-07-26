#pragma once

#include "Expressions.h"
#include "Primitives.h"
#include <string>

namespace Ast
{
	class Operation : public Expression
	{
	public:
		virtual std::string OperatorString() = 0;
		virtual const int OperatorId() = 0;
		virtual bool IsBinary() = 0;
		virtual bool IsArithmetic() { return false; }
		virtual bool IsLogical() { return false; }
		virtual bool IsComparison() { return false; }
		virtual bool IsBoolean() { return false; }
		virtual bool IsBitwise() { return false; }
		virtual bool IsShift() { return false; }
	};

	/* Binary Operations */

	class BinaryOperation : public Operation
	{
	public:
		BinaryOperation(Expression* lhs, Expression* rhs) : _lhs(lhs), _rhs(rhs), _resultIsSigned(false) {}
		virtual bool IsBinary() override { return true; }
		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable) override
		{
			_rhsTypeInfo = _rhs->Evaluate(symbolTable);
			_lhsTypeInfo = _lhs->Evaluate(symbolTable);
			if (_rhsTypeInfo->IsConstant() && _lhsTypeInfo->IsConstant())
			{
				// They both end up evaluating to constant types, which isn't that helpful.
				// Try and make them resolve to best fit types.
				_rhsTypeInfo = std::dynamic_pointer_cast<ConstantExpression>(_rhs)->BestFitTypeInfo();
				_lhsTypeInfo = std::dynamic_pointer_cast<ConstantExpression>(_lhs)->BestFitTypeInfo();
			}

			_resultOfOperation = _lhsTypeInfo->EvaluateOperation(_implicitCastType, this, _rhsTypeInfo, symbolTable);
			if (_resultOfOperation->IsInteger())
			{
				_resultIsSigned = std::dynamic_pointer_cast<IntegerTypeInfo>(_resultOfOperation)->Signed();
			}
			else if (_lhsTypeInfo->IsInteger() && _rhsTypeInfo->IsInteger())
			{
				_resultIsSigned = std::dynamic_pointer_cast<IntegerTypeInfo>(_implicitCastType)->Signed();
			}
			return _resultOfOperation;
		}
	protected:
		std::shared_ptr<Expression> _lhs;
		std::shared_ptr<TypeInfo> _lhsTypeInfo;
		std::shared_ptr<Expression> _rhs;
		std::shared_ptr<TypeInfo> _rhsTypeInfo;
		std::shared_ptr<TypeInfo> _resultOfOperation;
		std::shared_ptr<TypeInfo> _implicitCastType;
		bool _resultIsSigned;
	};

	/* Arithmetic Binary Operations */

	class ArithmeticBinaryOperation : public BinaryOperation
	{
	public:
		ArithmeticBinaryOperation(Expression* lhs, Expression* rhs) : BinaryOperation(lhs,rhs) {}
		virtual bool IsArithmetic() override { return true; }
	};

	class AddOperation : public ArithmeticBinaryOperation
	{
	public:
		AddOperation(Expression* lhs, Expression* rhs) : ArithmeticBinaryOperation(lhs, rhs) {}
		static const int Id = 0x1;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "+"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class SubtractOperation : public ArithmeticBinaryOperation
	{
	public:
		SubtractOperation(Expression* lhs, Expression* rhs) : ArithmeticBinaryOperation(lhs, rhs) {}
		static const int Id = 0x2;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "-"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class MultiplyOperation : public ArithmeticBinaryOperation
	{
	public:
		MultiplyOperation(Expression* lhs, Expression* rhs) : ArithmeticBinaryOperation(lhs, rhs) {}
		static const int Id = 0x4;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "*"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class DivideOperation : public ArithmeticBinaryOperation
	{
	public:
		DivideOperation(Expression* lhs, Expression* rhs) : ArithmeticBinaryOperation(lhs, rhs) {}
		static const int Id = 0x8;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "/"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class RemainderOperation : public ArithmeticBinaryOperation
	{
	public:
		RemainderOperation(Expression* lhs, Expression* rhs) : ArithmeticBinaryOperation(lhs, rhs) {}
		static const int Id = 0x10;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "%"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	/* Logical Binary Comparison Operations */

	class LogicalBinaryOperation : public BinaryOperation
	{
	public:
		LogicalBinaryOperation(Expression* lhs, Expression* rhs) : BinaryOperation(lhs, rhs) {}
		virtual bool IsLogical() override { return true; }
	};

	class LogicalBinaryComparisonOperation : public LogicalBinaryOperation
	{
	public:
		LogicalBinaryComparisonOperation(Expression* lhs, Expression* rhs) : LogicalBinaryOperation(lhs, rhs) {}
		virtual bool IsComparison() override { return true; }
	};

	class GreaterThanOrEqualOperation : public LogicalBinaryComparisonOperation
	{
	public:
		GreaterThanOrEqualOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x20;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return ">="; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class LessThanOrEqualOperation : public LogicalBinaryComparisonOperation
	{
	public:
		LessThanOrEqualOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x40;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "<="; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class GreaterThanOperation : public LogicalBinaryComparisonOperation
	{
	public:
		GreaterThanOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x80;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return ">"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class LessThanOperation : public LogicalBinaryComparisonOperation
	{
	public:
		LessThanOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x100;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "<"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class EqualToOperation : public LogicalBinaryComparisonOperation
	{
	public:
		EqualToOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x200;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "=="; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class NotEqualToOperation : public LogicalBinaryComparisonOperation
	{
	public:
		NotEqualToOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x400;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "!="; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	/* Logical Binary Boolean Operations */
	class LogicalBinaryBooleanOperation : public LogicalBinaryOperation
	{
	public:
		LogicalBinaryBooleanOperation(Expression* lhs, Expression* rhs) : LogicalBinaryOperation(lhs, rhs) {}
		virtual bool IsBoolean() { return true; }
	};

	class LogicalAndOperation : public LogicalBinaryBooleanOperation
	{
	public:
		LogicalAndOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBooleanOperation(lhs, rhs) {}
		static const int Id = 0x800;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "&&"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class LogicalOrOperation : public LogicalBinaryBooleanOperation
	{
	public:
		LogicalOrOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBooleanOperation(lhs, rhs) {}
		static const int Id = 0x1000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "||"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	/* Logical Binary Bitwise Operations */
	class LogicalBinaryBitwiseOperation : public LogicalBinaryOperation
	{
	public:
		LogicalBinaryBitwiseOperation(Expression* lhs, Expression* rhs) : LogicalBinaryOperation(lhs, rhs) {}
		virtual bool IsBitwise() { return true; }
	};

	class BitwiseAndOperation : public LogicalBinaryBitwiseOperation
	{
	public:
		BitwiseAndOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBitwiseOperation(lhs, rhs) {}
		static const int Id = 0x2000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "&"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class BitwiseOrOperation : public LogicalBinaryBitwiseOperation
	{
	public:
		BitwiseOrOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBitwiseOperation(lhs, rhs) {}
		static const int Id = 0x4000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "|"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class BitwiseXorOperation : public LogicalBinaryBitwiseOperation
	{
	public:
		BitwiseXorOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBitwiseOperation(lhs, rhs) {}
		static const int Id = 0x8000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "^"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class BitwiseShiftLeftOperation : public LogicalBinaryBitwiseOperation
	{
	public:
		BitwiseShiftLeftOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBitwiseOperation(lhs, rhs) {}
		static const int Id = 0x10000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "<<"; }
		virtual bool IsShift() override { return true; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class BitwiseShiftRightOperation : public LogicalBinaryBitwiseOperation
	{
	public:
		BitwiseShiftRightOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBitwiseOperation(lhs, rhs) {}
		static const int Id = 0x20000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "|"; }
		virtual bool IsShift() override { return true; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class UnaryOperation : public Operation
	{
	public:
		UnaryOperation(Expression* expr) : _expr(expr) {}
		virtual bool IsBinary() override { return false; }

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable) override
		{
			_typeInfo = _expr->Evaluate(symbolTable);
			if (_typeInfo->IsConstant())
			{
				_typeInfo = std::dynamic_pointer_cast<ConstantExpression>(_expr)->BestFitTypeInfo();
			}
			auto result = _typeInfo->EvaluateOperation(_implicitCastType, this, nullptr, symbolTable);

			return result;
		}

	protected:
		std::shared_ptr<Expression> _expr;
		std::shared_ptr<TypeInfo> _typeInfo;
		std::shared_ptr<TypeInfo> _implicitCastType;
	};

	class PrefixOperation : public UnaryOperation
	{
	public:
		PrefixOperation(Expression* expr) : UnaryOperation(expr) {}
	};

	class PostOperation : public UnaryOperation
	{
	public:
		PostOperation(Expression* expr) : UnaryOperation(expr) {}
	};

	class ArithmeticPostUnaryOperation : public PostOperation
	{
	public:
		ArithmeticPostUnaryOperation(Expression* expr) : PostOperation(expr) {}
		virtual bool IsArithmetic() override { return true; }
	};

	class PostIncrementOperation : public ArithmeticPostUnaryOperation
	{
	public:
		PostIncrementOperation(Expression* expr) : ArithmeticPostUnaryOperation(expr) {}
		static const int Id = 0x40000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "++"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class PostDecrementOperation : public ArithmeticPostUnaryOperation
	{
	public:
		PostDecrementOperation(Expression* expr) : ArithmeticPostUnaryOperation(expr) {}
		static const int Id = 0x80000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "--"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class ArithmeticPreUnaryOperation : public PrefixOperation
	{
	public:
		ArithmeticPreUnaryOperation(Expression* expr) : PrefixOperation(expr) {}
		virtual bool IsArithmetic() override { return true; }
	};

	class PreIncrementOperation : public ArithmeticPreUnaryOperation
	{
	public:
		PreIncrementOperation(Expression* expr) : ArithmeticPreUnaryOperation(expr) {}
		static const int Id = 0x400000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "++"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class PreDecrementOperation : public ArithmeticPreUnaryOperation
	{
	public:
		PreDecrementOperation(Expression* expr) : ArithmeticPreUnaryOperation(expr) {}
		static const int Id = 0x800000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "--"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class LogicalPreUnaryOperation : public PrefixOperation
	{
	public:
		LogicalPreUnaryOperation(Expression* expr) : PrefixOperation(expr) {}
		virtual bool IsLogical() override { return true; }
	};

	class NegateOperation : public LogicalPreUnaryOperation
	{
	public:
		NegateOperation(Expression* expr) : LogicalPreUnaryOperation(expr) {}
		virtual bool IsBoolean() { return false; }
		static const int Id = 0x100000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "!"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class BitwisePreUnaryOperation : public LogicalPreUnaryOperation
	{
	public:
		BitwisePreUnaryOperation(Expression* expr) : LogicalPreUnaryOperation(expr) {}
		virtual bool IsBitwise() { return true; }
	};

	class ComplementOperation : public BitwisePreUnaryOperation
	{
	public:
		ComplementOperation(Expression* expr) : BitwisePreUnaryOperation(expr) {}
		static const int Id = 0x200000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "~"; }

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class CastOperation : public Operation
	{
	public:
		CastOperation(std::shared_ptr<TypeInfo> typeInfo, Expression* expression) : _castTo(typeInfo), _expression(expression)
		{
		}

		static const int Id = 0x400000;

		virtual const int OperatorId() override
		{
			return Id;
		}

		virtual std::string OperatorString() override
		{
			return "(T)";
		}

		virtual bool IsBinary() override
		{
			return false;
		}

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable) override
		{
			// TODO: What about types that can't be cast at all?
			_castFrom = _expression->Evaluate(symbolTable);
			return _castTo;
		}

		std::shared_ptr<TypeInfo> _castTo;
		std::shared_ptr<Expression> _expression;
		std::shared_ptr<TypeInfo> _castFrom;

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};
}