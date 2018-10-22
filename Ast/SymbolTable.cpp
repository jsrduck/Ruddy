#include "stdafx.h"
#include "SymbolTable.h"

namespace Ast
{
	SymbolTable::SymbolTable()
	{
	}

	SymbolTable::~SymbolTable()
	{
	}

	std::shared_ptr<SymbolTable::VariableBinding> SymbolTable::BindVariable(const std::string& symbolName, std::shared_ptr<TypeInfo> type)
	{
		if (_map.count(symbolName) > 0)
		{
			// We don't allow shadowing in Ruddy. It's evil.
			throw SymbolAlreadyDefinedInThisScopeException(symbolName);
		} // TODO: Check for class/namespace/member collisions in current namespace
		auto binding = std::make_shared<VariableBinding>(shared_from_this(), symbolName, type);
		_map[symbolName] = binding;
		_aux_stack.push(binding);
		return binding;
	}

	void SymbolTable::BindNamespace(const std::string& namespaceName, TypeCheckPass pass)
	{
		std::shared_ptr<NamespaceBinding> binding;
		if (pass == CLASS_AND_NAMESPACE_DECLARATIONS)
		{
			binding = std::make_shared<NamespaceBinding>(shared_from_this(), _currentNamespace.size() > 0 ? _currentNamespace.top()->GetName() : "", namespaceName);
			if (_map.count(binding->GetFullyQualifiedName()) > 0)
			{
				throw SymbolAlreadyDefinedInThisScopeException(binding->GetName());
			} // TODO: Check for class/namespace/member collisions in any nested namespace
			_map[binding->GetFullyQualifiedName()] = binding;
		}
		else
		{
			// Lookup the previously defined binding
			binding = std::dynamic_pointer_cast<NamespaceBinding>(Lookup(namespaceName));
		}
		_aux_stack.push(binding);
		_currentNamespace.push(binding);
		_currentAddressableNamespaces.push_back(binding);
	}

	void SymbolTable::BindExternalNamespace(const std::string& namespaceName, const std::string& parentNamespace)
	{
		auto binding = std::make_shared<NamespaceBinding>(shared_from_this(), parentNamespace, namespaceName);
		if (_map.count(binding->GetFullyQualifiedName()) > 0)
		{
			throw SymbolAlreadyDefinedInThisScopeException(binding->GetName());
		} // TODO: Check for class/namespace/member collisions in any nested namespace
		_map[binding->GetFullyQualifiedName()] = binding;
	}

	std::shared_ptr<SymbolTable::ClassBinding> SymbolTable::BindClass(const std::string& className, Visibility visibility, TypeCheckPass pass)
	{
		std::shared_ptr<ClassBinding> binding;
		if (pass == CLASS_AND_NAMESPACE_DECLARATIONS)
		{
			binding = std::make_shared<ClassBinding>(shared_from_this(), className, _currentNamespace.size() > 0 ? _currentNamespace.top()->GetFullyQualifiedName() : "", visibility);
			if (_map.count(binding->GetFullyQualifiedName()) > 0)
			{
				throw SymbolAlreadyDefinedInThisScopeException(binding->GetName());
			} // TODO: Check for class/namespace/member collisions in any nested namespace
			_map[binding->GetFullyQualifiedName()] = binding;
		}
		else
		{
			// Lookup the previously defined binding
			binding = std::dynamic_pointer_cast<ClassBinding>(Lookup(className));
		}
		_aux_stack.push(binding);
		_currentClass.push(binding);
		_currentAddressableNamespaces.push_back(binding);
		return binding;
	}

	std::shared_ptr<SymbolTable::ClassBinding> SymbolTable::BindExternalClass(const std::string& className, const std::string& fullyQualifiedNamespace)
	{
		std::shared_ptr<ClassBinding> binding;
		binding = std::make_shared<ClassBinding>(shared_from_this(), className, fullyQualifiedNamespace, Visibility::PUBLIC);
		if (_map.count(binding->GetFullyQualifiedName()) > 0)
		{
			throw SymbolAlreadyDefinedInThisScopeException(binding->GetName());
		} // TODO: Check for class/namespace/member collisions in any nested namespace
		_map[binding->GetFullyQualifiedName()] = binding;
		return binding;
	}

