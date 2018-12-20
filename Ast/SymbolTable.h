#pragma once

#include "TypeInfo.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <stack>
#include <functional>

namespace Ast
{
	class SymbolCodeGenerator;
	class Serializer;
	class Expression;
	class SymbolTable : public std::enable_shared_from_this<SymbolTable>
	{
	public:
		SymbolTable();

		~SymbolTable();

		class SymbolBinding : public std::enable_shared_from_this<SymbolBinding>
		{
		public:
			SymbolBinding(std::shared_ptr<Ast::SymbolTable> symbolTable, const std::string& name, const std::string& fullQualifiedName, Visibility visibility);

			virtual bool IsScopeMarker() { return false; }

			virtual bool IsVariableBinding() { return false; }

			virtual bool IsLocalVariableBinding() { return false; }

			virtual bool IsClassBinding() { return false; }

			virtual bool IsNamespaceBinding() { return false; }

			virtual bool IsFunctionBinding() { return false; }

			virtual bool IsClassMemberBinding() { return false; }

			virtual bool IsLoopBinding() { return false; }

			std::shared_ptr<SymbolCodeGenerator> GetCodeGen();

			virtual std::shared_ptr<TypeInfo> GetTypeInfo() = 0;

			std::string& GetName() { return _name; }

			std::string& GetFullyQualifiedName() { return _fullyQualifiedName; }

			std::string GetParentNamespaceName() { return _parentNamespace; }

			Visibility GetVisibility() { return _visibility; }

			std::shared_ptr<Ast::SymbolTable> SymbolTable() { return _symbolTable; }

			virtual std::shared_ptr<Serializer> GetSerializer();
			virtual std::shared_ptr<SymbolCodeGenerator> CreateCodeGen();

		protected:
			std::string _name;
			std::string _fullyQualifiedName;
			std::string _parentNamespace;
			Visibility _visibility;
			std::shared_ptr<SymbolCodeGenerator> _codeGen;
			std::shared_ptr<Serializer> _serializer;
			std::shared_ptr<Ast::SymbolTable> _symbolTable;
		};
		
		std::shared_ptr<SymbolBinding> Lookup(const std::string& symbolName, bool checkIsInitialized = false);

		/* Declare new scope. Caller must call exit after leaving scope. */
		void Enter();

		/* Serialize for importing */
		void Serialize(std::ostream& output, std::string libName);
		void LoadFrom(std::istream& input);
		void AddExternalLibrary(const std::string& libName, std::function<std::shared_ptr<SymbolTable>()> GetSymbolTable);
		void ActivateExternalLibrary(const std::string& libName);

		class ScopeMarker : public SymbolBinding
		{
		public:
			ScopeMarker(std::shared_ptr<Ast::SymbolTable> symbolTable) : SymbolBinding(symbolTable, "", "", Visibility::PUBLIC) { }
			bool IsScopeMarker() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
		};

		class FunctionInstanceBinding;
		class ClassBinding;
		class BaseVariableBinding : public SymbolBinding
		{
		public:
			BaseVariableBinding(std::shared_ptr<Ast::SymbolTable> symbolTable, const std::string& name, const std::string& fullQualifiedName, Visibility visibility);
			virtual std::shared_ptr<FunctionInstanceBinding> GetDestructor();
			virtual bool IsReferenceVariable();
			std::shared_ptr<ClassBinding> ClassBindingForThisType();
		protected:
			std::shared_ptr<ClassBinding> _classBindingForThisType;
		};

		/* Exit current scope */
		std::vector<std::shared_ptr<BaseVariableBinding>> Exit();
		std::vector<std::shared_ptr<BaseVariableBinding>> BreakFromCurrentLoop();
		std::vector<std::shared_ptr<BaseVariableBinding>> ReturnFromCurrentFunction();

		class VariableBinding : public BaseVariableBinding
		{
		public:
			VariableBinding(std::shared_ptr<Ast::SymbolTable> symbolTable, const std::string& name, std::shared_ptr<TypeInfo> variableType);

			virtual bool IsLocalVariableBinding() override { return true; }
			virtual bool IsVariableBinding() override { return true; }

			std::shared_ptr<TypeInfo> GetTypeInfo() override { return _variableType; }
			virtual std::shared_ptr<SymbolCodeGenerator> CreateCodeGen() override;

			std::shared_ptr<TypeInfo> _variableType;
		};
		std::shared_ptr<SymbolTable::VariableBinding> BindVariable(const std::string& symbolName, std::shared_ptr<TypeInfo> node);

		class NamespaceBinding : public SymbolBinding
		{
		public:
			NamespaceBinding(std::shared_ptr<Ast::SymbolTable> symbolTable, const std::string& parentFullyQualifiedName, const std::string& name);

			bool IsNamespaceBinding() override;
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
		};
		void BindNamespace(const std::string& namespaceName, TypeCheckPass pass);
		void BindExternalNamespace(const std::string& namespaceName, const std::string& parentNamespace);

