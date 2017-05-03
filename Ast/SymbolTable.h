#pragma once

#include "TypeInfo.h"
#include "Exceptions.h"
#include <memory>
#include <unordered_map>
#include <stack>

namespace Ast
{
	enum Visibility
	{
		PUBLIC,
		PRIVATE,
		PROTECTED
	};

	class ClassDeclaration;
	class FunctionDeclaration;
	class ClassMemberDeclaration;
	class Assignment;
	class SymbolTable : public std::enable_shared_from_this<SymbolTable>
	{
	public:
		SymbolTable();

		~SymbolTable();

		class SymbolBinding
		{
		public:
			SymbolBinding(const std::string& name, const std::string& fullQualifiedName, Visibility visibility) 
				: _name(name), _fullyQualifiedName(fullQualifiedName), _visibility(visibility)
			{
				auto lastPrefixDelimitter = _fullyQualifiedName.find_last_of('.');
				if (lastPrefixDelimitter != std::string::npos)
				{
					_parentNamespace = _fullyQualifiedName.substr(0, lastPrefixDelimitter);
				}
				else
				{
					_parentNamespace = "";
				}
			}

			virtual bool IsScopeMarker() { return false; }

			virtual bool IsVariableBinding() { return false; }

			virtual bool IsClassBinding() { return false; }

			virtual bool IsNamespaceBinding() { return false; }

			virtual bool IsFunctionBinding() { return false; }

			virtual bool IsClassMemberBinding() { return false; }

			virtual bool IsLoopBinding() { return false; }

			virtual llvm::BasicBlock* GetEndOfScopeBlock(llvm::LLVMContext* context)
			{
				throw UnexpectedException();
			}

			virtual std::shared_ptr<TypeInfo> GetTypeInfo() = 0;

			virtual void BindIRValue(llvm::Value* value)
			{
				_value = value;
			}

			virtual llvm::Value* GetIRValue()
			{
				return _value;
			}

			virtual llvm::AllocaInst* CreateAllocationInstance(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
			{
				throw UnexpectedException();
			}

			std::string& GetName() { return _name; }

			std::string& GetFullyQualifiedName() { return _fullyQualifiedName; }

			std::string GetParentNamespaceName() { return _parentNamespace; }

			Visibility GetVisibility() { return _visibility; }

		protected:
			std::string _name;
			std::string _fullyQualifiedName;
			std::string _parentNamespace;
			Visibility _visibility;
			llvm::Value* _value = nullptr;
		};
		
		std::shared_ptr<SymbolBinding> Lookup(const std::string& symbolName, bool checkIsInitialized = false);

		/* Declare new scope. Caller must call exit after leaving scope. */
		void Enter();

		/* Exit current scope */
		void Exit();

		class ScopeMarker : public SymbolBinding
		{
		public:
			ScopeMarker() : SymbolBinding("", "", Visibility::PUBLIC) { }
			bool IsScopeMarker() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override { throw UnexpectedException(); }
		};

		class VariableBinding : public SymbolBinding
		{
		public:
			VariableBinding(const std::string& name, std::shared_ptr<TypeInfo> variableType)
				: SymbolBinding(name, name, Visibility::PUBLIC), _variableType(variableType)
			{
			}

			bool IsVariableBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override { return _variableType; }
			virtual llvm::AllocaInst* CreateAllocationInstance(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) override;

			std::shared_ptr<TypeInfo> _variableType;
		};
		std::shared_ptr<SymbolTable::VariableBinding> BindVariable(const std::string& symbolName, std::shared_ptr<TypeInfo> node);

		class NamespaceBinding : public SymbolBinding
		{
		public:
			NamespaceBinding(const std::string& parentFullyQualifiedName, const std::string& name) : 
				SymbolBinding(name, parentFullyQualifiedName.empty() ? name : parentFullyQualifiedName + "." + name, Visibility::PUBLIC)
			{
			}

			bool IsNamespaceBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override { throw UnexpectedException(); }
		};
		void BindNamespace(const std::string& namespaceName);

		class FunctionBinding : public SymbolBinding
		{
		public:
			FunctionBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<FunctionDeclaration> functionDeclaration);
			bool IsFunctionBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			std::shared_ptr<FunctionDeclaration> _functionDeclaration;
			std::shared_ptr<FunctionTypeInfo> _typeInfo;
		};
		std::shared_ptr<FunctionBinding> BindFunction(const std::string& functionName, std::shared_ptr<FunctionDeclaration> functionDeclaration);
		std::shared_ptr<FunctionBinding> GetCurrentFunction();

