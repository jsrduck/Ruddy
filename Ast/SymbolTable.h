#pragma once

#include "TypeInfo.h"
#include "Exceptions.h"
#include <memory>
#include <unordered_map>
#include <map>
#include <stack>
#include <functional>

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
	class StackConstructionExpression;
	class DestructorDeclaration;
	class FunctionCall;
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

			virtual llvm::Value* GetIRValue(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
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

			std::shared_ptr<FunctionCall> _onExit;
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
		std::vector<std::shared_ptr<FunctionCall>> Exit();
		std::vector<std::shared_ptr<FunctionCall>> BreakFromCurrentLoop();
		std::vector<std::shared_ptr<FunctionCall>> ReturnFromCurrentFunction();

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
		void BindNamespace(const std::string& namespaceName, TypeCheckPass pass);

		class FunctionBinding : public SymbolBinding
		{
		public:
			FunctionBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<FunctionDeclaration> functionDeclaration);
			bool IsFunctionBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			std::shared_ptr<FunctionDeclaration> _functionDeclaration;
			std::shared_ptr<FunctionTypeInfo> _typeInfo;
		};
		std::shared_ptr<FunctionBinding> BindFunction(const std::string& functionName, std::shared_ptr<FunctionDeclaration> functionDeclaration, TypeCheckPass pass);
		std::shared_ptr<FunctionBinding> GetCurrentFunction();

		class FunctionInstanceBinding : public FunctionBinding
		{
		public:
			FunctionInstanceBinding(std::shared_ptr<FunctionBinding> functionBinding, std::shared_ptr<SymbolBinding> reference) :
				FunctionBinding(functionBinding->GetName(), functionBinding->GetParentNamespaceName(), functionBinding->_functionDeclaration),
				_functionBinding(functionBinding),
				_reference(reference)
			{
			}

			std::shared_ptr<FunctionBinding> _functionBinding;
			std::shared_ptr<SymbolBinding> _reference;
		};

		class ConstructorBinding : public FunctionBinding
		{
		public:
			ConstructorBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<FunctionDeclaration> functionDeclaration) :
				FunctionBinding(name, fullyQualifiedClassName, functionDeclaration)
			{
			}

			void AddInitializerBinding(const std::string& memberName, std::shared_ptr<StackConstructionExpression> assignment);
			std::unordered_map<std::string, std::shared_ptr<StackConstructionExpression>> _initializers;
		};
		std::shared_ptr<ConstructorBinding> BindConstructor(std::shared_ptr<FunctionDeclaration> functionDeclaration, TypeCheckPass pass);
		std::shared_ptr<ConstructorBinding> GetCurrentConstructor();
		void BindInitializer(const std::string& memberName, std::shared_ptr<StackConstructionExpression> assignment);

		class ClassBinding;
		class MemberBinding : public SymbolBinding
		{
		public:
			MemberBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<ClassMemberDeclaration> memberDeclaration, std::shared_ptr<ClassBinding> classBinding);
			bool IsClassMemberBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			int Index() { return _index; }

			std::shared_ptr<ClassMemberDeclaration> _memberDeclaration;
			std::shared_ptr<TypeInfo> _typeInfo;
			std::shared_ptr<ClassBinding> _classBinding;
		protected:
			MemberBinding(std::shared_ptr<MemberBinding> memberBinding);
			int _index = 0;
		};
		std::shared_ptr<MemberBinding> BindMemberVariable(const std::string& variableName, std::shared_ptr<ClassMemberDeclaration> memberVariable, TypeCheckPass pass);

		class MemberInstanceBinding : public MemberBinding
		{
		public:
			MemberInstanceBinding(std::shared_ptr<MemberBinding> memberBinding, std::shared_ptr<SymbolBinding> reference) : 
				MemberBinding(memberBinding),
				_reference(reference)
			{
			}

			llvm::Value* GetIRValue(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;
			bool IsReferenceToThisPointer()
			{
				return _reference->IsFunctionBinding();
			}

		protected:
			std::shared_ptr<SymbolBinding> _reference;
		};

		class ClassBinding : public SymbolBinding, public std::enable_shared_from_this<ClassBinding>
		{
		public:
			ClassBinding(const std::string& name, const std::string& fullyQualifiedNamespaceName, std::shared_ptr<ClassDeclaration> classDeclaration);

			std::shared_ptr<ConstructorBinding> AddConstructorBinding(std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<SymbolTable> symbolTable);
			std::shared_ptr<FunctionBinding> AddFunctionBinding(const std::string& name, std::shared_ptr<FunctionDeclaration> functionDeclaration);
			std::shared_ptr<MemberBinding> AddMemberVariableBinding(const std::string& name, std::shared_ptr<ClassMemberDeclaration> classMemberDeclaration);
			void BindType(llvm::Type* type)
			{
				_typeInfo->BindType(type);
			}

			bool IsClassBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			size_t NumMembers();

			std::shared_ptr<ClassDeclaration> _classDeclaration;
			std::shared_ptr<ClassDeclarationTypeInfo> _typeInfo;
			std::vector<std::shared_ptr<FunctionBinding>> _ctors;
			std::shared_ptr<DestructorDeclaration> _dtor;
			std::unordered_map<std::string, std::shared_ptr<FunctionBinding>> _functions;
			std::vector<std::shared_ptr<MemberBinding>> _members;
		};
		std::shared_ptr<ClassBinding> BindClass(const std::string& className, std::shared_ptr<ClassDeclaration> classDeclaration, TypeCheckPass pass);
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
		std::shared_ptr<LoopBinding> BindLoop();
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