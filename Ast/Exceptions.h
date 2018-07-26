#pragma once
#include "FileLocationContext.h"
#include "Node.h"

namespace Ast {

	class Exception : public std::exception
	{
	public:
		Exception();
		std::string Message();

	protected:
		std::string _message;
		FileLocation _location;
	};

	class UnexpectedException : public Exception
	{
	public:
		UnexpectedException();

	private:
		std::string _callstack;
	};

	class OverflowException : public Exception
	{
	public:
		OverflowException()
		{
			_message = "Overflow exception";
		}
	};

	class UnknownControlCharacterException : public Exception
	{
	public:
		UnknownControlCharacterException(const std::string& charString)
		{
			_message = "Unknown control character: " + charString;
		}
	};

	class TypeInfo;
	class TypeMismatchException : public Exception
	{
	public: 
		TypeMismatchException(std::shared_ptr<TypeInfo> expected, std::shared_ptr<TypeInfo> actual);
	};

	class TypeAlreadyExistsException : public Exception
	{
	public:
		TypeAlreadyExistsException(std::shared_ptr<TypeInfo> type);
	};

	class NoMatchingFunctionSignatureFoundException : public Exception
	{
	public:
		NoMatchingFunctionSignatureFoundException(std::shared_ptr<TypeInfo> type);
	};

	class OperationNotDefinedException : public Exception
	{
	public:
		OperationNotDefinedException(std::string& operatorName, std::shared_ptr<TypeInfo> type);
	};

	class FunctionMustBeDeclaredInClassScopeException : public Exception
	{
	public:
		FunctionMustBeDeclaredInClassScopeException(const std::string& functionName) : Exception()
		{
			_message = "Function can only be declared inside class scope: " + functionName;
		}
	};

	class ReturnStatementMustBeDeclaredInFunctionScopeException : public Exception
	{
	public:
		ReturnStatementMustBeDeclaredInFunctionScopeException()
		{
			_message = "Return statements must be declared inside function scope";
		}
	};

	class VariablesCannotBeDeclaredOutsideOfScopesOrFunctionsException : public Exception
	{
	public:
		VariablesCannotBeDeclaredOutsideOfScopesOrFunctionsException(const std::string& varName)
		{
			_message = "Variables must be declared inside class or function scope: " + varName;
		}
	};

	class VariablesMustBeInitializedException : public Exception
	{
	public:
		VariablesMustBeInitializedException(const std::string& varName)
		{
			_message = "Variables must be initialized: " + varName;
		}
	};

	class SymbolNotAccessableException : public Exception
	{
	public:
		SymbolNotAccessableException(const std::string& symbolName)
		{
			_message = "Symbol \"" + symbolName + "\" not accessable from this location.";
		}
	};

	class SymbolAlreadyDefinedInThisScopeException : public Exception
	{
	public:
		SymbolAlreadyDefinedInThisScopeException(const std::string& symbolName)
		{
			_message = "Symbol \"" + symbolName + "\" already defined in this scope";
		}
	};

	class SymbolNotDefinedException : public Exception
	{
	public:
		SymbolNotDefinedException(const std::string& symbolName)
		{
			_message = "Symbol \"" + symbolName + "\" not defined";
		}
	};

	class SymbolWrongTypeException : public Exception
	{
	public:
		SymbolWrongTypeException(const std::string& symbolName)
		{
			_message = "Symbol \"" + symbolName + "\" exists, but is the wrong type here.";
		}
	};

	class NonStaticMemberReferencedFromStaticContextException : public Exception
	{
	public:
		NonStaticMemberReferencedFromStaticContextException(const std::string& symbolName)
		{
			_message = "Non static member variable \"" + symbolName + "\" cannot be referenced from a non-static context.";
		}
	};

	class BreakInWrongPlaceException : public Exception
	{
	public:
		BreakInWrongPlaceException()
		{
			_message = "Break statement can only be declared inside loop scope";
		}
	};

	class ExpectedValueTypeException : public Exception
	{
	public:
		ExpectedValueTypeException(const std::string& symbolName)
		{
			_message = "Expected symbol with a value type, found reference type instead: " + symbolName;
		}
	};

	class CannotReinitializeMemberException : public Exception
	{
	public:
		CannotReinitializeMemberException(const std::string& symbolName)
		{
			_message = "Member variable already initiailzed: " + symbolName;
		}
	};

	class UninitializedVariableReferencedException : public Exception
	{
	public:
		UninitializedVariableReferencedException(const std::string& symbolName)
		{
			_message = "Must initialize reference variable before referencing it: " + symbolName;
		}
	};

	class ValueTypeMustBeInitializedException : public Exception
	{
	public:
		ValueTypeMustBeInitializedException(const std::string& symbolName)
		{
			_message =
				"Class has reference-value member with no default constructor - it must be explicitly initialized in the initializer list: " 
				+ symbolName;
		}
	};

	class NotSupportedByAutoTypeException : public Exception
	{
	public:
		NotSupportedByAutoTypeException()
		{
			_message = "Operation not supported for auto type";
		}
	};
}
