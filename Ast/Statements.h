#pragma once

#include "Node.h"

#include <memory>
#include "TypeInfo.h"
#include "Primitives.h"
#include "SymbolTable.h"
#include "Expressions.h"
#include "TypeExceptions.h"

namespace Ast
{
	class VisibilityNode : public Node
	{
	public:
		VisibilityNode(Visibility vis) : _vis(vis) {}
		Visibility Get() { return _vis; }
		virtual std::string ToString() override { return "Visibility"; }
	private:
		Visibility _vis;
	};

	class FunctionDeclaration;
	class Statement : public Node
	{
	public:
		virtual ~Statement() { }
		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable) = 0;
	};

	class GlobalStatement : public Statement, public std::enable_shared_from_this<GlobalStatement>
	{
	public:
		virtual ~GlobalStatement() { }
	};

	class GlobalStatements : public GlobalStatement
	{
	public:
		GlobalStatements(GlobalStatement* stmt, GlobalStatements* list) :
			_stmt(stmt), _next(list)
		{}
		
		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual std::string ToString() override { return "GlobalStatements"; }
		std::shared_ptr<GlobalStatement> _stmt;
		std::shared_ptr<GlobalStatements> _next;
	};

	class NamespaceDeclaration : public GlobalStatement
	{
	public:
		NamespaceDeclaration(const std::string& name, GlobalStatements* stmts) : _name(name), _stmts(stmts)
		{}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual std::string ToString() override { return "NamespaceDeclaration"; }
		const std::string _name;
		std::shared_ptr<GlobalStatements> _stmts;
	};

	class LineStatement : public Statement
	{
	public:
		virtual ~LineStatement() {}
	};

	class LineStatements : public LineStatement
	{
	public:
		LineStatements(LineStatement* statement, LineStatements* list = nullptr) :
			_statement(statement), _next(list)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual std::string ToString() override { return "LineStatements"; }
		std::shared_ptr<LineStatement> _statement;
		std::shared_ptr<LineStatements> _next;
	};

	class IfStatement : public LineStatement
	{
	public:
		IfStatement(Expression* condition, LineStatement* statement, LineStatement* elseStatement = nullptr) :
			_condition(condition), _statement(statement), _elseStatement(elseStatement)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual std::string ToString() override { return "IfStatement"; }

		std::shared_ptr<Expression> _condition; 
		std::shared_ptr<LineStatement> _statement; 
		std::shared_ptr<LineStatement> _elseStatement;
	};

	class WhileStatement : public LineStatement
	{
	public:
		WhileStatement(Expression* condition, LineStatement* statement) :
			_condition(condition), _statement(statement)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual std::string ToString() override { return "WhileStatement"; }

		std::shared_ptr<Expression> _condition;
		std::shared_ptr<LineStatement> _statement;
	};

	class AssignFromSingle
	{
	public:
		virtual std::shared_ptr<TypeInfo> Resolve(std::shared_ptr<SymbolTable> symbolTable) = 0;
		virtual void Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs) = 0;
	};

	class AssignFrom : public Node
	{
	public:
		AssignFrom(AssignFromSingle* thisOne, AssignFrom* next = nullptr) : _thisOne(thisOne), _next(next)
		{
		}

		std::shared_ptr<TypeInfo> Resolve(std::shared_ptr<SymbolTable> symbolTable)
		{
			auto firstType = _thisOne->Resolve(symbolTable);
			if (_next == nullptr)
				return firstType;
			auto next = _next->Resolve(symbolTable);
			auto nextAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(next);
			if (nextAsComposite == nullptr)
				nextAsComposite = std::make_shared<CompositeTypeInfo>(next);
			return std::make_shared<CompositeTypeInfo>(firstType, nextAsComposite);
		}

		void Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs)
		{
			if (_next == nullptr)
			{
				auto rhsAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(rhs);
				if (rhsAsComposite != nullptr)
				{
					// It better be a single type
					if (rhsAsComposite->_next != nullptr)
						throw TypeMismatchException(_thisOne->Resolve(symbolTable), rhs);
					_thisOne->Bind(symbolTable, rhsAsComposite->_thisType);
				}
				else
				{
					_thisOne->Bind(symbolTable, rhs);
				}
			}
			else
			{
				auto rhsAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(rhs);
				if (rhsAsComposite == nullptr)
					throw TypeMismatchException(Resolve(symbolTable), rhs);
				_thisOne->Bind(symbolTable, rhsAsComposite->_thisType);
				_next->Bind(symbolTable, rhsAsComposite->_next);
			}
		}

	protected:
		std::shared_ptr<AssignFromSingle> _thisOne;
		std::shared_ptr<AssignFrom> _next;
	};

	class AssignFromReference : public AssignFromSingle
	{
	public: 
		AssignFromReference(const std::string& ref) : _ref(ref)
		{
		}

		std::shared_ptr<TypeInfo> Resolve(std::shared_ptr<SymbolTable> symbolTable) override;

		void Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs) override
		{
			// Not necessary, it's already been bound to a type
		}

		const std::string _ref;
	};

	class DeclareVariable : public AssignFromSingle
	{
	public:
		DeclareVariable(TypeInfo* typeInfo, const std::string& name) 
			: _typeInfo(typeInfo), _name(name)
		{
		}

		std::shared_ptr<TypeInfo> Resolve(std::shared_ptr<SymbolTable> symbolTable) override;

		void Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs) override;

		std::shared_ptr<TypeInfo> _typeInfo;
		const std::string _name;
	};

	class Assignment : public LineStatement
	{
	public:
		Assignment(AssignFrom* lhs, Expression* rhs) :
			_lhs(lhs), _rhs(rhs)
		{ 
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual std::string ToString() override
		{
			return "Assignment";
		}

		std::shared_ptr<AssignFrom> _lhs;
		std::shared_ptr<Expression> _rhs;
	};

	class ScopedStatements : public LineStatement
	{
	public:
		ScopedStatements(LineStatements* statements) : _statements(statements)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual std::string ToString() override { return "ScopedStatement"; }

		std::shared_ptr<LineStatements> _statements;
	};

	class ExpressionAsStatement : public LineStatement
	{
	public:
		ExpressionAsStatement(Expression* expr) : _expr(expr)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual std::string ToString() override { return "ExpressionAsStatement"; }

		std::shared_ptr<Expression> _expr;
	};

	class ReturnStatement : public LineStatement
	{
	public:
		ReturnStatement(Expression* idList) : _idList(idList)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable) override;

		std::shared_ptr<Expression> _idList;
	};
}