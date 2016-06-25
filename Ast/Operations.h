#pragma once

#include "Expressions.h"
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
		virtual bool IsAssignment() { return false; }
	};

	/* Binary Operations */

	class BinaryOperation : public Operation
	{
	public:
		BinaryOperation(Expression* lhs, Expression* rhs) : _lhs(lhs), _rhs(rhs) {}
		virtual bool IsBinary() override { return true; }
		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override
		{
			_lhsTypeInfo = _lhs->Evaluate(symbolTable);
			_rhsTypeInfo = _rhs->Evaluate(symbolTable);
			_resultOfOperation = _lhsTypeInfo->EvaluateOperation(this, _rhsTypeInfo);
			return _resultOfOperation;
		}
	protected:
		std::shared_ptr<Expression> _lhs;
		std::shared_ptr<TypeInfo> _lhsTypeInfo;
		std::shared_ptr<Expression> _rhs;
		std::shared_ptr<TypeInfo> _rhsTypeInfo;
		std::shared_ptr<TypeInfo> _resultOfOperation;
	};

	class AssignmentOperation : public BinaryOperation
	{
	public:
		virtual bool IsAssignment() { return true; }
		virtual std::string OperatorString() override { return "="; }

		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}
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
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class SubtractOperation : public ArithmeticBinaryOperation
	{
	public:
		SubtractOperation(Expression* lhs, Expression* rhs) : ArithmeticBinaryOperation(lhs, rhs) {}
		static const int Id = 0x2;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "-"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class MultiplyOperation : public ArithmeticBinaryOperation
	{
	public:
		MultiplyOperation(Expression* lhs, Expression* rhs) : ArithmeticBinaryOperation(lhs, rhs) {}
		static const int Id = 0x4;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "*"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class DivideOperation : public ArithmeticBinaryOperation
	{
	public:
		DivideOperation(Expression* lhs, Expression* rhs) : ArithmeticBinaryOperation(lhs, rhs) {}
		static const int Id = 0x8;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "/"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class RemainderOperation : public ArithmeticBinaryOperation
	{
	public:
		RemainderOperation(Expression* lhs, Expression* rhs) : ArithmeticBinaryOperation(lhs, rhs) {}
		static const int Id = 0x10;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "%"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
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
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class LessThanOrEqualOperation : public LogicalBinaryComparisonOperation
	{
	public:
		LessThanOrEqualOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x40;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "<="; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class GreaterThanOperation : public LogicalBinaryComparisonOperation
	{
	public:
		GreaterThanOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x80;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return ">"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class LessThanOperation : public LogicalBinaryComparisonOperation
	{
	public:
		LessThanOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x100;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "<"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class EqualToOperation : public LogicalBinaryComparisonOperation
	{
	public:
		EqualToOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x200;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "=="; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class NotEqualToOperation : public LogicalBinaryComparisonOperation
	{
	public:
		NotEqualToOperation(Expression* lhs, Expression* rhs) : LogicalBinaryComparisonOperation(lhs, rhs) {}
		static const int Id = 0x400;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "!="; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
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
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}
	};

	class LogicalOrOperation : public LogicalBinaryBooleanOperation
	{
	public:
		LogicalOrOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBooleanOperation(lhs, rhs) {}
		static const int Id = 0x1000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "&&"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}
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
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}
	};

	class BitwiseOrOperation : public LogicalBinaryBitwiseOperation
	{
	public:
		BitwiseOrOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBitwiseOperation(lhs, rhs) {}
		static const int Id = 0x4000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "|"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}
	};

	class BitwiseXorOperation : public LogicalBinaryBitwiseOperation
	{
	public:
		BitwiseXorOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBitwiseOperation(lhs, rhs) {}
		static const int Id = 0x8000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "^"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}
	};

	class BitwiseShiftLeftOperation : public LogicalBinaryBitwiseOperation
	{
	public:
		BitwiseShiftLeftOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBitwiseOperation(lhs, rhs) {}
		static const int Id = 0x10000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "<<"; }
		virtual bool IsShift() override { return true; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}
	};

	class BitwiseShiftRightOperation : public LogicalBinaryBitwiseOperation
	{
	public:
		BitwiseShiftRightOperation(Expression* lhs, Expression* rhs) : LogicalBinaryBitwiseOperation(lhs, rhs) {}
		static const int Id = 0x20000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "|"; }
		virtual bool IsShift() override { return true; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}
	};

	class UnaryOperation : public Operation
	{
	public:
		UnaryOperation(Expression* expr) : _expr(expr) {}
		virtual bool IsBinary() override { return false; }

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _expr->Evaluate(symbolTable);
		}

	protected:
		std::shared_ptr<Expression> _expr;
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
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class PostDecrementOperation : public ArithmeticPostUnaryOperation
	{
	public:
		PostDecrementOperation(Expression* expr) : ArithmeticPostUnaryOperation(expr) {}
		static const int Id = 0x80000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "--"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
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
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
	};

	class PreDecrementOperation : public ArithmeticPreUnaryOperation
	{
	public:
		PreDecrementOperation(Expression* expr) : ArithmeticPreUnaryOperation(expr) {}
		static const int Id = 0x800000;
		virtual const int OperatorId() override { return Id; }
		virtual std::string OperatorString() override { return "--"; }
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
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
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
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
		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}
	};
}