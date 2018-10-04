#pragma once

#include "Statements.h"
#include "Expressions.h"

namespace llvm {
class FunctionType;
class Function;
}

namespace Ast
{
	class ClassStatement : public Statement, public std::enable_shared_from_this<ClassStatement>
	{
	public:
		ClassStatement(FileLocation& location) : Statement(location) { }
		virtual void TypeCheck(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass = CLASS_AND_NAMESPACE_DECLARATIONS) override;
		std::shared_ptr<Ast::SymbolTable::ClassBinding> _classBinding;
	};

	class ClassMemberDeclaration : public ClassStatement
	{
	public:
		ClassMemberDeclaration(Visibility visibility, Modifier* modifiers, std::shared_ptr<TypeInfo> typeInfo, const std::string& name, FileLocation& location,
			ConstantExpression* defaultValue = nullptr) :
			ClassStatement(location),
			_visibility(visibility),
			_mods(modifiers),
			_typeInfo(typeInfo),
			_name(name),
			_defaultValue(defaultValue)
		{}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass) override;
		virtual void CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		Visibility _visibility;
		std::shared_ptr<Modifier> _mods;
		std::shared_ptr<TypeInfo> _typeInfo; 
		const std::string _name;
		std::shared_ptr<ConstantExpression> _defaultValue;
	};

	class Argument : public Node
	{
	public:
		Argument(std::shared_ptr<TypeInfo> typeInfo, const std::string& name) : 
			_typeInfo(typeInfo), _name(name)
		{}

		std::shared_ptr<TypeInfo> _typeInfo;
		const std::string _name;
	};

	class ArgumentList : public Node
	{
	public:
		ArgumentList(Argument* argument, ArgumentList* list = nullptr) :_argument(argument), _next(list)
		{
		}

		ArgumentList(std::shared_ptr<Argument> arg, std::shared_ptr<ArgumentList> next) : _argument(arg), _next(next)
		{
		}

		std::shared_ptr<Argument> _argument;
		std::shared_ptr<ArgumentList> _next;
	};

	class Initializer : public Statement
	{
	public:
		Initializer(const std::string& name, Expression* expression, FileLocation& location) :
			Statement(location), _name(name), _expr(expression)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass);
		virtual void CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		std::string _name;
		Expression* _expr;
		std::shared_ptr<Ast::SymbolTable::MemberInstanceBinding> _memberBinding;
		std::shared_ptr<StackConstructionExpression> _stackAssignment;
	};

	class InitializerList : public Node
	{
	public:
		InitializerList(Initializer* initializer, InitializerList* list = nullptr) : _thisInitializer(initializer), _next(list)
		{
		}

		InitializerList(std::shared_ptr<Initializer> initializer, std::shared_ptr<InitializerList> list) : _thisInitializer(initializer), _next(list)
		{
		}

		std::shared_ptr<Initializer> _thisInitializer;
		std::shared_ptr<InitializerList> _next;
	};

	class InitializerStatement : public Statement
	{
	public:
		InitializerStatement(InitializerList* list, FileLocation& location) :
			Statement(location), _list(list)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass);
		virtual void CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		std::shared_ptr<InitializerList> _list;
	};

	class FunctionDeclaration : public ClassStatement
	{
	public:
		FunctionDeclaration(Visibility visibility, Modifier* modifiers, ArgumentList* returnArgs, 
			const std::string& name, ArgumentList* inputArgs, LineStatement* body, FileLocation& location) :
			ClassStatement(location),
			_visibility(visibility), _mods(modifiers), _returnArgs(returnArgs), _name(name), _inputArgs(inputArgs), _body(body)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass) override;
		virtual void CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;
		virtual std::string Name()
		{
			return _name;
		}

		Visibility _visibility; 
		std::shared_ptr<Modifier> _mods;
		const std::string _name;
		std::shared_ptr<ArgumentList> _returnArgs;
		std::shared_ptr<ArgumentList> _inputArgs; 
		std::shared_ptr<LineStatement> _body;
		std::shared_ptr<Ast::TypeInfo> _inputArgsType;
		std::shared_ptr<Ast::TypeInfo> _outputArgsType;
	protected:
		void TypeCheckArgumentList(std::shared_ptr<Ast::SymbolTable::FunctionBinding> binding, std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass);
		void CodeGenEnter(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, llvm::FunctionType** ft, llvm::Function** function);
		void CodeGenLeave(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, llvm::FunctionType* ft, llvm::Function* function);
		std::shared_ptr<Ast::SymbolTable::FunctionBinding> _functionBinding;
		std::vector<std::shared_ptr<SymbolTable::SymbolBinding>> _argBindings;
		std::shared_ptr<SymbolTable::SymbolBinding> _thisPtrBinding;
		std::vector<std::shared_ptr<FunctionCall>> _endScopeDtors;
	};

	class ConstructorDeclaration : public FunctionDeclaration
	{
	public:
		ConstructorDeclaration(Visibility visibility, const std::string& name,
			ArgumentList* inputArgs, InitializerStatement* initializer, LineStatement* body, FileLocation& location) :
			FunctionDeclaration(visibility, new Modifier(Modifier::Modifiers::NONE), nullptr, name, inputArgs, body, location),
			_initializerStatement(initializer)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass) override;
		virtual void CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;
		std::shared_ptr<InitializerStatement> _initializerStatement;
	};

	class DestructorDeclaration : public FunctionDeclaration
	{
	public:
		DestructorDeclaration(const std::string& name, LineStatement* body, FileLocation& location) :
			FunctionDeclaration(Visibility::PRIVATE, new Modifier(Modifier::Modifiers::NONE), nullptr, name, nullptr, body, location)
		{
		}

		virtual std::string Name()
		{
			return "~" + _name;
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass) override;

		void AppendDtor(std::shared_ptr<FunctionCall> dtor);
		//virtual void CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;
	};

	class ClassStatementList : public Statement
	{
	public:
		ClassStatementList(ClassStatement* statement, ClassStatementList* list, FileLocation& location) :
			Statement(location),
			_statement(statement), _next(list)
		{
		}

		ClassStatementList(std::shared_ptr<ClassStatement> statement, std::shared_ptr<ClassStatementList> list, FileLocation& location) :
			Statement(location),
			_statement(statement), _next(list)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass) override;
		virtual void CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		std::shared_ptr<ClassStatement> _statement;
		std::shared_ptr<ClassStatementList> _next;
	};

	class ClassDeclaration : public GlobalStatement
	{
	public:
		ClassDeclaration(Visibility visibility, const std::string& name, ClassStatementList* list, FileLocation& location) :
			GlobalStatement(location),
			_visibility(visibility), _name(name), _list(list)
		{
		}

		virtual void TypeCheckInternal(std::shared_ptr<SymbolTable> symbolTable, TypeCheckPass pass) override;
		virtual void CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

		Visibility _visibility;
		const std::string _name;
		std::shared_ptr<ClassStatementList> _list;
		std::shared_ptr<Ast::SymbolTable::ClassBinding> _classBinding;
	};

	class NewExpression : public Expression
	{
	public:
		NewExpression(const std::string& className, Expression* expression, FileLocation& location) :
			Expression(location),
			_className(className), _expression(expression)
		{
		}

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList) override;

		const std::string _className;
		std::shared_ptr<Expression> _expression;

	protected:
		virtual llvm::Value* CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override
		{
			throw UnexpectedException();
		}
	};

	class FunctionCall : public Expression
	{
	public:
		FunctionCall(const std::string& name, Expression* expression, FileLocation& location) :
			Expression(location),
			_name(name), _expression(expression)
		{
		}

		FunctionCall(std::shared_ptr<FunctionTypeInfo> info, std::shared_ptr<Expression> expression, std::shared_ptr<Ast::SymbolTable::SymbolBinding> binding, std::shared_ptr<Ast::SymbolTable::SymbolBinding> varBinding, FileLocation& location) :
			Expression(info, location),
			_functionTypeInfo(info),
			_expression(expression),
			_name(info->Name()),
			_varBinding(varBinding)
		{
			_symbolBinding = binding;
		}

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList) override;

		const std::string _name;
		std::shared_ptr<FunctionTypeInfo> _functionTypeInfo;
		std::shared_ptr<Expression> _expression;
		std::vector<llvm::AllocaInst*> _outputValues;
	protected:
		virtual llvm::Value* CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
		std::shared_ptr<TypeInfo> _argsExprTypeInfo;
		std::shared_ptr<Ast::SymbolTable::SymbolBinding> _varBinding;
	};

	class StackConstructionExpression : public Expression
	{
	public:
		StackConstructionExpression(const std::string& className, const std::string& variableName, Expression* argumentExpression, FileLocation& location) :
			Expression(location),
			_className(className), _argumentExpression(argumentExpression), _varName(variableName)
		{
		}

		StackConstructionExpression(std::shared_ptr<Ast::SymbolTable::MemberInstanceBinding> varBinding, Expression* argumentExp, FileLocation& location) :
			Expression(location),
			_className(varBinding->GetTypeInfo()->Name()),
			_varName(varBinding->GetName()),
			_argumentExpression(argumentExp),
			_varBinding(varBinding)
		{
		}

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable, bool inInitializerList) override;

		const std::string _className;
		const std::string _varName;
		std::shared_ptr<Expression> _argumentExpression;
	protected:
		virtual llvm::Value* CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint = nullptr) override;
		std::shared_ptr<FunctionCall> _ctorCall;
		std::shared_ptr<Ast::SymbolTable::SymbolBinding> _varBinding;
	};
}