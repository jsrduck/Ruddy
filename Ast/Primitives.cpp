#include "stdafx.h"
#include "Primitives.h"
#include "Operations.h"

#include <unordered_map>
#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>

namespace Ast
{
	std::shared_ptr<IntegerConstantType> IntegerConstant::_staticTypeInfo = std::make_shared<IntegerConstantType>();
	std::shared_ptr<TypeInfo> IntegerConstantType::Get()
	{
		return IntegerConstant::_staticTypeInfo;
	};
	std::shared_ptr<FloatingConstantType> FloatingConstant::_staticTypeInfo = std::make_shared<FloatingConstantType>();
	std::shared_ptr<TypeInfo> FloatingConstantType::Get()
	{
		return FloatingConstant::_staticTypeInfo;
	};
	std::shared_ptr<BoolConstantType> BoolConstant::_staticTypeInfo = std::make_shared<BoolConstantType>();
	std::shared_ptr<TypeInfo> BoolConstantType::Get()
	{
		return BoolConstant::_staticTypeInfo;
	};
	std::shared_ptr<CharConstantType> CharConstant::_staticTypeInfo = std::make_shared<CharConstantType>();
	std::shared_ptr<TypeInfo> CharConstantType::Get()
	{
		return CharConstant::_staticTypeInfo;
	};
	std::shared_ptr<StringConstantType> StringConstant::_staticTypeInfo = std::make_shared<StringConstantType>();
	std::shared_ptr<TypeInfo> StringConstantType::Get()
	{
		return StringConstant::_staticTypeInfo;
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

	llvm::Value* StringTypeInfo::GetDefaultValue(llvm::LLVMContext* context)
	{
		return llvm::ConstantPointerNull::get(this->GetIRType(context)->getPointerTo());
	}

	/* IntegerTypeInfo */
	int IntegerTypeInfo::CreateCast(std::shared_ptr<TypeInfo> castTo)
	{
		if (castTo->IsInteger())
		{
			auto castToInteger = std::dynamic_pointer_cast<IntegerTypeInfo>(castTo);
			if (Bits() > castToInteger->Bits())
			{
				return llvm::Instruction::CastOps::Trunc;
			}
			else if(!Signed())
			{
				return llvm::Instruction::CastOps::ZExt;
			}
			else
			{
				return llvm::Instruction::CastOps::SExt;
			}
		}
		else if (castTo->IsFloatingPoint())
		{
			if (Signed())
				return llvm::Instruction::CastOps::SIToFP;
			else
				return llvm::Instruction::CastOps::UIToFP;
		}
		else
		{
			throw UnexpectedException();
		}
	}

	llvm::Value* IntegerTypeInfo::GetDefaultValue(llvm::LLVMContext* context)
	{
		return CreateValue(context, 0); // All int types default to zero
	}

	/* FloatingTypeInfo */
	int FloatingTypeInfo::CreateCast(std::shared_ptr<TypeInfo> castTo)
	{
		llvm::Instruction::CastOps ops;
		if (castTo->IsFloatingPoint())
		{
			if (std::dynamic_pointer_cast<Float64TypeInfo>(castTo) != nullptr)
				return llvm::Instruction::CastOps::FPExt;
			else
				return llvm::Instruction::CastOps::FPTrunc;
		}
		else if (castTo->IsInteger())
		{
			auto castToInteger = std::dynamic_pointer_cast<IntegerTypeInfo>(castTo);
			if (castToInteger->Signed())
				return llvm::Instruction::CastOps::FPToSI;
			else
				return llvm::Instruction::CastOps::FPToUI;
		}
		else
		{
			throw UnexpectedException();
		}
	}

	llvm::Value* FloatingTypeInfo::GetDefaultValue(llvm::LLVMContext* context)
	{
		return llvm::ConstantFP::get(this->GetIRType(context), 0.0);
	}

	/* Int32 */
	IMPL_PRIMITIVE_TYPE_INFO5(Int32TypeInfo, "int32", ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType, CharConstantType)
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
	IMPL_PRIMITIVE_TYPE_INFO7(Int64TypeInfo, "int64", Int32TypeInfo, UInt32TypeInfo, ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType, CharConstantType)
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
	IMPL_PRIMITIVE_TYPE_INFO5(UInt32TypeInfo, "uint32", ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType, CharConstantType)
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
	IMPL_PRIMITIVE_TYPE_INFO6(UInt64TypeInfo, "uint64", UInt32TypeInfo, ByteTypeInfo, CharByteTypeInfo, CharTypeInfo, IntegerConstantType, CharConstantType)
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
	IMPL_PRIMITIVE_TYPE_INFO2(CharByteTypeInfo, "charbyte", CharConstantType, IntegerConstantType)
	IMPL_PRIMITIVE_INTEGER_TYPE(CharByteTypeInfo, 8, false)
	// CharByte is implicitly convertable to integer, so all integer operations are de facto "supported"
	int CharByteTypeInfo::_supportedOperations =
		PostDecrementOperation::Id | PostIncrementOperation::Id | PreDecrementOperation::Id | PreIncrementOperation::Id;
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
	IMPL_PRIMITIVE_TYPE_INFO2(CharTypeInfo, "char", CharConstantType, IntegerConstantType)
	IMPL_PRIMITIVE_INTEGER_TYPE(CharTypeInfo, 16, false)
	// Char is implicitly convertable to integer, so all integer operations are de facto "supported"
	int CharTypeInfo::_supportedOperations = 
		PostDecrementOperation::Id | PostIncrementOperation::Id | PreDecrementOperation::Id | PreIncrementOperation::Id;
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
	llvm::Value* BoolTypeInfo::GetDefaultValue(llvm::LLVMContext* context)
	{
		return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), llvm::APInt(1, 0, false));
	}

	/* Byte */
	IMPL_PRIMITIVE_TYPE_INFO2(ByteTypeInfo, "byte", CharByteTypeInfo, IntegerConstantType)
	IMPL_PRIMITIVE_ALL_OPERATORS(ByteTypeInfo)
	IMPL_PRIMITIVE_INTEGER_TYPE(ByteTypeInfo, 8, true)
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

	std::unordered_map<std::string, std::shared_ptr<TypeInfo>> s_typeInfoMap;
	std::shared_ptr<TypeInfo> PrimitiveTypeInfo::LoadFrom(const std::string & typeName)
	{
		if (s_typeInfoMap.size() == 0)
		{
			s_typeInfoMap[Int32TypeInfo::Get()->SerializedName(nullptr)] = Int32TypeInfo::Get();
			s_typeInfoMap[Int64TypeInfo::Get()->SerializedName(nullptr)] = Int64TypeInfo::Get();
			s_typeInfoMap[UInt32TypeInfo::Get()->SerializedName(nullptr)] = UInt32TypeInfo::Get();
			s_typeInfoMap[UInt64TypeInfo::Get()->SerializedName(nullptr)] = UInt64TypeInfo::Get();
			s_typeInfoMap[Float32TypeInfo::Get()->SerializedName(nullptr)] = Float32TypeInfo::Get();
			s_typeInfoMap[Float64TypeInfo::Get()->SerializedName(nullptr)] = Float64TypeInfo::Get();
			s_typeInfoMap[CharByteTypeInfo::Get()->SerializedName(nullptr)] = CharByteTypeInfo::Get();
			s_typeInfoMap[CharTypeInfo::Get()->SerializedName(nullptr)] = CharTypeInfo::Get();
			s_typeInfoMap[BoolTypeInfo::Get()->SerializedName(nullptr)] = BoolTypeInfo::Get();
			s_typeInfoMap[ByteTypeInfo::Get()->SerializedName(nullptr)] = ByteTypeInfo::Get();
			s_typeInfoMap[StringTypeInfo::Get()->SerializedName(nullptr)] = StringTypeInfo::Get();
		}
		if (s_typeInfoMap.count(typeName) == 0)
			return nullptr;
		return s_typeInfoMap[typeName];
	}
}