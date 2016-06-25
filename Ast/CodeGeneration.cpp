#include "stdafx.h"
#include "Expressions.h"
#include "Statements.h"
#include "Classes.h"
#include "Operations.h"

#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>
#include <llvm\IR\Verifier.h>

//static llvm::IRBuilder<> Builder(*context);

namespace Ast {

	llvm::AllocaInst* DeclareVariable::GetAllocation(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return symbolTable->Lookup(_name)->CreateAllocationInstance(_name, builder, context);
	}

	llvm::AllocaInst* AssignFromReference::GetAllocation(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return symbolTable->Lookup(_ref)->GetAllocationInstance();
	}

	void AssignFrom::CodeGen(std::shared_ptr<Expression> rhs, std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		// Worry about multi-types later
		if (_next != nullptr)
		{
			throw UnexpectedException();
		}
		else
		{
			auto rhValue = rhs->CodeGen(symbolTable, builder, context, module, _thisType);
			auto alloc = _thisOne->GetAllocation(symbolTable, builder, context);
			builder->CreateStore(rhValue, alloc);
		}
	}

	llvm::Value* DebugPrintStatement::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto value = _expression->CodeGen(symbolTable, builder, context, module);
		std::vector<llvm::Value*> args;

		if (value->getType()->isIntegerTy())
		{
			// Create a string on the stack, and then print it
			auto strConst = llvm::ConstantDataArray::getString(*context, llvm::StringRef("%d"));
			auto alloc = builder->CreateAlloca(strConst->getType(), builder->getInt32(llvm::dyn_cast<llvm::ConstantDataSequential>(strConst)->getNumElements()));
			builder->CreateStore(strConst, alloc);
			std::vector<llvm::Value*> index_vector;
			index_vector.push_back(builder->getInt32(0));
			index_vector.push_back(builder->getInt32(0));
			auto valueAsPtr = builder->CreateGEP(alloc, index_vector);

			// printf("%d", val)
			args.push_back(valueAsPtr);
			args.push_back(value);
		}
		else if (value->getType()->isDoubleTy())
		{
			// Create a string on the stack, and then print it
			auto strConst = llvm::ConstantDataArray::getString(*context, llvm::StringRef("%f"));
			auto alloc = builder->CreateAlloca(strConst->getType(), builder->getInt32(llvm::dyn_cast<llvm::ConstantDataSequential>(strConst)->getNumElements()));
			builder->CreateStore(strConst, alloc);
			std::vector<llvm::Value*> index_vector;
			index_vector.push_back(builder->getInt32(0));
			index_vector.push_back(builder->getInt32(0));
			auto valueAsPtr = builder->CreateGEP(alloc, index_vector);

			// printf("%f", val)
			args.push_back(valueAsPtr);
			args.push_back(value);
		}
		else if (llvm::isa<llvm::AllocaInst>(value))
		{
			std::vector<llvm::Value*> index_vector;
			index_vector.push_back(builder->getInt32(0));
			index_vector.push_back(builder->getInt32(0));
			auto valueAsPtr = builder->CreateGEP(value, index_vector);
			args.push_back(valueAsPtr);
		}
		else if (value->getType()->isArrayTy())
		{
			// Store the string
			auto alloc = builder->CreateAlloca(value->getType(), builder->getInt32(llvm::dyn_cast<llvm::ConstantDataSequential>(value)->getNumElements()));
			builder->CreateStore(value, alloc);

			std::vector<llvm::Value*> index_vector;
			index_vector.push_back(builder->getInt32(0));
			index_vector.push_back(builder->getInt32(0));
			auto valueAsPtr = builder->CreateGEP(alloc, index_vector);
			args.push_back(valueAsPtr);
		}
		else
		{
			throw UnexpectedException();
		}
		builder->CreateCall(module->getFunction("_os_printf"), args);
		return nullptr; // ???
	}

	llvm::Value* Reference::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto symbol = symbolTable->Lookup(_id);
		if (symbol->GetTypeInfo()->IsPrimitiveType())
			return builder->CreateLoad(symbol->GetAllocationInstance(), _id);
		else
			return symbol->GetAllocationInstance();
	}

	llvm::Value* ExpressionList::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		throw UnexpectedException();
		//return _symbol->GetAllocationInstance();
	}

	/*
	 * Constants
	*/


	llvm::Value* StringConstant::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		return llvm::ConstantDataArray::getString(*context, llvm::StringRef(_input.c_str()));
	}

	llvm::Value* IntegerConstant::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		std::shared_ptr<IntegerTypeInfo> hintAsInteger = nullptr;
		if (hint != nullptr)
			hintAsInteger = std::dynamic_pointer_cast<IntegerTypeInfo>(hint);
		if (hintAsInteger == nullptr)
		{
			// Trying to fit auto type, pick the best type we have
			if (FitsInt32())
				return llvm::ConstantInt::get(*context, llvm::APInt(32, GetRaw(), true));
			else if (FitsInt64())
				return llvm::ConstantInt::get(*context, llvm::APInt(64, GetRaw(), true));
			else
				return llvm::ConstantInt::get(*context, llvm::APInt(64, GetRaw(), false));
		}
		else
		{
			// Try to fit it in the given type
			return hintAsInteger->CreateValue(context, GetRaw());
		}
	}

	llvm::Value* FloatingConstant::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		if (hint != nullptr && std::dynamic_pointer_cast<Float32TypeInfo>(hint) != nullptr)
			return llvm::ConstantFP::get(*context, llvm::APFloat(AsFloat32()));

		// Just assume double
		return llvm::ConstantFP::get(*context, llvm::APFloat(AsFloat64()));
	}

	llvm::Value* BoolConstant::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		if (_value)
			return llvm::ConstantInt::getTrue(*context);
		else
			return llvm::ConstantInt::getFalse(*context);
	}

	/*
	 * Operations
	 */

	llvm::Value* AddOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);

		if (std::dynamic_pointer_cast<IntegerTypeInfo>(_resultOfOperation) != nullptr || _resultOfOperation->Equals(IntegerConstantType::Get()))
		{
			return builder->CreateAdd(lhs, rhs); // TODO: Think about overflow
		}
		else if (_resultOfOperation->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFAdd(lhs, rhs);
		}
		// TODO: Non integer types that implement add
		throw UnexpectedException();
	}

	llvm::Value* SubtractOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);

		if (std::dynamic_pointer_cast<IntegerTypeInfo>(_resultOfOperation) != nullptr || std::dynamic_pointer_cast<IntegerConstantType>(_resultOfOperation) != nullptr)
		{
			return builder->CreateSub(lhs, rhs); // TODO: Think about overflow, think about my rules (float + int) vs what LLVM will actually do
		}
		else if (_resultOfOperation->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFSub(lhs, rhs);
		}
		// TODO: Non integer types that implement subtract
		throw UnexpectedException();
	}

	llvm::Value* MultiplyOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);

		if (std::dynamic_pointer_cast<IntegerTypeInfo>(_resultOfOperation) != nullptr || std::dynamic_pointer_cast<IntegerConstantType>(_resultOfOperation) != nullptr)
		{
			return builder->CreateMul(lhs, rhs); // TODO: Think about overflow, think about my rules (float + int) vs what LLVM will actually do
		}
		else if (_resultOfOperation->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFMul(lhs, rhs);
		}
		// TODO: Non integer types that implement multiply
		throw UnexpectedException();
	}

	llvm::Value* DivideOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);

		if (std::dynamic_pointer_cast<IntegerTypeInfo>(_resultOfOperation) != nullptr || std::dynamic_pointer_cast<IntegerConstantType>(_resultOfOperation) != nullptr)
		{
			return builder->CreateSDiv(lhs, rhs); // TODO: Think about overflow, think about my rules (float + int) vs what LLVM will actually do
		}
		else if (_resultOfOperation->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFDiv(lhs, rhs);
		}
		// TODO: Non integer types that implement divide
		throw UnexpectedException();
	}

	llvm::Value* RemainderOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);

		if (std::dynamic_pointer_cast<IntegerTypeInfo>(_resultOfOperation) != nullptr || std::dynamic_pointer_cast<IntegerConstantType>(_resultOfOperation) != nullptr)
		{
			return builder->CreateSRem(lhs, rhs); // TODO: Think about overflow, think about my rules (float + int) vs what LLVM will actually do
		}
		else if (_resultOfOperation->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFRem(lhs, rhs);
		}
		// TODO: Non integer types that implement divide
		throw UnexpectedException();
	}

	llvm::Value* GreaterThanOrEqualOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		// At this point, we've already done the type checking, so one should be implicitly assignable to the other
		std::shared_ptr<TypeInfo> implicitCastType;
		if (_rhsTypeInfo->IsImplicitlyAssignableFrom(_lhsTypeInfo, symbolTable))
			implicitCastType = _lhsTypeInfo;
		else
			implicitCastType = _rhsTypeInfo;

		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, implicitCastType);

		auto intType = std::dynamic_pointer_cast<IntegerTypeInfo>(implicitCastType);
		if (intType != nullptr || implicitCastType->Equals(IntegerConstantType::Get()))
		{
			if (intType == nullptr || intType->Signed())
				return builder->CreateICmpSGE(lhs, rhs); // TODO: Think about overflow, think about IntegerConstantType being big enough to be unsigned
			else
				return builder->CreateICmpUGE(lhs, rhs);
		}
		else if (implicitCastType->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFCmpOGE(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GTE
		throw UnexpectedException();
	}

	llvm::Value* LessThanOrEqualOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		// At this point, we've already done the type checking, so one should be implicitly assignable to the other
		std::shared_ptr<TypeInfo> implicitCastType;
		if (_rhsTypeInfo->IsImplicitlyAssignableFrom(_lhsTypeInfo, symbolTable))
			implicitCastType = _lhsTypeInfo;
		else
			implicitCastType = _rhsTypeInfo;

		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, implicitCastType);

		auto intType = std::dynamic_pointer_cast<IntegerTypeInfo>(implicitCastType);
		if (intType != nullptr || implicitCastType->Equals(IntegerConstantType::Get()))
		{
			if (intType == nullptr || intType->Signed())
				return builder->CreateICmpSLE(lhs, rhs); // TODO: Think about overflow, think about IntegerConstantType being big enough to be unsigned
			else
				return builder->CreateICmpULE(lhs, rhs);
		}
		else if (implicitCastType->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFCmpOGE(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GTE
		throw UnexpectedException();
	}

	llvm::Value* GreaterThanOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		// At this point, we've already done the type checking, so one should be implicitly assignable to the other
		std::shared_ptr<TypeInfo> implicitCastType;
		if (_rhsTypeInfo->IsImplicitlyAssignableFrom(_lhsTypeInfo, symbolTable))
			implicitCastType = _lhsTypeInfo;
		else
			implicitCastType = _rhsTypeInfo;

		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, implicitCastType);

		auto intType = std::dynamic_pointer_cast<IntegerTypeInfo>(implicitCastType);
		if (intType != nullptr || implicitCastType->Equals(IntegerConstantType::Get()))
		{
			if (intType == nullptr || intType->Signed())
				return builder->CreateICmpSGT(lhs, rhs); // TODO: Think about overflow, think about IntegerConstantType being big enough to be unsigned
			else
				return builder->CreateICmpUGT(lhs, rhs);
		}
		else if (implicitCastType->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFCmpOGT(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GT
		throw UnexpectedException();
	}

	llvm::Value* LessThanOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		// At this point, we've already done the type checking, so one should be implicitly assignable to the other
		std::shared_ptr<TypeInfo> implicitCastType;
		if (_rhsTypeInfo->IsImplicitlyAssignableFrom(_lhsTypeInfo, symbolTable))
			implicitCastType = _lhsTypeInfo;
		else
			implicitCastType = _rhsTypeInfo;

		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, implicitCastType);

		auto intType = std::dynamic_pointer_cast<IntegerTypeInfo>(implicitCastType);
		if (intType != nullptr || implicitCastType->Equals(IntegerConstantType::Get()))
		{
			if (intType == nullptr || intType->Signed())
				return builder->CreateICmpSLT(lhs, rhs); // TODO: Think about overflow, think about IntegerConstantType being big enough to be unsigned
			else
				return builder->CreateICmpULT(lhs, rhs);
		}
		else if (implicitCastType->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFCmpOLT(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GT
		throw UnexpectedException();
	}

	llvm::Value* EqualToOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		// At this point, we've already done the type checking, so one should be implicitly assignable to the other
		std::shared_ptr<TypeInfo> implicitCastType;
		if (_rhsTypeInfo->IsImplicitlyAssignableFrom(_lhsTypeInfo, symbolTable))
			implicitCastType = _lhsTypeInfo;
		else
			implicitCastType = _rhsTypeInfo;

		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, implicitCastType);

		auto intType = std::dynamic_pointer_cast<IntegerTypeInfo>(implicitCastType);
		if (intType != nullptr || implicitCastType->Equals(IntegerConstantType::Get()))
		{
			builder->CreateICmpEQ(lhs, rhs);
		}
		else if (implicitCastType->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFCmpOEQ(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GT
		throw UnexpectedException();
	}

	llvm::Value* NotEqualToOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		// At this point, we've already done the type checking, so one should be implicitly assignable to the other
		std::shared_ptr<TypeInfo> implicitCastType;
		if (_rhsTypeInfo->IsImplicitlyAssignableFrom(_lhsTypeInfo, symbolTable))
			implicitCastType = _lhsTypeInfo;
		else
			implicitCastType = _rhsTypeInfo;

		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, implicitCastType);

		auto intType = std::dynamic_pointer_cast<IntegerTypeInfo>(implicitCastType);
		if (intType != nullptr || implicitCastType->Equals(IntegerConstantType::Get()))
		{
			builder->CreateICmpNE(lhs, rhs);
		}
		else if (implicitCastType->Equals(FloatingConstantType::Get()) || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr || std::dynamic_pointer_cast<Float64TypeInfo>(implicitCastType) != nullptr)
		{
			// doubles (float64)
			return builder->CreateFCmpONE(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GT
		throw UnexpectedException();
	}

	// A single static constant representing "1" for all the increment/decrement operators
	static std::shared_ptr<IntegerConstant> s_one = std::make_shared<IntegerConstant>("1");

	llvm::Value* PostIncrementOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expr->CodeGen(symbolTable, builder, context, module, hint);
		// Now increment it by one
		auto result = builder->CreateAdd(val, s_one->CodeGen(symbolTable, builder, context, module, hint));
		// Now store the result. For now, we assume it's a reference (nothing else supports these operations)
		auto exprAsReference = std::dynamic_pointer_cast<Reference>(_expr);
		if (exprAsReference == nullptr)
			throw UnexpectedException(); // TODO: What if user makes syntax error, like 0++. What do we want to report?
		auto symbol = symbolTable->Lookup(exprAsReference->Id());
		builder->CreateStore(result, symbol->GetAllocationInstance());
		return val; // Return the result before the increment
	}

	llvm::Value* PostDecrementOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expr->CodeGen(symbolTable, builder, context, module, hint);
		// Now decrement it by one
		auto result = builder->CreateSub(val, s_one->CodeGen(symbolTable, builder, context, module, hint));
		// Now store the result. For now, we assume it's a reference (nothing else supports these operations)
		auto exprAsReference = std::dynamic_pointer_cast<Reference>(_expr);
		if (exprAsReference == nullptr)
			throw UnexpectedException(); // TODO: What if user makes syntax error, like 0++. What do we want to report?
		auto symbol = symbolTable->Lookup(exprAsReference->Id());
		builder->CreateStore(result, symbol->GetAllocationInstance());
		return val; // Return the result before the decrement
	}

	llvm::Value* PreIncrementOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		// Evaluate expression first
		auto val = _expr->CodeGen(symbolTable, builder, context, module, hint);
		// Now increment it by one
		auto result = builder->CreateAdd(val, s_one->CodeGen(symbolTable, builder, context, module, hint));
		// Now store the result. For now, we assume it's a reference (nothing else supports these operations)
		auto exprAsReference = std::dynamic_pointer_cast<Reference>(_expr);
		if (exprAsReference == nullptr)
			throw UnexpectedException(); // TODO: What if user makes syntax error, like 0++. What do we want to report?
		auto symbol = symbolTable->Lookup(exprAsReference->Id());
		builder->CreateStore(result, symbol->GetAllocationInstance());
		return result; // Return the result after the increment
	}

	llvm::Value* PreDecrementOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		// Evaluate expression first
		auto val = _expr->CodeGen(symbolTable, builder, context, module, hint);
		// Now decrement it by one
		auto result = builder->CreateSub(val, s_one->CodeGen(symbolTable, builder, context, module, hint));
		// Now store the result. For now, we assume it's a reference (nothing else supports these operations)
		auto exprAsReference = std::dynamic_pointer_cast<Reference>(_expr);
		if (exprAsReference == nullptr)
			throw UnexpectedException(); // TODO: What if user makes syntax error, like 0++. What do we want to report?
		auto symbol = symbolTable->Lookup(exprAsReference->Id());
		builder->CreateStore(result, symbol->GetAllocationInstance());
		return result; // Return the result after the decrement
	}

	llvm::Value* NegateOperation::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val =_expr->CodeGen(symbolTable, builder, context, module, BoolTypeInfo::Get());
		// Now return the negation of that
		return builder->CreateNot(val);
	}
}