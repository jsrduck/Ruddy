#include "stdafx.h"

#include "Exceptions.h"
#include "TypeInfo.h"

#include <sstream>
#include <ostream>

using namespace std;

namespace Ast {

	Exception::Exception() : _location(FileLocationContext::CurrentLocation())
	{
	}

	std::string Exception::Message()
	{
		stringstream ss("");
		ss << "Ruddy Compiler Error(" << _location.LineNumber << "," << _location.ColumnNumber << "): " << _message;
		return ss.str();
	}

	UnexpectedException::UnexpectedException()
	{
		stringstream ss("");
		ss << "Unexpected compiler exception: please tell the developer that he sucks." << endl;
		_message = ss.str();
	}

	TypeMismatchException::TypeMismatchException(std::shared_ptr<TypeInfo> expected, std::shared_ptr<TypeInfo> actual)
	{
		_message = std::string("Type MismatchException: expected type \"");
		_message.append(expected ? expected->Name() : "void");
		_message.append("\" found type \"");
		_message.append(actual ? actual->Name() : "void");
		_message.append("\"");
	}

	TypeAlreadyExistsException::TypeAlreadyExistsException(std::shared_ptr<TypeInfo> type)
	{
		_message = std::string("Type doubly defined, already exists: \"");
		_message.append(type->Name());
		_message.append("\"");
	}

	NoMatchingFunctionSignatureFoundException::NoMatchingFunctionSignatureFoundException(std::shared_ptr<TypeInfo> type)
	{
		_message = std::string("No function override found with signature \"");
		_message.append(type ? type->Name() : "void");
		_message.append("\"");
	}

	OperationNotDefinedException::OperationNotDefinedException(std::string& operatorName, std::shared_ptr<TypeInfo> type)
	{
		_message = "Operation \"" + operatorName + "\" not defined on type \"" + type->Name() + "\"";
	}

}