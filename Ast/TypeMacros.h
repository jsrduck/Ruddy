#pragma once


namespace Ast {

#define DECLARE_PRIMITIVE_TYPE_INFO_OVERRIDES(CLASS_NAME) \
	public: \
		virtual bool IsLegalTypeForAssignment(std::shared_ptr<SymbolTable> symbolTable) override { return true; } \
		virtual bool IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) override; \
		virtual const std::string& Name() override; \
		virtual bool SupportsOperator(Operation* operation) override; \
		static std::shared_ptr<TypeInfo> Get() { return _staticInstance; }; \
		virtual llvm::Value* CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module* module) override; \
		virtual llvm::Type* GetIRType(llvm::LLVMContext* context, bool isOutput = false) override; \
		virtual std::string SerializedName(std::shared_ptr<SymbolTable> symbolTable) override; \
	private: \
		static const std::string _name; \
		static int _supportedOperations; \
		static std::shared_ptr<CLASS_NAME> _staticInstance; \

#define DECLARE_PRIMITIVE_INTEGER_TYPE_INFO(CLASS_NAME,SIGNED,BITS) \
	class CLASS_NAME : public IntegerTypeInfo \
	{ \
	public: \
		virtual llvm::Value* CreateValue(llvm::LLVMContext* context, uint64_t constant) override; \
		virtual bool Signed() override { return SIGNED; } \
		virtual int Bits() override { return BITS; } \
		DECLARE_PRIMITIVE_TYPE_INFO_OVERRIDES(CLASS_NAME) \
	};

#define DECLARE_PRIMITIVE_FLOATING_TYPE_INFO(CLASS_NAME) \
	class CLASS_NAME : public FloatingTypeInfo \
	{ \
		DECLARE_PRIMITIVE_TYPE_INFO_OVERRIDES(CLASS_NAME) \
	};

#define DECLARE_PRIMITIVE_TYPE_INFO(CLASS_NAME) \
	class CLASS_NAME : public PrimitiveTypeInfo \
	{ \
		DECLARE_PRIMITIVE_TYPE_INFO_OVERRIDES(CLASS_NAME) \
		public: \
			virtual llvm::Value* GetDefaultValue(llvm::LLVMContext* context) override; \
	};

#define DECLARE_PRIMITIVE_TYPE_INFO_AUTO_IMPLICIT_CAST_TO(CLASS_NAME, SIGNED, BITS, AUTO_IMPLICIT_CAST_TO_CLASS_NAME) \
	class CLASS_NAME : public IntegerTypeInfo \
	{ \
	public: \
		virtual llvm::Value* CreateValue(llvm::LLVMContext* context, uint64_t constant) override; \
		virtual bool Signed() override { return SIGNED; } \
		virtual int Bits() override { return BITS; } \
		DECLARE_PRIMITIVE_TYPE_INFO_OVERRIDES(CLASS_NAME) \
	public: \
		virtual bool IsImplicitlyAssignableToAnotherTypeThatSupportsOperation(Operation* operation, std::shared_ptr<TypeInfo>& implicitCastTypeOut) override \
		{ \
			if (AUTO_IMPLICIT_CAST_TO_CLASS_NAME::Get()->SupportsOperator(operation)) \
			{ \
				implicitCastTypeOut = AUTO_IMPLICIT_CAST_TO_CLASS_NAME::Get(); \
				return true; \
			} \
			return false; \
		} \
	};

#define IMPL_PRIMITIVE_ALL_OPERATORS(CLASS_NAME) \
	int CLASS_NAME::_supportedOperations = 0xFFFFFFFF;

#define IMPL_PRIMITIVE_NON_BITWISE_OPERATORS(CLASS_NAME) \
	int CLASS_NAME::_supportedOperations = 0xFFFFFFFF & ~BitwiseAndOperation::Id & ~BitwiseOrOperation::Id \
		& ~BitwiseXorOperation::Id & ~BitwiseShiftLeftOperation::Id & ~BitwiseShiftRightOperation::Id & ~ComplementOperation::Id;

