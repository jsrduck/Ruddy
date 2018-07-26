#include "stdafx.h"
#include "Expressions.h"
#include "Statements.h"
#include "Classes.h"
#include "Operations.h"

#include <llvm\IR\Module.h>
#include <llvm\IR\IRBuilder.h>
#include <llvm\IR\Verifier.h>

namespace Ast {

	llvm::Value* CreateAssignment(std::shared_ptr<Ast::TypeInfo> expectedType, llvm::Value* lhs, llvm::Value* rhs, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		if (rhs->getType()->isPointerTy())
		{
			if (std::dynamic_pointer_cast<BaseClassTypeInfo>(expectedType)->IsValueType())
			{
				auto asAllocInst = static_cast<llvm::AllocaInst*>(lhs);
				if (asAllocInst == nullptr)
					throw UnexpectedException();
				auto llvmStructType = asAllocInst->getType()->getElementType();
				auto size = module->getDataLayout().getTypeAllocSize(expectedType->GetIRType(context)); // TODO: Why doesn't this work if structs aren't packed? It's supposed to return the size including padding...
				builder->CreateMemCpy(lhs, rhs, size, 0);
			}
			else
			{
				throw UnexpectedException(); // Not implemented yet
			}
		}
		else
		{
			builder->CreateStore(rhs, lhs);
		}
	}

	llvm::Value* DeclareVariable::GetIRValue(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		FileLocationContext locationContext(_location);
		return _symbolBinding->CreateAllocationInstance(_name, builder, context);
	}

	llvm::Value* AssignFromReference::GetIRValue(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		FileLocationContext locationContext(_location);
		return _symbolBinding->GetIRValue(builder, context, module);
	}

	void AssignFrom::CodeGen(std::shared_ptr<Expression> rhs, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		FileLocationContext locationContext(_location);
		if (_next != nullptr)
		{
			// The rhs must be an expression list or function to support this
			auto rhsAsList = std::dynamic_pointer_cast<ExpressionList>(rhs);
			auto rhsAsFunction = std::dynamic_pointer_cast<FunctionCall>(rhs);
			if (rhsAsList != nullptr)
			{
				auto rhValue = rhsAsList->_left->CodeGen(builder, context, module, _thisType);
				auto alloc = _thisOne->GetIRValue(builder, context, module);
				CreateAssignment(_thisType, alloc, rhValue, builder, context, module);
				_next->CodeGen(rhsAsList->_right, builder, context, module);
			}
			else if (rhsAsFunction != nullptr)
			{
				auto nextAsComp = std::dynamic_pointer_cast<CompositeTypeInfo>(_nextType);
				if (nextAsComp == nullptr)
					nextAsComp = std::make_shared<CompositeTypeInfo>(_nextType);

				auto compTypeExpected = std::dynamic_pointer_cast<CompositeTypeInfo>(_thisType);
				if (compTypeExpected == nullptr)
					compTypeExpected = std::make_shared<CompositeTypeInfo>(_thisType, nextAsComp);
				else
					compTypeExpected->_next = nextAsComp;
				auto functionCall = (llvm::CallInst*)rhsAsFunction->CodeGen(builder, context, module, compTypeExpected);

				// Store each output value in a value from the lhs
				auto lhsVar = _next;
				std::shared_ptr<TypeInfo> outputArgTypeInfo = std::dynamic_pointer_cast<CompositeTypeInfo>(rhsAsFunction->_typeInfo);
				for (auto &outputVal : rhsAsFunction->_outputValues)
				{
					if (outputArgTypeInfo == nullptr)
						throw UnexpectedException();
					auto asComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(outputArgTypeInfo);
					auto thisType = asComposite != nullptr ? asComposite->_thisType : outputArgTypeInfo;
					auto alloc = lhsVar->_thisOne->GetIRValue(builder, context, module);
					CreateAssignment(_thisType, alloc, builder->CreateLoad(outputVal), builder, context, module);
					lhsVar = lhsVar->_next;
					outputArgTypeInfo = asComposite != nullptr ? asComposite->_next : nullptr;
				}

				// Store the first guy that we do return
				auto alloc = _thisOne->GetIRValue(builder, context, module);
				CreateAssignment(_thisType, alloc, functionCall, builder, context, module);
			}
			else
			{
				throw UnexpectedException();
			}
		}
		else
		{
			auto rhValue = rhs->CodeGen(builder, context, module, _thisType->IsAutoType() ? nullptr : _thisType);
			auto alloc = _thisOne->GetIRValue(builder, context, module);
			CreateAssignment(_thisType, alloc, rhValue, builder, context, module);
		}
	}

	void ArgumentList::AddIRTypesToVector(std::vector<llvm::Type*>& inputVector, llvm::LLVMContext* context, bool asOutput)
	{
		if (_argument != nullptr)
		{
			auto irType = _argument->_typeInfo->GetIRType(context, asOutput);
			if (irType->isStructTy())
				irType = irType->getPointerTo();
			inputVector.push_back(irType);
		}
		if (_next != nullptr)
			_next->AddIRTypesToVector(inputVector, context, asOutput);
	}

	llvm::Value* FunctionCall::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		if (_functionTypeInfo == nullptr)
			throw UnexpectedException();
		auto function = _symbolBinding->GetIRValue(builder, context, module); // module->getFunction(_symbolBinding->GetFullyQualifiedName());
		if (function == nullptr)
			throw UnexpectedException();
		std::vector<llvm::Value*> args;

		if (_functionTypeInfo->IsMethod())
		{
			// Push "this" ptr on first
			auto val = _varBinding->GetIRValue(builder, context, module);
			args.push_back(val);
		}

