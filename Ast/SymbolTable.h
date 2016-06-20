#pragma once

#include "TypeInfo.h"
#include "TypeExceptions.h"
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
	class SymbolTable
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

			virtual std::shared_ptr<TypeInfo> GetTypeInfo() = 0;

			std::string& GetName() { return _name; }

			std::string& GetFullyQualifiedName() { return _fullyQualifiedName; }

			std::string GetParentNamespaceName() { return _parentNamespace; }

			Visibility GetVisibility() { return _visibility; }

		protected:
			std::string _name;
			std::string _fullyQualifiedName;
			std::string _parentNamespace;
			Visibility _visibility;
		};

		void BindVariable(const std::string& symbolName, std::shared_ptr<TypeInfo> node);
		void BindNamespace(const std::string& namespaceName);
		void BindClass(const std::string& className, std::shared_ptr<ClassDeclaration> classDeclaration);
		void BindFunction(const std::string& functionName, std::shared_ptr<FunctionDeclaration> functionDeclaration);
		void BindMemberVariable(const std::string& variableName, std::shared_ptr<ClassMemberDeclaration> memberVariable);

		std::shared_ptr<SymbolBinding> Lookup(const std::string& symbolName);

		std::shared_ptr<SymbolBinding> GetCurrentFunction();

		std::shared_ptr<SymbolBinding> GetCurrentClass();

		/* Declare new scope. Caller must call exit after leaving scope. */
		void Enter();

		/* Exit current scope */
		void Exit();

	private:

		std::shared_ptr<SymbolTable::SymbolBinding> Lookup(const std::string& underNamespace, const std::string& symbolName);
		std::shared_ptr<SymbolTable::SymbolBinding> LookupInImplicitNamespaces(const std::string& symbolName);
		bool IsVisibleFromCurrentContext(std::shared_ptr<SymbolTable::SymbolBinding> binding);

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

			std::shared_ptr<TypeInfo> _variableType;
		};

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

		class FunctionBinding : public SymbolBinding
		{
		public:
			FunctionBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<FunctionDeclaration> functionDeclaration);
			bool IsFunctionBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			std::shared_ptr<FunctionDeclaration> _functionDeclaration;
			std::shared_ptr<FunctionTypeInfo> _typeInfo;
		};

		class MemberBinding : public SymbolBinding
		{
		public:
			MemberBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<ClassMemberDeclaration> memberDeclaration);
			bool IsClassMemberBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			std::shared_ptr<ClassMemberDeclaration> _memberDeclaration;
			std::shared_ptr<TypeInfo> _typeInfo;
		};

		class ClassBinding : public SymbolBinding
		{
		public:
			ClassBinding(const std::string& name, const std::string& fullyQualifiedNamespaceName, std::shared_ptr<ClassDeclaration> classDeclaration);

			std::shared_ptr<FunctionBinding> AddFunctionBinding(const std::string& name, std::shared_ptr<FunctionDeclaration> functionDeclaration);
			std::shared_ptr<MemberBinding> AddMemberVariableBinding(const std::string& name, std::shared_ptr<ClassMemberDeclaration> classMemberDeclaration);

			bool IsClassBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			std::shared_ptr<ClassDeclaration> _classDeclaration;
			std::shared_ptr<ClassTypeInfo> _typeInfo;
			std::unordered_map<std::string, std::shared_ptr<FunctionBinding>> _functions;
			std::unordered_map<std::string, std::shared_ptr<MemberBinding>> _members;
		};

		std::unordered_map<std::string, std::shared_ptr<SymbolBinding>> _map;
		std::stack<std::shared_ptr<SymbolBinding>> _aux_stack;
		std::stack<std::shared_ptr<NamespaceBinding>> _currentNamespace;
		std::stack<std::shared_ptr<ClassBinding>> _currentClass;
		std::stack<std::shared_ptr<FunctionBinding>> _currentFunction;
		std::vector<std::shared_ptr<SymbolBinding>> _currentAddressableNamespaces; // Anything addressable, ie, classes and namespaces
	};
}