#define IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME, TYPE_NAME) \
	const std::string CLASS_NAME::_name = std::string(TYPE_NAME); \
	const std::string& CLASS_NAME::Name() { return _name; } \
	std::shared_ptr<CLASS_NAME> CLASS_NAME::_staticInstance = std::make_shared<CLASS_NAME>(); \
	bool CLASS_NAME::SupportsOperator(Operation* operation) \
	{ \
		return (operation->OperatorId() & _supportedOperations) != 0; \
	} \
	std::string CLASS_NAME::SerializedName(std::shared_ptr<SymbolTable>) \
	{ \
		return TYPE_NAME; \
	}

#define IMPL_PRIMITIVE_TYPE_INFO(CLASS_NAME,TYPE_NAME) \
	IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME,TYPE_NAME) \
	bool CLASS_NAME::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) \
	{ \
		return std::dynamic_pointer_cast<CLASS_NAME>(other) != nullptr; \
	} 

#define IMPL_PRIMITIVE_TYPE_INFO1(CLASS_NAME,TYPE_NAME,ASSIGNABLE_FROM_CLASS_NAME1) \
	IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME,TYPE_NAME) \
	bool CLASS_NAME::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) \
	{ \
		return std::dynamic_pointer_cast<CLASS_NAME>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME1>(other) != nullptr; \
	}

#define IMPL_PRIMITIVE_TYPE_INFO2(CLASS_NAME,TYPE_NAME,ASSIGNABLE_FROM_CLASS_NAME1,ASSIGNABLE_FROM_CLASS_NAME2) \
	IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME,TYPE_NAME) \
	bool CLASS_NAME::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) \
	{ \
		return std::dynamic_pointer_cast<CLASS_NAME>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME1>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME2>(other) != nullptr; \
	} 

#define IMPL_PRIMITIVE_TYPE_INFO3(CLASS_NAME,TYPE_NAME,ASSIGNABLE_FROM_CLASS_NAME1,ASSIGNABLE_FROM_CLASS_NAME2,ASSIGNABLE_FROM_CLASS_NAME3) \
	IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME,TYPE_NAME) \
	bool CLASS_NAME::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) \
	{ \
		return std::dynamic_pointer_cast<CLASS_NAME>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME1>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME2>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME3>(other) != nullptr; \
	} 

#define IMPL_PRIMITIVE_TYPE_INFO4(CLASS_NAME,TYPE_NAME,ASSIGNABLE_FROM_CLASS_NAME1,ASSIGNABLE_FROM_CLASS_NAME2,ASSIGNABLE_FROM_CLASS_NAME3,ASSIGNABLE_FROM_CLASS_NAME4) \
	IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME,TYPE_NAME) \
	bool CLASS_NAME::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) \
	{ \
		return std::dynamic_pointer_cast<CLASS_NAME>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME1>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME2>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME3>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME4>(other) != nullptr; \
	}

#define IMPL_PRIMITIVE_TYPE_INFO5(CLASS_NAME,TYPE_NAME,ASSIGNABLE_FROM_CLASS_NAME1,ASSIGNABLE_FROM_CLASS_NAME2,ASSIGNABLE_FROM_CLASS_NAME3,ASSIGNABLE_FROM_CLASS_NAME4,ASSIGNABLE_FROM_CLASS_NAME5) \
	IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME,TYPE_NAME) \
	bool CLASS_NAME::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) \
	{ \
		return std::dynamic_pointer_cast<CLASS_NAME>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME1>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME2>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME3>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME4>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME5>(other) != nullptr; \
	} 

