#pragma once

namespace Ast {
	class TypeException : public std::exception
	{
	};

	class UnexpectedException : public std::exception
	{
		const char* what() const override
		{
			return "Unexpected compiler exception: please tell the developer that he sucks.";
		}
	};

	class TypeInfo;
	class TypeMismatchException : public TypeException
	{
	public: 
		TypeMismatchException(std::shared_ptr<TypeInfo> expected, std::shared_ptr<TypeInfo> actual);

		const char* what() const override
		{
			return error.c_str();
		}
	private:
		std::string error;
	};

	class TypeAlreadyExistsException : public std::exception
	{
	public:
		TypeAlreadyExistsException(std::shared_ptr<TypeInfo> type);

		const char* what() const override
		{
			return error.c_str();
		}
	private:
		std::string error;
	};

	class OperationNotDefinedException : public std::exception
	{
	public:
		OperationNotDefinedException(std::string& operatorName) : _operatorName(operatorName)
		{
		}
	private:
		std::string _operatorName;
	};

	class FunctionMustBeDeclaredInClassScopeException : public std::exception
	{
	public:
		FunctionMustBeDeclaredInClassScopeException(const std::string& functionName) : _functionName(functionName)
		{
		}

		const char* what() const override
		{
			return ("Functions must be declared inside class scope: " + _functionName).c_str();
		}
	private:
		std::string _functionName;
	};

	class ReturnStatementMustBeDeclaredInFunctionScopeException : public std::exception
	{
		const char* what() const override
		{
			return "Return statements must be declared inside function scope";
		}
	};

	class VariablesCannotBeDeclaredOutsideOfScopesOrFunctionsException : public std::exception
	{
	public:
		VariablesCannotBeDeclaredOutsideOfScopesOrFunctionsException(const std::string& varName) : _varName(varName)
		{
		}
		const char* what() const override
		{
			return ("Variables must be declared inside class or function scope: " + _varName).c_str();
		}
	private:
		std::string _varName;
	};

	class VariablesMustBeInitializedException : public std::exception
	{
	public:
		VariablesMustBeInitializedException(const std::string& varName) : _varName(varName)
		{
		}
		const char* what() const override
		{
			return ("Variables must be initialized: " + _varName).c_str();
		}
	private:
		std::string _varName;
	};

	class SymbolNotAccessableException : public std::exception
	{
	public:
		SymbolNotAccessableException(const std::string& symbolName) : _symbolName(symbolName)
		{
		}
		const char* what() const override
		{
			return ("Symbol " + _symbolName + " not accessable from this location.").c_str();
		}
	private:
	private:
		std::string _symbolName;
	};

	class SymbolAlreadyDefinedInThisScopeException : public std::exception
	{
	public:
		SymbolAlreadyDefinedInThisScopeException(const std::string& symbolName) : _symbolName(symbolName)
		{
		}
		const char* what() const override
		{
			return _symbolName.c_str();
		}
	private:
		std::string _symbolName;
	};

	class SymbolNotDefinedException : public std::exception
	{
	public:
		SymbolNotDefinedException(const std::string& symbolName) : _symbolName(symbolName)
		{
		}
		const char* what() const override
		{
			return _symbolName.c_str();
		}
	private:
		std::string _symbolName;
	};

	class SymbolWrongTypeException : public std::exception
	{
	public:
		SymbolWrongTypeException(const std::string& symbolName) : _symbolName(symbolName)
		{
		}
		const char* what() const override
		{
			return _symbolName.c_str();
		}
	private:
		std::string _symbolName;
	};
}
