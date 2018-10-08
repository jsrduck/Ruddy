#include "stdafx.h"
#include "SymbolTable.h"
#include "Classes.h"
#include "Statements.h"
#include <algorithm>

using namespace Ast;

Ast::SymbolTable::SymbolBinding::SymbolBinding(const std::string& name, const std::string& fullQualifiedName, Visibility visibility)
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

Ast::SymbolTable::VariableBinding::VariableBinding(const std::string& name, std::shared_ptr<TypeInfo> variableType)
	: SymbolBinding(name, name, Visibility::PUBLIC), _variableType(variableType)
{
}

Ast::SymbolTable::NamespaceBinding::NamespaceBinding(const std::string& parentFullyQualifiedName, const std::string& name) :
	SymbolBinding(name, parentFullyQualifiedName.empty() ? name : parentFullyQualifiedName + "." + name, Visibility::PUBLIC)
{
}

bool Ast::SymbolTable::NamespaceBinding::IsNamespaceBinding()
{
	return true;
}

std::shared_ptr<TypeInfo> Ast::SymbolTable::NamespaceBinding::GetTypeInfo()
{
	throw UnexpectedException();
}

std::shared_ptr<SymbolTable::ConstructorBinding> SymbolTable::ClassBinding::AddConstructorBinding(std::shared_ptr<SymbolTable> symbolTable, std::shared_ptr<TypeInfo> inputArgs, Visibility visibility, std::shared_ptr<Modifier> mods)
{
	auto binding = std::make_shared<SymbolTable::ConstructorBinding>(_name, GetFullyQualifiedName(), visibility, inputArgs, mods, std::dynamic_pointer_cast<SymbolTable::ClassBinding>(shared_from_this()));
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
	auto binding = std::make_shared<SymbolTable::ConstructorBinding>(_name, GetFullyQualifiedName(), Visibility::PUBLIC, inputArgs, mods, std::dynamic_pointer_cast<SymbolTable::ClassBinding>(shared_from_this()));
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

std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::ClassBinding::AddDestructorBinding(std::shared_ptr<SymbolTable> symbolTable)
{
	auto binding = std::make_shared<SymbolTable::FunctionBinding>(GetName(), GetFullyQualifiedName(), Visibility::PRIVATE, nullptr, nullptr, std::make_shared<Modifier>(Modifier::Modifiers::NONE), std::dynamic_pointer_cast<SymbolTable::ClassBinding>(shared_from_this()));
	_dtorBinding = binding;
	return binding;
}

std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::ClassBinding::AddExternalDestructorBinding(std::shared_ptr<SymbolTable> symbolTable)
{
	auto binding = std::make_shared<SymbolTable::FunctionBinding>(GetName(), GetFullyQualifiedName(), Visibility::PRIVATE, nullptr, nullptr, std::make_shared<Modifier>(Modifier::Modifiers::NONE), std::dynamic_pointer_cast<SymbolTable::ClassBinding>(shared_from_this()));
	_dtorBinding = binding;
	return binding;
}

std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::ClassBinding::AddFunctionBinding(std::shared_ptr<SymbolTable> symbolTable, Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods)
{
	auto binding = std::make_shared<SymbolTable::FunctionBinding>(name, GetFullyQualifiedName(), visibility, inputArgs, outputArgs, std::make_shared<Modifier>(mods), std::dynamic_pointer_cast<SymbolTable::ClassBinding>(shared_from_this()));
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

std::shared_ptr<SymbolTable::FunctionBinding> SymbolTable::ClassBinding::AddExternalFunctionBinding(std::shared_ptr<SymbolTable> symbolTable, Visibility visibility, const std::string& name, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<TypeInfo> outputArgs, Modifier::Modifiers mods)
{
	auto binding = std::make_shared<SymbolTable::FunctionBinding>(name, GetFullyQualifiedName(), visibility, inputArgs, outputArgs, std::make_shared<Modifier>(mods), std::dynamic_pointer_cast<SymbolTable::ClassBinding>(shared_from_this()));
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

std::shared_ptr<SymbolTable::MemberBinding> SymbolTable::ClassBinding::AddMemberVariableBinding(const std::string& name, std::shared_ptr<TypeInfo> typeInfo, Visibility visibility, Modifier::Modifiers mods)
{
	auto binding = std::make_shared<SymbolTable::MemberBinding>(name, typeInfo, std::dynamic_pointer_cast<SymbolTable::ClassBinding>(shared_from_this()), visibility, mods);
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
	auto binding = std::make_shared<SymbolTable::MemberBinding>(name, typeInfo, std::dynamic_pointer_cast<SymbolTable::ClassBinding>(shared_from_this()), visibility, mods);
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

std::shared_ptr<SymbolTable::OverloadedFunctionBinding> SymbolTable::FunctionBinding::GetOverloadedBinding()
{
	return nullptr;
}

bool SymbolTable::FunctionBinding::IsMethod()
{
	return !_typeInfo->_mods->IsStatic();
}

/* OverloadedFunctionBinding*/
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

bool Ast::SymbolTable::OverloadedFunctionBinding::IsOverridden()
{
	return true;
}

std::shared_ptr<Ast::SymbolTable::OverloadedFunctionBinding> Ast::SymbolTable::OverloadedFunctionBinding::GetOverloadedBinding()
{
	return std::dynamic_pointer_cast<OverloadedFunctionBinding>(shared_from_this());
}

std::shared_ptr<TypeInfo> Ast::SymbolTable::OverloadedFunctionBinding::GetTypeInfo()
{
	throw UnexpectedException();
}

bool Ast::SymbolTable::OverloadedFunctionBinding::IsMethod()
{
	for (auto& binding : _bindings)
	{
		if (!binding->_typeInfo->_mods->IsStatic())
			return true;
	}
}

/* FunctionInstanceBinding */
Ast::SymbolTable::FunctionInstanceBinding::FunctionInstanceBinding(std::shared_ptr<FunctionBinding> functionBinding, std::shared_ptr<SymbolBinding> reference) :
	FunctionBinding(functionBinding->GetName(), functionBinding->GetParentNamespaceName(), functionBinding->GetVisibility(), functionBinding->_typeInfo->InputArgsType(), functionBinding->_typeInfo->OutputArgsType(), functionBinding->_typeInfo->_mods, functionBinding->_classBinding),
	_functionBinding(functionBinding),
	_reference(reference)
{
}

bool Ast::SymbolTable::FunctionInstanceBinding::IsOverridden()
{
	return _functionBinding->IsOverridden();
}

std::shared_ptr<Ast::SymbolTable::OverloadedFunctionBinding> Ast::SymbolTable::FunctionInstanceBinding::GetOverloadedBinding()
{
	return _functionBinding->GetOverloadedBinding();
}

std::shared_ptr<TypeInfo> Ast::SymbolTable::FunctionInstanceBinding::GetTypeInfo()
{
	return _functionBinding->GetTypeInfo();
}

/* ConstructorBinding */
SymbolTable::ConstructorBinding::ConstructorBinding(const std::string& name, const std::string& fullyQualifiedClassName, Visibility visibility, std::shared_ptr<TypeInfo> inputArgs, std::shared_ptr<Modifier> mods, std::shared_ptr<ClassBinding> classBinding) :
	FunctionBinding(name, fullyQualifiedClassName, visibility, inputArgs, nullptr /*outputArgs*/, mods, classBinding)
{
}

void SymbolTable::ConstructorBinding::AddInitializerBinding(const std::string& memberName)
{
	if (_initializers.count(memberName) > 0)
	{
		throw CannotReinitializeMemberException(memberName);
	}
	_initializers.insert(memberName);
}

/* ClassBinding */
SymbolTable::ClassBinding::ClassBinding(const std::string& name, const std::string& fullyQualifiedNamespaceName, Visibility visibility)
	: SymbolBinding(name, fullyQualifiedNamespaceName.empty() ? name : fullyQualifiedNamespaceName + "." + name, visibility)
{
	_typeInfo = std::make_shared<ClassDeclarationTypeInfo>(GetName(), GetFullyQualifiedName());
}

std::shared_ptr<TypeInfo> SymbolTable::ClassBinding::GetTypeInfo()
{
	return _typeInfo;
}

bool SymbolTable::ClassBinding::IsClassBinding()
{
	return true;
}

/* MemberBinding */
SymbolTable::MemberBinding::MemberBinding(const std::string& name, std::shared_ptr<TypeInfo> typeInfo, std::shared_ptr<ClassBinding> classBinding, Visibility visibility, Modifier::Modifiers mods)
	: SymbolBinding(name, classBinding->GetFullyQualifiedName() + "." + name, visibility),
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

std::shared_ptr<TypeInfo> SymbolTable::MemberBinding::GetTypeInfo()
{
	return _typeInfo;
}

/* MemberInstanceBinding */
SymbolTable::MemberInstanceBinding::MemberInstanceBinding(std::shared_ptr<MemberBinding> memberBinding, std::shared_ptr<SymbolBinding> reference) :
	MemberBinding(memberBinding),
	_reference(reference)
{
}

bool SymbolTable::MemberInstanceBinding::IsReferenceToThisPointer()
{
	return _reference->IsFunctionBinding();
}

/* LoopBinding */
SymbolTable::LoopBinding::LoopBinding() : SymbolBinding("", "", Visibility::PUBLIC)
{
}

bool SymbolTable::LoopBinding::IsLoopBinding()
{
	return true;
}

std::shared_ptr<TypeInfo> SymbolTable::LoopBinding::GetTypeInfo()
{
	throw UnexpectedException();
}

/* ScopeMarker */
std::shared_ptr<TypeInfo> SymbolTable::ScopeMarker::GetTypeInfo()
{
	throw UnexpectedException();
}