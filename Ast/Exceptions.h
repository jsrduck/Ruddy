#pragma once
#include "FileLocationContext.h"
#include "Node.h"

namespace Ast {

	class Exception : public std::exception
	{
	public:
		Exception();
		std::string Message();

		FileLocation Location()
		{
			return _location;
		}

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

	class TypeNotAssignableException : public Exception
	{
	public:
		TypeNotAssignableException(std::shared_ptr<TypeInfo> type);
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
		FunctionMustBeDeclaredInClassScopeException(const std::string& functionName);
	};

	class ReturnStatementMustBeDeclaredInFunctionScopeException : public Exception
	{
	public:
		ReturnStatementMustBeDeclaredInFunctionScopeException();
	};

	class VariablesCannotBeDeclaredOutsideOfScopesOrFunctionsException : public Exception
	{
	public:
		VariablesCannotBeDeclaredOutsideOfScopesOrFunctionsException(const std::string& varName);
	};

	class VariablesMustBeInitializedException : public Exception
	{
	public:
		VariablesMustBeInitializedException(const std::string& varName);
	};

	class SymbolNotAccessableException : public Exception
	{
	public:
		SymbolNotAccessableException(const std::string& symbolName);
	};

	class SymbolAlreadyDefinedInThisScopeException : public Exception
	{
	public:
		SymbolAlreadyDefinedInThisScopeException(const std::string& symbolName);
	};

	class SymbolNotDefinedException : public Exception
	{
	public:
		SymbolNotDefinedException(const std::string& symbolName);
	};

	class SymbolWrongTypeException : public Exception
	{
	public:
		SymbolWrongTypeException(const std::string& symbolName);
	};

	class OnlyClassTypesCanBeDerefencedException : public Exception
	{
	public:
		OnlyClassTypesCanBeDerefencedException(const std::string& actualType);
	};

	class NonStaticMemberReferencedFromStaticContextException : public Exception
	{
	public:
		NonStaticMemberReferencedFromStaticContextException(const std::string& symbolName);
	};

	class BreakInWrongPlaceException : public Exception
	{
	public:
		BreakInWrongPlaceException();
	};

	class ExpectedValueTypeException : public Exception
	{
	public:
		ExpectedValueTypeException(const std::string& symbolName);
	};

	class CannotReinitializeMemberException : public Exception
	{
	public:
		CannotReinitializeMemberException(const std::string& symbolName);
	};

	class UninitializedVariableReferencedException : public Exception
	{
	public:
		UninitializedVariableReferencedException(const std::string& symbolName);
	};

	class ValueTypeMustBeInitializedException : public Exception
	{
	public:
		ValueTypeMustBeInitializedException(const std::string& symbolName);
	};

	class NotSupportedByAutoTypeException : public Exception
	{
	public:
		NotSupportedByAutoTypeException();
	};

	class DuplicateLibraryException : public Exception
	{
	public:
		DuplicateLibraryException(const std::string& libName);
	};

	class UnknownLibraryException : public Exception
	{
	public:
		UnknownLibraryException(const std::string& libName);
	};

	class CannotNestUnsafeContextsException : public Exception
	{
	public:
		CannotNestUnsafeContextsException();
	};

	class CannotCallUnsafeFunctionFromSafeContextException : public Exception
	{
	public:
		CannotCallUnsafeFunctionFromSafeContextException(const std::string& functionName);
	};

	class CannotReferenceUnsafeMemberFromSafeContextException : public Exception
	{
	public:
		CannotReferenceUnsafeMemberFromSafeContextException(const std::string& unsafeMemberName);
	};

	class ConstructorMustHaveSameNameAsClassException : public Exception
	{
	public:
		ConstructorMustHaveSameNameAsClassException(const std::string& functionName, const std::string& className);
	};
}
