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

	llvm::Value* DeclareVariable::GetIRValue(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		FileLocationContext locationContext(_location);
		return symbolTable->Lookup(_name)->CreateAllocationInstance(_name, builder, context);
	}

	llvm::Value* AssignFromReference::GetIRValue(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context)
	{
		FileLocationContext locationContext(_location);
		return symbolTable->Lookup(_ref)->GetIRValue();
	}

	void AssignFrom::CodeGen(std::shared_ptr<Expression> rhs, std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module)
	{
		FileLocationContext locationContext(_location);
		if (_next != nullptr)
		{
			// The rhs must be an expression list or function to support this
			auto rhsAsList = std::dynamic_pointer_cast<ExpressionList>(rhs);
			auto rhsAsFunction = std::dynamic_pointer_cast<FunctionCall>(rhs);
			if (rhsAsList != nullptr)
			{
				auto rhValue = rhsAsList->_left->CodeGen(symbolTable, builder, context, module, _thisType);
				auto alloc = _thisOne->GetIRValue(symbolTable, builder, context);
				builder->CreateStore(rhValue, alloc);
				_next->CodeGen(rhsAsList->_right, symbolTable, builder, context, module);
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
				auto functionCall = (llvm::CallInst*)rhsAsFunction->CodeGen(symbolTable, builder, context, module, compTypeExpected);

				// Store each output value in a value from the lhs
				auto lhsVar = _next;
				std::shared_ptr<TypeInfo> outputArgTypeInfo = std::dynamic_pointer_cast<CompositeTypeInfo>(rhsAsFunction->_typeInfo);
				for (auto &outputVal : rhsAsFunction->_outputValues)
				{
					if (outputArgTypeInfo == nullptr)
						throw UnexpectedException();
					auto asComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(outputArgTypeInfo);
					auto thisType = asComposite != nullptr ? asComposite->_thisType : outputArgTypeInfo;
					auto alloc = lhsVar->_thisOne->GetIRValue(symbolTable, builder, context);
					builder->CreateStore(builder->CreateLoad(outputVal), alloc);
					lhsVar = lhsVar->_next;
					outputArgTypeInfo = asComposite != nullptr ? asComposite->_next : nullptr;
				}

				// Store the first guy that we do return
				auto alloc = _thisOne->GetIRValue(symbolTable, builder, context);
				builder->CreateStore(functionCall, alloc);
			}
			else
			{
				throw UnexpectedException();
			}
		}
		else
		{
			auto rhValue = rhs->CodeGen(symbolTable, builder, context, module, _thisType->IsAutoType() ? nullptr : _thisType);
			auto alloc = _thisOne->GetIRValue(symbolTable, builder, context);
			builder->CreateStore(rhValue, alloc);
		}
	}

	void ArgumentList::AddIRTypesToVector(std::vector<llvm::Type*>& inputVector, llvm::LLVMContext* context, bool asOutput)
	{
		if (_argument != nullptr)
		{
			inputVector.push_back(_argument->_typeInfo->GetIRType(context, asOutput));
		}
		if (_next != nullptr)
			_next->AddIRTypesToVector(inputVector, context, asOutput);
	}

	llvm::Value* FunctionCall::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto functionBinding = symbolTable->Lookup(_name);
		auto functionType = std::dynamic_pointer_cast<FunctionTypeInfo>(functionBinding->GetTypeInfo());
		if (functionType == nullptr)
			throw UnexpectedException();
		if (_expression == nullptr)
		{
			if (functionType->InputArgsType() != nullptr)
			{
				throw TypeMismatchException(functionType->InputArgsType(), nullptr);
			}
		}
		else
		{
			auto evaluatedType = _expression->Evaluate(symbolTable);
			if (!functionType->InputArgsType()->IsImplicitlyAssignableFrom(evaluatedType, symbolTable))
			{
				throw TypeMismatchException(functionType->InputArgsType(), evaluatedType);
			}
		}
		auto function = module->getFunction(_name);
		std::vector<llvm::Value*> args;
		auto exprList = _expression ? std::dynamic_pointer_cast<ExpressionList>(_expression) : nullptr;
		if (exprList != nullptr)
		{
			auto argList = std::dynamic_pointer_cast<CompositeTypeInfo>(functionType->_inputArgs);
			while (exprList != nullptr)
			{
				auto val = exprList->_left->CodeGen(symbolTable, builder, context, module, argList->_thisType);
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
						auto argsValue = right->CodeGen(symbolTable, builder, context, module, argList->_next->_thisType);
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
			auto argsValue = _expression->CodeGen(symbolTable, builder, context, module, functionType->_inputArgs);
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
		auto outputAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(functionType->OutputArgsType());
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
		outputAsComposite = std::dynamic_pointer_cast<CompositeTypeInfo>(functionType->OutputArgsType());
		if (outputAsComposite != nullptr)
			outputAsComposite = outputAsComposite->_next;
		auto expectedType = hint != nullptr && hint->IsComposite() ? std::dynamic_pointer_cast<CompositeTypeInfo>(hint)->_next : nullptr;
		for(auto& outputVal : outputVals)
		{
			auto currentExpectedType = expectedType != nullptr && expectedType->IsComposite() ? std::dynamic_pointer_cast<CompositeTypeInfo>(expectedType)->_thisType : expectedType;
			auto tempVal = outputVal;
			if (currentExpectedType != nullptr && !currentExpectedType->IsAutoType() && outputAsComposite->_thisType != currentExpectedType)
			{
				if (!currentExpectedType->IsImplicitlyAssignableFrom(outputAsComposite->_thisType, symbolTable))
					throw UnexpectedException(); // We should have caught the type exception already
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

	llvm::Value* DebugPrintStatement::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto value = _expression->CodeGen(symbolTable, builder, context, module);
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

	llvm::Value* Reference::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto symbol = symbolTable->Lookup(_id);
		auto typeInfo = symbol->GetTypeInfo();
		if (typeInfo->IsPrimitiveType())
		{
			return builder->CreateLoad(symbol->GetIRValue(), _id);
		}
		else
			return symbol->GetIRValue();
	}

	llvm::Value* ExpressionList::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		throw UnexpectedException();
		//return _symbol->GetAllocationInstance();
	}

	/*
	 * Constants
	*/


	llvm::Value* StringConstant::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		std::wstring asWideString(_input.begin(), _input.end()); // TODO: Doesn't handle inlined unicode characters
		llvm::SmallVector<uint16_t, 8> ElementVals;
		ElementVals.append(asWideString.begin(), asWideString.end());
		ElementVals.push_back(0);
		return llvm::ConstantDataArray::get(*context, ElementVals);
	}

	llvm::Value* IntegerConstant::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
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

	llvm::Value* FloatingConstant::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		if (hint != nullptr && std::dynamic_pointer_cast<Float32TypeInfo>(hint) != nullptr)
			return llvm::ConstantFP::get(*context, llvm::APFloat(AsFloat32()));

		// Just assume double
		return llvm::ConstantFP::get(*context, llvm::APFloat(AsFloat64()));
	}

	llvm::Value* BoolConstant::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		if (_value)
			return llvm::ConstantInt::getTrue(*context);
		else
			return llvm::ConstantInt::getFalse(*context);
	}

	llvm::Value* CharConstant::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = llvm::ConstantInt::get(*context, llvm::APInt(hint == CharByteTypeInfo::Get() ? 8 : 16, Value()));
		if (hint != nullptr && hint != _typeInfo && hint != CharByteTypeInfo::Get() && hint != CharTypeInfo::Get())
			if (hint->IsImplicitlyAssignableFrom(_typeInfo, symbolTable))
				return builder->CreateCast(llvm::Instruction::CastOps::ZExt, val, hint->GetIRType(context));
			else
				throw UnexpectedException();
		return val;
	}

	/*
	 * Operations
	 */

	llvm::Value* AddOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);

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

	llvm::Value* SubtractOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);

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

	llvm::Value* MultiplyOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);

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

	llvm::Value* DivideOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);

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

	llvm::Value* RemainderOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _resultOfOperation);

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

	llvm::Value* GreaterThanOrEqualOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);

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

	llvm::Value* LessThanOrEqualOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);

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

	llvm::Value* GreaterThanOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);

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

	llvm::Value* LessThanOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);

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

	llvm::Value* EqualToOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);

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

	llvm::Value* NotEqualToOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, _implicitCastType);

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

	llvm::Value* PostIncrementOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto type = hint ? hint : _typeInfo;
		auto val = _expr->CodeGen(symbolTable, builder, context, module, type);
		// Now increment it by one
		llvm::Value* result;
		if (val->getType()->isIntegerTy())
		{
			result = builder->CreateAdd(val, s_one->CodeGen(symbolTable, builder, context, module, type));
		}
		else if (val->getType()->isFloatingPointTy())
		{
			result = builder->CreateFAdd(val, s_oneFloat->CodeGen(symbolTable, builder, context, module, type));
		}
		else
		{
			throw UnexpectedException();
		}
		// Now store the result. For now, we assume it's a reference (nothing else supports these operations)
		auto exprAsReference = std::dynamic_pointer_cast<Reference>(_expr);
		if (exprAsReference == nullptr)
			throw UnexpectedException(); // TODO: What if user makes syntax error, like 0++. What do we want to report?
		auto symbol = symbolTable->Lookup(exprAsReference->Id());
		builder->CreateStore(result, symbol->GetIRValue());
		return val; // Return the result before the increment
	}

	llvm::Value* PostDecrementOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expr->CodeGen(symbolTable, builder, context, module, hint ? hint : _typeInfo);
		// Now decrement it by one
		llvm::Value* result;
		if (val->getType()->isIntegerTy())
		{
			result = builder->CreateSub(val, s_one->CodeGen(symbolTable, builder, context, module, hint ? hint : _typeInfo));
		}
		else if (val->getType()->isFloatingPointTy())
		{
			result = builder->CreateFSub(val, s_oneFloat->CodeGen(symbolTable, builder, context, module, hint ? hint : _typeInfo));
		}
		else
		{
			throw UnexpectedException();
		}
		// Now store the result. For now, we assume it's a reference (nothing else supports these operations)
		auto exprAsReference = std::dynamic_pointer_cast<Reference>(_expr);
		if (exprAsReference == nullptr)
			throw UnexpectedException(); // TODO: What if user makes syntax error, like 0++. What do we want to report?
		auto symbol = symbolTable->Lookup(exprAsReference->Id());
		builder->CreateStore(result, symbol->GetIRValue());
		return val; // Return the result before the decrement
	}

	llvm::Value* PreIncrementOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expr->CodeGen(symbolTable, builder, context, module, hint ? hint : _typeInfo);
		// Now increment it by one
		llvm::Value* result;
		if (val->getType()->isIntegerTy())
		{
			result = builder->CreateAdd(val, s_one->CodeGen(symbolTable, builder, context, module, hint ? hint : _typeInfo));
		}
		else if (val->getType()->isFloatingPointTy())
		{
			result = builder->CreateFAdd(val, s_oneFloat->CodeGen(symbolTable, builder, context, module, hint ? hint : _typeInfo));
		}
		else
		{
			throw UnexpectedException();
		}
		// Now store the result. For now, we assume it's a reference (nothing else supports these operations)
		auto exprAsReference = std::dynamic_pointer_cast<Reference>(_expr);
		if (exprAsReference == nullptr)
			throw UnexpectedException(); // TODO: What if user makes syntax error, like 0++. What do we want to report?
		auto symbol = symbolTable->Lookup(exprAsReference->Id());
		builder->CreateStore(result, symbol->GetIRValue());
		return result; // Return the result after the increment
	}

	llvm::Value* PreDecrementOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expr->CodeGen(symbolTable, builder, context, module, hint ? hint : _typeInfo);
		// Now decrement it by one
		llvm::Value* result;
		if (val->getType()->isIntegerTy())
		{
			result = builder->CreateSub(val, s_one->CodeGen(symbolTable, builder, context, module, hint ? hint : _typeInfo));
		}
		else if (val->getType()->isFloatingPointTy())
		{
			result = builder->CreateFSub(val, s_oneFloat->CodeGen(symbolTable, builder, context, module, hint ? hint : _typeInfo));
		}
		else
		{
			throw UnexpectedException();
		}
		// Now store the result. For now, we assume it's a reference (nothing else supports these operations)
		auto exprAsReference = std::dynamic_pointer_cast<Reference>(_expr);
		if (exprAsReference == nullptr)
			throw UnexpectedException(); // TODO: What if user makes syntax error, like 0++. What do we want to report?
		auto symbol = symbolTable->Lookup(exprAsReference->Id());
		builder->CreateStore(result, symbol->GetIRValue());
		return result; // Return the result after the decrement
	}

	llvm::Value* NegateOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val =_expr->CodeGen(symbolTable, builder, context, module, BoolTypeInfo::Get());
		// Now return the negation of that
		return builder->CreateNot(val);
	}

	llvm::Value* LogicalAndOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, BoolTypeInfo::Get());
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, BoolTypeInfo::Get());

		return builder->CreateAnd(lhs, rhs);
	}

	llvm::Value* LogicalOrOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, BoolTypeInfo::Get());
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, BoolTypeInfo::Get());

		return builder->CreateOr(lhs, rhs);
	}

	llvm::Value* BitwiseAndOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, hint ? hint : _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, hint ? hint : _resultOfOperation);

		if (_resultOfOperation->IsInteger())
		{
			return builder->CreateAnd(lhs, rhs);
		}
		// TODO: Non integer types that implement &
		throw UnexpectedException();
	}

	llvm::Value* BitwiseOrOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, hint ? hint : _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, hint ? hint : _resultOfOperation);

		if (_resultOfOperation->IsInteger())
		{
			return builder->CreateOr(lhs, rhs);
		}
		// TODO: Non integer types that implement |
		throw UnexpectedException();
	}

	llvm::Value* BitwiseXorOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, hint ? hint : _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, hint ? hint : _resultOfOperation);

		if (_resultOfOperation->IsInteger())
		{
			return builder->CreateXor(lhs, rhs);
		}
		// TODO: Non integer types that implement ^
		throw UnexpectedException();
	}

	llvm::Value* BitwiseShiftLeftOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, hint ? hint : _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, hint ? hint : _resultOfOperation);

		if (_lhsTypeInfo->IsInteger())
		{
			return builder->CreateShl(lhs, rhs); // Only has logical shl, why?
		}
		// TODO: Non integer types that implement <<
		throw UnexpectedException();
	}

	llvm::Value* BitwiseShiftRightOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto lhs = _lhs->CodeGen(symbolTable, builder, context, module, hint ? hint : _resultOfOperation);
		auto rhs = _rhs->CodeGen(symbolTable, builder, context, module, hint ? hint : _resultOfOperation);

		if (_lhsTypeInfo)
		{
			return builder->CreateLShr(lhs, rhs); // TODO: Pick logical or arith shift
		}
		// TODO: Non integer types that implement >>
		throw UnexpectedException();
	}

	llvm::Value* ComplementOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expr->CodeGen(symbolTable, builder, context, module, hint ? hint : _typeInfo);
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

	llvm::Value* CastOperation::CodeGenInternal(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		auto val = _expression->CodeGen(symbolTable, builder, context, module);
		return builder->CreateCast((llvm::Instruction::CastOps)_castFrom->CreateCast(_castTo), val, _castTo->GetIRType(context));
	}

	llvm::Value* Expression::CodeGen(std::shared_ptr<SymbolTable> symbolTable, llvm::IRBuilder<>* builder, llvm::LLVMContext* context, llvm::Module * module, std::shared_ptr<TypeInfo> hint)
	{
		FileLocationContext locationContext(_location);
		auto val = CodeGenInternal(symbolTable, builder, context, module, hint);
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
			else if (hint->GetIRType(context) != val->getType())
			{
				if (!hint->IsImplicitlyAssignableFrom(_typeInfo, symbolTable))
					throw UnexpectedException(); // Somehow we're trying to implicitly cast something that doesn't implicitly cast

				return builder->CreateCast(static_cast<llvm::Instruction::CastOps>(_typeInfo->CreateCast(hint)), val, hint->GetIRType(context));
			}
		}
		return val;
	}
}