		auto exprList = _expression ? std::dynamic_pointer_cast<ExpressionList>(_expression) : nullptr;
		if (exprList != nullptr)
		{
			auto argList = std::dynamic_pointer_cast<CompositeTypeInfo>(_functionTypeInfo->_inputArgs);
			while (exprList != nullptr)
			{
				auto val = exprList->_left->CodeGen(builder, context, module, argList->_thisType);
				args.push_back(val);

				// Check if we just codegen'd a function call. If so, we need to add any extra output parameters
				// to the arglist
				auto asFunctionCall = std::dynamic_pointer_cast<FunctionCall>(exprList->_left);
				if (asFunctionCall != nullptr)
				{
					for (auto& outputVal : asFunctionCall->_outputValues)
					{
						args.push_back(builder->CreateLoad(outputVal));
					}
				}

				if (exprList->_right != nullptr)
				{
					auto right = exprList->_right;
					exprList = std::dynamic_pointer_cast<ExpressionList>(right);
					if (exprList == nullptr)
					{
						auto argsValue = right->CodeGen(builder, context, module, argList->_next->_thisType);
						args.push_back(argsValue);

						// once again, check if we just codegen'd a function call
						auto rightAsFunctionCall = std::dynamic_pointer_cast<FunctionCall>(right);
						if (rightAsFunctionCall != nullptr)
						{
							for (auto& outputVal : rightAsFunctionCall->_outputValues)
							{
								args.push_back(builder->CreateLoad(outputVal));
							}
						}
					}
				}
				argList = argList->_next;
			}
		}
		else if (_expression != nullptr)
		{
			auto argsValue = _expression->CodeGen(builder, context, module, _functionTypeInfo->_inputArgs);
			args.push_back(argsValue);
			auto asFunctionCall = std::dynamic_pointer_cast<FunctionCall>(_expression);
			if (asFunctionCall != nullptr)
			{
				// Get the other output args, pass as input params
				for (auto& outputVal : asFunctionCall->_outputValues)
				{
					args.push_back(builder->CreateLoad(outputVal));
				}
			}
		}

		// Are there multiple return types? If so, we need to add the extras as "out" parameters
		auto outputAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(_functionTypeInfo->OutputArgsType());
		if (outputAsComposite != nullptr)
			outputAsComposite = outputAsComposite->_next;
		std::vector<llvm::AllocaInst*> outputVals;
		while (outputAsComposite != nullptr && outputAsComposite->_thisType != nullptr)
		{
			auto tempVal = outputAsComposite->_thisType->CreateAllocation(outputAsComposite->_name, builder, context);
			outputVals.push_back(tempVal);
			args.push_back(tempVal);
			outputAsComposite = outputAsComposite->_next;
		}
		auto retVal = builder->CreateCall(function, args);

