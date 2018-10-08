#pragma once

#include <boost\property_tree\json_parser.hpp>

namespace Ast 
{
	class SymbolTable;
	class Serializer
	{
	public:
		Serializer(const std::string& fullySerializedName);
		boost::property_tree::ptree Serialize(std::shared_ptr<Ast::SymbolTable> symbolTable);
		virtual void SerializeInternal(std::shared_ptr<Ast::SymbolTable> symbolTable, boost::property_tree::ptree&);
	protected:
		const std::string _fullySerializedName;
	};
}
