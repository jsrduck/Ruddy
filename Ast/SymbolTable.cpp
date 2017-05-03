#include "stdafx.h"
#include "SymbolTable.h"
#include "Classes.h"
#include "Statements.h"

#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>

namespace Ast
{
	SymbolTable::SymbolTable()
	{
		Enter(); // Global scope
	}

	SymbolTable::~SymbolTable()
	{
		Exit(); // Exit Global Scope
	}

	std::shared_ptr<SymbolTable::VariableBinding> SymbolTable::BindVariable(const std::string& symbolName, std::shared_ptr<TypeInfo> type)
	{
		if (_map.count(symbolName) > 0)
		{
			// We don't allow shadowing in Ruddy. It's evil.
			throw SymbolAlreadyDefinedInThisScopeException(symbolName);
		} // TODO: Check for class/namespace/member collisions in current namespace
		auto binding = std::make_shared<VariableBinding>(symbolName, type);
		_map[symbolName] = binding;
		_aux_stack.push(binding);
		return binding;
	}

	void SymbolTable::BindNamespace(const std::string& namespaceName)
	{
		auto binding = std::make_shared<NamespaceBinding>(_currentNamespace.size() > 0 ? _currentNamespace.top()->GetName() : "", namespaceName);
		if (_map.count(binding->GetFullyQualifiedName()) > 0)
		{
			throw SymbolAlreadyDefinedInThisScopeException(binding->GetName());
		} // TODO: Check for class/namespace/member collisions in any nested namespace
		_map[binding->GetFullyQualifiedName()] = binding;
		_aux_stack.push(binding);
		_currentNamespace.push(binding);
		_currentAddressableNamespaces.push_back(binding);
	}

	std::shared_ptr<SymbolTable::ClassBinding> SymbolTable::BindClass(const std::string& className, std::shared_ptr<ClassDeclaration> classDeclaration)
	{
		auto binding = std::make_shared<ClassBinding>(className, _currentNamespace.size() > 0 ? _currentNamespace.top()->GetFullyQualifiedName() : "", classDeclaration);
		if (_map.count(binding->GetFullyQualifiedName()) > 0)
		{
			throw SymbolAlreadyDefinedInThisScopeException(binding->GetName());
		} // TODO: Check for class/namespace/member collisions in any nested namespace
		_map[binding->GetFullyQualifiedName()] = binding;
		_aux_stack.push(binding);
		_currentClass.push(binding);
		_currentAddressableNamespaces.push_back(binding);
		return binding;
	}

	std::shared_ptr<SymbolTable::ConstructorBinding> SymbolTable::BindConstructor(std::shared_ptr<FunctionDeclaration> functionDeclaration)
	{
		if (_currentFunction.size() > 0 || _currentClass.size() == 0)
		{
			throw UnexpectedException();
		}
		auto binding = _currentClass.top()->AddConstructorBinding(functionDeclaration, shared_from_this());
		if (_map.count(binding->GetFullyQualifiedName()) > 0)
		{
			throw UnexpectedException();
		}
		//_map[binding->GetFullyQualifiedName()] = binding;
		_aux_stack.push(binding);
		_currentFunction.push(binding);
		return binding;
	}

	std::shared_ptr<SymbolTable::ConstructorBinding> SymbolTable::GetCurrentConstructor()
	{
		auto function = GetCurrentFunction();
		auto ctorBinding = std::dynamic_pointer_cast<ConstructorBinding>(function);
		return ctorBinding;
	}

	void SymbolTable::BindInitializer(const std::string& memberName, std::shared_ptr<Assignment> assignment)
	{
		if (_currentFunction.size() == 0 || _currentClass.size() == 0)
		{
			throw UnexpectedException();
		}
		auto binding = GetCurrentFunction();
		auto ctorBinding = std::dynamic_pointer_cast<ConstructorBinding>(binding);
		if (ctorBinding == nullptr)
		{
			throw UnexpectedException();
		}
		ctorBinding->AddInitializerBinding(memberName, assignment);
	}

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::BindFunction(const std::string& functionName, std::shared_ptr<FunctionDeclaration> functionDeclaration)
	{
		if (_currentFunction.size() > 0 || _currentClass.size() == 0)
		{
			throw FunctionMustBeDeclaredInClassScopeException(functionName);
		}
		auto binding = _currentClass.top()->AddFunctionBinding(functionName, functionDeclaration);
		if (_map.count(binding->GetFullyQualifiedName()) > 0)
		{
			throw UnexpectedException();
		}
		_map[binding->GetFullyQualifiedName()] = binding;
		_aux_stack.push(binding);
		_currentFunction.push(binding);
		return binding;
	}

