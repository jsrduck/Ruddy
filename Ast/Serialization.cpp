#include "stdafx.h"
#include "SymbolTable.h"
#include "Serializer.h"
#include "TypeInfo.h"
#include "Primitives.h"

#include <vector>

#include <boost\property_tree\ptree_serialization.hpp>
#include <boost\property_tree\json_parser.hpp>
#include <boost\algorithm\string.hpp>

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;
using namespace Ast;

/* SymbolTable */
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
			symbols.add_child("Class", asClass->GetSerializer()->Serialize(shared_from_this()));
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

std::shared_ptr<SymbolTable::ClassBinding> LoadClass(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree & classTree)
{
	auto fullyQualifiedName = classTree.get<std::string>("Name");
	auto endOfNamespace = fullyQualifiedName.find_last_of('.');
	auto name = endOfNamespace != std::string::npos ? fullyQualifiedName.substr(endOfNamespace + 1) : fullyQualifiedName;
	auto namespaceName = endOfNamespace != std::string::npos ? fullyQualifiedName.substr(0, endOfNamespace) : "";
	auto classBinding = symbolTable->BindExternalClass(name, namespaceName);

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
			LoadClass(shared_from_this(), symbol.second);
		}
		else
		{
			throw UnexpectedException();
		}
	}
}

/* Default Serializer */
Serializer::Serializer(const std::string& fullySerializedName) : _fullySerializedName(fullySerializedName)
{
}

boost::property_tree::ptree Serializer::Serialize(std::shared_ptr<Ast::SymbolTable> symbolTable)
{
	ptree symbol;
	symbol.add("Name", _fullySerializedName);
	SerializeInternal(symbolTable, symbol);
	return symbol;
}

void Serializer::SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree&)
{
	throw UnexpectedException();
}

std::shared_ptr<Serializer> Ast::SymbolTable::SymbolBinding::GetSerializer()
{
	return std::make_shared<Serializer>(GetFullyQualifiedName());
}

/* Functions */
class FunctionSerializer : public Serializer
{
public:
	FunctionSerializer(const std::string& fullySerializedName, std::shared_ptr<FunctionTypeInfo> typeInfo, Visibility visibility) : _typeInfo(typeInfo), _visibility(visibility), Serializer(fullySerializedName) { }

	virtual void SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& symbol) override
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
		symbol.add("Visibility", static_cast<int>(_visibility));
	}

protected:
	std::shared_ptr<FunctionTypeInfo> _typeInfo;
	Visibility _visibility;
};
std::shared_ptr<Serializer> Ast::SymbolTable::FunctionBinding::GetSerializer()
{
	return std::make_shared<FunctionSerializer>(GetFullyQualifiedName(), _typeInfo, GetVisibility());
}

class OverloadedFunctionSerializer : public Serializer
{
public:
	OverloadedFunctionSerializer(const std::string& fullySerializedName, std::vector<std::shared_ptr<Serializer>>& serializers) : _serializers(serializers), Serializer(fullySerializedName)
	{
	}

	virtual void SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& symbol) override
	{
		for (auto& funSerializer : _serializers)
		{
			boost::property_tree::ptree bindingTree;
			funSerializer->SerializeInternal(symbolTable, bindingTree);
			symbol.add_child("Overload", bindingTree);
		}
	}
protected:
	std::vector<std::shared_ptr<Serializer>> _serializers;
};
std::shared_ptr<Serializer> Ast::SymbolTable::OverloadedFunctionBinding::GetSerializer()
{
	std::vector<std::shared_ptr<Serializer>> serializers;
	for (auto& binding : _bindings)
	{
		if (binding->GetVisibility() != Visibility::PRIVATE)
		{
			serializers.push_back(binding->GetSerializer());
		}
	}
	return std::make_shared<OverloadedFunctionSerializer>(GetFullyQualifiedName(), serializers);
}

/* Members */
class MemberSerializer : public Serializer
{
public:
	MemberSerializer(const std::string& fullySerializedName, std::shared_ptr<TypeInfo> typeInfo, std::shared_ptr<Modifier> mod, Visibility visibility) : _typeInfo(typeInfo), _visibility(visibility), _modifier(mod), Serializer(fullySerializedName)
	{
	}

	virtual void SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& symbol) override
	{
		symbol.add("Type", _typeInfo->SerializedName(symbolTable));
		if (_modifier->Get() != Modifier::Modifiers::NONE)
			symbol.add("Mod", static_cast<int>(_modifier->Get()));
		symbol.add("Visibility", static_cast<int>(_visibility));
	}
protected:
	std::shared_ptr<TypeInfo> _typeInfo;
	Visibility _visibility;
	std::shared_ptr<Modifier> _modifier;
};
std::shared_ptr<Serializer> Ast::SymbolTable::MemberBinding::GetSerializer()
{
	return std::make_shared<MemberSerializer>(GetFullyQualifiedName(), GetTypeInfo(), _modifier, GetVisibility());
}

/* Classes */
class ClassSerializer : public Serializer
{
public:
	ClassSerializer(const std::string& fullySerializedName, std::vector<std::shared_ptr<Serializer>>& ctors, std::vector<std::shared_ptr<Serializer>>& members, std::vector<std::shared_ptr<Serializer>>& functions) 
		: _ctors(ctors), _members(members), _functions(functions), Serializer(fullySerializedName)
	{
	}

	virtual void SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree& symbol) override
	{
		ptree ctors;
		for (auto& ctor : _ctors)
		{
			ctors.add_child("Ctor", ctor->Serialize(symbolTable));
		}

		symbol.add_child("Ctors", ctors);

		if (_members.size() > 0)
		{
			ptree members;
			for (auto& mem : _members)
			{
				members.add_child("Member", mem->Serialize(symbolTable));
			}
			symbol.add_child("Mems", members);
		}

		if (_functions.size() > 0)
		{
			ptree funs;
			for (auto& fun : _functions)
			{
				funs.add_child("Function", fun->Serialize(symbolTable));
			}
			symbol.add_child("Funs", funs);
		}
	}
protected:
	std::vector<std::shared_ptr<Serializer>> _ctors;
	std::vector<std::shared_ptr<Serializer>> _members;
	std::vector<std::shared_ptr<Serializer>> _functions;
};
std::shared_ptr<Serializer> Ast::SymbolTable::ClassBinding::GetSerializer()
{
	std::vector<std::shared_ptr<Serializer>> ctors;
	for (auto& ctor : _ctors)
	{
		if (ctor->GetVisibility() != Visibility::PRIVATE)
			ctors.push_back(ctor->GetSerializer());
	}

	std::vector<std::shared_ptr<Serializer>> members;
	for (auto& mem : _members)
	{
		if (mem->GetVisibility() != Visibility::PRIVATE)
			members.push_back(mem->GetSerializer());
	}

	std::vector<std::shared_ptr<Serializer>> functions;
	for (auto& fun : _functions)
	{
		if (fun.second->GetVisibility() != Visibility::PRIVATE)
			functions.push_back(fun.second->GetSerializer());
	}

	return std::make_shared<ClassSerializer>(GetFullyQualifiedName(), ctors, members, functions);
}