#define IMPL_PRIMITIVE_TYPE_INFO6(CLASS_NAME,TYPE_NAME,ASSIGNABLE_FROM_CLASS_NAME1,ASSIGNABLE_FROM_CLASS_NAME2,ASSIGNABLE_FROM_CLASS_NAME3,ASSIGNABLE_FROM_CLASS_NAME4,ASSIGNABLE_FROM_CLASS_NAME5,ASSIGNABLE_FROM_CLASS_NAME6) \
	IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME,TYPE_NAME) \
	bool CLASS_NAME::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) \
	{ \
		return std::dynamic_pointer_cast<CLASS_NAME>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME1>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME2>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME3>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME4>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME5>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME6>(other) != nullptr; \
	} 

#define IMPL_PRIMITIVE_TYPE_INFO7(CLASS_NAME,TYPE_NAME,ASSIGNABLE_FROM_CLASS_NAME1,ASSIGNABLE_FROM_CLASS_NAME2,ASSIGNABLE_FROM_CLASS_NAME3,ASSIGNABLE_FROM_CLASS_NAME4,ASSIGNABLE_FROM_CLASS_NAME5,ASSIGNABLE_FROM_CLASS_NAME6,ASSIGNABLE_FROM_CLASS_NAME7) \
	IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME,TYPE_NAME) \
	bool CLASS_NAME::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) \
	{ \
		return std::dynamic_pointer_cast<CLASS_NAME>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME1>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME2>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME3>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME4>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME5>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME6>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME7>(other) != nullptr; \
	} 

#define IMPL_PRIMITIVE_TYPE_INFO8(CLASS_NAME,TYPE_NAME,ASSIGNABLE_FROM_CLASS_NAME1,ASSIGNABLE_FROM_CLASS_NAME2,ASSIGNABLE_FROM_CLASS_NAME3,ASSIGNABLE_FROM_CLASS_NAME4,ASSIGNABLE_FROM_CLASS_NAME5,ASSIGNABLE_FROM_CLASS_NAME6,ASSIGNABLE_FROM_CLASS_NAME7,ASSIGNABLE_FROM_CLASS_NAME8) \
	IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME,TYPE_NAME) \
	bool CLASS_NAME::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) \
	{ \
		return std::dynamic_pointer_cast<CLASS_NAME>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME1>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME2>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME3>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME4>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME5>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME6>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME7>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME8>(other) != nullptr; \
	} 

#define IMPL_PRIMITIVE_TYPE_INFO9(CLASS_NAME,TYPE_NAME,ASSIGNABLE_FROM_CLASS_NAME1,ASSIGNABLE_FROM_CLASS_NAME2,ASSIGNABLE_FROM_CLASS_NAME3,ASSIGNABLE_FROM_CLASS_NAME4,ASSIGNABLE_FROM_CLASS_NAME5,ASSIGNABLE_FROM_CLASS_NAME6,ASSIGNABLE_FROM_CLASS_NAME7,ASSIGNABLE_FROM_CLASS_NAME8,ASSIGNABLE_FROM_CLASS_NAME9) \
	IMPL_PRIMITIVE_TYPE_INFO_STD(CLASS_NAME,TYPE_NAME) \
	bool CLASS_NAME::IsImplicitlyAssignableFrom(std::shared_ptr<TypeInfo> other, std::shared_ptr<SymbolTable> symbolTable) \
	{ \
		return std::dynamic_pointer_cast<CLASS_NAME>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME1>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME2>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME3>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME4>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME5>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME6>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME7>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME8>(other) != nullptr || \
			std::dynamic_pointer_cast<ASSIGNABLE_FROM_CLASS_NAME9>(other) != nullptr; \
	} 

#define IMPL_PRIMITIVE_INTEGER_TYPE(CLASS_NAME,BITS,SIGNED) \
	llvm::Value* CLASS_NAME::CreateValue(llvm::LLVMContext* context, uint64_t constant) \
	{ \
		return llvm::ConstantInt::get(*context, llvm::APInt(BITS, constant, SIGNED /*isSigned*/)); \
	}
}