	std::shared_ptr<SymbolTable::ConstructorBinding> SymbolTable::BindConstructor(std::shared_ptr<TypeInfo> inputArgs, Visibility visibility, std::shared_ptr<Modifier> mods, TypeCheckPass pass)
	{
		if (_currentFunction.size() > 0 || _currentClass.size() == 0)
		{
			throw UnexpectedException();
		}
		std::shared_ptr<ConstructorBinding> binding;
		if (pass == METHOD_DECLARATIONS)
		{
			binding = _currentClass.top()->AddConstructorBinding(inputArgs, visibility, mods);
			if (_map.count(binding->GetFullyQualifiedName()) > 0)
			{
				throw UnexpectedException();
			}
			//_map[binding->GetFullyQualifiedName()] = binding;
		}
		else
		{
			binding = std::dynamic_pointer_cast<ConstructorBinding>(_currentClass.top()->_ctors[0]); // TODO: Multiple c'tors
		}
		_aux_stack.push(binding);
		_currentFunction.push(binding);
		return binding;
	}

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::BindDestructor(TypeCheckPass pass)
	{
		if (_currentFunction.size() > 0 || _currentClass.size() == 0)
		{
			throw UnexpectedException();
		}
		std::shared_ptr<FunctionBinding> binding;
		if (pass == METHOD_DECLARATIONS)
		{
			binding = _currentClass.top()->AddDestructorBinding();
			if (_map.count(binding->GetFullyQualifiedName()) > 0)
			{
				throw UnexpectedException();
			}
			//_map[binding->GetFullyQualifiedName()] = binding;
		}
		else
		{
			binding = std::dynamic_pointer_cast<ConstructorBinding>(_currentClass.top()->_ctors[0]);
		}
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

	void SymbolTable::BindInitializer(const std::string& memberName)
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
		ctorBinding->AddInitializerBinding(memberName);
	}

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::BindFunction(Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods, TypeCheckPass pass)
	{
		if (_currentFunction.size() > 0 || _currentClass.size() == 0)
		{
			throw FunctionMustBeDeclaredInClassScopeException(name);
		}
		std::shared_ptr<FunctionBinding> binding;
		if (pass == METHOD_DECLARATIONS)
		{
			binding = _currentClass.top()->AddFunctionBinding(visibility, name, inputArgs, outputArgs, mods);
			if (_map.count(binding->GetFullyQualifiedName()) == 0)
			{
				_map[binding->GetFullyQualifiedName()] = binding;
			}
			else
			{
				// Must have an overloaded function now
				_map[binding->GetFullyQualifiedName()] = _currentClass.top()->_functions[name];
			}
		}
		else
		{
			binding = std::dynamic_pointer_cast<FunctionBinding>(Lookup(name));
			
			if (binding->IsOverridden())
			{
				auto asOverloaded = binding->GetOverloadedBinding();
				binding = asOverloaded->GetMatching(inputArgs);
				if (!binding)
					throw UnexpectedException();
			}
		}
		_aux_stack.push(binding);
		_currentFunction.push(binding);
		return binding;
	}

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::BindExternalFunction(std::shared_ptr<SymbolTable::ClassBinding> classBinding, Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods)
	{
		std::shared_ptr<FunctionBinding> binding;

		binding = classBinding->AddExternalFunctionBinding(visibility, name, inputArgs, outputArgs, mods);
		if (_map.count(binding->GetFullyQualifiedName()) == 0)
		{
			_map[binding->GetFullyQualifiedName()] = binding;
		}
		else
		{
			// Must have an overloaded function now
			_map[binding->GetFullyQualifiedName()] = classBinding->_functions[name];
		}

		return binding;
	}

	std::shared_ptr<Ast::SymbolTable::MemberBinding> SymbolTable::BindMemberVariable(const std::string& variableName, std::shared_ptr<TypeInfo> typeInfo, Visibility visibility, Modifier::Modifiers mods, TypeCheckPass pass)
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

