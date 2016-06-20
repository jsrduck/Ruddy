#pragma once

#include <memory>
#include "Expressions.h"
#include "TypeMacros.h"
#include <safeint.h>

namespace Ast
{

	class OverflowException : public std::exception
	{
	};

	class UnknownControlCharacterException : public std::exception
	{
	public:
		UnknownControlCharacterException(const std::string& charString) : _charString(charString)
		{
		}
		const char* what() const override
		{
			return _charString.c_str();
		}
	private:
		const std::string _charString;
	};

	class PrimitiveTypeInfo : public TypeInfo
	{
	};

	// TODO: String should be defined as a class IN Ruddy code, but the compiler needs to be smart enough 
	// to allow assigning string constants to strings. Same with arrays.

	DECLARE_PRIMITIVE_TYPE_INFO(StringTypeInfo)

	DECLARE_PRIMITIVE_TYPE_INFO(Int32TypeInfo)

	//class Int32Constant : public PrimitiveExpression
	//{
	//public:
	//	Int32Constant(int32_t value) : _value(value) {}
	//	int32_t Value() { return _value; }
	//private:
	//	int32_t _value;
	//	DECLARE_TYPE_INFO()
	//};

	DECLARE_PRIMITIVE_TYPE_INFO(Int64TypeInfo)

	//class Int64Constant : public PrimitiveExpression
	//{
	//public:
	//	Int64Constant(int64_t value) : _value(value) {}
	//	int64_t Value() { return _value; }
	//private:
	//	int64_t _value;
	//	DECLARE_TYPE_INFO()
	//};


	DECLARE_PRIMITIVE_TYPE_INFO(UInt32TypeInfo)

	//class UInt32Constant : public PrimitiveExpression
	//{
	//public:
	//	UInt32Constant(uint32_t value) : _value(value) {}
	//	uint32_t Value() { return _value; }
	//private:
	//	uint32_t _value;
	//	DECLARE_TYPE_INFO()
	//};

	DECLARE_PRIMITIVE_TYPE_INFO(UInt64TypeInfo)

	//class UInt64Constant : public PrimitiveExpression
	//{
	//public:
	//	UInt64Constant(uint64_t value) : _value(value) {}
	//	uint64_t Value() { return _value; }
	//private:
	//	uint64_t _value;
	//	DECLARE_TYPE_INFO()
	//};

	DECLARE_PRIMITIVE_TYPE_INFO(Float32TypeInfo)

	//class Float32Constant : public PrimitiveExpression
	//{
	//public:
	//	Float32Constant(float value) : _value(value) {}
	//	float Value() { return _value; }
	//private:
	//	float _value;
	//	DECLARE_TYPE_INFO()
	//};

	DECLARE_PRIMITIVE_TYPE_INFO(Float64TypeInfo)

	//class Float64Constant : public PrimitiveExpression
	//{
	//public:
	//	Float64Constant(double value) : _value(value) {}
	//	double Value() { return _value; }
	//private:
	//	double _value;
	//	DECLARE_TYPE_INFO()
	//};

	DECLARE_PRIMITIVE_TYPE_INFO(CharByteTypeInfo)

	//class CharByteConstant : public PrimitiveExpression
	//{
	//public:
	//	CharByteConstant(char value) : _value(value) {}
	//	char Value() { return _value; }
	//private:
	//	char _value;
	//	DECLARE_TYPE_INFO()
	//};

	DECLARE_PRIMITIVE_TYPE_INFO(CharTypeInfo)

	//class CharConstant : public PrimitiveExpression
	//{
	//public:
	//	CharConstant(uint16_t value) : _value(value) {}
	//	uint16_t Value() { return _value; }
	//private:
	//	uint16_t _value;
	//	DECLARE_TYPE_INFO()
	//};

	DECLARE_PRIMITIVE_TYPE_INFO(BoolTypeInfo)

	//class BoolConstant : public PrimitiveExpression
	//{
	//public:
	//	BoolConstant(bool value) : _value(value) {}
	//	bool Value() { return _value; }
	//private:
	//	bool _value;
	//	DECLARE_TYPE_INFO()
	//};

	DECLARE_PRIMITIVE_TYPE_INFO(ByteTypeInfo)

	//class ByteConstant : public PrimitiveExpression
	//{
	//public:
	//	ByteConstant(uint8_t value) : _value(value) {}
	//	uint8_t Value() { return _value; }
	//private:
	//	uint8_t _value;
	//	DECLARE_TYPE_INFO()
	//};

	//class UnresolvedPrimitive : public PrimitiveExpression, public std::enable_shared_from_this<UnresolvedPrimitive>
	//{
	//public:
	//	enum class PrimitiveType
	//	{
	//		integer,
	//		floating
	//	};

	//	UnresolvedPrimitive(const std::string& input, PrimitiveType type, bool negate = false)
	//	{
	//		_typeInfo = std::make_shared<CTypeInfo>(input, type, negate);
	//	}

