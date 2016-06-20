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

		std::string Id()
		{
			return _id;
		}

	private:
		const std::string _id;
	};

	class ExpressionList : public Expression
	{
	public:
		ExpressionList(Expression* left, Expression* right = nullptr) : _left(left), _right(right)
		{
		}

		virtual std::string ToString() override { return "ExpressionList"; }

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override;

		std::shared_ptr<Expression> _left;
		std::shared_ptr<Expression> _right;
	};

}