	void SymbolTable::BindMemberVariable(const std::string& variableName, std::shared_ptr<ClassMemberDeclaration> memberVariable)
	{
		if (_currentFunction.size() > 0)
		{
			// The user is trying to declare a variable without initializing it. This is not allowed in Ruddy.
			throw VariablesMustBeInitializedException(variableName);
		}
		if (_currentClass.size() == 0)
		{
			throw VariablesCannotBeDeclaredOutsideOfScopesOrFunctionsException(variableName);
		}
		auto binding = _currentClass.top()->AddMemberVariableBinding(variableName, memberVariable);
		if (_map.count(binding->GetFullyQualifiedName()) > 0)
		{
			throw UnexpectedException();
		}
		_map[binding->GetFullyQualifiedName()] = binding;
		_aux_stack.push(binding);
	}

	void SymbolTable::BindLoop()
	{
		auto binding = std::make_shared<LoopBinding>();
		_aux_stack.push(binding);
		_currentLoop.push(binding);
	}

	std::shared_ptr<SymbolTable::SymbolBinding> SymbolTable::Lookup(const std::string& symbolName, bool checkIsInitialized)
	{
		// If this is a reference (ie, separated by period(s)), recursively lookup each prefix
		auto lastPrefixDelimitter = symbolName.find_last_of('.');
		if (lastPrefixDelimitter != std::string::npos)
		{
			// Recursively look up every symbol preceding the final one.
			auto prefixSymbol = Lookup(symbolName.substr(0, lastPrefixDelimitter), checkIsInitialized);
			if (prefixSymbol == nullptr)
				return nullptr;
			if (prefixSymbol->IsVariableBinding() || prefixSymbol->IsClassMemberBinding())
			{
				// We need the symbol for the class name
				prefixSymbol = Lookup(prefixSymbol->GetTypeInfo()->Name(), checkIsInitialized);
			}
			else if (prefixSymbol->IsFunctionBinding())
			{
				return nullptr; // Functions don't have submembers to reference
			}
			return Lookup(prefixSymbol->GetFullyQualifiedName(), symbolName.substr(lastPrefixDelimitter+1), checkIsInitialized);
		}
		return LookupInImplicitNamespaces(symbolName, checkIsInitialized);
	}

	std::shared_ptr<SymbolTable::SymbolBinding> SymbolTable::LookupInImplicitNamespaces(const std::string& symbolName, bool checkIsInitialized)
	{
		for (auto iter = _currentAddressableNamespaces.rbegin(); iter != _currentAddressableNamespaces.rend(); ++iter)
		{
			auto symbol = Lookup((*iter)->GetFullyQualifiedName(), symbolName, checkIsInitialized);
			if (symbol != nullptr)
				return symbol;
		}
		// Try the global namespace
		return Lookup("", symbolName, checkIsInitialized);
	}

	std::shared_ptr<SymbolTable::SymbolBinding> SymbolTable::Lookup(const std::string& underNamespace, const std::string& symbolName, bool checkIsInitialized)
	{
		std::string name;
		if (!underNamespace.empty())
		{
			name = underNamespace + "." + symbolName;
		}
		else
		{
			name = symbolName;
		}
		if (_map.count(name) == 0)
			return nullptr;
		auto binding = _map[name];
		if (!IsVisibleFromCurrentContext(binding))
		{
			throw SymbolNotAccessableException(name);
		}

		// Special case: if we're trying to deref a reference variable that hasn't been initialized yet,
		// we should throw. This can only happen in an initializer list.
		if (checkIsInitialized && binding->IsClassMemberBinding())
		{
			auto ctorBinding = GetCurrentConstructor();
			if (ctorBinding != nullptr)
			{
				auto memberBinding = std::dynamic_pointer_cast<MemberBinding>(binding);
				if (!memberBinding)
					throw UnexpectedException();
				if (memberBinding->_classBinding == GetCurrentClass() && ctorBinding->_initializers.count(binding->GetName()) == 0)
				{
					throw UninitializedVariableReferencedException(binding->GetName());
				}
			}
		}

		return binding;
	}