	//	class CTypeInfo : public TypeInfo
	//	{
	//	public:
	//		CTypeInfo(const std::string& input, PrimitiveType type, bool negate = false) :
	//			_input(input), _type(type), _negate(negate)
	//		{
	//		}
	//		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override { return false; }
	//		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override 
	//		{ 
	//			throw UnexpectedException(); 
	//		}
	//		virtual const std::string& Name() override { return _name; }

	//		// Operator logic

	//		virtual bool SupportsOperator(Operation* operation) override
	//		{
	//			return true;
	//		}

	//		std::shared_ptr<Primitive> GetAsPrimitive(const type_info& primitiveType);

	//	private:
	//		const std::string _name = "Primitive";
	//		const std::string _input;
	//		const PrimitiveType _type;
	//		const bool _negate;
	//	};

	//	virtual std::shared_ptr<TypeInfo> Type() override
	//	{
	//		return _typeInfo;
	//	}

	//	virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable)
	//	{
	//		return Type();
	//	}

	//	std::shared_ptr<Primitive> GetAsPrimitive(const type_info& primitiveType) { return _typeInfo->GetAsPrimitive(primitiveType); }

	//private:
	//	std::shared_ptr<CTypeInfo> _typeInfo;
	//};
	// An expression that recognizes a static constant, ie the zero in int i = 0;
	class ConstantExpression : public Expression
	{
	};

	class IntegerConstantType : public TypeInfo
	{
		bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return false;
		}

		bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override
		{
			// Only assignable from other constants.
			return std::dynamic_pointer_cast<IntegerConstantType>(other) != nullptr;
		}

		const std::string& Name() override
		{
			return _name;
		}

		bool SupportsOperator(Operation* operation) override
		{
			return Int64TypeInfo::Get()->SupportsOperator(operation);
		}

