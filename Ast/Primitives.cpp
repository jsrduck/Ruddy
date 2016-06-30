#include "stdafx.h"
#include "Primitives.h"
#include "Operations.h"

#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>

namespace Ast
{

	std::shared_ptr<IntegerConstantType> IntegerConstant::_typeInfo = std::make_shared<IntegerConstantType>();
	std::shared_ptr<TypeInfo> IntegerConstantType::Get()
	{
		return IntegerConstant::_typeInfo;
	};
	std::shared_ptr<FloatingConstantType> FloatingConstant::_typeInfo = std::make_shared<FloatingConstantType>();
	std::shared_ptr<TypeInfo> FloatingConstantType::Get()
	{
		return FloatingConstant::_typeInfo;
	};
	std::shared_ptr<BoolConstantType> BoolConstant::_typeInfo = std::make_shared<BoolConstantType>();
	std::shared_ptr<TypeInfo> BoolConstantType::Get()
	{
		return BoolConstant::_typeInfo;
	};
	std::shared_ptr<CharConstantType> CharConstant::_typeInfo = std::make_shared<CharConstantType>();
	std::shared_ptr<TypeInfo> CharConstantType::Get()
	{
		return CharConstant::_typeInfo;
	};
	std::shared_ptr<StringConstantType> StringConstant::_typeInfo = std::make_shared<StringConstantType>();
	std::shared_ptr<TypeInfo> StringConstantType::Get()
	{
		return StringConstant::_typeInfo;
	};