		std::shared_ptr<MemberBinding> binding;
		if (pass == CLASS_VARIABLES)
		{
			binding = _currentClass.top()->AddMemberVariableBinding(variableName, typeInfo, visibility, mods);
			if (_map.count(binding->GetFullyQualifiedName()) > 0)
			{
				throw UnexpectedException();
			}
			_map[binding->GetFullyQualifiedName()] = binding;
		}
		else
		{
			binding = std::make_shared<SymbolTable::MemberBinding>(shared_from_this(), variableName, typeInfo, _currentClass.top(), visibility, mods);
			binding = std::dynamic_pointer_cast<MemberBinding>(_map[binding->GetFullyQualifiedName()]);
		}
		_aux_stack.push(binding);
		return binding;
	}

	std::shared_ptr<SymbolTable::MemberBinding> SymbolTable::BindExternalMemberVariable(std::shared_ptr<SymbolTable::ClassBinding> classBinding, const std::string & variableName, std::shared_ptr<TypeInfo> typeInfo, Visibility visibility, Modifier::Modifiers mods)
	{
		std::shared_ptr<MemberBinding> binding;
		binding = classBinding->AddExternalMemberVariableBinding(variableName, typeInfo, visibility, mods);
		if (_map.count(binding->GetFullyQualifiedName()) > 0)
		{
			throw UnexpectedException();
		}
		_map[binding->GetFullyQualifiedName()] = binding;
		return binding;
	}

	std::shared_ptr<Ast::SymbolTable::LoopBinding> SymbolTable::BindLoop()
	{
		auto binding = std::make_shared<LoopBinding>(shared_from_this());
		_aux_stack.push(binding);
		_currentLoop.push(binding);
		return binding;
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
			auto resolvedPrefixSymbol = prefixSymbol;
			if (prefixSymbol->IsLocalVariableBinding() || prefixSymbol->IsClassMemberBinding())
			{
				// We need the symbol for the class name
				resolvedPrefixSymbol = Lookup(prefixSymbol->GetTypeInfo()->Name(), checkIsInitialized);
			}
			else if (prefixSymbol->IsFunctionBinding())
			{
				return nullptr; // Functions don't have submembers to reference
			}
			auto retVal = Lookup(resolvedPrefixSymbol->GetFullyQualifiedName(), symbolName.substr(lastPrefixDelimitter+1), checkIsInitialized);
			if (!retVal)
				return retVal;
			if (retVal->IsClassMemberBinding())
			{
				auto asClassMember = std::dynamic_pointer_cast<MemberBinding>(retVal);
				if (!asClassMember->_modifier->IsStatic())
				{
					return std::make_shared<MemberInstanceBinding>(asClassMember, prefixSymbol);
					// TODO: Static?
				}
			}
			else if (retVal->IsFunctionBinding() && (prefixSymbol->IsLocalVariableBinding() || prefixSymbol->IsClassMemberBinding()))
			{
				auto asFunctionBinding = std::dynamic_pointer_cast<FunctionBinding>(retVal);
				if (asFunctionBinding->IsMethod())
				{
					return std::make_shared<FunctionInstanceBinding>(asFunctionBinding, prefixSymbol);
				}
			}
			return retVal;
		}

		auto retVal = LookupInImplicitNamespaces(symbolName, checkIsInitialized);
		if (retVal != nullptr && retVal->IsClassMemberBinding())
		{
			auto asClassMember = std::dynamic_pointer_cast<MemberBinding>(retVal);
			if (!asClassMember->_modifier->IsStatic())
			{
				// Must be a binding to "this"
				auto thisSymbol = Lookup("this");
				if (thisSymbol)
					return std::make_shared<MemberInstanceBinding>(asClassMember, thisSymbol);
			}
		}
		else if (retVal != nullptr && retVal->IsFunctionBinding())
		{
			auto asMethod = std::dynamic_pointer_cast<FunctionBinding>(retVal);
			if (asMethod != nullptr && asMethod->IsMethod())
			{
				// Must be a binding to "this"
				auto thisSymbol = Lookup("this");
				if (thisSymbol)
					return std::make_shared<FunctionInstanceBinding>(asMethod, thisSymbol);
			}
		}

		// check external libraries if you can't find it here
		if (retVal == nullptr)
		{
			for (auto& library : _externalLibraries)
			{
				retVal = library->Lookup(symbolName, checkIsInitialized);
				if (retVal != nullptr)
					break;
			}
		}
		return retVal;
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
		std::shared_ptr<SymbolBinding> binding;
		if (_map.count(name) == 0)
		{
			// Check external libs for this name
			for (auto& library : _externalLibraries)
			{
				binding = library->Lookup(underNamespace, symbolName, checkIsInitialized);
				if (binding != nullptr)
					break;
			}
			if (binding == nullptr)
				return nullptr;
		}
		else
		{
			binding = _map[name];
		}
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
				if (memberBinding->_classBindingForParentType == GetCurrentClass() && ctorBinding->_initializers.count(binding->GetName()) == 0)
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
		_aux_stack.push(std::make_shared<ScopeMarker>(shared_from_this()));
	}

	std::vector<std::shared_ptr<SymbolTable::BaseVariableBinding>> SymbolTable::Exit()
	{
		std::shared_ptr<SymbolBinding> binding;
		std::vector<std::shared_ptr<BaseVariableBinding>> dtors;
		do
		{
			if (_aux_stack.size() == 0)
				throw UnexpectedException();
			binding = _aux_stack.top();
			_aux_stack.pop();
			if (binding->IsVariableBinding())
			{
				auto asVariable = std::dynamic_pointer_cast<BaseVariableBinding>(binding);
				if (asVariable->IsReferenceVariable())
				{
					dtors.push_back(asVariable);
				}
			}
			if (binding->IsLocalVariableBinding())
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
		return dtors;
	}

	std::vector<std::shared_ptr<SymbolTable::BaseVariableBinding>> SymbolTable::BreakFromCurrentLoop()
	{
		std::shared_ptr<SymbolBinding> binding;
		std::vector<std::shared_ptr<BaseVariableBinding>> dtors;
		std::stack<std::shared_ptr<SymbolBinding>> replacementStack;
		bool leftScopeMarker = false;
		do
		{
			if (_aux_stack.size() == 0)
				break;
			binding = _aux_stack.top();
			_aux_stack.pop();
			if (!leftScopeMarker)
				leftScopeMarker = binding->IsScopeMarker() || binding->IsFunctionBinding() || binding->IsLoopBinding();
			if (leftScopeMarker)
				replacementStack.push(binding);
			else
				_map.erase(binding->GetFullyQualifiedName());
			if (binding->IsVariableBinding())
			{
				auto asVariable = std::dynamic_pointer_cast<BaseVariableBinding>(binding);
				if (asVariable->IsReferenceVariable())
				{
					dtors.push_back(asVariable);
				}
			}
		} while (!binding->IsLoopBinding());

		while (replacementStack.size() > 0)
		{
			binding = replacementStack.top();
			replacementStack.pop();
			_aux_stack.push(binding);
		}
		return dtors;
	}

	std::vector<std::shared_ptr<SymbolTable::BaseVariableBinding>> SymbolTable::ReturnFromCurrentFunction()
	{
		std::shared_ptr<SymbolBinding> binding;
		std::vector<std::shared_ptr<BaseVariableBinding>> dtors;
		std::stack<std::shared_ptr<SymbolBinding>> replacementStack;
		bool leftScopeMarker = false;
		do
		{
			if (_aux_stack.size() == 0)
				break;
			binding = _aux_stack.top();
			_aux_stack.pop();
			if (!leftScopeMarker)
				leftScopeMarker = binding->IsScopeMarker() || binding->IsFunctionBinding() || binding->IsLoopBinding();
			if (leftScopeMarker)
				replacementStack.push(binding);
			else
				_map.erase(binding->GetFullyQualifiedName());

			if (binding->IsVariableBinding())
			{
				auto asVariable = std::dynamic_pointer_cast<BaseVariableBinding>(binding);
				if (asVariable->IsReferenceVariable())
				{
					dtors.push_back(asVariable);
				}
			}
		} while (!binding->IsFunctionBinding());

		while (replacementStack.size() > 0)
		{
			binding = replacementStack.top();
			replacementStack.pop();
			_aux_stack.push(binding);
		}
		return dtors;
	}

	void Ast::SymbolTable::AddExternalLibrary(const std::string& libName, std::function<std::shared_ptr<SymbolTable>()> GetSymbolTable)
	{
		if (_unactivatedExternalLibraries.count(libName) != 0)
			throw DuplicateLibraryException(libName);
		_unactivatedExternalLibraries[libName] = GetSymbolTable;
	}

	void Ast::SymbolTable::ActivateExternalLibrary(const std::string& libName)
	{
		if (_unactivatedExternalLibraries.count(libName) == 0)
			throw UnknownLibraryException(libName);
		if (_activatedLibraries.count(libName) == 0)
		{
			auto library = _unactivatedExternalLibraries[libName]();
			_activatedLibraries.emplace(libName);
			_externalLibraries.push_back(library);
		}
	}
}