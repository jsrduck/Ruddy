#pragma once

#include "Node.h"

#include <memory>
#include "TypeInfo.h"
#include "SymbolTable.h"

namespace Ast
{
	class Expression : public Node
	{
	public:
		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) = 0;

		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) = 0;
	};

	class Reference : public Expression
	{
	public:
		Reference(const std::string& id) : _id(id)
		{
		}

		Reference(const std::string& left, const std::string& right) : _id(left + "." + right)
		{
		}

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

		std::string Id()
		{
			return _id;
		}

	private:
		const std::string _id;
		std::shared_ptr<SymbolTable::SymbolBinding> _symbol;
	};

	class ExpressionList : public Expression
	{
	public:
		ExpressionList(Expression* left, Expression* right = nullptr) : _left(left), _right(right)
		{
		}

		virtual std::string ToString() override { return "ExpressionList"; }

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

		std::shared_ptr<Expression> _left;
		std::shared_ptr<Expression> _right;
	};

	class DebugPrintStatement : public Expression
	{
	public:
		DebugPrintStatement(Expression* expression) :
			_expression(expression)
		{
		}

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

		std::shared_ptr<Expression> _expression;
		std::shared_ptr<TypeInfo> _expressionTypeInfo;
	};

}