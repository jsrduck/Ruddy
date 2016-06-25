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

	void SymbolTable::BindVariable(const std::string& symbolName, std::shared_ptr<TypeInfo> type)
	{
		if (_map.count(symbolName) > 0)
		{
			// We don't allow shadowing in Ruddy. It's evil.
			throw SymbolAlreadyDefinedInThisScopeException(symbolName);
		} // TODO: Check for class/namespace/member collisions in current namespace
		auto binding = std::make_shared<VariableBinding>(symbolName, type);
		_map[symbolName] = binding;
		_aux_stack.push(binding);
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

	void SymbolTable::BindClass(const std::string& className, std::shared_ptr<ClassDeclaration> classDeclaration)
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
	}

	void SymbolTable::BindFunction(const std::string& functionName, std::shared_ptr<FunctionDeclaration> functionDeclaration)
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

	std::shared_ptr<SymbolTable::SymbolBinding> SymbolTable::Lookup(const std::string& symbolName)
	{
		// If this is a reference (ie, separated by period(s)), recursively lookup each prefix
		auto lastPrefixDelimitter = symbolName.find_last_of('.');
		if (lastPrefixDelimitter != std::string::npos)
		{
			// Recursively look up every symbol preceding the final one.
			auto prefixSymbol = Lookup(symbolName.substr(0, lastPrefixDelimitter));
			if (prefixSymbol == nullptr)
				return nullptr;
			if (prefixSymbol->IsVariableBinding() || prefixSymbol->IsClassMemberBinding())
			{
				// We need the symbol for the class name
				prefixSymbol = Lookup(prefixSymbol->GetTypeInfo()->Name());
			}
			else if (prefixSymbol->IsFunctionBinding())
			{
				return nullptr; // Functions don't have submembers to reference
			}
			return Lookup(prefixSymbol->GetFullyQualifiedName(), symbolName.substr(lastPrefixDelimitter+1));
		}
		return LookupInImplicitNamespaces(symbolName);
	}

	std::shared_ptr<SymbolTable::SymbolBinding> SymbolTable::LookupInImplicitNamespaces(const std::string& symbolName)
	{
		for (auto iter = _currentAddressableNamespaces.rbegin(); iter != _currentAddressableNamespaces.rend(); ++iter)
		{
			auto symbol = Lookup((*iter)->GetFullyQualifiedName(), symbolName);
			if (symbol != nullptr)
				return symbol;
		}
		// Try the global namespace
		return Lookup("", symbolName);
	}

	std::shared_ptr<SymbolTable::SymbolBinding> SymbolTable::Lookup(const std::string& underNamespace, const std::string& symbolName)
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
		if (!IsVisibleFromCurrentContext(_map[name]))
		{
			throw SymbolNotAccessableException(name);
		}
		return _map[name];
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

	std::shared_ptr<SymbolTable::SymbolBinding> SymbolTable::GetCurrentFunction()
	{
		if (_currentFunction.size() == 0)
			return nullptr;
		return _currentFunction.top();
	}

	std::shared_ptr<SymbolTable::SymbolBinding> SymbolTable::GetCurrentClass()
	{
		if (_currentClass.size() == 0)
			return nullptr;
		return _currentClass.top();
	}

	std::shared_ptr<SymbolTable::SymbolBinding> SymbolTable::GetCurrentLoop()
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
		auto binding = std::make_shared<SymbolTable::MemberBinding>(name, GetFullyQualifiedName(), classMemberDeclaration);
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
		_typeInfo = std::make_shared<ClassTypeInfo>(classDeclaration);
	}

	std::shared_ptr<TypeInfo> SymbolTable::ClassBinding::GetTypeInfo()
	{
		return _typeInfo;
	}

	SymbolTable::MemberBinding::MemberBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<ClassMemberDeclaration> memberDeclaration) 
		: SymbolBinding(name, fullyQualifiedClassName.empty() ? name : fullyQualifiedClassName + "." + name, memberDeclaration->_visibility),
		_memberDeclaration(memberDeclaration)
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
		_allocation = _variableType->CreateAllocation(name, builder, context);
		return _allocation;
	}
}