		class ClassBinding;
		class OverloadedFunctionBinding;
		class FunctionBinding : public SymbolBinding
		{
		public:
			FunctionBinding(std::shared_ptr<Ast::SymbolTable> symbolTable, const std::string& name, const std::string & fullyQualifiedClassName, Visibility visibility, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, std::shared_ptr<Modifier> mods, std::shared_ptr<ClassBinding> classBinding);
			bool IsFunctionBinding() override { return true; }
			static bool HaveSameSignatures(std::shared_ptr<TypeInfo> inputArgs1, std::shared_ptr<TypeInfo> inputArgs2, std::shared_ptr<Ast::SymbolTable> symbolTable);
			virtual bool IsOverridden() { return false; }
			virtual std::shared_ptr<OverloadedFunctionBinding> GetOverloadedBinding();

			virtual std::shared_ptr<Serializer> GetSerializer() override;

			virtual bool IsMethod();

			virtual std::shared_ptr<SymbolCodeGenerator> CreateCodeGen() override;

			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			std::shared_ptr<FunctionTypeInfo> _typeInfo;
			std::shared_ptr<ClassBinding> _classBinding;
		};
		std::shared_ptr<FunctionBinding> BindFunction(Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods, TypeCheckPass pass);
		std::shared_ptr<FunctionBinding> BindExternalFunction(std::shared_ptr<ClassBinding> classBinding, Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods);
		std::shared_ptr<FunctionBinding> GetCurrentFunction();

		class OverloadedFunctionBinding : public FunctionBinding
		{
		public:
			OverloadedFunctionBinding(std::shared_ptr<FunctionBinding> functionBinding1, std::shared_ptr<FunctionBinding> functionBinding2);
			void AddBinding(std::shared_ptr<FunctionBinding> functionBinding);
			std::shared_ptr<FunctionBinding> GetMatching(std::shared_ptr<TypeInfo> inputArgs);

			virtual bool IsOverridden();

			virtual std::shared_ptr<SymbolCodeGenerator> CreateCodeGen() override;

			virtual std::shared_ptr<OverloadedFunctionBinding> GetOverloadedBinding();

			virtual std::shared_ptr<TypeInfo> GetTypeInfo() override;

			// TODO: We have a weird situation here where we could have some static or some non-static methods. Should they be the same type? Methods internally take a this pointer
			// as a first param. Should overrides be the same? We can't tell from inside the symbol table lookup which one the caller is asking for.
			virtual bool IsMethod() override;

			virtual std::shared_ptr<Serializer> GetSerializer() override;

			std::vector<std::shared_ptr<FunctionBinding>> _bindings;
		};

		class FunctionInstanceBinding : public FunctionBinding
		{
		public:
			FunctionInstanceBinding(std::shared_ptr<FunctionBinding> functionBinding, std::shared_ptr<SymbolBinding> reference);

			virtual bool IsOverridden() override;

			virtual std::shared_ptr<OverloadedFunctionBinding> GetOverloadedBinding() override;

			virtual std::shared_ptr<TypeInfo> GetTypeInfo() override;

			virtual std::shared_ptr<SymbolCodeGenerator> CreateCodeGen() override;

			std::shared_ptr<FunctionBinding> _functionBinding;
			std::shared_ptr<SymbolBinding> _reference;
		};

		class ConstructorBinding : public FunctionBinding
		{
		public:
			ConstructorBinding(std::shared_ptr<Ast::SymbolTable> symbolTable, const std::string& name, const std::string& fullyQualifiedClassName, Visibility visibility, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<Modifier> mods, std::shared_ptr<ClassBinding> classBinding);

			void AddInitializerBinding(const std::string& memberName);

			std::unordered_set<std::string> _initializers;
		};
		std::shared_ptr<ConstructorBinding> BindConstructor(std::shared_ptr<TypeInfo> inputArgs, Visibility visibility, std::shared_ptr<Modifier> mods, TypeCheckPass pass);
		std::shared_ptr<ConstructorBinding> GetCurrentConstructor();
		void BindInitializer(const std::string& memberName);

		class MemberBinding : public BaseVariableBinding
		{
		public:
			MemberBinding(std::shared_ptr<Ast::SymbolTable> symbolTable, const std::string& name, std::shared_ptr<TypeInfo> typeInfo, std::shared_ptr<ClassBinding> classBindingForParentType, Visibility visibility, Modifier::Modifiers mods);
			bool IsClassMemberBinding() override { return true; }
			virtual bool IsVariableBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			int Index() { return _index; }
			virtual std::shared_ptr<Serializer> GetSerializer() override;

			std::shared_ptr<TypeInfo> _typeInfo;
			std::shared_ptr<Modifier> _modifier;
			std::shared_ptr<ClassBinding> _classBindingForParentType;
		protected:
			MemberBinding(std::shared_ptr<MemberBinding> memberBinding);
			int _index = 0;
		};
		std::shared_ptr<MemberBinding> BindMemberVariable(const std::string& variableName, std::shared_ptr<TypeInfo> typeInfo, Visibility visibility, Modifier::Modifiers mods, TypeCheckPass pass);
		std::shared_ptr<MemberBinding> BindExternalMemberVariable(std::shared_ptr<ClassBinding> classBinding, const std::string& variableName, std::shared_ptr<TypeInfo> typeInfo, Visibility visibility, Modifier::Modifiers mods);

