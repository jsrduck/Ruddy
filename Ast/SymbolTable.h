#pragma once

#include "TypeInfo.h"
#include "Exceptions.h"
#include <memory>
#include <unordered_map>
#include <map>
#include <stack>
#include <functional>

#include <boost\property_tree\json_parser.hpp>

namespace Ast
{
	// DO NOT DELETE ENTRIES: reordering breaks our symbol table serialization
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
	class Expression;
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

			// Serialize for import
			boost::property_tree::ptree Serialize(std::shared_ptr<Ast::SymbolTable> symbolTable);
			virtual void SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree&) { throw UnexpectedException(); }

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

		/* Serialize for importing */
		void Serialize(std::ostream& output, std::string libName);
		void LoadFrom(std::istream& input);

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
		void BindExternalNamespace(const std::string& namespaceName, const std::string& parentNamespace);

		class ClassBinding;
		class OverloadedFunctionBinding;
		class FunctionBinding : public SymbolBinding, public std::enable_shared_from_this<FunctionBinding>
		{
		public:
			FunctionBinding(const std::string& fullyQualifiedClassName, std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<ClassBinding> classBinding);
			FunctionBinding(const std::string& name, const std::string & fullyQualifiedClassName, Visibility visibility, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, std::shared_ptr<Modifier> mods, std::shared_ptr<ClassBinding> classBinding);
			bool IsFunctionBinding() override { return true; }
			static bool HaveSameSignatures(std::shared_ptr<TypeInfo> inputArgs1, std::shared_ptr<TypeInfo> inputArgs2, std::shared_ptr<SymbolTable> symbolTable);
			virtual bool IsOverridden() { return false; }
			virtual std::shared_ptr<OverloadedFunctionBinding> GetOverloadedBinding()
			{
				return nullptr;
			}
			virtual llvm::Value* GetIRValue(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module) override;

			virtual void SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& symbol) override;

			std::shared_ptr<FunctionCall> CreateCall(std::shared_ptr<SymbolBinding> varBinding, FileLocation& location, std::shared_ptr<Expression> expression = nullptr);

			virtual bool IsMethod()
			{
				return !_typeInfo->_mods->IsStatic();
			}

			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			std::shared_ptr<FunctionTypeInfo> _typeInfo;
			std::shared_ptr<ClassBinding> _classBinding;
		};
		std::shared_ptr<FunctionBinding> BindFunction(std::shared_ptr<FunctionDeclaration> functionDeclaration, TypeCheckPass pass);
		std::shared_ptr<FunctionBinding> BindExternalFunction(std::shared_ptr<ClassBinding> classBinding, Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods);
		std::shared_ptr<FunctionBinding> GetCurrentFunction();

		class OverloadedFunctionBinding : public FunctionBinding
		{
		public:
			OverloadedFunctionBinding(std::shared_ptr<FunctionBinding> functionBinding1, std::shared_ptr<FunctionBinding> functionBinding2);
			void AddBinding(std::shared_ptr<FunctionBinding> functionBinding);
			std::shared_ptr<FunctionBinding> GetMatching(std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<SymbolTable> symbolTable);

			virtual bool IsOverridden()
			{
				return true;
			}

			virtual std::shared_ptr<OverloadedFunctionBinding> GetOverloadedBinding()
			{
				return std::dynamic_pointer_cast<OverloadedFunctionBinding>(shared_from_this());
			}

			std::shared_ptr<TypeInfo> GetTypeInfo() override
			{
				throw UnexpectedException();
			}

			virtual void BindIRValue(llvm::Value* value)
			{
				throw UnexpectedException();
			}

			virtual llvm::Value* GetIRValue(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
			{
				throw UnexpectedException();
			}

			virtual llvm::AllocaInst* CreateAllocationInstance(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
			{
				throw UnexpectedException();
			}

			// TODO: We have a weird situation here where we could have some static or some non-static methods. Should they be the same type? Methods internally take a this pointer
			// as a first param. Should overrides be the same? We can't tell from inside the symbol table lookup which one the caller is asking for.
			virtual bool IsMethod() override
			{
				for (auto& binding : _bindings)
				{
					if (!binding->_typeInfo->_mods->IsStatic())
						return true;
				}
			}

			virtual void SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& symbol) override;

			std::vector<std::shared_ptr<FunctionBinding>> _bindings;
		};

		class FunctionInstanceBinding : public FunctionBinding
		{
		public:
			FunctionInstanceBinding(std::shared_ptr<FunctionBinding> functionBinding, std::shared_ptr<SymbolBinding> reference) :
				FunctionBinding(functionBinding->GetName(), functionBinding->GetParentNamespaceName(), functionBinding->GetVisibility(), functionBinding->_typeInfo->InputArgsType(), functionBinding->_typeInfo->OutputArgsType(), functionBinding->_typeInfo->_mods, functionBinding->_classBinding),
				_functionBinding(functionBinding),
				_reference(reference)
			{
			}

			virtual bool IsOverridden()
			{
				return _functionBinding->IsOverridden();
			}

			virtual std::shared_ptr<OverloadedFunctionBinding> GetOverloadedBinding()
			{
				return _functionBinding->GetOverloadedBinding();
			}

			std::shared_ptr<TypeInfo> GetTypeInfo() override
			{
				return _functionBinding->GetTypeInfo();
			}

			virtual void BindIRValue(llvm::Value* value)
			{
				_functionBinding->BindIRValue(value);
			}

			virtual llvm::Value* GetIRValue(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
			{
				return _functionBinding->GetIRValue(builder, context, module);
			}

			virtual llvm::AllocaInst* CreateAllocationInstance(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
			{
				return _functionBinding->CreateAllocationInstance(name, builder, context);
			}

			std::shared_ptr<FunctionBinding> _functionBinding;
			std::shared_ptr<SymbolBinding> _reference;
		};

		class ConstructorBinding : public FunctionBinding
		{
		public:
			ConstructorBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<ClassBinding> classBinding) :
				FunctionBinding(fullyQualifiedClassName, functionDeclaration, classBinding)
			{
			}

			ConstructorBinding(const std::string& name, const std::string& fullyQualifiedClassName, Visibility visibility, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<Modifier> mods, std::shared_ptr<ClassBinding> classBinding) :
				FunctionBinding(name, fullyQualifiedClassName, visibility, inputArgs, nullptr /*outputArgs*/, mods, classBinding)
			{
			}

			void AddInitializerBinding(const std::string& memberName, std::shared_ptr<StackConstructionExpression> assignment);
			std::unordered_map<std::string, std::shared_ptr<StackConstructionExpression>> _initializers;
		};
		std::shared_ptr<ConstructorBinding> BindConstructor(std::shared_ptr<FunctionDeclaration> functionDeclaration, TypeCheckPass pass);
		std::shared_ptr<ConstructorBinding> GetCurrentConstructor();
		void BindInitializer(const std::string& memberName, std::shared_ptr<StackConstructionExpression> assignment);

		class MemberBinding : public SymbolBinding
		{
		public:
			MemberBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<ClassMemberDeclaration> memberDeclaration, std::shared_ptr<ClassBinding> classBinding);
			MemberBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<TypeInfo> typeInfo, std::shared_ptr<ClassBinding> classBinding, Visibility visibility, Modifier::Modifiers mods);
			bool IsClassMemberBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			int Index() { return _index; }
			virtual void SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& symbol) override;

			std::shared_ptr<TypeInfo> _typeInfo;
			std::shared_ptr<ClassBinding> _classBinding;
			std::shared_ptr<Modifier> _modifier;
		protected:
			MemberBinding(std::shared_ptr<MemberBinding> memberBinding);
			int _index = 0;
		};
		std::shared_ptr<MemberBinding> BindMemberVariable(const std::string& variableName, std::shared_ptr<ClassMemberDeclaration> memberVariable, TypeCheckPass pass);
		std::shared_ptr<MemberBinding> BindExternalMemberVariable(std::shared_ptr<ClassBinding> classBinding, const std::string& variableName, std::shared_ptr<TypeInfo> typeInfo, Visibility visibility, Modifier::Modifiers mods);

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
			ClassBinding(const std::string& name, const std::string& fullyQualifiedNamespaceName, Visibility visibility);

			std::shared_ptr<ConstructorBinding> AddConstructorBinding(std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<SymbolTable> symbolTable);
			std::shared_ptr<ConstructorBinding> AddExternalConstructorBinding(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<Modifier> mods);
			std::shared_ptr<FunctionBinding> AddDestructorBinding(std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<SymbolTable> symbolTable);
			std::shared_ptr<FunctionBinding> AddExternalDestructorBinding(std::shared_ptr<SymbolTable> symbolTable);
			std::shared_ptr<FunctionBinding> AddFunctionBinding(std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<SymbolTable> symbolTable);
			std::shared_ptr<FunctionBinding> AddExternalFunctionBinding(std::shared_ptr<SymbolTable> symbolTable, Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods);
			std::shared_ptr<MemberBinding> AddMemberVariableBinding(const std::string& name, std::shared_ptr<ClassMemberDeclaration> classMemberDeclaration);
			std::shared_ptr<MemberBinding> AddExternalMemberVariableBinding(const std::string& name, std::shared_ptr<TypeInfo> typeInfo, Visibility visibility, Modifier::Modifiers mods);
			void BindType(llvm::Type* type)
			{
				_typeInfo->BindType(type);
			}

			bool IsClassBinding() override { return true; }
			std::shared_ptr<TypeInfo> GetTypeInfo() override;
			size_t NumMembers();

			
			virtual void SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& symbol) override;
			static std::shared_ptr<ClassBinding> LoadFrom(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& classTree);

			std::shared_ptr<ClassDeclarationTypeInfo> _typeInfo;
			std::vector<std::shared_ptr<FunctionBinding>> _ctors;
			std::shared_ptr<FunctionBinding> _dtorBinding;
			std::unordered_map<std::string, std::shared_ptr<FunctionBinding>> _functions;
			std::vector<std::shared_ptr<MemberBinding>> _members;
		};
		std::shared_ptr<ClassBinding> BindClass(const std::string& className, std::shared_ptr<ClassDeclaration> classDeclaration, TypeCheckPass pass);
		std::shared_ptr<ClassBinding> BindExternalClass(const std::string& className, const std::string& fullyQualifiedClassName);
		std::shared_ptr<ClassBinding> GetCurrentClass();
		std::shared_ptr<FunctionBinding> BindDestructor(std::shared_ptr<DestructorDeclaration> functionDeclaration, TypeCheckPass pass);

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
	void LoadFunction(boost::property_tree::basic_ptree<std::string, std::string> &funPtree, std::shared_ptr<Ast::SymbolTable::ClassBinding> &classBinding, std::shared_ptr<Ast::SymbolTable> &symbolTable, std::string &funName);
}