	bool SymbolTable::IsVisibleFromCurrentContext(std::shared_ptr<SymbolTable::SymbolBinding> binding)
	{
		switch (binding->GetVisibility())
		{
			case Visibility::PUBLIC:
				return true;
			case Visibility::PRIVATE:
			case Visibility::PROTECTED:
			{
				// Only if we're privy to this information, ie in the same class namespace
				auto ns = binding->GetParentNamespaceName();
				if (ns.empty())
					return true;
				if (_map.count(ns) == 0)
					throw UnexpectedException();
				auto nsBinding = _map[ns];
				for (auto iter = _currentAddressableNamespaces.rbegin(); iter != _currentAddressableNamespaces.rend(); ++iter)
				{
					// Just do a pointer compare
					if ((*iter).get() == nsBinding.get())
						return true;
				}
				return false;
			}
			default:
				throw UnexpectedException();
		}
	}

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::GetCurrentFunction()
	{
		if (_currentFunction.size() == 0)
			return nullptr;
		return _currentFunction.top();
	}

	std::shared_ptr<SymbolTable::ClassBinding> SymbolTable::GetCurrentClass()
	{
		if (_currentClass.size() == 0)
			return nullptr;
		return _currentClass.top();
	}

	std::shared_ptr<SymbolTable::LoopBinding> SymbolTable::GetCurrentLoop()
	{
		if (_currentLoop.size() == 0)
			return nullptr;
		return _currentLoop.top();
	}

	void SymbolTable::Enter()
	{
		_aux_stack.push(std::make_shared<ScopeMarker>());
	}

	void SymbolTable::Exit()
	{
		std::shared_ptr<SymbolBinding> binding;
		do
		{
			if (_aux_stack.size() == 0)
				throw UnexpectedException();
			binding = _aux_stack.top();
			_aux_stack.pop();
			if (binding->IsVariableBinding())
			{
				_map.erase(binding->GetFullyQualifiedName());
			}
			else if (binding->IsNamespaceBinding())
			{
				_currentNamespace.pop();
				_currentAddressableNamespaces.pop_back();
			}
			else if (binding->IsClassBinding())
			{
				_currentClass.pop();
				_currentAddressableNamespaces.pop_back();
			}
			else if (binding->IsFunctionBinding())
			{
				_currentFunction.pop();
			}
			else if (binding->IsLoopBinding())
			{
				_currentLoop.pop();
			}
		} while (!binding->IsScopeMarker());
	}

	std::shared_ptr<SymbolTable::ConstructorBinding> SymbolTable::ClassBinding::AddConstructorBinding(std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<SymbolTable> symbolTable)
	{
		auto binding = std::make_shared<SymbolTable::ConstructorBinding>(_name, GetFullyQualifiedName(), functionDeclaration);
		auto thisTypeInfo = binding->GetTypeInfo();
		auto functionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(thisTypeInfo);
		for (auto ctor : _ctors)
		{
			auto otherFunctionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(ctor->GetTypeInfo());
			if ((functionTypeInfo->InputArgsType() == nullptr && otherFunctionTypeInfo->InputArgsType() != nullptr) ||
				(functionTypeInfo->InputArgsType() != nullptr && otherFunctionTypeInfo->InputArgsType() == nullptr))
			{
				continue;
			}
			if ((otherFunctionTypeInfo->InputArgsType() != nullptr && otherFunctionTypeInfo->InputArgsType()->IsImplicitlyAssignableFrom(functionTypeInfo->InputArgsType(), symbolTable))
				|| (functionTypeInfo->InputArgsType() != nullptr && functionTypeInfo->InputArgsType()->IsImplicitlyAssignableFrom(otherFunctionTypeInfo->InputArgsType(), symbolTable))
				|| (functionTypeInfo->InputArgsType() == nullptr && otherFunctionTypeInfo->InputArgsType() == nullptr))
			{
				// Can't have ambiguous c'tors
				throw SymbolAlreadyDefinedInThisScopeException(_name);
			}
		}
		_ctors.push_back(binding);
		return binding;
	}

