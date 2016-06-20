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
	IMPL_PRIMITIVE_TYPE_INFO1(StringTypeInfo, "string", StringConstantType)
	int StringTypeInfo::_supportedOperations = 0x0; // TODO: maybe add + as a concat

	/* Int32Constant */
	IMPL_PRIMITIVE_TYPE_INFO4(Int32TypeInfo, "int32", ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(Int32TypeInfo)

	/* Int64Constant*/
	IMPL_PRIMITIVE_TYPE_INFO6(Int64TypeInfo, "int64", Int32TypeInfo, UInt32TypeInfo, ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(Int64TypeInfo)

	/* UInt32Constant */
	IMPL_PRIMITIVE_TYPE_INFO4(UInt32TypeInfo, "uint32", ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(UInt32TypeInfo)

	/* UInt64Constant */
	IMPL_PRIMITIVE_TYPE_INFO5(UInt64TypeInfo, "uint64", UInt32TypeInfo, ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(UInt64TypeInfo)

	/* Float32Constant */
	IMPL_PRIMITIVE_TYPE_INFO8(Float32TypeInfo, "float", ByteTypeInfo, Int32TypeInfo, UInt32TypeInfo, Int64TypeInfo, UInt64TypeInfo, CharByteTypeInfo, CharTypeInfo, FloatingConstantType)
	IMPL_PRIMITIVE_NON_BITWISE_OPERATORS(Float32TypeInfo)

	/* Float64Constant */
	IMPL_PRIMITIVE_TYPE_INFO9(Float64TypeInfo, "float64", Float32TypeInfo, ByteTypeInfo, Int32TypeInfo, UInt32TypeInfo, Int64TypeInfo, UInt64TypeInfo, CharByteTypeInfo, CharTypeInfo, FloatingConstantType)
	IMPL_PRIMITIVE_NON_BITWISE_OPERATORS(Float64TypeInfo)

	/* CharByteConstant */
	IMPL_PRIMITIVE_TYPE_INFO1(CharByteTypeInfo, "char", CharConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(CharByteTypeInfo)

	/* CharConstant*/
	IMPL_PRIMITIVE_TYPE_INFO1(CharTypeInfo, "wchar", CharConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(CharTypeInfo)

	/* BoolConstant */
	IMPL_PRIMITIVE_TYPE_INFO1(BoolTypeInfo, "bool", BoolConstantType)
	int BoolTypeInfo::_supportedOperations = 
		EqualToOperation::Id | NotEqualToOperation::Id | LogicalAndOperation::Id | LogicalOrOperation::Id | NegateOperation::Id;

	/* ByteConstant */
	IMPL_PRIMITIVE_TYPE_INFO2(ByteTypeInfo, "byte", CharByteTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(ByteTypeInfo)

}