		class MemberInstanceBinding : public MemberBinding
		{
		public:
			MemberInstanceBinding(std::shared_ptr<MemberBinding> memberBinding, std::shared_ptr<SymbolBinding> reference);

			virtual std::shared_ptr<SymbolCodeGenerator> CreateCodeGen() override;
			static std::shared_ptr<SymbolCodeGenerator> CreateCodeGenFromValue(llvm::Value* referenceValue, bool isClassValueType, std::shared_ptr<MemberBinding> memberBinding);

			bool IsReferenceToThisPointer();

		protected:
			std::shared_ptr<SymbolBinding> _reference;
		};

		/* A binding that represents an expression that can't be bound to a symbol in the symbol table
		   during the type-checking phase. The value must be computed during the code generation phase instead. */
		class DeferredExpressionBinding : public SymbolBinding
		{
		public:
			DeferredExpressionBinding(std::shared_ptr<Ast::SymbolTable> symbolTable, std::shared_ptr<Ast::Expression> expression, std::shared_ptr<TypeInfo> exprTypeInfo);
			virtual std::shared_ptr<SymbolCodeGenerator> CreateCodeGen() override;
			virtual std::shared_ptr<TypeInfo> GetTypeInfo() override;
		protected:
			std::shared_ptr<Expression> _expression;
			std::shared_ptr<TypeInfo> _exprTypeInfo;
		};

		class ClassBinding : public SymbolBinding
		{
		public:
			ClassBinding(std::shared_ptr<Ast::SymbolTable> symbolTable, const std::string& name, const std::string& fullyQualifiedNamespaceName, Visibility visibility);

			std::shared_ptr<ConstructorBinding> AddConstructorBinding(std::shared_ptr<TypeInfo> inputArgs, Visibility visibility, std::shared_ptr<Modifier> mods);
			std::shared_ptr<ConstructorBinding> AddExternalConstructorBinding(std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<Modifier> mods);
			std::shared_ptr<FunctionBinding> AddDestructorBinding();
			std::shared_ptr<FunctionBinding> AddExternalDestructorBinding();
			std::shared_ptr<FunctionBinding> AddFunctionBinding(Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods);
			std::shared_ptr<FunctionBinding> AddExternalFunctionBinding(Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods);
			std::shared_ptr<MemberBinding> AddMemberVariableBinding(const std::string& name, std::shared_ptr<TypeInfo> typeInfo, Visibility visibility, Modifier::Modifiers mods);
			std::shared_ptr<MemberBinding> AddExternalMemberVariableBinding(const std::string& name, std::shared_ptr<TypeInfo> typeInfo, Visibility visibility, Modifier::Modifiers mods);

			std::shared_ptr<MemberBinding> GetMemberBinding(const std::string& member);
			std::shared_ptr<FunctionBinding> GetMethodBinding(const std::string& member);
			virtual bool IsClassBinding() override;
			virtual std::shared_ptr<TypeInfo> GetTypeInfo() override;
			size_t NumMembers();

			virtual std::shared_ptr<Serializer> GetSerializer() override;

			std::shared_ptr<ClassDeclarationTypeInfo> _typeInfo;
			std::vector<std::shared_ptr<FunctionBinding>> _ctors;
			std::shared_ptr<FunctionBinding> _dtorBinding;
			std::unordered_map<std::string, std::shared_ptr<FunctionBinding>> _functions;
			std::vector<std::shared_ptr<MemberBinding>> _members;
		};
		std::shared_ptr<ClassBinding> BindClass(const std::string& className, Visibility visibility, TypeCheckPass pass);
		std::shared_ptr<ClassBinding> BindExternalClass(const std::string& className, const std::string& fullyQualifiedNamespace);
		std::shared_ptr<ClassBinding> GetCurrentClass();
		std::shared_ptr<FunctionBinding> BindDestructor(TypeCheckPass pass);

		class LoopBinding : public SymbolBinding
		{
		public:
			LoopBinding(std::shared_ptr<Ast::SymbolTable> symbolTable);

			virtual std::shared_ptr<SymbolCodeGenerator> CreateCodeGen() override;

			virtual bool IsLoopBinding() override;
			virtual std::shared_ptr<TypeInfo> GetTypeInfo();
		};
		std::shared_ptr<LoopBinding> BindLoop();
		std::shared_ptr<LoopBinding> GetCurrentLoop();

		bool EnterUnsafeContext();
		void ExitUnsafeContext();
		bool IsInUnsafeContext();

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
		std::unordered_map<std::string, std::function<std::shared_ptr<SymbolTable>()>> _unactivatedExternalLibraries;
		std::vector<std::shared_ptr<SymbolTable>> _externalLibraries;
		std::unordered_set<std::string> _activatedLibraries;
		bool _unsafeContext = false;
	};
}