	void SymbolTable::ConstructorBinding::AddInitializerBinding(const std::string& memberName, std::shared_ptr<Assignment> assignment)
	{
		if (_initializers.count(memberName) > 0)
		{
			throw CannotReinitializeMemberException(memberName);
		}
		_initializers[memberName] = assignment;
	}

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::ClassBinding::AddFunctionBinding(const std::string& name, std::shared_ptr<FunctionDeclaration> functionDeclaration)
	{
		auto binding = std::make_shared<SymbolTable::FunctionBinding>(name, GetFullyQualifiedName(), functionDeclaration);
		if (_functions.count(name) > 0)
		{
			throw SymbolAlreadyDefinedInThisScopeException(name);
		}
		_functions[name] = binding;
		return binding;
	}

	std::shared_ptr<SymbolTable::MemberBinding> SymbolTable::ClassBinding::AddMemberVariableBinding(const std::string& name, std::shared_ptr<ClassMemberDeclaration> classMemberDeclaration)
	{
		auto binding = std::make_shared<SymbolTable::MemberBinding>(name, GetFullyQualifiedName(), classMemberDeclaration, shared_from_this());
		if (_members.count(name) > 0)
		{
			throw SymbolAlreadyDefinedInThisScopeException(name);
		}
		_members[name] = binding;
		return binding;
	}


	SymbolTable::FunctionBinding::FunctionBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<FunctionDeclaration> functionDeclaration) 
		: SymbolBinding(name, fullyQualifiedClassName.empty() ? name : fullyQualifiedClassName + "." + name, functionDeclaration->_visibility),
		_functionDeclaration(functionDeclaration)
	{
		_typeInfo = std::make_shared<FunctionTypeInfo>(functionDeclaration);
	}

	std::shared_ptr<TypeInfo> SymbolTable::FunctionBinding::GetTypeInfo()
	{
		return _typeInfo;
	}

	SymbolTable::ClassBinding::ClassBinding(const std::string& name, const std::string& fullyQualifiedNamespaceName, std::shared_ptr<ClassDeclaration> classDeclaration) 
		: SymbolBinding(name, fullyQualifiedNamespaceName.empty() ? name : fullyQualifiedNamespaceName + "." + name, classDeclaration->_visibility),
		  _classDeclaration(classDeclaration)
	{
		_typeInfo = std::make_shared<ClassDeclarationTypeInfo>(classDeclaration);
	}

	std::shared_ptr<TypeInfo> SymbolTable::ClassBinding::GetTypeInfo()
	{
		return _typeInfo;
	}

	SymbolTable::MemberBinding::MemberBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<ClassMemberDeclaration> memberDeclaration, std::shared_ptr<ClassBinding> classBinding)
		: SymbolBinding(name, fullyQualifiedClassName.empty() ? name : fullyQualifiedClassName + "." + name, memberDeclaration->_visibility),
		_memberDeclaration(memberDeclaration), _classBinding(classBinding)
	{
		_typeInfo = memberDeclaration->_typeInfo;
	}

	std::shared_ptr<TypeInfo> SymbolTable::MemberBinding::GetTypeInfo()
	{
		return _typeInfo;
	}

	SymbolTable::LoopBinding::LoopBinding() : SymbolBinding("", "", Visibility::PUBLIC), _endOfScope(nullptr)
	{
	}

	llvm::BasicBlock* SymbolTable::LoopBinding::GetEndOfScopeBlock(llvm::LLVMContext* context)
	{
		if (_endOfScope == nullptr)
			_endOfScope = llvm::BasicBlock::Create(*context);
		return _endOfScope;
	}


	llvm::AllocaInst* SymbolTable::VariableBinding::CreateAllocationInstance(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		auto alloc = _variableType->CreateAllocation(name, builder, context);
		_value = alloc;
		return alloc;
	}
}