	/* String */
	IMPL_PRIMITIVE_TYPE_INFO1(StringTypeInfo, "string", StringConstantType)
	int StringTypeInfo::_supportedOperations = 0x0; // TODO: maybe add + as a concat
	llvm::AllocaInst* StringTypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		// TODO
		//return builder->CreateAlloca(llvm::ArrayType::get(llvm::Type::getInt8Ty(*context), ))
		throw UnexpectedException();
	}
	llvm::Type* StringTypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		// TODO
		throw UnexpectedException();
	}

	/* Int32 */
	IMPL_PRIMITIVE_TYPE_INFO4(Int32TypeInfo, "int32", ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(Int32TypeInfo)
	IMPL_PRIMITIVE_INTEGER_TYPE(Int32TypeInfo, 32, true)
	llvm::AllocaInst* Int32TypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(llvm::Type::getInt32Ty(*context), nullptr, name.c_str());
	}
	llvm::Type* Int32TypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		if (asOutput)
			return llvm::Type::getInt32PtrTy(*context);
		else
			return llvm::Type::getInt32Ty(*context);
	}

	/* Int64*/
	IMPL_PRIMITIVE_TYPE_INFO6(Int64TypeInfo, "int64", Int32TypeInfo, UInt32TypeInfo, ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(Int64TypeInfo)
	IMPL_PRIMITIVE_INTEGER_TYPE(Int64TypeInfo, 64, true)
	llvm::AllocaInst* Int64TypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(llvm::Type::getInt64Ty(*context), nullptr, name.c_str());
	}
	llvm::Type* Int64TypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		if (asOutput)
			return llvm::Type::getInt64PtrTy(*context);
		else
			return llvm::Type::getInt64Ty(*context);
	}

	/* UInt32 */
	IMPL_PRIMITIVE_TYPE_INFO4(UInt32TypeInfo, "uint32", ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(UInt32TypeInfo)
	IMPL_PRIMITIVE_INTEGER_TYPE(UInt32TypeInfo, 32, false)
	llvm::AllocaInst* UInt32TypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(llvm::Type::getInt32Ty(*context), nullptr, name.c_str());
	}
	llvm::Type* UInt32TypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		if (asOutput)
			return llvm::Type::getInt32PtrTy(*context);
		else
			return llvm::Type::getInt32Ty(*context);
	}

	/* UInt64 */
	IMPL_PRIMITIVE_TYPE_INFO5(UInt64TypeInfo, "uint64", UInt32TypeInfo, ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(UInt64TypeInfo)
	IMPL_PRIMITIVE_INTEGER_TYPE(UInt64TypeInfo, 64, false)
	llvm::AllocaInst* UInt64TypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(llvm::Type::getInt64Ty(*context), nullptr, name.c_str());
	}
	llvm::Type* UInt64TypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		if (asOutput)
			return llvm::Type::getInt64PtrTy(*context);
		else
			return llvm::Type::getInt64Ty(*context);
	}

	/* Float32 */
	IMPL_PRIMITIVE_TYPE_INFO8(Float32TypeInfo, "float", ByteTypeInfo, Int32TypeInfo, UInt32TypeInfo, Int64TypeInfo, UInt64TypeInfo, CharByteTypeInfo, CharTypeInfo, FloatingConstantType)
	IMPL_PRIMITIVE_NON_BITWISE_OPERATORS(Float32TypeInfo)
	llvm::AllocaInst* Float32TypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(llvm::Type::getFloatTy(*context), nullptr, name.c_str());
	}
	llvm::Type* Float32TypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		if (asOutput)
			return llvm::Type::getFloatPtrTy(*context);
		else
			return llvm::Type::getFloatTy(*context);
	}

	/* Float64 */
	IMPL_PRIMITIVE_TYPE_INFO9(Float64TypeInfo, "float64", Float32TypeInfo, ByteTypeInfo, Int32TypeInfo, UInt32TypeInfo, Int64TypeInfo, UInt64TypeInfo, CharByteTypeInfo, CharTypeInfo, FloatingConstantType)
	IMPL_PRIMITIVE_NON_BITWISE_OPERATORS(Float64TypeInfo)
	llvm::AllocaInst* Float64TypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(llvm::Type::getDoubleTy(*context), nullptr, name.c_str());
	}
	llvm::Type* Float64TypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		if (asOutput)
			return llvm::Type::getDoublePtrTy(*context);
		else
			return llvm::Type::getDoubleTy(*context);
	}

	/* CharByte */
	IMPL_PRIMITIVE_TYPE_INFO1(CharByteTypeInfo, "char", CharConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(CharByteTypeInfo)
	llvm::AllocaInst* CharByteTypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(llvm::Type::getInt8Ty(*context), nullptr, name.c_str());
	}
	llvm::Type* CharByteTypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		if (asOutput)
			return llvm::Type::getInt8PtrTy(*context);
		else
			return llvm::Type::getInt8Ty(*context);
	}

	/* Char */
	IMPL_PRIMITIVE_TYPE_INFO1(CharTypeInfo, "wchar", CharConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(CharTypeInfo)
	llvm::AllocaInst* CharTypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(llvm::Type::getInt16Ty(*context), nullptr, name.c_str());
	}
	llvm::Type* CharTypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		if (asOutput)
			return llvm::Type::getInt16PtrTy(*context);
		else
			return llvm::Type::getInt16Ty(*context);
	}

	/* Bool */
	IMPL_PRIMITIVE_TYPE_INFO1(BoolTypeInfo, "bool", BoolConstantType)
	int BoolTypeInfo::_supportedOperations = 
		EqualToOperation::Id | NotEqualToOperation::Id | LogicalAndOperation::Id | LogicalOrOperation::Id | NegateOperation::Id;
	llvm::AllocaInst* BoolTypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(llvm::Type::getInt1Ty(*context), nullptr, name.c_str());
	}
	llvm::Type* BoolTypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		if (asOutput)
			return llvm::Type::getInt1PtrTy(*context);
		else
			return llvm::Type::getInt1Ty(*context);
	}

	/* Byte */
	IMPL_PRIMITIVE_TYPE_INFO1(ByteTypeInfo, "byte", CharByteTypeInfo)
	IMPL_PRIMITIVE_ALL_OPERATORS(ByteTypeInfo)
	llvm::AllocaInst* ByteTypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(llvm::Type::getInt8Ty(*context), nullptr, name.c_str());
	}
	llvm::Type* ByteTypeInfo::GetIRType(llvm::LLVMContext* context, bool asOutput)
	{
		if (asOutput)
			return llvm::Type::getInt8PtrTy(*context);
		else
			return llvm::Type::getInt8Ty(*context);
	}

}