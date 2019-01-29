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
		Expression(std::shared_ptr<TypeInfo> typeInfo, FileLocation& location) : _typeInfo(typeInfo), _location(location) { }
		virtual ~Expression() { }

		std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList = false);

		llvm::Value* CodeGen(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr);

		virtual bool IsConstantExpression()
		{
			return false;
		}
		std::shared_ptr<TypeInfo> _typeInfo;

		std::shared_ptr<Ast::SymbolTable::SymbolBinding> SymbolBinding()
		{
			return _symbolBinding;
		}

	protected:
		FileLocation _location;
		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList) = 0;
		virtual llvm::Value* CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) = 0;
		std::shared_ptr<Ast::SymbolTable::SymbolBinding> _symbolBinding;
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
		virtual llvm::Value* CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

	private:
		const std::string _id;
	};

	class PeriodSeparatedId : public Expression
	{
	public:
		PeriodSeparatedId(const std::string& id, FileLocation& location) : Expression(location), _id(id)
		{
		}

		PeriodSeparatedId(const std::string& lhs, const std::string& rhs, FileLocation& location) : Expression(location), _id(lhs + "." + rhs)
		{
		}

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList) override;

		std::string Id()
		{
			return _id;
		}

	protected:
		virtual llvm::Value* CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

	private:
		const std::string _id;
	};

	class DereferencedExpression : public Expression
	{
	public:
		DereferencedExpression(Expression* expr, const std::string& id, FileLocation& location) : Expression(location), _expr(expr), _id(id)
		{
		}

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList) override;

		bool ExpectFunction = false;
	protected:
		virtual llvm::Value* CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

	private:
		std::shared_ptr<Expression> _expr;
		std::shared_ptr<TypeInfo> _exprTypeInfo;
		const std::string _id;
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
		virtual llvm::Value* CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
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
		virtual llvm::Value* CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;
	};

}