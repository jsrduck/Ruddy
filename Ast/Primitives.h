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
		virtual bool IsPrimitiveType() override
		{
			return true;
		}
	};

	class IntegerConstant;
	class IntegerTypeInfo : public PrimitiveTypeInfo
	{
	public:
		virtual llvm::Value* CreateValue(llvm::LLVMContext* context, uint64_t constant) = 0;
		virtual bool Signed() = 0;
		virtual bool IsInteger() override
		{
			return true;
		}
		virtual int Bits() = 0;
		virtual int CreateCast(std::shared_ptr<TypeInfo> castTo) override;
	};

	class FloatingTypeInfo : public PrimitiveTypeInfo
	{
	public:
		virtual bool IsFloatingPoint() override
		{
			return true;
		}
		virtual int CreateCast(std::shared_ptr<TypeInfo> castTo) override;
	};

	// TODO: String should be defined as a class IN Ruddy code, but the compiler needs to be smart enough 
	// to allow assigning string constants to strings. Same with arrays. For now, we're treating them as 
	// primitives to make debugging code generation easier

	DECLARE_PRIMITIVE_TYPE_INFO(StringTypeInfo)

	DECLARE_PRIMITIVE_INTEGER_TYPE_INFO(Int32TypeInfo, true, 32)

	DECLARE_PRIMITIVE_INTEGER_TYPE_INFO(Int64TypeInfo, true, 64)

	DECLARE_PRIMITIVE_INTEGER_TYPE_INFO(UInt32TypeInfo, false, 32)

	DECLARE_PRIMITIVE_INTEGER_TYPE_INFO(UInt64TypeInfo, false, 64)

	DECLARE_PRIMITIVE_FLOATING_TYPE_INFO(Float32TypeInfo)

	DECLARE_PRIMITIVE_FLOATING_TYPE_INFO(Float64TypeInfo)

	DECLARE_PRIMITIVE_TYPE_INFO_AUTO_IMPLICIT_CAST_TO(CharByteTypeInfo, false, 8, Int32TypeInfo)

	DECLARE_PRIMITIVE_TYPE_INFO_AUTO_IMPLICIT_CAST_TO(CharTypeInfo, false, 16, Int32TypeInfo)

	DECLARE_PRIMITIVE_TYPE_INFO(BoolTypeInfo)

	DECLARE_PRIMITIVE_INTEGER_TYPE_INFO(ByteTypeInfo, false, 8)


	// An expression that recognizes a static constant, ie the zero in int i = 0;
	class ConstantExpression : public Expression
	{
	public:
		virtual std::shared_ptr<TypeInfo> BestFitTypeInfo() = 0;
		virtual bool IsConstantExpression() override
		{
			return true;
		}
	};

	class ConstantType : public TypeInfo
	{

		virtual llvm::AllocaInst* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context) override
		{
			// You can't have a variable with a constant type, it should be resolved to an actual type by then
			throw UnexpectedException();
		}
		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool asOutput) override
		{
			// You can't have a variable with a constant type, it should be resolved to an actual type by then
			throw UnexpectedException();
		}
		virtual bool IsConstant() override
		{
			return true;
		}
	};

	class IntegerConstantType : public ConstantType
	{
	public:
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

		virtual bool IsInteger() override
		{
			return true;
		}

		static std::shared_ptr<TypeInfo> Get();

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
				else if (input.substr(0, 2).compare("0x") == 0 || input.substr(0, 2).compare("0X") == 0)
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

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return IntegerConstantType::Get();
		}

		virtual std::shared_ptr<TypeInfo> BestFitTypeInfo() override
		{
			// Need to pick the best fit
			if (FitsInt32())
				return Int32TypeInfo::Get();
			else if (FitsInt64())
				return Int64TypeInfo::Get();
			else
				return UInt64TypeInfo::Get();
		}

		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

		uint8_t AsByte()
		{
			return GetAs<uint8_t>();
		}

		bool FitsByte()
		{
			try
			{
				auto result = AsByte();
				return true;
			}
			catch (OverflowException&)
			{
				return false;
			}
		}

		int32_t AsInt32()
		{
			return GetAs<int32_t>();
		}

		bool FitsInt32()
		{
			try
			{
				auto result = AsInt32();
				return true;
			}
			catch (OverflowException&)
			{
				return false;
			}
		}

		int64_t AsInt64()
		{
			return GetAs<int64_t>();
		}

		bool FitsInt64()
		{
			try
			{
				auto result = AsInt64();
				return true;
			}
			catch (OverflowException&)
			{
				return false;
			}
		}

		uint32_t AsUInt32()
		{
			return GetAs<uint32_t>();
		}

		bool FitsUInt32()
		{
			try
			{
				auto result = AsUInt32();
				return true;
			}
			catch (OverflowException&)
			{
				return false;
			}
		}

		uint64_t AsUInt64()
		{
			return GetAs<uint64_t>();
		}

		bool FitsUInt64()
		{
			try
			{
				auto result = AsUInt64();
				return true;
			}
			catch (OverflowException&)
			{
				return false;
			}
		}

		uint64_t GetRaw()
		{
			if (_usingHex)
			{
				return static_cast<uint64_t>(_asHex);
			}
			else if (_usingInt64)
			{
				return static_cast<uint64_t>(_asInt64);
			}
			else
			{
				return _asUint64;
			}
		}

		static std::shared_ptr<IntegerConstantType> _staticTypeInfo;

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

		int64_t _asInt64;
		uint64_t _asUint64;
		uint64_t _asHex;
		bool _usingInt64;
		bool _usingHex;
	};

	class FloatingConstantType : public ConstantType
	{
	public:
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

		static std::shared_ptr<TypeInfo> Get();

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

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _staticTypeInfo;
		}

		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

		virtual std::shared_ptr<TypeInfo> BestFitTypeInfo() override
		{
			// Just assume double, it's higher precision.
			return Float64TypeInfo::Get();
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

		static std::shared_ptr<FloatingConstantType> _staticTypeInfo;

	private:
		double _asDouble;
		float _asFloat;
		bool _fitsInFloat;
	};

	class BoolConstantType : public ConstantType
	{
	public:
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

		static std::shared_ptr<TypeInfo> Get();

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

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _staticTypeInfo;
		}

		virtual std::shared_ptr<TypeInfo> BestFitTypeInfo() override
		{
			return BoolTypeInfo::Get();
		}

		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

		static std::shared_ptr<BoolConstantType> _staticTypeInfo;
	private:
		bool _value;
	};

	class CharConstantType : public ConstantType
	{
	public:
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

		static std::shared_ptr<TypeInfo> Get();

	private:
		std::string _name = "CharConstant";
	};

	class CharConstant : public ConstantExpression
	{
	public:
		CharConstant(const std::string& input)
		{
			if (input[0] != '\'' || input[input.size() - 1] != '\'')
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
						auto result = std::stoul(input.substr(3, input.size() - 4), 0, 16);
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

		virtual std::shared_ptr<TypeInfo> BestFitTypeInfo() override
		{
			return CharTypeInfo::Get();
		}

		uint16_t Value()
		{
			return _value;
		}

		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _staticTypeInfo;
		}

		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

		static std::shared_ptr<CharConstantType> _staticTypeInfo;
	private:
		uint16_t _value;
	};

	class StringConstantType : public ConstantType
	{
	public:
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

		static std::shared_ptr<TypeInfo> Get();

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
		virtual std::shared_ptr<TypeInfo> EvaluateInternal(std::shared_ptr<SymbolTable> symbolTable) override
		{
			return _staticTypeInfo;
		}

		virtual std::shared_ptr<TypeInfo> BestFitTypeInfo() override
		{
			return StringTypeInfo::Get();
		}

		virtual llvm::Value* CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint) override;

		static std::shared_ptr<StringConstantType> _staticTypeInfo;
	private:
		std::string _input;
	};
}