		class ConstructorBinding : public FunctionBinding
		{
		public:
			ConstructorBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<FunctionDeclaration> functionDeclaration) :
				FunctionBinding(name, fullyQualifiedClassName, functionDeclaration)
			{
			}

			void AddInitializerBinding(const std::string& memberName, std::shared_ptr<Assignment> assignment);
			std::unordered_map<std::string, std::shared_ptr<Assignment>> _initializers;
		};
		std::shared_ptr<ConstructorBinding> BindConstructor(std::shared_ptr<FunctionDeclaration> functionDeclaration);
		std::shared_ptr<ConstructorBinding> GetCurrentConstructor();
		void BindInitializer(const std::string& memberName, std::shared_ptr<Assignment> assignment);

		class ClassBinding;
		class MemberBinding : public SymbolBinding
		{
		public:
			MemberBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<ClassMemberDeclaration> memberDeclaration, std::shared_ptr<ClassBinding> classBinding);
			bool IsClassMemberBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			std::shared_ptr<ClassMemberDeclaration> _memberDeclaration;
			std::shared_ptr<TypeInfo> _typeInfo;
			std::shared_ptr<ClassBinding> _classBinding;
		};
		void BindMemberVariable(const std::string& variableName, std::shared_ptr<ClassMemberDeclaration> memberVariable);

		class ClassBinding : public SymbolBinding, public std::enable_shared_from_this<ClassBinding>
		{
		public:
			ClassBinding(const std::string& name, const std::string& fullyQualifiedNamespaceName, std::shared_ptr<ClassDeclaration> classDeclaration);

			std::shared_ptr<ConstructorBinding> AddConstructorBinding(std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<SymbolTable> symbolTable);
			std::shared_ptr<FunctionBinding> AddFunctionBinding(const std::string& name, std::shared_ptr<FunctionDeclaration> functionDeclaration);
			std::shared_ptr<MemberBinding> AddMemberVariableBinding(const std::string& name, std::shared_ptr<ClassMemberDeclaration> classMemberDeclaration);

			bool IsClassBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			std::shared_ptr<ClassDeclaration> _classDeclaration;
			std::shared_ptr<ClassDeclarationTypeInfo> _typeInfo;
			std::vector<std::shared_ptr<FunctionBinding>> _ctors;
			std::unordered_map<std::string, std::shared_ptr<FunctionBinding>> _functions;
			std::unordered_map<std::string, std::shared_ptr<MemberBinding>> _members;
		};
		std::shared_ptr<ClassBinding> BindClass(const std::string& className, std::shared_ptr<ClassDeclaration> classDeclaration);
		std::shared_ptr<ClassBinding> GetCurrentClass();

		class LoopBinding : public SymbolBinding
		{
		public:
			LoopBinding();

			bool IsLoopBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override { throw UnexpectedException(); }
			virtual llvm::BasicBlock* GetEndOfScopeBlock(llvm::LLVMContext* context) override;

		private:
			llvm::BasicBlock* _endOfScope;
		};
		void BindLoop();
		std::shared_ptr<LoopBinding> GetCurrentLoop();

	private:
		std::shared_ptr<SymbolTable::SymbolBinding> Lookup(const std::string& underNamespace, const std::string& symbolName, bool checkIsInitialized);
		std::shared_ptr<SymbolTable::SymbolBinding> LookupInImplicitNamespaces(const std::string& symbolName, bool checkIsInitialized);
		bool IsVisibleFromCurrentContext(std::shared_ptr<SymbolTable::SymbolBinding> binding);

		std::unordered_map<std::string, std::shared_ptr<SymbolBinding>> _map;
		std::stack<std::shared_ptr<SymbolBinding>> _aux_stack;
		std::stack<std::shared_ptr<NamespaceBinding>> _currentNamespace;
		std::stack<std::shared_ptr<ClassBinding>> _currentClass;
		std::stack<std::shared_ptr<FunctionBinding>> _currentFunction;
		std::stack<std::shared_ptr<LoopBinding>> _currentLoop;
		std::vector<std::shared_ptr<SymbolBinding>> _currentAddressableNamespaces; // Anything addressable, ie, classes and namespaces
	};
}