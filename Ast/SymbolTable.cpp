#include "stdafx.h"
#include "SymbolTable.h"
#include "Classes.h"
#include "Statements.h"

#include <boost\property_tree\ptree_serialization.hpp>
#include <boost\property_tree\json_parser.hpp>
#include <boost\algorithm\string.hpp>

#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

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
		if (type->IsClassType())
		{
			auto classTypeInfo = std::dynamic_pointer_cast<BaseClassTypeInfo>(type);
			if (classTypeInfo->IsValueType())
			{
				auto classBinding = std::dynamic_pointer_cast<SymbolTable::ClassBinding>(Lookup(classTypeInfo->FullyQualifiedName(shared_from_this())));
				binding->_onExit = classBinding->_dtorBinding->CreateCall(binding, FileLocationContext::CurrentLocation());
			}
		}
		_map[symbolName] = binding;
		_aux_stack.push(binding);
		return binding;
	}

	void SymbolTable::BindNamespace(const std::string& namespaceName, TypeCheckPass pass)
	{
		std::shared_ptr<NamespaceBinding> binding;
		if (pass == CLASS_AND_NAMESPACE_DECLARATIONS)
		{
			binding = std::make_shared<NamespaceBinding>(_currentNamespace.size() > 0 ? _currentNamespace.top()->GetName() : "", namespaceName);
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
		auto binding = std::make_shared<NamespaceBinding>(parentNamespace, namespaceName);
		if (_map.count(binding->GetFullyQualifiedName()) > 0)
		{
			throw SymbolAlreadyDefinedInThisScopeException(binding->GetName());
		} // TODO: Check for class/namespace/member collisions in any nested namespace
		_map[binding->GetFullyQualifiedName()] = binding;
	}

	std::shared_ptr<SymbolTable::ClassBinding> SymbolTable::BindClass(const std::string& className, std::shared_ptr<ClassDeclaration> classDeclaration, TypeCheckPass pass)
	{
		std::shared_ptr<ClassBinding> binding;
		if (pass == CLASS_AND_NAMESPACE_DECLARATIONS)
		{
			binding = std::make_shared<ClassBinding>(className, _currentNamespace.size() > 0 ? _currentNamespace.top()->GetFullyQualifiedName() : "", classDeclaration);
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

	std::shared_ptr<SymbolTable::ClassBinding> SymbolTable::BindExternalClass(const std::string& className, const std::string& fullyQualifiedName)
	{
		std::shared_ptr<ClassBinding> binding;
		binding = std::make_shared<ClassBinding>(className, fullyQualifiedName, Visibility::PUBLIC);
		if (_map.count(binding->GetFullyQualifiedName()) > 0)
		{
			throw SymbolAlreadyDefinedInThisScopeException(binding->GetName());
		} // TODO: Check for class/namespace/member collisions in any nested namespace
		_map[binding->GetFullyQualifiedName()] = binding;
		return binding;
	}

	std::shared_ptr<SymbolTable::ConstructorBinding> SymbolTable::BindConstructor(std::shared_ptr<FunctionDeclaration> functionDeclaration, TypeCheckPass pass)
	{
		if (_currentFunction.size() > 0 || _currentClass.size() == 0)
		{
			throw UnexpectedException();
		}
		std::shared_ptr<ConstructorBinding> binding;
		if (pass == METHOD_DECLARATIONS)
		{
			binding = _currentClass.top()->AddConstructorBinding(functionDeclaration, shared_from_this());
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

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::BindDestructor(std::shared_ptr<DestructorDeclaration> functionDeclaration, TypeCheckPass pass)
	{
		if (_currentFunction.size() > 0 || _currentClass.size() == 0)
		{
			throw UnexpectedException();
		}
		std::shared_ptr<FunctionBinding> binding;
		if (pass == METHOD_DECLARATIONS)
		{
			binding = _currentClass.top()->AddDestructorBinding(functionDeclaration, shared_from_this());
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

	void SymbolTable::BindInitializer(const std::string& memberName, std::shared_ptr<StackConstructionExpression> assignment)
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

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::BindFunction(std::shared_ptr<FunctionDeclaration> functionDeclaration, TypeCheckPass pass)
	{
		if (_currentFunction.size() > 0 || _currentClass.size() == 0)
		{
			throw FunctionMustBeDeclaredInClassScopeException(functionDeclaration->Name());
		}
		std::shared_ptr<FunctionBinding> binding;
		if (pass == METHOD_DECLARATIONS)
		{
			binding = _currentClass.top()->AddFunctionBinding(functionDeclaration, shared_from_this());
			if (_map.count(binding->GetFullyQualifiedName()) == 0)
			{
				_map[binding->GetFullyQualifiedName()] = binding;
			}
			else
			{
				// Must have an overloaded function now
				_map[binding->GetFullyQualifiedName()] = _currentClass.top()->_functions[functionDeclaration->Name()];
			}
		}
		else
		{
			binding = std::dynamic_pointer_cast<FunctionBinding>(Lookup(functionDeclaration->Name()));
			
			if (binding->IsOverridden())
			{
				auto asOverloaded = binding->GetOverloadedBinding();
				binding = asOverloaded->GetMatching(functionDeclaration->_inputArgsType, shared_from_this());
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

		binding = classBinding->AddExternalFunctionBinding(shared_from_this(), visibility, name, inputArgs, outputArgs, mods);
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

	std::shared_ptr<Ast::SymbolTable::MemberBinding> SymbolTable::BindMemberVariable(const std::string& variableName, std::shared_ptr<ClassMemberDeclaration> memberVariable, TypeCheckPass pass)
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
			binding = _currentClass.top()->AddMemberVariableBinding(variableName, memberVariable);
			if (_map.count(binding->GetFullyQualifiedName()) > 0)
			{
				throw UnexpectedException();
			}
			_map[binding->GetFullyQualifiedName()] = binding;
		}
		else
		{
			binding = std::make_shared<SymbolTable::MemberBinding>(variableName, _currentClass.top()->GetFullyQualifiedName(), memberVariable, _currentClass.top());
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
		auto binding = std::make_shared<LoopBinding>();
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
			if (prefixSymbol->IsVariableBinding() || prefixSymbol->IsClassMemberBinding())
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
			else if (retVal->IsFunctionBinding() && (prefixSymbol->IsVariableBinding() || prefixSymbol->IsClassMemberBinding()))
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

	std::vector<std::shared_ptr<FunctionCall>> SymbolTable::Exit()
	{
		std::shared_ptr<SymbolBinding> binding;
		std::vector<std::shared_ptr<FunctionCall>> dtors;
		do
		{
			if (_aux_stack.size() == 0)
				throw UnexpectedException();
			binding = _aux_stack.top();
			_aux_stack.pop();
			if (binding->_onExit != nullptr)
				dtors.push_back(binding->_onExit);
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
		return dtors;
	}

	std::vector<std::shared_ptr<FunctionCall>> SymbolTable::BreakFromCurrentLoop()
	{
		std::shared_ptr<SymbolBinding> binding;
		std::vector<std::shared_ptr<FunctionCall>> dtors;
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
			if (binding->_onExit != nullptr)
				dtors.push_back(binding->_onExit);
		} while (!binding->IsLoopBinding());

		while (replacementStack.size() > 0)
		{
			binding = replacementStack.top();
			replacementStack.pop();
			_aux_stack.push(binding);
		}
		return dtors;
	}

	std::vector<std::shared_ptr<FunctionCall>> Ast::SymbolTable::ReturnFromCurrentFunction()
	{
		std::shared_ptr<SymbolBinding> binding;
		std::vector<std::shared_ptr<FunctionCall>> dtors;
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
			if (binding->_onExit != nullptr)
				dtors.push_back(binding->_onExit);
		} while (!binding->IsFunctionBinding());

		while (replacementStack.size() > 0)
		{
			binding = replacementStack.top();
			replacementStack.pop();
			_aux_stack.push(binding);
		}
		return dtors;
	}

	std::shared_ptr<SymbolTable::ConstructorBinding> SymbolTable::ClassBinding::AddConstructorBinding(std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<SymbolTable> symbolTable)
	{
		auto binding = std::make_shared<SymbolTable::ConstructorBinding>(_name, GetFullyQualifiedName(), functionDeclaration, shared_from_this());
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

	std::shared_ptr<SymbolTable::ConstructorBinding> SymbolTable::ClassBinding::AddExternalConstructorBinding(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<Modifier> mods)
	{
		auto binding = std::make_shared<SymbolTable::ConstructorBinding>(_name, GetFullyQualifiedName(), Visibility::PUBLIC, inputArgs, mods, shared_from_this());
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

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::ClassBinding::AddDestructorBinding(std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<SymbolTable> symbolTable)
	{
		auto binding = std::make_shared<SymbolTable::FunctionBinding>(GetFullyQualifiedName(), functionDeclaration, shared_from_this());
		_dtorBinding = binding;
		return binding;
	}

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::ClassBinding::AddExternalDestructorBinding(std::shared_ptr<SymbolTable> symbolTable)
	{
		auto binding = std::make_shared<SymbolTable::FunctionBinding>(GetName(), GetFullyQualifiedName(), Visibility::PRIVATE, nullptr, nullptr, nullptr, shared_from_this());
		_dtorBinding = binding;
		return binding;
	}

	void SymbolTable::ConstructorBinding::AddInitializerBinding(const std::string& memberName, std::shared_ptr<StackConstructionExpression> assignment)
	{
		if (_initializers.count(memberName) > 0)
		{
			throw CannotReinitializeMemberException(memberName);
		}
		_initializers[memberName] = assignment;
	}

	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::ClassBinding::AddFunctionBinding(std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<SymbolTable> symbolTable)
	{
		auto binding = std::make_shared<SymbolTable::FunctionBinding>(GetFullyQualifiedName(), functionDeclaration, shared_from_this());
		if (_functions.count(functionDeclaration->Name()) > 0)
		{
			// Is it overloaded?
			auto existingBinding = _functions[functionDeclaration->Name()];
			if (existingBinding->IsOverridden())
			{
				auto asOverloaded = existingBinding->GetOverloadedBinding();
				for (auto otherBinding : asOverloaded->_bindings)
				{
					if (FunctionBinding::HaveSameSignatures(std::dynamic_pointer_cast<FunctionTypeInfo>(otherBinding->GetTypeInfo())->InputArgsType(), std::dynamic_pointer_cast<FunctionTypeInfo>(binding->GetTypeInfo())->InputArgsType(), symbolTable))
					{
						throw SymbolAlreadyDefinedInThisScopeException(functionDeclaration->Name());
					}
				}
				asOverloaded->AddBinding(binding);
				return binding;
			}
			else
			{
				if (FunctionBinding::HaveSameSignatures(std::dynamic_pointer_cast<FunctionTypeInfo>(existingBinding->GetTypeInfo())->InputArgsType(), std::dynamic_pointer_cast<FunctionTypeInfo>(binding->GetTypeInfo())->InputArgsType(), symbolTable))
				{
					throw SymbolAlreadyDefinedInThisScopeException(functionDeclaration->Name());
				}
				_functions[functionDeclaration->Name()] = std::make_shared<OverloadedFunctionBinding>(existingBinding, binding);
				return binding;
			}
		}
		_functions[functionDeclaration->Name()] = binding;
		return binding;
	}
	
	std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::ClassBinding::AddExternalFunctionBinding(std::shared_ptr<SymbolTable> symbolTable, Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods)
	{
		auto binding = std::make_shared<SymbolTable::FunctionBinding>(name, GetFullyQualifiedName(), visibility, inputArgs, outputArgs, std::make_shared<Modifier>(mods), shared_from_this());
		if (_functions.count(name) > 0)
		{
			// Is it overloaded?
			auto existingBinding = _functions[name];
			if (existingBinding->IsOverridden())
			{
				auto asOverloaded = existingBinding->GetOverloadedBinding();
				for (auto otherBinding : asOverloaded->_bindings)
				{
					if (FunctionBinding::HaveSameSignatures(std::dynamic_pointer_cast<FunctionTypeInfo>(otherBinding->GetTypeInfo())->InputArgsType(), std::dynamic_pointer_cast<FunctionTypeInfo>(binding->GetTypeInfo())->InputArgsType(), symbolTable))
					{
						throw SymbolAlreadyDefinedInThisScopeException(name);
					}
				}
				asOverloaded->AddBinding(binding);
				return binding;
			}
			else
			{
				if (FunctionBinding::HaveSameSignatures(std::dynamic_pointer_cast<FunctionTypeInfo>(existingBinding->GetTypeInfo())->InputArgsType(), std::dynamic_pointer_cast<FunctionTypeInfo>(binding->GetTypeInfo())->InputArgsType(), symbolTable))
				{
					throw SymbolAlreadyDefinedInThisScopeException(name);
				}
				_functions[name] = std::make_shared<OverloadedFunctionBinding>(existingBinding, binding);
				return binding;
			}
		}
		_functions[name] = binding;
		return binding;
	}

	size_t Ast::SymbolTable::ClassBinding::NumMembers()
	{
		return _members.size();
	}

	std::shared_ptr<SymbolTable::MemberBinding> SymbolTable::ClassBinding::AddMemberVariableBinding(const std::string& name, std::shared_ptr<ClassMemberDeclaration> classMemberDeclaration)
	{
		auto binding = std::make_shared<SymbolTable::MemberBinding>(name, GetFullyQualifiedName(), classMemberDeclaration, shared_from_this());
		if (std::any_of(_members.begin(), _members.end(), [&](std::shared_ptr<Ast::SymbolTable::MemberBinding> binding)
		{
			return binding->GetName().compare(name) == 0;
		}))
		{
			throw SymbolAlreadyDefinedInThisScopeException(name);
		}
		_members.push_back(binding);
		return binding;
	}

	std::shared_ptr<SymbolTable::MemberBinding> SymbolTable::ClassBinding::AddExternalMemberVariableBinding(const std::string& name, std::shared_ptr<TypeInfo> typeInfo, Visibility visibility, Modifier::Modifiers mods)
	{
		auto binding = std::make_shared<SymbolTable::MemberBinding>(name, GetFullyQualifiedName(), typeInfo, shared_from_this(), visibility, mods);
		if (std::any_of(_members.begin(), _members.end(), [&](std::shared_ptr<Ast::SymbolTable::MemberBinding> binding)
		{
			return binding->GetName().compare(name) == 0;
		}))
		{
			throw SymbolAlreadyDefinedInThisScopeException(name);
		}
		_members.push_back(binding);
		return binding;
	}

	SymbolTable::FunctionBinding::FunctionBinding(const std::string& fullyQualifiedClassName, std::shared_ptr<FunctionDeclaration> functionDeclaration, std::shared_ptr<ClassBinding> classBinding)
		: SymbolBinding(functionDeclaration->Name(), fullyQualifiedClassName.empty() ? functionDeclaration->Name() : fullyQualifiedClassName + "." + functionDeclaration->Name(), functionDeclaration->_visibility)
	{
		_typeInfo = std::make_shared<FunctionTypeInfo>(functionDeclaration);
		_classBinding = classBinding;
	}

	SymbolTable::FunctionBinding::FunctionBinding(const std::string& name, const std::string & fullyQualifiedClassName, Visibility visibility, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, std::shared_ptr<Modifier> mods, std::shared_ptr<ClassBinding> classBinding)
		: SymbolBinding(name, fullyQualifiedClassName.empty() ? name : fullyQualifiedClassName + "." + name, visibility)
	{
		_typeInfo = std::make_shared<FunctionTypeInfo>(name, inputArgs, outputArgs, mods);
		_classBinding = classBinding;
	}

	std::shared_ptr<TypeInfo> SymbolTable::FunctionBinding::GetTypeInfo()
	{
		return _typeInfo;
	}

	bool Ast::SymbolTable::FunctionBinding::HaveSameSignatures(std::shared_ptr<TypeInfo> inputArgs1, std::shared_ptr<TypeInfo> inputArgs2, std::shared_ptr<SymbolTable> symbolTable)
	{
		if ((inputArgs1 == nullptr && inputArgs2 != nullptr) ||
			(inputArgs1 != nullptr && inputArgs2 == nullptr))
		{
			return false;
		}
		else if (inputArgs2 != nullptr &&
			!inputArgs2->IsImplicitlyAssignableFrom(inputArgs1, symbolTable))
		{
			return false;
		}
		else
		{
			// Found it!
			return true;
		}
	}

	std::shared_ptr<FunctionCall> SymbolTable::FunctionBinding::CreateCall(std::shared_ptr<Ast::SymbolTable::SymbolBinding> varBinding, FileLocation& location, std::shared_ptr<Expression> expression)
	{
		return std::make_shared<FunctionCall>(std::dynamic_pointer_cast<FunctionTypeInfo>(GetTypeInfo()), expression, shared_from_this(), varBinding, location);
	}

	llvm::Value * Ast::SymbolTable::FunctionBinding::GetIRValue(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		if (_value != nullptr)
			return _value;

		std::vector<llvm::Type*> argTypes;
		bool isMethod = !_typeInfo->_mods->IsStatic();
		llvm::Type* ptrToThisType = nullptr;
		if (isMethod)
		{
			// For non-static methods, add a pointer to "this" as the 1st argument
			//auto argument = std::make_shared<Argument>(_classBinding->GetTypeInfo(), "this");
			//_inputArgs = std::make_shared<ArgumentList>(argument, _inputArgs);
			auto thisType = _classBinding->GetTypeInfo()->GetIRType(context);
			ptrToThisType = llvm::PointerType::get(thisType, 0);
			argTypes.push_back(ptrToThisType);
		}
		if (_typeInfo->InputArgsType() != nullptr)
			_typeInfo->InputArgsType()->AddIRTypesToVector(argTypes, context);

		// Return type
		llvm::Type* retType = llvm::Type::getVoidTy(*context);
		auto outputArgsType = _typeInfo->OutputArgsType();
		auto multiArgType = std::dynamic_pointer_cast<CompositeTypeInfo>(outputArgsType);
		if (outputArgsType != nullptr)
		{
			if (outputArgsType->IsComposite())
			{
				retType = multiArgType->_thisType->GetIRType(context);
				// LLVM doesn't support multiple return types, so we'll treat them as out
				// paramaters
				multiArgType->_next->AddIRTypesToVector(argTypes, context, true /*asOutput*/);
			}
			else
			{
				retType = outputArgsType->GetIRType(context);
			}
		}

		auto name = GetFullyQualifiedName();
		if (name.compare("Program.main") == 0)
			name = "main";

		llvm::Function* function = llvm::Function::Create(llvm::FunctionType::get(retType, argTypes, false /*isVarArg*/), llvm::Function::ExternalLinkage /*TODO*/, name, module);
		if (_visibility == PUBLIC)
			function->setDLLStorageClass(llvm::GlobalValue::DLLStorageClassTypes::DLLExportStorageClass);
		_value = function;
		return _value;
	}

	Ast::SymbolTable::OverloadedFunctionBinding::OverloadedFunctionBinding(std::shared_ptr<FunctionBinding> functionBinding1, std::shared_ptr<FunctionBinding> functionBinding2)
		: FunctionBinding(functionBinding1->GetName(), functionBinding1->GetParentNamespaceName(), functionBinding1->GetVisibility(), nullptr, nullptr, nullptr, functionBinding1->_classBinding)
	{
		_bindings.push_back(functionBinding1);
		_bindings.push_back(functionBinding2);
	}

	void Ast::SymbolTable::OverloadedFunctionBinding::AddBinding(std::shared_ptr<FunctionBinding> functionBinding)
	{
		_bindings.push_back(functionBinding);
	}

	std::shared_ptr<Ast::SymbolTable::FunctionBinding> Ast::SymbolTable::OverloadedFunctionBinding::GetMatching(std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<SymbolTable> symbolTable)
	{
		for (auto binding : _bindings)
		{
			if (FunctionBinding::HaveSameSignatures(inputArgs, std::dynamic_pointer_cast<FunctionTypeInfo>(binding->GetTypeInfo())->InputArgsType(), symbolTable))
				return binding;
		}
		return nullptr;
	}

	SymbolTable::ClassBinding::ClassBinding(const std::string& name, const std::string& fullyQualifiedNamespaceName, std::shared_ptr<ClassDeclaration> classDeclaration) 
		: SymbolBinding(name, fullyQualifiedNamespaceName.empty() ? name : fullyQualifiedNamespaceName + "." + name, classDeclaration->_visibility)
	{
		_typeInfo = std::make_shared<ClassDeclarationTypeInfo>(classDeclaration, GetFullyQualifiedName());
	}

	SymbolTable::ClassBinding::ClassBinding(const std::string& name, const std::string& fullyQualifiedNamespaceName, Visibility visibility)
		: SymbolBinding(name, fullyQualifiedNamespaceName, visibility)
	{
		_typeInfo = std::make_shared<ClassDeclarationTypeInfo>(name, fullyQualifiedNamespaceName);
	}

	std::shared_ptr<TypeInfo> SymbolTable::ClassBinding::GetTypeInfo()
	{
		return _typeInfo;
	}

	SymbolTable::MemberBinding::MemberBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<ClassMemberDeclaration> memberDeclaration, std::shared_ptr<ClassBinding> classBinding)
		: SymbolBinding(name, fullyQualifiedClassName.empty() ? name : fullyQualifiedClassName + "." + name, memberDeclaration->_visibility),
		_classBinding(classBinding), _index(classBinding->NumMembers()), _modifier(memberDeclaration->_mods)
	{
		_typeInfo = memberDeclaration->_typeInfo;
	}

	SymbolTable::MemberBinding::MemberBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<TypeInfo> typeInfo, std::shared_ptr<ClassBinding> classBinding, Visibility visibility, Modifier::Modifiers mods)
		: SymbolBinding(name, fullyQualifiedClassName.empty() ? name : fullyQualifiedClassName + "." + name, visibility),
		_classBinding(classBinding), _index(classBinding->NumMembers()), _modifier(std::make_shared<Modifier>(mods))
	{
		_typeInfo = typeInfo;
	}

	SymbolTable::MemberBinding::MemberBinding(std::shared_ptr<MemberBinding> other)
		: SymbolBinding(other->GetName(), other->GetFullyQualifiedName(), other->GetVisibility()),
		_classBinding(other->_classBinding), _index(other->_index)
	{
		_typeInfo = other->_typeInfo;
	}

	llvm::Value * SymbolTable::MemberInstanceBinding::GetIRValue(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		std::vector<llvm::Value*> idxVec;
		idxVec.push_back(llvm::ConstantInt::get(*context, llvm::APInt(32, 0, true)));
		idxVec.push_back(llvm::ConstantInt::get(*context, llvm::APInt(32, _index, true)));
		return builder->CreateGEP(_reference->GetIRValue(builder, context, module), idxVec);
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

	/* Serialization */

	void SymbolTable::Serialize(std::ostream& output, std::string libName)
	{
		ptree root;
		root.put("Name", libName);

		ptree symbols;
		ptree namespaces;
		for (auto& symbol : _map)
		{
			if (symbol.second->IsClassBinding() && symbol.second->GetVisibility() == Visibility::PUBLIC)
			{
				auto asClass = std::dynamic_pointer_cast<ClassBinding>(symbol.second);
				symbols.add_child("Class", asClass->Serialize(shared_from_this()));
			}
			else if (symbol.second->IsNamespaceBinding())
			{
				ptree namespaceName;
				namespaceName.put("", symbol.second->GetFullyQualifiedName());
				namespaces.push_back(std::make_pair("", namespaceName));
			}
		}

		root.add_child("Namespaces", namespaces);
		root.add_child("Symbols", symbols);
		boost::property_tree::write_json(output, root, true /*prettyPrint*/);
	}

	boost::property_tree::ptree Ast::SymbolTable::SymbolBinding::Serialize(std::shared_ptr<Ast::SymbolTable> symbolTable)
	{
		ptree symbol;
		symbol.add("Name", GetFullyQualifiedName());
		SerializeInternal(symbolTable, symbol);
		return symbol;
	}

	void Ast::SymbolTable::FunctionBinding::SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& symbol)
	{
		if (_typeInfo->InputArgsType() != nullptr)
		{
			symbol.add("Input", _typeInfo->InputArgsType()->SerializedName(symbolTable));
			// TODO non-public types in args
		}
		if (_typeInfo->OutputArgsType() != nullptr)
		{
			symbol.add("Output", _typeInfo->OutputArgsType()->SerializedName(symbolTable));
			// TODO non-public types in args
		}
		if (_typeInfo->_mods->Get() != Modifier::Modifiers::NONE)
		{
			symbol.add("Mod", static_cast<int>(_typeInfo->_mods->Get()));
		}
		symbol.add("Visibility", static_cast<int>(GetVisibility()));
	}

	void Ast::SymbolTable::OverloadedFunctionBinding::SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& symbol)
	{
		for (auto& binding : _bindings)
		{
			if (binding->GetVisibility() == Visibility::PUBLIC)
			{
				boost::property_tree::ptree bindingTree;
				binding->SerializeInternal(symbolTable, bindingTree);
				symbol.add_child("Overload", bindingTree);
			}
		}
	}

	void Ast::SymbolTable::MemberBinding::SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree & symbol)
	{
		symbol.add("Type", _typeInfo->SerializedName(symbolTable));
		if (_modifier->Get() != Modifier::Modifiers::NONE)
			symbol.add("Mod", static_cast<int>(_modifier->Get()));
		symbol.add("Visibility", static_cast<int>(GetVisibility()));
	}

	void Ast::SymbolTable::ClassBinding::SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree & symbol)
	{
		ptree ctors;
		for (auto& ctor : _ctors)
		{
			if (ctor->GetVisibility() != Visibility::PRIVATE)
				ctors.add_child("Ctor", ctor->Serialize(symbolTable));
		}
		
		symbol.add_child("Ctors", ctors);

		if (_members.size() > 0)
		{
			ptree members;
			for (auto& mem : _members)
			{
				if (mem->GetVisibility() != Visibility::PRIVATE)
					members.add_child("Member", mem->Serialize(symbolTable));
			}
			symbol.add_child("Mems", members);
		}

		if (_functions.size() > 0)
		{
			ptree funs;
			for (auto& fun : _functions)
			{
				if (fun.second->GetVisibility() != Visibility::PRIVATE)
					funs.add_child("Function", fun.second->Serialize(symbolTable));
			}
			symbol.add_child("Funs", funs);
		}
	}

	void SymbolTable::LoadFrom(std::istream& input)
	{
		ptree root;
		boost::property_tree::read_json(input, root);
		auto namespaces = root.get_child("Namespaces");
		for (auto& namespaceEntry : namespaces)
		{
			auto namespaceFullyQualified = namespaceEntry.second.get_value<std::string>();
			auto namespaceEndOfParent = namespaceFullyQualified.find_last_of('.');
			auto name = namespaceEndOfParent != std::string::npos ? namespaceFullyQualified.substr(namespaceEndOfParent + 1) : namespaceFullyQualified;
			auto parent = namespaceEndOfParent != std::string::npos ? namespaceFullyQualified.substr(0, namespaceEndOfParent) : "";
			BindExternalNamespace(name, parent);
		}

		auto symbolTree = root.get_child("Symbols");

		for (auto& symbol : symbolTree)
		{
			if (symbol.first.compare("Class") == 0)
			{
				ClassBinding::LoadFrom(shared_from_this(), symbol.second);
			}
			else
			{
				throw UnexpectedException();
			}
		}
	}

	std::shared_ptr<TypeInfo> LoadTypeInfo(const std::string& name)
	{
		if (name.empty())
			return nullptr;
		std::vector<std::string> types;
		boost::split(types, name, boost::is_any_of(","));

		if (types.size() == 1)
		{
			auto typeInfo = PrimitiveTypeInfo::LoadFrom(types[0]);
			if (typeInfo == nullptr)
			{
				typeInfo = std::make_shared<UnresolvedClassTypeInfo>(types[0]);
			}
			return typeInfo;
		}
		else
		{
			std::shared_ptr<CompositeTypeInfo> last = nullptr;
			std::shared_ptr<CompositeTypeInfo> retVal = nullptr;
			for (auto& typeName : types)
			{
				auto typeInfo = PrimitiveTypeInfo::LoadFrom(typeName);
				if (typeInfo == nullptr)
				{
					typeInfo = std::make_shared<UnresolvedClassTypeInfo>(typeName);
				}
				auto asComposite = std::make_shared<CompositeTypeInfo>(typeInfo);;
				if (last == nullptr)
				{
					last = asComposite;
					retVal = last;
				}
				else
				{
					last->_next = asComposite;
					last = asComposite;
				}
			}
			return retVal;
		}
	}

	void LoadFunction(boost::property_tree::basic_ptree<std::string, std::string> &funPtree, std::shared_ptr<Ast::SymbolTable::ClassBinding> &classBinding, std::shared_ptr<Ast::SymbolTable> &symbolTable, std::string &funName)
	{
		// Get input type
		std::shared_ptr<TypeInfo> inputArgTypeInfo;
		auto input = funPtree.get_optional<std::string>("Input");
		if (input)
			inputArgTypeInfo = LoadTypeInfo(*input);

		// Get output type
		std::shared_ptr<TypeInfo> outputArgTypeInfo;
		auto output = funPtree.get_optional<std::string>("Output");
		if (output)
			outputArgTypeInfo = LoadTypeInfo(*output);

		// Get modifiers
		auto mods = funPtree.get_optional<int>("Mod");

		// Get visibility
		auto visibility = static_cast<Visibility>(funPtree.get<int>("Visibility"));

		symbolTable->BindExternalFunction(classBinding, visibility, funName, inputArgTypeInfo, outputArgTypeInfo, mods ? static_cast<Modifier::Modifiers>(*mods) : Modifier::Modifiers::NONE);
	}

	std::shared_ptr<SymbolTable::ClassBinding> SymbolTable::ClassBinding::LoadFrom(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree & classTree)
	{
		auto fullyQualifiedName = classTree.get<std::string>("Name");
		auto endOfNamespace = fullyQualifiedName.find_last_of('.');
		auto name = endOfNamespace != std::string::npos ? fullyQualifiedName.substr(endOfNamespace+1) : fullyQualifiedName;
		auto classBinding = symbolTable->BindExternalClass(name, fullyQualifiedName);

		auto ctors = classTree.get_child("Ctors");
		for (auto& ctor : ctors)
		{
			// Get input type
			std::shared_ptr<TypeInfo> inputArgTypeInfo;
			auto input = ctor.second.get_optional<std::string>("Input");
			if (input)
				inputArgTypeInfo = LoadTypeInfo(*input);

			classBinding->AddExternalConstructorBinding(symbolTable, inputArgTypeInfo, std::make_shared<Modifier>(Modifier::Modifiers::NONE));
		}

		// Add dtor, since all classes have one
		classBinding->AddExternalDestructorBinding(symbolTable);

		// Add Members
		auto mems = classTree.get_child_optional("Mems");
		if (mems)
		{
			for (auto& mem : *mems)
			{
				auto fullyQualifiedMemName = mem.second.get<std::string>("Name");
				auto endOfMemNameNamespace = fullyQualifiedMemName.find_last_of('.');
				auto memName = endOfMemNameNamespace != std::string::npos ? fullyQualifiedMemName.substr(endOfMemNameNamespace + 1) : fullyQualifiedMemName;

				auto typeName = mem.second.get<std::string>("Type");
				auto typeInfo = LoadTypeInfo(typeName);
				auto mods = mem.second.get_optional<int>("Mod");
				auto visibility = static_cast<Visibility>(mem.second.get<int>("Visibility"));
				symbolTable->BindExternalMemberVariable(classBinding, memName, typeInfo, visibility, mods ? static_cast<Modifier::Modifiers>(*mods) : Modifier::Modifiers::NONE);
			}
		}

		// Add methods
		auto funs = classTree.get_child_optional("Funs");
		if (funs)
		{
			for (auto& fun : *funs)
			{
				auto fullyQualifiedFunName = fun.second.get<std::string>("Name");
				auto endOfFunNameNamespace = fullyQualifiedFunName.find_last_of('.');
				auto funName = endOfFunNameNamespace != std::string::npos ? fullyQualifiedFunName.substr(endOfFunNameNamespace + 1) : fullyQualifiedFunName;

				// Is it overloaded?
				auto overloads = fun.second.get_child_optional("Overload");
				if (overloads)
				{
					for (auto& overload : fun.second)
					{
						if (overload.first.compare("Overload") == 0)
							LoadFunction(overload.second, classBinding, symbolTable, funName);
					}
				}
				else
				{
					LoadFunction(fun.second, classBinding, symbolTable, funName);
				}
			}
		}

		return classBinding;
	}
}