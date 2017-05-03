#pragma once

#include "FileLocationContext.h"
#include "Node.h"

#include <memory>
#include "TypeInfo.h"
#include "SymbolTable.h"

namespace Ast
{
	class Expression : public Node
	{
	public:
		Expression(FileLocation& location) : _location(location) { }
		virtual ~Expression() { }

		std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList = false);

		llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr);

		virtual bool IsConstantExpression()
		{
			return false;
		}
		std::shared_ptr<TypeInfo> _typeInfo;

	protected:
		FileLocation _location;
		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList) = 0;
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) = 0;
	};

	class Reference : public Expression
	{
	public:
		Reference(const std::string& id, FileLocation& location) : Expression(location), _id(id)
		{
		}

		Reference(const std::string& id) : Reference(id, FileLocation(-1,-1))
		{
		}

		Reference(const std::string& left, const std::string& right, FileLocation& location) : Expression(location), _id(left + "." + right)
		{
		}

		virtual ~Reference() { }

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList) override;

		std::string Id()
		{
			return _id;
		}

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

	private:
		const std::string _id;
		std::shared_ptr<SymbolTable::SymbolBinding> _symbol;
	};

	class ExpressionList : public Expression
	{
	public:
		ExpressionList(Expression* left, FileLocation& location, Expression* right = nullptr) : Expression(location), _left(left), _right(right)
		{
		}

		virtual ~ExpressionList() { }

		virtual std::string ToString() override { return "ExpressionList"; }

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList) override;

		std::shared_ptr<Expression> _left;
		std::shared_ptr<Expression> _right;

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

	class DebugPrintStatement : public Expression
	{
	public:
		DebugPrintStatement(Expression* expression, FileLocation& location) :
			Expression(location),
			_expression(expression)
		{
		}

		virtual ~DebugPrintStatement() { }

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList) override;

		std::shared_ptr<Expression> _expression;
		std::shared_ptr<TypeInfo> _expressionTypeInfo;

	protected:
		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

}