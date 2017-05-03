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
		Statement(FileLocation& location) : _location(location) { }
		virtual ~Statement() { }
		void TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr);
	protected:
		// Without the llvm types, TypeCheckInternal simply does a type check without generating any code. With the llvm parameters, it will also output generated code as it walks
		// the abstract syntax tree.
		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) = 0;
	
	private:
		FileLocation _location;
	};

	class GlobalStatement : public Statement, public std::enable_shared_from_this<GlobalStatement>
	{
	public:
		GlobalStatement(FileLocation& location) : Statement(location) { }

		virtual ~GlobalStatement() { }
	};

	class GlobalStatements : public GlobalStatement
	{
	public:
		GlobalStatements(GlobalStatement* stmt, GlobalStatements* list, FileLocation& location) :
			GlobalStatement(location),
			_stmt(stmt), _next(list)
		{}
		
		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		virtual std::string ToString() override { return "GlobalStatements"; }
		std::shared_ptr<GlobalStatement> _stmt;
		std::shared_ptr<GlobalStatements> _next;
	};

	class NamespaceDeclaration : public GlobalStatement
	{
	public:
		NamespaceDeclaration(const std::string& name, GlobalStatements* stmts, FileLocation& location) : 
			GlobalStatement(location),
			_name(name), _stmts(stmts)
		{}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		virtual std::string ToString() override { return "NamespaceDeclaration"; }
		const std::string _name;
		std::shared_ptr<GlobalStatements> _stmts;
	};

	class LineStatement : public Statement
	{
	public:
		LineStatement(FileLocation& location) : Statement(location) { }
		virtual ~LineStatement() {}
	};

	class LineStatements : public LineStatement
	{
	public:
		LineStatements(LineStatement* statement, FileLocation& location, LineStatements* list = nullptr) :
			LineStatement(location),
			_statement(statement), _next(list)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		virtual std::string ToString() override { return "LineStatements"; }
		std::shared_ptr<LineStatement> _statement;
		std::shared_ptr<LineStatements> _next;
	};

	class IfStatement : public LineStatement
	{
	public:
		IfStatement(Expression* condition, LineStatement* statement, FileLocation& location, LineStatement* elseStatement = nullptr) :
			LineStatement(location),
			_condition(condition), _statement(statement), _elseStatement(elseStatement)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		virtual std::string ToString() override { return "IfStatement"; }

		std::shared_ptr<Expression> _condition; 
		std::shared_ptr<LineStatement> _statement; 
		std::shared_ptr<LineStatement> _elseStatement;
	};

	class WhileStatement : public LineStatement
	{
	public:
		WhileStatement(Expression* condition, LineStatement* statement, FileLocation& location) :
			LineStatement(location),
			_condition(condition), _statement(statement)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		virtual std::string ToString() override { return "WhileStatement"; }

		std::shared_ptr<Expression> _condition;
		std::shared_ptr<LineStatement> _statement;
	};

	class BreakStatement : public LineStatement
	{
	public:
		BreakStatement(FileLocation& location) : LineStatement(location) { }

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		virtual std::string ToString() override
		{
			return "BreakStatement";
		}
	};

	class AssignFromSingle
	{
	public:
		AssignFromSingle(FileLocation& location) : _location(location) { }
		virtual ~AssignFromSingle() { }
		virtual std::shared_ptr<TypeInfo> Resolve(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhsTypeInfo, std::shared_ptr<Expression> rhsExpr) = 0;
		virtual void Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs) = 0;
		virtual llvm::Value* GetIRValue(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) = 0;
	protected:
		FileLocation _location;
	};

	class AssignFrom : public Node
	{
	public:
		AssignFrom(AssignFromSingle* thisOne, FileLocation& location, AssignFrom* next = nullptr) : _thisOne(thisOne), _next(next), _location(location)
		{
		}

		AssignFrom(AssignFromSingle* thisOne, AssignFrom* next = nullptr) : AssignFrom(thisOne, FileLocation(-1,-1), next) { }

		std::shared_ptr<TypeInfo> Resolve(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhsTypeInfo, std::shared_ptr<Expression> rhsExpr);

		void Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs);

		void CodeGen(std::shared_ptr<Expression> rhs, std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module);

		std::shared_ptr<AssignFromSingle> _thisOne;
		std::shared_ptr<TypeInfo> _thisType;
		std::shared_ptr<AssignFrom> _next;
		std::shared_ptr<TypeInfo> _nextType;
		FileLocation _location;
	};

	class AssignFromReference : public AssignFromSingle
	{
	public: 
		AssignFromReference(const std::string& ref, FileLocation& location) : AssignFromSingle(location), _ref(ref)
		{
		}

		virtual ~AssignFromReference() { }

		std::shared_ptr<TypeInfo> Resolve(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhsTypeInfo, std::shared_ptr<Expression> rhsExpr) override;

		void Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs) override
		{
			// Not necessary, it's already been bound to a type
		}

		virtual llvm::Value* GetIRValue(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) override;

		const std::string _ref;
	};

	class DeclareVariable : public AssignFromSingle
	{
	public:
		DeclareVariable(std::shared_ptr<TypeInfo> typeInfo, const std::string& name, FileLocation& location) 
			: AssignFromSingle(location), _typeInfo(typeInfo), _name(name)
		{
		}

		DeclareVariable(std::shared_ptr<TypeInfo> typeInfo, const std::string& name)
			: DeclareVariable(typeInfo, name, FileLocation(-1,-1))
		{
		}

		virtual ~DeclareVariable() { }

		std::shared_ptr<TypeInfo> Resolve(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhsTypeInfo, std::shared_ptr<Expression> rhsExpr) override;

		void Bind(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> rhs) override;

		virtual llvm::Value* GetIRValue(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) override;

		std::shared_ptr<TypeInfo> _typeInfo;
		const std::string _name;
	};

	class Assignment : public LineStatement
	{
	public:
		Assignment(AssignFrom* lhs, Expression* rhs, FileLocation& location, bool inInitializerList = false) :
			LineStatement(location),
			_lhs(lhs), _rhs(rhs), _inInitializerList(inInitializerList)
		{ 
		}

		Assignment(AssignFrom* lhs, Expression* rhs, bool inInitializerList = false) : Assignment(lhs, rhs, FileLocation(-1,-1), inInitializerList) { }

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		virtual std::string ToString() override
		{
			return "Assignment";
		}

		std::shared_ptr<AssignFrom> _lhs;
		std::shared_ptr<TypeInfo> _lhsTypeInfo;
		std::shared_ptr<Expression> _rhs;
		std::shared_ptr<TypeInfo> _rhsTypeInfo;
		bool _inInitializerList;
	};

	class ScopedStatements : public LineStatement
	{
	public:
		ScopedStatements(LineStatements* statements, FileLocation& location) : 
			LineStatement(location), _statements(statements)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		virtual std::string ToString() override { return "ScopedStatement"; }

		std::shared_ptr<LineStatements> _statements;
	};

	class ExpressionAsStatement : public LineStatement
	{
	public:
		ExpressionAsStatement(Expression* expr, FileLocation& location) : 
			LineStatement(location), _expr(expr)
		{
		}

		ExpressionAsStatement(Expression* expr) : ExpressionAsStatement(expr, FileLocation(-1,-1)) { }

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		virtual std::string ToString() override { return "ExpressionAsStatement"; }

		std::shared_ptr<Expression> _expr;
	};

	class ReturnStatement : public LineStatement
	{
	public:
		ReturnStatement(Expression* idList, FileLocation& location) : 
			LineStatement(location), _idList(idList)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		std::shared_ptr<Expression> _idList;
		std::shared_ptr<TypeInfo> _returnType;
	};
}