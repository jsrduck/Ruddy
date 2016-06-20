#include "stdafx.h"
#include "Primitives.h"
#include "Operations.h"

namespace Ast
{

	std::shared_ptr<IntegerConstantType> IntegerConstant::_typeInfo = std::make_shared<IntegerConstantType>();
	std::shared_ptr<FloatingConstantType> FloatingConstant::_typeInfo = std::make_shared<FloatingConstantType>();
	std::shared_ptr<BoolConstantType> BoolConstant::_typeInfo = std::make_shared<BoolConstantType>();
	std::shared_ptr<CharConstantType> CharConstant::_typeInfo = std::make_shared<CharConstantType>();
	std::shared_ptr<StringConstantType> StringConstant::_typeInfo = std::make_shared<StringConstantType>();

	/* StringConstant */
	IMPL_PRIMITIVE_TYPE_INFO(StringTypeInfo, "string")
	int StringTypeInfo::_supportedOperations = 0x0; // TODO: maybe add + as a concat
	//IMPL_TYPE_INFO(StringConstant, "string")
	//int StringConstant::CTypeInfo::_supportedOperations = 0x0; // TODO: maybe add + as a concat

	/* Int32Constant */
	IMPL_PRIMITIVE_TYPE_INFO4(Int32TypeInfo, "int32", ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(Int32TypeInfo)
	//IMPL_TYPE_INFO3(Int32Constant, "int", ByteConstant, CharByteConstant, CharConstant)
	//IMPL_ALL_OPERATORS(Int32Constant)

	/* Int64Constant*/
	IMPL_PRIMITIVE_TYPE_INFO6(Int64TypeInfo, "int64", Int32TypeInfo, UInt32TypeInfo, ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(Int64TypeInfo)
	//IMPL_TYPE_INFO5(Int64Constant, "int64", Int32Constant, UInt32Constant, ByteConstant, CharByteConstant, CharConstant)
	//IMPL_ALL_OPERATORS(Int64Constant)

	/* UInt32Constant */
	IMPL_PRIMITIVE_TYPE_INFO4(UInt32TypeInfo, "uint32", ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(UInt32TypeInfo)
	//IMPL_TYPE_INFO3(UInt32Constant, "uint", ByteConstant, CharByteConstant, CharConstant)
	//IMPL_ALL_OPERATORS(UInt32Constant)

	/* UInt64Constant */
	IMPL_PRIMITIVE_TYPE_INFO5(UInt64TypeInfo, "uint64", UInt32TypeInfo, ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(UInt64TypeInfo)
	//IMPL_TYPE_INFO4(UInt64Constant, "uint64", UInt32Constant, ByteConstant, CharByteConstant, CharConstant)
	//IMPL_ALL_OPERATORS(UInt64Constant)

	/* Float32Constant */
	IMPL_PRIMITIVE_TYPE_INFO8(Float32TypeInfo, "float", ByteTypeInfo, Int32TypeInfo, UInt32TypeInfo, Int64TypeInfo, UInt64TypeInfo, CharByteTypeInfo, CharTypeInfo, FloatingConstantType)
	IMPL_PRIMITIVE_NON_BITWISE_OPERATORS(Float32TypeInfo)
	//IMPL_TYPE_INFO7(Float32Constant, "float", ByteConstant, Int32Constant, UInt32Constant, Int64Constant, UInt64Constant, CharByteConstant, CharConstant)
	//IMPL_NON_BITWISE_OPERATORS(Float32Constant)

	/* Float64Constant */
	IMPL_PRIMITIVE_TYPE_INFO9(Float64TypeInfo, "float64", Float32TypeInfo, ByteTypeInfo, Int32TypeInfo, UInt32TypeInfo, Int64TypeInfo, UInt64TypeInfo, CharByteTypeInfo, CharTypeInfo, FloatingConstantType)
	IMPL_PRIMITIVE_NON_BITWISE_OPERATORS(Float64TypeInfo)
	//IMPL_TYPE_INFO8(Float64Constant, "float64", Float32Constant, ByteConstant, Int32Constant, UInt32Constant, Int64Constant, UInt64Constant, CharByteConstant, CharConstant)
	//IMPL_NON_BITWISE_OPERATORS(Float64Constant)

	/* CharByteConstant */
	IMPL_PRIMITIVE_TYPE_INFO1(CharByteTypeInfo, "char", CharConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(CharByteTypeInfo)
	//IMPL_TYPE_INFO(CharByteConstant, "char")
	//IMPL_ALL_OPERATORS(CharByteConstant)

	/* CharConstant*/
	IMPL_PRIMITIVE_TYPE_INFO1(CharTypeInfo, "wchar", CharConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(CharTypeInfo)
	//IMPL_TYPE_INFO(CharConstant, "wchar")
	//IMPL_ALL_OPERATORS(CharConstant)

	/* BoolConstant */
	IMPL_PRIMITIVE_TYPE_INFO1(BoolTypeInfo, "bool", BoolConstantType)
	int BoolTypeInfo::_supportedOperations = 
		EqualToOperation::Id | NotEqualToOperation::Id | LogicalAndOperation::Id | LogicalOrOperation::Id | NegateOperation::Id;
	//IMPL_TYPE_INFO(BoolConstant, "bool")
	//int BoolConstant::CTypeInfo::_supportedOperations = 
	//	EqualToOperation::Id | NotEqualToOperation::Id | LogicalAndOperation::Id | LogicalOrOperation::Id | NegateOperation::Id;

	/* ByteConstant */
	IMPL_PRIMITIVE_TYPE_INFO2(ByteTypeInfo, "byte", CharByteTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(ByteTypeInfo)
	//IMPL_TYPE_INFO1(ByteConstant, "byte", CharByteConstant)
	//IMPL_ALL_OPERATORS(ByteConstant)

	/*std::shared_ptr<Primitive> UnresolvedPrimitive::CTypeInfo::GetAsPrimitive(const type_info& primitiveType)
	{
		if (primitiveType == typeid(Int32Constant))
		{
			if (_type != PrimitiveType::integer)
				throw TypeMismatchException(Int32Constant::CTypeInfo::_typeInfo, shared_from_this());
			auto output = std::stol(_input);
			if (_negate)
				output *= -1;
			if (output >= std::numeric_limits<int32_t>::min() && output <= std::numeric_limits<int32_t>::max())
			{
				return std::make_shared<Int32Constant>(output);
			}
			else
			{
				throw OverflowException();
			}
		}
		else if (primitiveType == typeid(UInt32Constant))
		{
			if (_type != PrimitiveType::integer)
				throw TypeMismatchException(Int32Constant::CTypeInfo::_typeInfo, shared_from_this());
			if (_negate)
				throw OverflowException();
			auto output = std::stoul(_input);
			if (output >= std::numeric_limits<uint32_t>::min() && output <= std::numeric_limits<uint32_t>::max())
			{
				return std::make_shared<UInt32Constant>(output);
			}
			else
			{
				throw OverflowException();
			}
		}
		else if (primitiveType == typeid(Int64Constant))
		{
			if (_type != PrimitiveType::integer)
				throw TypeMismatchException(Int32Constant::CTypeInfo::_typeInfo, shared_from_this());
			auto output = std::stoll(_input);
			if (_negate)
				output *= -1;
			if (output >= std::numeric_limits<int64_t>::min() && output <= std::numeric_limits<int64_t>::max())
			{
				return std::make_shared<Int64Constant>(output);
			}
			else
			{
				throw OverflowException();
			}
		}
		else if (primitiveType == typeid(UInt64Constant))
		{
			if (_type != PrimitiveType::integer)
				throw TypeMismatchException(Int32Constant::CTypeInfo::_typeInfo, shared_from_this());
			if (_negate)
				throw OverflowException();
			auto output = std::stoull(_input);
			if (output >= std::numeric_limits<uint64_t>::min() && output <= std::numeric_limits<uint64_t>::max())
			{
				return std::make_shared<UInt64Constant>(output);
			}
			else
			{
				throw OverflowException();
			}
		}
		else if (primitiveType == typeid(ByteConstant))
		{
			if (_type != PrimitiveType::integer)
				throw TypeMismatchException(Int32Constant::CTypeInfo::_typeInfo, shared_from_this());
			auto output = std::stoul(_input);
			if (output >= std::numeric_limits<uint8_t>::min() && output <= std::numeric_limits<uint8_t>::max())
			{
				return std::make_shared<ByteConstant>((uint8_t) output);
			}
			else
			{
				throw OverflowException();
			}
		}
		else if (primitiveType == typeid(Float32Constant))
		{
			if (_type != PrimitiveType::floating)
				throw TypeMismatchException(Int32Constant::CTypeInfo::_typeInfo, shared_from_this());
			auto output = std::stof(_input);
			if (output >= std::numeric_limits<float>::min() && output <= std::numeric_limits<float>::max())
			{
				return std::make_shared<Float32Constant>(output);
			}
			else
			{
				throw OverflowException();
			}
		}
		else if (primitiveType == typeid(Float64Constant))
		{
			if (_type != PrimitiveType::floating)
				throw TypeMismatchException(Int32Constant::CTypeInfo::_typeInfo, shared_from_this());
			auto output = std::stod(_input);
			if (output >= std::numeric_limits<double>::min() && output <= std::numeric_limits<double>::max())
			{
				return std::make_shared<Float64Constant>(output);
			}
			else
			{
				throw OverflowException();
			}
		}
		else
		{
			throw UnexpectedException();
		}
	}*/

}