	private:
		std::string _name = "IntegerConstant";
	};

	class IntegerConstant : public ConstantExpression
	{
	public:
		IntegerConstant(const std::string& input, bool negate = false)
		{
			// Try and fit it in a int64 or a uint64. If it doesn't fit in either, it isn't supported
			// in Ruddy at the moment
			try
			{
				if (negate)
				{
					// Fit it in an int64
					_asInt64 = std::stoll("-" + input);
					_usingInt64 = true;
					_usingHex = false;
				}
				else if (input.substr(0, 2).compare("0x") || input.substr(0, 2).compare("0X"))
				{
					_usingHex = true;
					_usingInt64 = false;
					_asHex = std::stoull(input, 0, 0);
				}
				else
				{
					// Fit it in a uint64
					_asUint64 = std::stoull(input);
					_usingInt64 = false;
					_usingHex = false;
				}
			}
			catch (std::out_of_range&)
			{
				throw OverflowException();
			}
		}

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _typeInfo;
		}

		uint8_t IntegerConstant::AsByte()
		{
			return GetAs<uint8_t>();
		}

		int32_t IntegerConstant::AsInt32()
		{
			return GetAs<int32_t>();
		}

		int64_t IntegerConstant::AsInt64()
		{
			return GetAs<int64_t>();
		}

		uint32_t IntegerConstant::AsUInt32()
		{
			return GetAs<uint32_t>();
		}

		uint64_t IntegerConstant::AsUInt64()
		{
			return GetAs<uint64_t>();
		}

	private:
		template<class INTEGERTYPE> INTEGERTYPE GetAs()
		{
			if (_usingHex)
			{
				// Check that we're not truncating any bits. If we are, that's an overflow exception.
				auto bits = sizeof(INTEGERTYPE) * 8;
				if ((_asHex >> bits != 0))
					throw OverflowException();
				return static_cast<INTEGERTYPE>(_asHex);
			}
			else if (_usingInt64)
			{
				if (_asInt64 >= msl::utilities::SafeInt<INTEGERTYPE>(std::numeric_limits<INTEGERTYPE>::min()) 
					&& _asInt64 <= msl::utilities::SafeInt<INTEGERTYPE>(std::numeric_limits<INTEGERTYPE>::max()))
				{
					return _asInt64;
				}
				else
				{
					throw OverflowException();
				}
			}
			else
			{
				if (_asUint64 >= msl::utilities::SafeInt<INTEGERTYPE>(std::numeric_limits<INTEGERTYPE>::min())
					&& _asUint64 <= msl::utilities::SafeInt<INTEGERTYPE>(std::numeric_limits<INTEGERTYPE>::max()))
				{
					return _asUint64;
				}
				else
				{
					throw OverflowException();
				}
			}
		}

		msl::utilities::SafeInt<int64_t> _asInt64;
		msl::utilities::SafeInt<uint64_t> _asUint64;
		uint64_t _asHex;
		bool _usingInt64;
		bool _usingHex;
		static std::shared_ptr<IntegerConstantType> _typeInfo;
	};

	class FloatingConstantType : public TypeInfo
	{
		bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return false;
		}

		bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override
		{
			// Only assignable from other constants.
			return std::dynamic_pointer_cast<FloatingConstantType>(other) != nullptr ||
				std::dynamic_pointer_cast<IntegerConstantType>(other) != nullptr;
		}

		const std::string& Name() override
		{
			return _name;
		}

		bool SupportsOperator(Operation* operation) override
		{
			return Float64TypeInfo::Get()->SupportsOperator(operation);
		}

	private:
		std::string _name = "FloatingConstant";
	};

	class FloatingConstant : public ConstantExpression
	{
	public:
		FloatingConstant(const std::string& input, bool negate = false)
		{
			// try to fit it in a double
			try
			{
				if (negate)
				{
					_asDouble = std::stod("-" + input);
					try
					{
						_asFloat = std::stof("-" + input);
						_fitsInFloat = true;
					} 
					catch (std::out_of_range&)
					{
						_fitsInFloat = false;
					}
				}
				else
				{
					_asDouble = std::stod(input);
					try
					{
						_asFloat = std::stof(input);
						_fitsInFloat = true;
					}
					catch (std::out_of_range&)
					{
						_fitsInFloat = false;
					}
				}
			}
			catch (std::out_of_range&)
			{
				throw OverflowException();
			}
		}

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _typeInfo;
		}

		float AsFloat32()
		{
			if (!_fitsInFloat)
				throw OverflowException();
			return _asFloat;
		}

		double AsFloat64()
		{
			return _asDouble;
		}

	private:
		double _asDouble;
		float _asFloat;
		bool _fitsInFloat;
		static std::shared_ptr<FloatingConstantType> _typeInfo;
	};

	class BoolConstantType : public TypeInfo
	{
		bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return false;
		}

		bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override
		{
			// Only assignable from other constants.
			return std::dynamic_pointer_cast<BoolConstantType>(other) != nullptr;
		}

		const std::string& Name() override
		{
			return _name;
		}

		bool SupportsOperator(Operation* operation) override
		{
			return BoolTypeInfo::Get()->SupportsOperator(operation);
		}

	private:
		std::string _name = "BoolConstant";
	};

	class BoolConstant : public ConstantExpression
	{
	public:
		BoolConstant(bool value) : _value(value)
		{
		}

		bool Value()
		{
			return _value;
		}

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _typeInfo;
		}

	private:
		bool _value;
		static std::shared_ptr<BoolConstantType> _typeInfo;
	};

	class CharConstantType : public TypeInfo
	{
		bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return false;
		}

		bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override
		{
			// Only assignable from other constants.
			return std::dynamic_pointer_cast<CharConstantType>(other) != nullptr;
		}

		const std::string& Name() override
		{
			return _name;
		}

		bool SupportsOperator(Operation* operation) override
		{
			return CharTypeInfo::Get()->SupportsOperator(operation);
		}

	private:
		std::string _name = "CharConstant";
	};

	class CharConstant : public ConstantExpression
	{
	public:
		CharConstant(const std::string& input)
		{
			if (input[0] != '\'' || input[input.size()-1] != '\'')
				throw UnexpectedException();

			if (input[1] == '\\')
			{
				// escape sequence
				switch (input[2])
				{
					case '\\':
						_value = '\\';
						break;
					case '0':
						_value = '\0';
						break;
					case 'n':
						_value = '\n';
						break;
					case 'r':
						_value = '\r';
						break;
					case 't':
						_value = '\t';
						break;
					case '\'':
						_value = '\'';
						break;
					case 'u':
					case 'x':
					{
						// hex
						auto result = std::stoul(input.substr(3, input.size()-4), 0, 16);
						if (result > 0xFFFF)
							throw UnknownControlCharacterException(input);
						_value = static_cast<uint16_t>(result);
					}
						break;
					default:
						// Should have already been caught by lexer
						throw UnknownControlCharacterException(input);
				}
			}
			else
			{
				_value = input[1];
			}
		}

		uint16_t Value() { return _value; }

		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _typeInfo;
		}
	private:
		uint16_t _value;
		static std::shared_ptr<CharConstantType> _typeInfo;
	};

	class StringConstantType : public TypeInfo
	{
		bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return false;
		}

		bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override
		{
			// Only assignable from other constants.
			return std::dynamic_pointer_cast<StringConstantType>(other) != nullptr;
		}

		const std::string& Name() override
		{
			return _name;
		}

		bool SupportsOperator(Operation* operation) override
		{
			return StringTypeInfo::Get()->SupportsOperator(operation);
		}

	private:
		std::string _name = "StringConstant";
	};

	class StringConstant : public ConstantExpression
	{
	public:
		StringConstant(std::string& input) : _input(input)
		{
		}
		std::string Value()
		{
			return _input;
		}
		virtual std::shared_ptr<TypeInfo> Evaluate(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _typeInfo;
		}
	private:
		std::string _input;
		static std::shared_ptr<StringConstantType> _typeInfo;
		/*DECLARE_TYPE_INFO()*/
	};
}