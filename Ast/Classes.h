#pragma once

#include "Statements.h"
#include "Expressions.h"

namespace Ast
{
	class ClassStatement : public Statement, public std::enable_shared_from_this<ClassStatement>
	{
	};

	class Modifier : public Node
	{
	public:
		enum class Modifiers
		{
			NONE		= 0x0,
			STATIC		= 0x1
		};

		Modifier(Modifiers mods) : _mods(mods)
		{
		}

		Modifiers Get()
		{
			return _mods;
		}

		bool IsStatic()
		{
			return (int)_mods & (int)Modifiers::STATIC;
		}

	private:
		Modifiers _mods;
	};

	class ClassMemberDeclaration : public ClassStatement
	{
	public:
		ClassMemberDeclaration(Visibility visibility, Modifier* modifiers, std::shared_ptr<TypeInfo> typeInfo, const std::string& name,
			ConstantExpression* defaultValue = nullptr) :
			_visibility(visibility),
			_mods(modifiers),
			_typeInfo(typeInfo),
			_name(name),
			_defaultValue(defaultValue)
		{}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		Visibility _visibility;
		std::shared_ptr<Modifier> _mods;
		std::shared_ptr<TypeInfo> _typeInfo; 
		const std::string _name;
		std::shared_ptr<ConstantExpression> _defaultValue;
	};

	class Argument : public Node
	{
	public:
		Argument(std::shared_ptr<TypeInfo> typeInfo, const std::string& name) : _typeInfo(typeInfo), _name(name)
		{}

		std::shared_ptr<TypeInfo> _typeInfo;
		const std::string _name;
	};

	class ArgumentList : public Node
	{
	public:
		ArgumentList(Argument* argument, ArgumentList* list = nullptr) : _argument(argument), _next(list)
		{
		}

		std::shared_ptr<Argument> _argument;
		std::shared_ptr<ArgumentList> _next;
	};

	class FunctionDeclaration : public ClassStatement
	{
	public:
		FunctionDeclaration(Visibility visibility, Modifier* modifiers, ArgumentList* returnArgs, 
			const std::string& name, ArgumentList* inputArgs, LineStatement* body) :
			_visibility(visibility), _mods(modifiers), _returnArgs(returnArgs), _name(name), _inputArgs(inputArgs), _body(body)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		Visibility _visibility; 
		std::shared_ptr<Modifier> _mods;
		std::shared_ptr<ArgumentList> _returnArgs;
		const std::string _name;
		std::shared_ptr<ArgumentList> _inputArgs; 
		std::shared_ptr<LineStatement> _body;
	};

	class ConstructorDeclaration : public FunctionDeclaration
	{
	public:
		ConstructorDeclaration(Visibility visibility, const std::string& name,
			ArgumentList* inputArgs, LineStatement* body) : FunctionDeclaration(visibility, new Modifier(Modifier::Modifiers::NONE), nullptr, name, inputArgs, body)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;
	};

	class DestructorDeclaration : public FunctionDeclaration
	{
	public:
		DestructorDeclaration(const std::string& name, LineStatement* body) : FunctionDeclaration(Visibility::PRIVATE, new Modifier(Modifier::Modifiers::NONE), nullptr, name, nullptr, body)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;
	};

	class ClassStatementList : public Statement
	{
	public:
		ClassStatementList(ClassStatement* statement, ClassStatementList* list) :
			_statement(statement), _next(list)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		std::shared_ptr<ClassStatement> _statement;
		std::shared_ptr<ClassStatementList> _next;
	};

	class ClassDeclaration : public GlobalStatement
	{
	public:
		ClassDeclaration(Visibility visibility, const std::string& name, ClassStatementList* list) :
			_visibility(visibility), _name(name), _list(list)
		{
		}

		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder = nullptr, llvm::LLVMContext* context = nullptr, llvm::Module * module = nullptr) override;

		Visibility _visibility;
		const std::string _name;
		std::shared_ptr<ClassStatementList> _list;
	};

	class NewExpression : public Expression
	{
	public:
		NewExpression(const std::string& className, Expression* expression) :
			_className(className), _expression(expression)
		{
		}

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}

		const std::string _className;
		std::shared_ptr<Expression> _expression;
	};

	class FunctionCall : public Expression
	{
	public:
		FunctionCall(const std::string& name, Expression* expression) :
			_name(name), _expression(expression)
		{
		}

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override;

		virtual llvm::Value* CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}

		const std::string _name;
		std::shared_ptr<Expression> _expression;
	};
}