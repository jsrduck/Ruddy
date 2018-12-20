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

	TypeNotAssignableException::TypeNotAssignableException(std::shared_ptr<TypeInfo> type)
	{
		_message = "Can't assign to a value of type " + type->Name();
	}

	FunctionMustBeDeclaredInClassScopeException::FunctionMustBeDeclaredInClassScopeException(const std::string & functionName) : Exception()
	{
		_message = "Function can only be declared inside class scope: " + functionName;
	}

	ReturnStatementMustBeDeclaredInFunctionScopeException::ReturnStatementMustBeDeclaredInFunctionScopeException()
	{
		_message = "Return statements must be declared inside function scope";
	}

	VariablesCannotBeDeclaredOutsideOfScopesOrFunctionsException::VariablesCannotBeDeclaredOutsideOfScopesOrFunctionsException(const std::string & varName)
	{
		_message = "Variables must be declared inside class or function scope: " + varName;
	}

	VariablesMustBeInitializedException::VariablesMustBeInitializedException(const std::string & varName)
	{
		_message = "Variables must be initialized: " + varName;
	}

	SymbolNotAccessableException::SymbolNotAccessableException(const std::string & symbolName)
	{
		_message = "Symbol \"" + symbolName + "\" not accessable from this location.";
	}

	SymbolAlreadyDefinedInThisScopeException::SymbolAlreadyDefinedInThisScopeException(const std::string & symbolName)
	{
		_message = "Symbol \"" + symbolName + "\" already defined in this scope";
	}

	SymbolNotDefinedException::SymbolNotDefinedException(const std::string & symbolName)
	{
		_message = "Symbol \"" + symbolName + "\" not defined";
	}

	SymbolWrongTypeException::SymbolWrongTypeException(const std::string & symbolName)
	{
		_message = "Symbol \"" + symbolName + "\" exists, but is the wrong type here.";
	}

	NonStaticMemberReferencedFromStaticContextException::NonStaticMemberReferencedFromStaticContextException(const std::string & symbolName)
	{
		_message = "Non static member variable \"" + symbolName + "\" cannot be referenced from a non-static context.";
	}

	BreakInWrongPlaceException::BreakInWrongPlaceException()
	{
		_message = "Break statement can only be declared inside loop scope";
	}

	ExpectedValueTypeException::ExpectedValueTypeException(const std::string & symbolName)
	{
		_message = "Expected symbol with a value type, found reference type instead: " + symbolName;
	}

	CannotReinitializeMemberException::CannotReinitializeMemberException(const std::string & symbolName)
	{
		_message = "Member variable already initiailzed: " + symbolName;
	}

	UninitializedVariableReferencedException::UninitializedVariableReferencedException(const std::string & symbolName)
	{
		_message = "Must initialize reference variable before referencing it: " + symbolName;
	}

	ValueTypeMustBeInitializedException::ValueTypeMustBeInitializedException(const std::string & symbolName)
	{
		_message =
			"Class has reference-value member with no default constructor - it must be explicitly initialized in the initializer list: "
			+ symbolName;
	}

	NotSupportedByAutoTypeException::NotSupportedByAutoTypeException()
	{
		_message = "Operation not supported for auto type";
	}

	DuplicateLibraryException::DuplicateLibraryException(const std::string & libName)
	{
		_message = "Can't disambiguate multiple libraries with the same name: ";
		_message.append(libName);
	}

	UnknownLibraryException::UnknownLibraryException(const std::string & libName)
	{
		_message = "Imported library \'";
		_message.append(libName);
		_message.append("\' unknown. Make sure ribs and rincs to link to are passed to the compiler.");
	}

	CannotNestUnsafeContextsException::CannotNestUnsafeContextsException()
	{
		_message = "Cannot nest unsafe contexts inside each other.";
	}

	CannotCallUnsafeFunctionFromSafeContextException::CannotCallUnsafeFunctionFromSafeContextException(const std::string & functionName)
	{
		_message = "Function: \"";
		_message.append(functionName);
		_message.append("\" is marked unsafe, and may only be called from an unsafe context.");
	}

	OnlyClassTypesCanBeDerefencedException::OnlyClassTypesCanBeDerefencedException(const std::string & actualType)
	{
		_message = "Cannot dereference expression that evaluates to non-class type " + actualType;
	}

	ConstructorMustHaveSameNameAsClassException::ConstructorMustHaveSameNameAsClassException(const std::string & functionName, const std::string & className)
	{
		_message = "Constructor named " + functionName + " in class " + className + ". Constructor names must match class names.";
	}

}