		// After call, cast any output vals that need implicit casting
		// Do our own casting on the output values here
		outputAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(_functionTypeInfo->OutputArgsType());
		if (outputAsComposite != nullptr)
			outputAsComposite = outputAsComposite->_next;
		auto expectedType = hint != nullptr && hint->IsComposite() ? std::dynamic_pointer_cast<CompositeTypeInfo>(hint)->_next : nullptr;
		for(auto& outputVal : outputVals)
		{
			auto currentExpectedType = expectedType != nullptr && expectedType->IsComposite() ? std::dynamic_pointer_cast<CompositeTypeInfo>(expectedType)->_thisType : expectedType;
			auto tempVal = outputVal;
			if (currentExpectedType != nullptr && !currentExpectedType->IsAutoType() && !outputAsComposite->_thisType->IsSameType(currentExpectedType))
			{
				auto loadInstr = builder->CreateLoad(tempVal);
				auto castGuy = builder->CreateCast(static_cast<llvm::Instruction::CastOps>(outputAsComposite->_thisType->CreateCast(currentExpectedType)), loadInstr, currentExpectedType->GetIRType(context));
				tempVal = currentExpectedType->CreateAllocation("", builder, context);
				builder->CreateStore(castGuy, tempVal);
			}
			_outputValues.push_back(tempVal);
			outputAsComposite = outputAsComposite->_next;
		}
		return retVal;
	}

	llvm::Value* DebugPrintStatement::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto value = _expression->CodeGen(builder, context, module);
		std::vector<llvm::Value*> args;

		if (value->getType()->isIntegerTy())
		{
			// Create a string on the stack, and then print it
			std::wstring specifier;
			bool isSigned = false;

			auto asInteger = std::dynamic_pointer_cast<IntegerTypeInfo>(_expressionTypeInfo);
			if (asInteger == nullptr)
			{
				auto exprAsConstant = std::dynamic_pointer_cast<IntegerConstant>(_expression);
				if (!exprAsConstant)
					throw UnexpectedException();
				asInteger = std::dynamic_pointer_cast<IntegerTypeInfo>(exprAsConstant->BestFitTypeInfo());
			}
			isSigned = asInteger->Signed();

			switch (value->getType()->getIntegerBitWidth())
			{
				case 8:
					if (_expressionTypeInfo == CharByteTypeInfo::Get())
						specifier = L"%c";
					else if (isSigned)
						specifier = L"%hhd";
					else
						specifier = L"%hhu";
					break;
				case 16:
					if (_expressionTypeInfo == CharTypeInfo::Get())
						specifier = L"%lc";
					else if (isSigned)
						specifier = L"%hd";
					else
						specifier = L"%hu";
					break;
				case 32:
					if (isSigned)
						specifier = L"%d";
					else
						specifier = L"%u";
					break;
				case 64:
					if (isSigned)
						specifier = L"%lld";
					else
						specifier = L"%llu";
					break;
				default:
					throw UnexpectedException();
			}
			llvm::SmallVector<uint16_t, 8> ElementVals;
			ElementVals.append(specifier.begin(), specifier.end());
			ElementVals.push_back(0);
			auto strConst = llvm::ConstantDataArray::get(*context, ElementVals);
			auto alloc = builder->CreateAlloca(strConst->getType(), builder->getInt32(llvm::dyn_cast<llvm::ConstantDataSequential>(strConst)->getNumElements()));
			builder->CreateStore(strConst, alloc);
			std::vector<llvm::Value*> index_vector;
			index_vector.push_back(builder->getInt32(0));
			index_vector.push_back(builder->getInt32(0));
			auto valueAsPtr = builder->CreateGEP(alloc, index_vector);

			// printf(specifier, val)
			args.push_back(valueAsPtr);
			args.push_back(value);
		}
		else if (value->getType()->isFloatingPointTy())
		{
			// Create a string on the stack, and then print it
			std::wstring specifier = L"%g";
			llvm::SmallVector<uint16_t, 8> ElementVals;
			ElementVals.append(specifier.begin(), specifier.end());
			ElementVals.push_back(0);
			auto strConst = llvm::ConstantDataArray::get(*context, ElementVals);
			auto alloc = builder->CreateAlloca(strConst->getType(), builder->getInt32(llvm::dyn_cast<llvm::ConstantDataSequential>(strConst)->getNumElements()));
			builder->CreateStore(strConst, alloc);
			std::vector<llvm::Value*> index_vector;
			index_vector.push_back(builder->getInt32(0));
			index_vector.push_back(builder->getInt32(0));
			auto valueAsPtr = builder->CreateGEP(alloc, index_vector);

			// printf("%g", val)
			args.push_back(valueAsPtr);
			if (value->getType()->isFloatTy())
				value = builder->CreateCast(llvm::Instruction::CastOps::FPExt, value, llvm::Type::getDoubleTy(*context));
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

	llvm::Value* Reference::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto typeInfo = _symbolBinding->GetTypeInfo();
		if (typeInfo->IsPrimitiveType())
		{
			return builder->CreateLoad(_symbolBinding->GetIRValue(builder, context, module), _id);
		}
		else
			return _symbolBinding->GetIRValue(builder, context, module);
	}

	llvm::Value* ExpressionList::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		throw UnexpectedException();
		//return _symbol->GetAllocationInstance();
	}

	/*
	 * Constants
	*/


	llvm::Value* StringConstant::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		std::wstring asWideString(_input.begin(), _input.end()); // TODO: Doesn't handle inlined unicode characters
		llvm::SmallVector<uint16_t, 8> ElementVals;
		ElementVals.append(asWideString.begin(), asWideString.end());
		ElementVals.push_back(0);
		return llvm::ConstantDataArray::get(*context, ElementVals);
	}

	llvm::Value* IntegerConstant::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		if (hint == nullptr || hint->IsConstant())
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
			auto hintAsInteger = std::dynamic_pointer_cast<IntegerTypeInfo>(hint);
			if (hintAsInteger != nullptr)
			{
				return hintAsInteger->CreateValue(context, GetRaw());
			}
			throw UnexpectedException();
		}
	}

	llvm::Value* FloatingConstant::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		if (hint != nullptr && std::dynamic_pointer_cast<Float32TypeInfo>(hint) != nullptr)
			return llvm::ConstantFP::get(*context, llvm::APFloat(AsFloat32()));

		// Just assume double
		return llvm::ConstantFP::get(*context, llvm::APFloat(AsFloat64()));
	}

	llvm::Value* BoolConstant::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		if (_value)
			return llvm::ConstantInt::getTrue(*context);
		else
			return llvm::ConstantInt::getFalse(*context);
	}

	llvm::Value* CharConstant::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = llvm::ConstantInt::get(*context, llvm::APInt(hint == CharByteTypeInfo::Get() ? 8 : 16, Value()));
		if (hint != nullptr && hint != _typeInfo && hint != CharByteTypeInfo::Get() && hint != CharTypeInfo::Get())
			return builder->CreateCast(llvm::Instruction::CastOps::ZExt, val, hint->GetIRType(context));
		return val;
	}

	/*
	 * Operations
	 */

	llvm::Value* AddOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto rhs = _rhs->CodeGen(builder, context, module, _implicitCastType);
		auto lhs = _lhs->CodeGen(builder, context, module, _implicitCastType);

		if (_resultOfOperation->IsInteger())
		{
			return builder->CreateAdd(lhs, rhs); // TODO: Think about overflow
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_resultOfOperation) != nullptr 
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr 
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_resultOfOperation) != nullptr)
		{
			// doubles (float64) and floats (float32)
			return builder->CreateFAdd(lhs, rhs);
		}
		// TODO: Non integer types that implement add
		throw UnexpectedException();
	}

	llvm::Value* SubtractOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(builder, context, module, _resultOfOperation);

		if (_resultOfOperation->IsInteger())
		{
			return builder->CreateSub(lhs, rhs); // TODO: Think about overflow, think about my rules (float + int) vs what LLVM will actually do
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_resultOfOperation) != nullptr 
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr 
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_resultOfOperation) != nullptr)
		{
			// doubles (float64) and floats
			return builder->CreateFSub(lhs, rhs);
		}
		// TODO: Non integer types that implement subtract
		throw UnexpectedException();
	}

	llvm::Value* MultiplyOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(builder, context, module, _resultOfOperation);

		if (_resultOfOperation->IsInteger())
		{
			return builder->CreateMul(lhs, rhs); // TODO: Think about overflow, think about my rules (float + int) vs what LLVM will actually do
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_resultOfOperation) != nullptr)
		{
			// doubles (float64) and floats
			return builder->CreateFMul(lhs, rhs);
		}
		// TODO: Non integer types that implement multiply
		throw UnexpectedException();
	}

	llvm::Value* DivideOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(builder, context, module, _resultOfOperation);

		if (_resultOfOperation->IsInteger())
		{
			if (_resultIsSigned)
				return builder->CreateSDiv(lhs, rhs); // TODO: Think about overflow, think about my rules (float + int) vs what LLVM will actually do
			else
				return builder->CreateUDiv(lhs, rhs);
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_resultOfOperation) != nullptr)
		{
			// doubles (float64) and floats
			return builder->CreateFDiv(lhs, rhs);
		}
		// TODO: Non integer types that implement divide
		throw UnexpectedException();
	}

	llvm::Value* RemainderOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(builder, context, module, _resultOfOperation);

		if (_resultOfOperation->IsInteger())
		{
			if (_resultIsSigned)
				return builder->CreateSRem(lhs, rhs); // TODO: Think about overflow, think about my rules (float + int) vs what LLVM will actually do
			else
				return builder->CreateURem(lhs, rhs);
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_resultOfOperation) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_resultOfOperation) != nullptr)
		{
			// doubles (float64) and floats
			return builder->CreateFRem(lhs, rhs);
		}
		// TODO: Non integer types that implement divide
		throw UnexpectedException();
	}

	llvm::Value* GreaterThanOrEqualOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(builder, context, module, _implicitCastType);

		if (_implicitCastType->IsInteger())
		{
			if (_resultIsSigned)
				return builder->CreateICmpSGE(lhs, rhs); // TODO: Think about overflow, think about IntegerConstantType being big enough to be unsigned
			else
				return builder->CreateICmpUGE(lhs, rhs);
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr)
		{
			// doubles (float64) and floats
			return builder->CreateFCmpOGE(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GTE
		throw UnexpectedException();
	}

	llvm::Value* LessThanOrEqualOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(builder, context, module, _implicitCastType);

		if (_implicitCastType->IsInteger())
		{
			if (_resultIsSigned)
				return builder->CreateICmpSLE(lhs, rhs); // TODO: Think about overflow, think about IntegerConstantType being big enough to be unsigned
			else
				return builder->CreateICmpULE(lhs, rhs);
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr)
		{
			// doubles (float64) and floats
			return builder->CreateFCmpOGE(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GTE
		throw UnexpectedException();
	}

	llvm::Value* GreaterThanOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(builder, context, module, _implicitCastType);

		if (_implicitCastType->IsInteger())
		{
			if (_resultIsSigned)
				return builder->CreateICmpSGT(lhs, rhs); // TODO: Think about overflow, think about IntegerConstantType being big enough to be unsigned
			else
				return builder->CreateICmpUGT(lhs, rhs);
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr)
		{
			// doubles (float64) and floats
			return builder->CreateFCmpOGT(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GT
		throw UnexpectedException();
	}

	llvm::Value* LessThanOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(builder, context, module, _implicitCastType);

		if (_implicitCastType->IsInteger())
		{
			if (_resultIsSigned)
				return builder->CreateICmpSLT(lhs, rhs); // TODO: Think about overflow, think about IntegerConstantType being big enough to be unsigned
			else
				return builder->CreateICmpULT(lhs, rhs);
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr)
		{
			// doubles (float64) and floats
			return builder->CreateFCmpOLT(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GT
		throw UnexpectedException();
	}

	llvm::Value* EqualToOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(builder, context, module, _implicitCastType);

		if (_implicitCastType->IsInteger())
		{
			return builder->CreateICmpEQ(lhs, rhs);
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr)
		{
			// doubles (float64) and floats
			return builder->CreateFCmpOEQ(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GT
		throw UnexpectedException();
	}

	llvm::Value* NotEqualToOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(builder, context, module, _implicitCastType);

		if (_implicitCastType->IsInteger())
		{
			return builder->CreateICmpNE(lhs, rhs);
		}
		else if (std::dynamic_pointer_cast<FloatingConstantType>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float64TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr
			|| std::dynamic_pointer_cast<Float32TypeInfo>(_implicitCastType) != nullptr)
		{
			// doubles (float64) and floats
			return builder->CreateFCmpONE(lhs, rhs); // TODO: Difference between ordered, unordered?
		}
		// TODO: Non integer types that implement GT
		throw UnexpectedException();
	}

	// A single static constant representing "1" for all the increment/decrement operators
	static std::shared_ptr<IntegerConstant> s_one = std::make_shared<IntegerConstant>("1", FileLocation(-1, -1));
	static std::shared_ptr<FloatingConstant> s_oneFloat = std::make_shared<FloatingConstant>("1.0", FileLocation(-1, -1));

	llvm::Value* PostIncrementOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto type = hint ? hint : _typeInfo;
		auto val = _expr->CodeGen(builder, context, module, type);
		// Now increment it by one
		llvm::Value* result;
		if (val->getType()->isIntegerTy())
		{
			result = builder->CreateAdd(val, s_one->CodeGen(builder, context, module, type));
		}
		else if (val->getType()->isFloatingPointTy())
		{
			result = builder->CreateFAdd(val, s_oneFloat->CodeGen(builder, context, module, type));
		}
		else
		{
			throw UnexpectedException();
		}
		// Now store the result.
		auto exprVal = _expr->SymbolBinding()->GetIRValue(builder, context, module);
		builder->CreateStore(result, exprVal);
		return val; // Return the result before the increment
	}

	llvm::Value* PostDecrementOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expr->CodeGen(builder, context, module, hint ? hint : _typeInfo);
		// Now decrement it by one
		llvm::Value* result;
		if (val->getType()->isIntegerTy())
		{
			result = builder->CreateSub(val, s_one->CodeGen(builder, context, module, hint ? hint : _typeInfo));
		}
		else if (val->getType()->isFloatingPointTy())
		{
			result = builder->CreateFSub(val, s_oneFloat->CodeGen(builder, context, module, hint ? hint : _typeInfo));
		}
		else
		{
			throw UnexpectedException();
		}
		// Now store the result.
		auto exprVal = _expr->SymbolBinding()->GetIRValue(builder, context, module);
		builder->CreateStore(result, exprVal);
		return val; // Return the result before the decrement
	}

	llvm::Value* PreIncrementOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expr->CodeGen(builder, context, module, hint ? hint : _typeInfo);
		// Now increment it by one
		llvm::Value* result;
		if (val->getType()->isIntegerTy())
		{
			result = builder->CreateAdd(val, s_one->CodeGen(builder, context, module, hint ? hint : _typeInfo));
		}
		else if (val->getType()->isFloatingPointTy())
		{
			result = builder->CreateFAdd(val, s_oneFloat->CodeGen(builder, context, module, hint ? hint : _typeInfo));
		}
		else
		{
			throw UnexpectedException();
		}
		// Now store the result.
		auto exprVal = _expr->SymbolBinding()->GetIRValue(builder, context, module);
		builder->CreateStore(result, exprVal);
		return result; // Return the result after the increment
	}

	llvm::Value* PreDecrementOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expr->CodeGen(builder, context, module, hint ? hint : _typeInfo);
		// Now decrement it by one
		llvm::Value* result;
		if (val->getType()->isIntegerTy())
		{
			result = builder->CreateSub(val, s_one->CodeGen(builder, context, module, hint ? hint : _typeInfo));
		}
		else if (val->getType()->isFloatingPointTy())
		{
			result = builder->CreateFSub(val, s_oneFloat->CodeGen(builder, context, module, hint ? hint : _typeInfo));
		}
		else
		{
			throw UnexpectedException();
		}
		// Now store the result.
		auto exprVal = _expr->SymbolBinding()->GetIRValue(builder, context, module);
		builder->CreateStore(result, exprVal);
		return result; // Return the result after the decrement
	}

	llvm::Value* NegateOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val =_expr->CodeGen(builder, context, module, BoolTypeInfo::Get());
		// Now return the negation of that
		return builder->CreateNot(val);
	}

	llvm::Value* LogicalAndOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, BoolTypeInfo::Get());
		auto rhs = _rhs->CodeGen(builder, context, module, BoolTypeInfo::Get());

		return builder->CreateAnd(lhs, rhs);
	}

	llvm::Value* LogicalOrOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, BoolTypeInfo::Get());
		auto rhs = _rhs->CodeGen(builder, context, module, BoolTypeInfo::Get());

		return builder->CreateOr(lhs, rhs);
	}

	llvm::Value* BitwiseAndOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, hint ? hint : _resultOfOperation);
		auto rhs = _rhs->CodeGen(builder, context, module, hint ? hint : _resultOfOperation);

		if (_resultOfOperation->IsInteger())
		{
			return builder->CreateAnd(lhs, rhs);
		}
		// TODO: Non integer types that implement &
		throw UnexpectedException();
	}

	llvm::Value* BitwiseOrOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, hint ? hint : _resultOfOperation);
		auto rhs = _rhs->CodeGen(builder, context, module, hint ? hint : _resultOfOperation);

		if (_resultOfOperation->IsInteger())
		{
			return builder->CreateOr(lhs, rhs);
		}
		// TODO: Non integer types that implement |
		throw UnexpectedException();
	}

	llvm::Value* BitwiseXorOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, hint ? hint : _resultOfOperation);
		auto rhs = _rhs->CodeGen(builder, context, module, hint ? hint : _resultOfOperation);

		if (_resultOfOperation->IsInteger())
		{
			return builder->CreateXor(lhs, rhs);
		}
		// TODO: Non integer types that implement ^
		throw UnexpectedException();
	}

	llvm::Value* BitwiseShiftLeftOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, hint ? hint : _resultOfOperation);
		auto rhs = _rhs->CodeGen(builder, context, module, hint ? hint : _resultOfOperation);

		if (_lhsTypeInfo->IsInteger())
		{
			return builder->CreateShl(lhs, rhs); // Only has logical shl, why?
		}
		// TODO: Non integer types that implement <<
		throw UnexpectedException();
	}

	llvm::Value* BitwiseShiftRightOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(builder, context, module, hint ? hint : _resultOfOperation);
		auto rhs = _rhs->CodeGen(builder, context, module, hint ? hint : _resultOfOperation);

		if (_lhsTypeInfo)
		{
			return builder->CreateLShr(lhs, rhs); // TODO: Pick logical or arith shift
		}
		// TODO: Non integer types that implement >>
		throw UnexpectedException();
	}

	llvm::Value* ComplementOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expr->CodeGen(builder, context, module, hint ? hint : _typeInfo);
		if (val->getType()->isIntegerTy())
		{
			// XOR with 0xFFFF....
			switch (val->getType()->getIntegerBitWidth())
			{
				case 8:
					return builder->CreateXor(val, 0xFF);
					break;
				case 16:
					return builder->CreateXor(val, 0xFFFF);
					break;
				case 32:
					return builder->CreateXor(val, 0xFFFFFFFF);
					break;
				case 64:
					return builder->CreateXor(val, 0xFFFFFFFFFFFFFFFF);
					break;
				default:
					throw UnexpectedException();
			}
		}
		// TODO: Non integer types that implement ~
		throw UnexpectedException();
	}

	llvm::Value* CastOperation::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expression->CodeGen(builder, context, module);
		return builder->CreateCast((llvm::Instruction::CastOps)_castFrom->CreateCast(_castTo), val, _castTo->GetIRType(context));
	}

	llvm::Value* Expression::CodeGen(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		FileLocationContext locationContext(_location);
		auto val = CodeGenInternal(builder, context, module, hint);
		if (hint != nullptr && !hint->IsConstant())
		{
			if (hint->IsComposite())
			{
				// Composites are a special case. They'll do the casting themselves for all but
				// the first type
				if (!_typeInfo->IsComposite())
					throw UnexpectedException(); // This should have been cleared up by now

				auto hintType = std::dynamic_pointer_cast<CompositeTypeInfo>(hint)->_thisType;
				auto actualType = std::dynamic_pointer_cast<CompositeTypeInfo>(_typeInfo)->_thisType;
				if (!hintType->IsAutoType() && hintType->GetIRType(context) != val->getType())
				{
					return builder->CreateCast(static_cast<llvm::Instruction::CastOps>(actualType->CreateCast(hintType)), val, hintType->GetIRType(context));
				}
			}
			// Try to implicitly cast it
			else if (!hint->IsClassType() && hint->GetIRType(context) != val->getType())
			{
				return builder->CreateCast(static_cast<llvm::Instruction::CastOps>(_typeInfo->CreateCast(hint)), val, hint->GetIRType(context));
			}
		}
		return val;
	}

	llvm::Value* StackConstructionExpression::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		llvm::Value* retVal = nullptr;
		// Only allocate local variables. Member variables are already allocated.
		if (_varBinding->IsVariableBinding())
		{
			// First, create the struct on the stack
			auto classTypeInfo = std::make_shared<ClassTypeInfo>(_symbolBinding->GetTypeInfo(), true /*valueType*/);
			retVal = classTypeInfo->CreateAllocation(_varName, builder, context);

			// Bind the variable to this struct value
			_varBinding->BindIRValue(retVal);
		}

		// Now, run the c'tor
		_ctorCall->CodeGen(builder, context, module);

		return retVal;
	}

	llvm::AllocaInst* ClassTypeInfo::CreateAllocation(const std::string& name, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		return builder->CreateAlloca(GetIRType(context), nullptr /*arraysize*/, name);
	}

	/* Statements */

	void Ast::Statement::CodeGen(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		FileLocationContext locationContext(_location);
		CodeGenInternal(builder, context, module);
	}

	void Ast::IfStatement::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		auto ifContBlock =llvm::BasicBlock::Create(*context); // The label after the if/else statement is over
		bool ifContBlockReachable = false;
		auto func = builder->GetInsertBlock()->getParent();
		auto elseBlock = _elseStatement != nullptr ? llvm::BasicBlock::Create(*context) : nullptr;
		auto cond = _condition->CodeGen(builder, context, module, BoolTypeInfo::Get());
		auto result = builder->CreateICmpNE(cond, builder->getInt1(0));

		auto ifBlock = llvm::BasicBlock::Create(*context, "", func);

		if (elseBlock != nullptr)
		{
			builder->CreateCondBr(cond, ifBlock, elseBlock);
		}
		else
		{
			builder->CreateCondBr(cond, ifBlock, ifContBlock);
			ifContBlockReachable = true;
		}

		builder->SetInsertPoint(ifBlock);

		_statement->CodeGen(builder, context, module);

		// Run any d'tors from the if block going out of scope
		for (auto& dtor : _ifStatementEndScopeDtors)
		{
			dtor->CodeGen(builder, context, module);
		}

		// Add a branch to the continue block (after the if/else)
		// but not if there's already a terminator, ie a return stmt.
		auto insertBlock = builder->GetInsertBlock();
		if (insertBlock->empty() || !insertBlock->back().isTerminator())
		{
			builder->CreateBr(ifContBlock);
			ifContBlockReachable = true;
		}

		if (_elseStatement != nullptr)
		{
			func->getBasicBlockList().push_back(elseBlock);
			builder->SetInsertPoint(elseBlock);
			_elseStatement->CodeGen(builder, context, module);

			// Run any d'tors from the else block going out of scope
			for (auto& dtor : _elseStatementEndScopeDtors)
			{
				dtor->CodeGen(builder, context, module);
			}

			// Add a branch to the continue block (after the if/else)
			// but not if there's already a terminator, ie a return stmt.
			elseBlock = builder->GetInsertBlock();
			if (elseBlock->empty() || !elseBlock->back().isTerminator())
			{
				builder->CreateBr(ifContBlock);
				ifContBlockReachable = true;
			}
		}

		if (ifContBlockReachable)
		{
			func->getBasicBlockList().push_back(ifContBlock);
			builder->SetInsertPoint(ifContBlock);
		}
	}

	void Ast::WhileStatement::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		auto func = builder->GetInsertBlock()->getParent();
		auto conditionBlock = llvm::BasicBlock::Create(*context, "", func);

		builder->CreateBr(conditionBlock);
		builder->SetInsertPoint(conditionBlock);
		auto loopBlock = llvm::BasicBlock::Create(*context);
		builder->CreateCondBr(_condition->CodeGen(builder, context, module, BoolTypeInfo::Get()), loopBlock, _currentLoopBinding->GetEndOfScopeBlock(context));
		func->getBasicBlockList().push_back(loopBlock);
		builder->SetInsertPoint(loopBlock);

		_statement->CodeGen(builder, context, module);

		builder->CreateBr(conditionBlock);

		auto fallthrough = _currentLoopBinding->GetEndOfScopeBlock(context);
		func->getBasicBlockList().push_back(fallthrough);
		builder->SetInsertPoint(fallthrough);

		// Run any d'tors from the while block going out of scope
		for (auto& dtor : _endScopeDtors)
		{
			dtor->CodeGen(builder, context, module);
		}
	}

	void Ast::BreakStatement::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		// First run any d'tors from the while block going out of scope
		for (auto& dtor : _endScopeDtors)
		{
			dtor->CodeGen(builder, context, module);
		}
		builder->CreateBr(_currentLoopBinding->GetEndOfScopeBlock(context));
	}

	void Ast::Assignment::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		_lhs->CodeGen(_rhs, builder, context, module);
	}

	void Ast::ScopedStatements::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		if (_statements != nullptr)
		{
			_statements->CodeGen(builder, context, module);

			// Run any d'tors from the while block going out of scope
			for (auto& dtor : _endScopeDtors)
			{
				dtor->CodeGen(builder, context, module);
			}
		}
	}

	void Ast::ExpressionAsStatement::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		_expr->CodeGen(builder, context, module);
	}

	void ReturnStatement::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		auto functionTypeInfo = std::dynamic_pointer_cast<FunctionTypeInfo>(_functionBinding->GetTypeInfo());
		auto outputTypeAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(functionTypeInfo->OutputArgsType());
		if (outputTypeAsComposite != nullptr)
		{
			// Return the 1st parameter, treat the rest as out parameters
			auto exprList = std::dynamic_pointer_cast<ExpressionList>(_idList);

			auto function = (llvm::Function*)_functionBinding->GetIRValue(builder, context, module);

			// Get the number of input arguments
			int inputArgCount = 0;
			if (functionTypeInfo->InputArgsType() != nullptr)
			{
				auto inputArgsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(functionTypeInfo->InputArgsType());
				if (inputArgsComposite == nullptr)
				{
					inputArgCount++;
				}
				else
				{
					auto inputArg = inputArgsComposite;
					while (inputArg != nullptr && inputArgsComposite->_thisType != nullptr)
					{
						inputArgCount++;
						inputArg = inputArg->_next;
					}
				}
			}
			auto currExprList = exprList;
			auto currOutputType = outputTypeAsComposite;
			if (functionTypeInfo->IsMethod())
				inputArgCount++;// Methods have "this" as an invisible 1st arg.
			int i = 0;
			for (auto &arg : function->args())
			{
				if (i++ < inputArgCount)
					continue;

				// Now we've gotten to output args
				// TODO: Passing fun with same output type???
				currOutputType = currOutputType->_next;
				auto currExpr = currExprList->_right;
				if (currExpr != nullptr)
				{
					currExprList = std::dynamic_pointer_cast<ExpressionList>(currExpr);
					if (currExprList == nullptr)
					{
						// Just finish up last parameter
						CreateAssignment(currOutputType->_thisType, &arg/*ptr (deref)*/, currExpr->CodeGen(builder, context, module, currOutputType->_thisType), builder, context, module);
						break;
					}
					else
					{
						CreateAssignment(currOutputType->_thisType, &arg, currExprList->_left->CodeGen(builder, context, module, currOutputType->_thisType), builder, context, module);
					}
				}
				else
				{
					throw UnexpectedException();
				}
			}

			// Ret has to go last
			llvm::Value* val;
			std::shared_ptr<TypeInfo> type;
			
			// TODO: Will this work for return functions with same type?
			if (exprList)
			{
				val = exprList->_left->CodeGen(builder, context, module, outputTypeAsComposite->_thisType);
				type = outputTypeAsComposite->_thisType;
			}
			else
			{
				val = _idList->CodeGen(builder, context, module, outputTypeAsComposite);
				type = outputTypeAsComposite;
			}
			val = GetValueToReturn(val, type, builder, context, module);

			// Before returning, destroy locally allocated variables
			for (auto& dtor : _endScopeDtors)
			{
				dtor->CodeGen(builder, context, module);
			}

			builder->CreateRet(val); 
		}
		else
		{
			auto val = _idList->CodeGen(builder, context, module, _returnType);
			val = GetValueToReturn(val, _idList->_typeInfo, builder, context, module);

			// Before returning, destroy locally allocated variables
			for (auto& dtor : _endScopeDtors)
			{
				dtor->CodeGen(builder, context, module);
			}

			builder->CreateRet(val);
		}
	}

	llvm::Value * Ast::ReturnStatement::GetValueToReturn(llvm::Value * value, std::shared_ptr<Ast::TypeInfo> typeInfo, llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		if (typeInfo->IsClassType() && std::dynamic_pointer_cast<BaseClassTypeInfo>(typeInfo)->IsValueType())
		{
			return builder->CreateLoad(value);
		}
		return value;
	}

	void Ast::ClassMemberDeclaration::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
	}

	void Ast::Initializer::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		if (_stackAssignment != nullptr)
			_stackAssignment->CodeGen(builder, context, module);
	}

	void Ast::InitializerStatement::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		auto list = _list;
		while (list != nullptr)
		{
			list->_thisInitializer->CodeGen(builder, context, module);
			list = list->_next;
		}
	}
	
	void Ast::FunctionDeclaration::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		llvm::FunctionType* ft = nullptr;
		llvm::Function* function = nullptr;
		CodeGenEnter(builder, context, module, &ft, &function);

		// Codegen body
		if (_body != nullptr)
			_body->CodeGen(builder, context, module);

		CodeGenLeave(builder, context, module, ft, function);
	}

	void Ast::FunctionDeclaration::CodeGenEnter(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module, llvm::FunctionType ** ft, llvm::Function ** function)
	{
		// Parameter type
		std::vector<llvm::Type*> argTypes;
		bool isMethod = !_mods->IsStatic();
		llvm::Type* ptrToThisType = nullptr;
		if (isMethod)
		{
			// For non-static methods, add a pointer to "this" as the 1st argument
			//auto argument = std::make_shared<Argument>(_classBinding->GetTypeInfo(), "this");
			//_inputArgs = std::make_shared<ArgumentList>(argument, _inputArgs);
			auto thisType = _classBinding->GetTypeInfo()->GetIRType(context);
			ptrToThisType = llvm::PointerType::get(thisType, 0);
			argTypes.push_back(ptrToThisType);
		}
		if (_inputArgs != nullptr)
			_inputArgs->AddIRTypesToVector(argTypes, context);

		// Return type
		llvm::Type* retType = llvm::Type::getVoidTy(*context);
		if (_returnArgs != nullptr && _returnArgs->_argument != nullptr)
		{
			retType = _returnArgs->_argument->_typeInfo->GetIRType(context);
		}
		if (_returnArgs != nullptr && _returnArgs->_next != nullptr)
		{
			// LLVM doesn't support multiple return types, so we'll treat them as out
			// paramaters
			_returnArgs->_next->AddIRTypesToVector(argTypes, context, true /*asOutput*/);
		}

		*ft = llvm::FunctionType::get(retType, argTypes, false /*isVarArg*/);
		auto name = _functionBinding->GetFullyQualifiedName();
		if (name.compare("Program.main") == 0)
			name = "main";
		*function = llvm::Function::Create(*ft, llvm::Function::ExternalLinkage /*TODO*/, name, module);
		_functionBinding->BindIRValue(*function);
		auto bb = llvm::BasicBlock::Create(*context, "", *function);
		builder->SetInsertPoint(bb);

		// Now store the args
		auto currArg = _inputArgs;
		int i = 0;
		for (auto &arg : (*function)->args())
		{
			if (i == 0 && isMethod)
				arg.setName("this");
			else
				arg.setName(currArg->_argument->_name);

			if (i >= _argBindings.size() + (isMethod ? 1 : 0))
			{
				// We've reached output args, we don't need to bind them.
				currArg = currArg->_next;
				i++;
				continue;
			}

			// Create bindings
			if (i == 0 && isMethod)
			{
				// Since "this" isn't a pointer that can be reassigned, there's no point in allocating a new pointer and storing it.
				// We'll just always refer to the current pointer
				_thisPtrBinding->BindIRValue(&arg);
			}
			else
			{
				auto binding = _argBindings[i - (isMethod ? 1 : 0)];
				// Create an allocation for the arg
				auto alloc = binding->CreateAllocationInstance(currArg->_argument->_name, builder, context);
				CreateAssignment(binding->GetTypeInfo(), alloc, &arg, builder, context, module);
				binding->BindIRValue(alloc);
				currArg = currArg->_next;
				if (currArg == nullptr && _returnArgs != nullptr && _returnArgs->_next != nullptr)
					currArg = _returnArgs->_next;
			}
			i++;
		}
	}

	void Ast::FunctionDeclaration::CodeGenLeave(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module, llvm::FunctionType * ft, llvm::Function * function)
	{
		if (_returnArgs == nullptr || _returnArgs->_argument == nullptr)
		{
			// Destroy anything that ran out of scope
			for (auto& dtor : _endScopeDtors)
			{
				dtor->CodeGen(builder, context, module);
			}

			builder->CreateRetVoid(); // What if user added return statement to the end of the block already?
		}

		llvm::verifyFunction(*function);
	}

	void Ast::ConstructorDeclaration::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		llvm::FunctionType* ft = nullptr;
		llvm::Function* function = nullptr;
		CodeGenEnter(builder, context, module, &ft, &function);

		// Once we enter the c'tor scope, the initializers are the first thing to get done
		if (_initializerStatement != nullptr)
			_initializerStatement->CodeGen(builder, context, module);

		// Are there any uninitialized value-types? Go ahead and initialize them, if we can
		for (auto& memberBinding : _classBinding->_members)
		{
			memberBinding = std::make_shared<Ast::SymbolTable::MemberInstanceBinding>(memberBinding, _thisPtrBinding); // Create a binding capable of referring to this ptr
			auto memberType = memberBinding->GetTypeInfo();
			if (memberType->IsClassType())
			{
				auto asClassType = std::dynamic_pointer_cast<BaseClassTypeInfo>(memberType);
				if (asClassType == nullptr)
				{
					throw UnexpectedException();
				}
				if (asClassType->IsValueType() &&
					std::dynamic_pointer_cast<Ast::SymbolTable::ConstructorBinding>(_functionBinding)->_initializers.count(memberBinding->_memberDeclaration->_name) == 0)
				{
					// Try to find a default c'tor and add it
					auto initer = std::make_shared<Initializer>(memberBinding->_memberDeclaration->_name, nullptr /*no args*/, FileLocationContext::CurrentLocation());
					try
					{
						initer->CodeGen(builder, context, module);
					}
					catch (NoMatchingFunctionSignatureFoundException&)
					{
						throw ValueTypeMustBeInitializedException(memberBinding->_memberDeclaration->_name);
					}
				}
				else if (!asClassType->IsValueType())
				{
					// Assign to nullptr
					builder->CreateStore(asClassType->GetDefaultValue(context), memberBinding->GetIRValue(builder, context, module));
				}
			}
			else if (memberType->IsPrimitiveType())
			{
				// Give local members the default value
				builder->CreateStore(memberBinding->GetTypeInfo()->GetDefaultValue(context), memberBinding->GetIRValue(builder, context, module));
			}
		}

		// Now do the body of the c'tor
		if (_body != nullptr)
			_body->CodeGen(builder, context, module);

		CodeGenLeave(builder, context, module, ft, function);
	}

	void Ast::ClassStatementList::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		_statement->CodeGen(builder, context, module);
		if (_next != nullptr)
			_next->CodeGen(builder, context, module);
	}

	void Ast::GlobalStatements::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		_stmt->CodeGen(builder, context, module);
		if (_next != nullptr)
			_next->CodeGen(builder, context, module);
	}

	void Ast::NamespaceDeclaration::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		_stmts->CodeGen(builder, context, module);
	}

	void Ast::ClassDeclaration::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		// Look through members first, and create a struct type
		std::vector<llvm::Type*> memberTypes;
		for (auto& member : _classBinding->_members)
		{
			auto val = member->GetTypeInfo()->GetIRType(context);
			memberTypes.push_back(val);
		}
		auto type = llvm::StructType::create(*context, memberTypes, _name, true /*isPacked*/);
		_classBinding->BindType(type);

		// Now codegen the rest (basically just methods since members don't need to do anything)
		if (_list != nullptr)
			_list->CodeGen(builder, context, module);
	}

	void Ast::LineStatements::CodeGenInternal(llvm::IRBuilder<>* builder, llvm::LLVMContext * context, llvm::Module * module)
	{
		_statement->CodeGen(builder, context, module);
		if (_next != nullptr)
			_next->CodeGen(builder, context, module);
	}
}