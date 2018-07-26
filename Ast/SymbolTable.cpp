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
		if (type->IsClassType())
		{
			auto classTypeInfo = std::dynamic_pointer_cast<BaseClassTypeInfo>(type);
			if (classTypeInfo->IsValueType())
			{
				auto classBinding = std::dynamic_pointer_cast<SymbolTable::ClassBinding>(Lookup(classTypeInfo->FullyQualifiedName(shared_from_this())));
				binding->_onExit = classBinding->_dtor->CreateCall(binding, FileLocationContext::CurrentLocation());
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
				if (!asClassMember->_memberDeclaration->_mods->IsStatic())
				{
					return std::make_shared<MemberInstanceBinding>(asClassMember, prefixSymbol);
					// TODO: Static?
				}
			}
			else if (retVal->IsFunctionBinding() && (prefixSymbol->IsVariableBinding() || prefixSymbol->IsClassMemberBinding()))
			{
				auto asFunctionBinding = std::dynamic_pointer_cast<FunctionBinding>(retVal);
				if (!asFunctionBinding->_functionDeclaration->_mods->IsStatic())
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
			if (!asClassMember->_memberDeclaration->_mods->IsStatic())
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
			if (asMethod != nullptr && !asMethod->_functionDeclaration->_mods->IsStatic())
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
		auto binding = std::make_shared<SymbolTable::FunctionBinding>(GetFullyQualifiedName(), functionDeclaration);
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


	SymbolTable::FunctionBinding::FunctionBinding(const std::string& fullyQualifiedClassName, std::shared_ptr<FunctionDeclaration> functionDeclaration) 
		: SymbolBinding(functionDeclaration->Name(), fullyQualifiedClassName.empty() ? functionDeclaration->Name() : fullyQualifiedClassName + "." + functionDeclaration->Name(), functionDeclaration->_visibility),
		_functionDeclaration(functionDeclaration)
	{
		_typeInfo = std::make_shared<FunctionTypeInfo>(functionDeclaration);
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

	Ast::SymbolTable::OverloadedFunctionBinding::OverloadedFunctionBinding(std::shared_ptr<FunctionBinding> functionBinding1, std::shared_ptr<FunctionBinding> functionBinding2)
		: FunctionBinding(functionBinding1->GetFullyQualifiedName(), functionBinding1->_functionDeclaration)
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
		: SymbolBinding(name, fullyQualifiedNamespaceName.empty() ? name : fullyQualifiedNamespaceName + "." + name, classDeclaration->_visibility),
		  _classDeclaration(classDeclaration)
	{
		_typeInfo = std::make_shared<ClassDeclarationTypeInfo>(classDeclaration, GetFullyQualifiedName());
	}

	std::shared_ptr<TypeInfo> SymbolTable::ClassBinding::GetTypeInfo()
	{
		return _typeInfo;
	}

	SymbolTable::MemberBinding::MemberBinding(const std::string& name, const std::string& fullyQualifiedClassName, std::shared_ptr<ClassMemberDeclaration> memberDeclaration, std::shared_ptr<ClassBinding> classBinding)
		: SymbolBinding(name, fullyQualifiedClassName.empty() ? name : fullyQualifiedClassName + "." + name, memberDeclaration->_visibility),
		_memberDeclaration(memberDeclaration), _classBinding(classBinding), _index(classBinding->NumMembers())
	{
		_typeInfo = memberDeclaration->_typeInfo;
	}

	SymbolTable::MemberBinding::MemberBinding(std::shared_ptr<MemberBinding> other)
		: SymbolBinding(other->GetName(), other->GetFullyQualifiedName(), other->GetVisibility()),
		_memberDeclaration(other->_memberDeclaration), _classBinding(other->_classBinding), _index(other->_index)
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
}