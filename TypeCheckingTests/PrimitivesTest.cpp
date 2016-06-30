#include "stdafx.h"
#include "CppUnitTest.h"
#include <Primitives.h>
#include <Operations.h>
#include <Classes.h>
#include "TypeExceptions.h"
#include "Utils.h"
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Ast;
using namespace std;
using namespace ParserTests;

namespace TypeCheckingTests
{		
	TEST_CLASS(PrimitivesTest)
	{
		TEST_CLASS_INITIALIZE(Setup)
		{
			_primitives.push_back(Int32TypeInfo::Get());
			_primitives.push_back(Int64TypeInfo::Get());
			_primitives.push_back(UInt32TypeInfo::Get());
			_primitives.push_back(UInt64TypeInfo::Get());
			_primitives.push_back(Float32TypeInfo::Get());
			_primitives.push_back(Float64TypeInfo::Get());
			_primitives.push_back(CharByteTypeInfo::Get());
			_primitives.push_back(CharTypeInfo::Get());
			_primitives.push_back(ByteTypeInfo::Get());
			_primitives.push_back(BoolTypeInfo::Get());
		}
	private:
		static vector<shared_ptr<TypeInfo>> _primitives;
	public:
		
		TEST_METHOD(IntAssignFrom)
		{
			Int32TypeInfo c;
			for (auto& pr : _primitives)
			{
				if (c.IsImplicitlyAssignableFrom(pr, nullptr))
				{
					Assert::IsTrue(
						dynamic_pointer_cast<Int32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr));
				}
				else
				{
					Assert::IsFalse(
						dynamic_pointer_cast<Int32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr));
				}
			}
		}

		TEST_METHOD(Int64AssignFrom)
		{
			Int64TypeInfo c;
			for (auto& pr : _primitives)
			{
				if (c.IsImplicitlyAssignableFrom(pr, nullptr))
				{
					Assert::IsTrue(
						dynamic_pointer_cast<Int64TypeInfo>(pr) ||
						dynamic_pointer_cast<Int32TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr));
				}
				else
				{
					Assert::IsFalse(
						dynamic_pointer_cast<Int64TypeInfo>(pr) ||
						dynamic_pointer_cast<Int32TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr));
				}
			}
		}

		TEST_METHOD(UInt32AssignFrom)
		{
			UInt32TypeInfo c;
			for (auto& pr : _primitives)
			{
				if (c.IsImplicitlyAssignableFrom(pr, nullptr))
				{
					Assert::IsTrue(
						dynamic_pointer_cast<UInt32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr));
				}
				else
				{
					Assert::IsFalse(
						dynamic_pointer_cast<UInt32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr));
				}
			}
		}

		TEST_METHOD(UInt64AssignFrom)
		{
			UInt64TypeInfo c;
			for (auto& pr : _primitives)
			{
				if (c.IsImplicitlyAssignableFrom(pr, nullptr))
				{
					Assert::IsTrue(
						dynamic_pointer_cast<UInt64TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr));
				}
				else
				{
					Assert::IsFalse(
						dynamic_pointer_cast<UInt64TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr));
				}
			}
		}

		TEST_METHOD(Float32AssignFrom)
		{
			Float32TypeInfo c;
			for (auto& pr : _primitives)
			{
				if (c.IsImplicitlyAssignableFrom(pr, nullptr))
				{
					Assert::IsTrue(
						dynamic_pointer_cast<Float32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<Int32TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt32TypeInfo>(pr) ||
						dynamic_pointer_cast<Int64TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt64TypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr));
				}
				else
				{
					Assert::IsFalse(
						dynamic_pointer_cast<Float32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<Int32TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt32TypeInfo>(pr) ||
						dynamic_pointer_cast<Int64TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt64TypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr));
				}
			}
		}

		TEST_METHOD(Float64AssignFrom)
		{
			Float64TypeInfo c;
			for (auto& pr : _primitives)
			{
				if (c.IsImplicitlyAssignableFrom(pr, nullptr))
				{
					Assert::IsTrue(
						dynamic_pointer_cast<Float64TypeInfo>(pr) ||
						dynamic_pointer_cast<Float32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<Int32TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt32TypeInfo>(pr) ||
						dynamic_pointer_cast<Int64TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt64TypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr));
				}
				else
				{
					Assert::IsFalse(
						dynamic_pointer_cast<Float64TypeInfo>(pr) ||
						dynamic_pointer_cast<Float32TypeInfo>(pr) ||
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<Int32TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt32TypeInfo>(pr) ||
						dynamic_pointer_cast<Int64TypeInfo>(pr) ||
						dynamic_pointer_cast<UInt64TypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharTypeInfo>(pr));
				}
			}
		}

		TEST_METHOD(CharAssignFrom)
		{
			CharByteTypeInfo c;
			for (auto& pr : _primitives)
			{
				if (c.IsImplicitlyAssignableFrom(pr, nullptr))
				{
					Assert::IsTrue(
						dynamic_pointer_cast<CharByteTypeInfo>(pr) != nullptr);
				}
				else
				{
					Assert::IsFalse(
						dynamic_pointer_cast<CharByteTypeInfo>(pr) != nullptr);
				}
			}
		}

		TEST_METHOD(WCharAssignFrom)
		{
			CharTypeInfo c;
			for (auto& pr : _primitives)
			{
				if (c.IsImplicitlyAssignableFrom(pr, nullptr))
				{
					Assert::IsTrue(
						dynamic_pointer_cast<CharTypeInfo>(pr) != nullptr);
				}
				else
				{
					Assert::IsFalse(
						dynamic_pointer_cast<CharTypeInfo>(pr) != nullptr);
				}
			}
		}

		TEST_METHOD(ByteAssignFrom)
		{
			ByteTypeInfo c;
			for (auto& pr : _primitives)
			{
				if (c.IsImplicitlyAssignableFrom(pr, nullptr))
				{
					Assert::IsTrue(
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr));
				}
				else
				{
					Assert::IsFalse(
						dynamic_pointer_cast<ByteTypeInfo>(pr) ||
						dynamic_pointer_cast<CharByteTypeInfo>(pr));
				}
			}
		}

		TEST_METHOD(BoolAssignFrom)
		{
			BoolTypeInfo c;
			for (auto& pr : _primitives)
			{
				if (c.IsImplicitlyAssignableFrom(pr, nullptr))
				{
					Assert::IsTrue(dynamic_pointer_cast<BoolTypeInfo>(pr) != nullptr);
				}
				else
				{
					Assert::IsFalse(dynamic_pointer_cast<BoolTypeInfo>(pr) != nullptr);
				}
			}
		}

	};

	TEST_CLASS(PrimitiveArithmeticOperationTest)
	{
		TEST_CLASS_INITIALIZE(Setup)
		{
			_arithmeticOperations.push_back(make_shared<AddOperation>(nullptr, nullptr));
			_arithmeticOperations.push_back(make_shared<SubtractOperation>(nullptr, nullptr));
			_arithmeticOperations.push_back(make_shared<MultiplyOperation>(nullptr, nullptr));
			_arithmeticOperations.push_back(make_shared<DivideOperation>(nullptr, nullptr));
			_arithmeticOperations.push_back(make_shared<RemainderOperation>(nullptr, nullptr));

			_nonBoolPrimitives.push_back(Int32TypeInfo::Get());
			_nonBoolPrimitives.push_back(Int64TypeInfo::Get());
			_nonBoolPrimitives.push_back(UInt32TypeInfo::Get());
			_nonBoolPrimitives.push_back(UInt64TypeInfo::Get());
			_nonBoolPrimitives.push_back(Float32TypeInfo::Get());
			_nonBoolPrimitives.push_back(Float64TypeInfo::Get());
			_nonBoolPrimitives.push_back(CharByteTypeInfo::Get());
			_nonBoolPrimitives.push_back(CharTypeInfo::Get());
			_nonBoolPrimitives.push_back(ByteTypeInfo::Get());
		}

	public:
		TEST_METHOD(IntAndInt64ArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Int64TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), Int64TypeInfo::Get()));
				Assert::IsTrue(Int64TypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get()));
			}
		}

		TEST_METHOD(IntAndFloatArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float32TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
				Assert::IsTrue(Float32TypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get()));
			}
		}

		TEST_METHOD(IntAndFloat64ArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float64TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
				Assert::IsTrue(Float64TypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get()));
			}
		}

		TEST_METHOD(IntAndCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Int32TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(Int32TypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get()));
			}
		}

		TEST_METHOD(IntAndWCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Int32TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(Int32TypeInfo::Get() == CharTypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get()));
			}
		}

		TEST_METHOD(IntAndByteArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Int32TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
				Assert::IsTrue(Int32TypeInfo::Get() == ByteTypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get()));
			}
		}

		TEST_METHOD(IntAndEverythingElseArithOperationsThrowsException)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get());
				}); 
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(Int64AndFloat32ArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float32TypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
				Assert::IsTrue(Float32TypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), Int64TypeInfo::Get()));
			}
		}

		TEST_METHOD(Int64AndFloat64ArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float64TypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
				Assert::IsTrue(Float64TypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), Int64TypeInfo::Get()));
			}
		}

		TEST_METHOD(Int64AndUIntArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Int64TypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get()));
				Assert::IsTrue(Int64TypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), Int64TypeInfo::Get()));
			}
		}

		TEST_METHOD(Int64AndByteArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Int64TypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
				Assert::IsTrue(Int64TypeInfo::Get() == ByteTypeInfo::Get()->EvaluateOperation(operation.get(), Int64TypeInfo::Get()));
			}
		}

		TEST_METHOD(Int64AndCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Int64TypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(Int64TypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), Int64TypeInfo::Get()));
			}
		}

		TEST_METHOD(Int64AndWCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Int64TypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(Int64TypeInfo::Get() == CharTypeInfo::Get()->EvaluateOperation(operation.get(), Int64TypeInfo::Get()));
			}
		}

		TEST_METHOD(Int64AndEverythingElseArithOperationsThrowsException)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int64TypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int64TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(UIntAndByteArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(UInt32TypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
				Assert::IsTrue(UInt32TypeInfo::Get() == ByteTypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get()));
			}
		}

		TEST_METHOD(UIntAndCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(UInt32TypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(UInt32TypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get()));
			}
		}

		TEST_METHOD(UIntAndWCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(UInt32TypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(UInt32TypeInfo::Get() == CharTypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get()));
			}
		}

		TEST_METHOD(UIntAndUInt64ArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(UInt64TypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get()));
				Assert::IsTrue(UInt64TypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get()));
			}
		}

		TEST_METHOD(UIntAndFloat32ArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float32TypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
				Assert::IsTrue(Float32TypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get()));
			}
		}

		TEST_METHOD(UIntAndFloat64ArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float64TypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
				Assert::IsTrue(Float64TypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get()));
			}
		}

		TEST_METHOD(UIntAndEverythingElseArithOperationsThrowsException)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(UInt64AndByteArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(UInt64TypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
				Assert::IsTrue(UInt64TypeInfo::Get() == ByteTypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get()));
			}
		}

		TEST_METHOD(UInt64AndCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(UInt64TypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(UInt64TypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get()));
			}
		}

		TEST_METHOD(UInt64AndWCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(UInt64TypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(UInt64TypeInfo::Get() == CharTypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get()));
			}
		}

		TEST_METHOD(UInt64AndFloat32ArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float32TypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
				Assert::IsTrue(Float32TypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get()));
			}
		}

		TEST_METHOD(UInt64AndFloat64ArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float64TypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
				Assert::IsTrue(Float64TypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get()));
			}
		}

		TEST_METHOD(UInt64AndEverythingElseArithOperationsThrowsException)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(Float32AndFloat64ArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float64TypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
				Assert::IsTrue(Float64TypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
			}
		}

		TEST_METHOD(Float32AndByteArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float32TypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
				Assert::IsTrue(Float32TypeInfo::Get() == ByteTypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
			}
		}

		TEST_METHOD(Float32AndCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float32TypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(Float32TypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
			}
		}

		TEST_METHOD(Float32AndWCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float32TypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(Float32TypeInfo::Get() == CharTypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
			}
		}

		TEST_METHOD(Float32AndEverythingElseArithOperationsThrowsException)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Float32TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(Float64AndByteArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float64TypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
				Assert::IsTrue(Float64TypeInfo::Get() == ByteTypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
			}
		}

		TEST_METHOD(Float64AndCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float64TypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(Float64TypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
			}
		}

		TEST_METHOD(Float64AndWCharArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(Float64TypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(Float64TypeInfo::Get() == CharTypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
			}
		}

		TEST_METHOD(Float64AndEverythingElseArithOperationsThrowsException)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Float64TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(CharAndByteArithOperations)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::IsTrue(ByteTypeInfo::Get() == ByteTypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(ByteTypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
			}
		}

		TEST_METHOD(CharAndEverythingElseArithOperationsThrowsException)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(WCharAndEverythingElseArithOperationsThrowsException)
		{
			for (auto& operation : _arithmeticOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					CharTypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					CharTypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(NumericUnaryArithmeticOperations)
		{
			for (auto& prim : _nonBoolPrimitives)
			{
				Assert::IsTrue(prim == prim->EvaluateOperation(&_preInc));
				Assert::IsTrue(prim == prim->EvaluateOperation(&_postInc));
				Assert::IsTrue(prim == prim->EvaluateOperation(&_preDec));
				Assert::IsTrue(prim == prim->EvaluateOperation(&_postDec));
			}
		}

		TEST_METHOD(NumericUnaryArithmeticOperationOnBoolThrowsException)
		{
			Assert::ExpectException<OperationNotDefinedException>([this]()
			{
				BoolTypeInfo::Get()->EvaluateOperation(&_preInc);
			});
			Assert::ExpectException<OperationNotDefinedException>([this]()
			{
				BoolTypeInfo::Get()->EvaluateOperation(&_postInc);
			});
			Assert::ExpectException<OperationNotDefinedException>([this]()
			{
				BoolTypeInfo::Get()->EvaluateOperation(&_preDec);
			});
			Assert::ExpectException<OperationNotDefinedException>([this]()
			{
				BoolTypeInfo::Get()->EvaluateOperation(&_postDec);
			});
		}

	private:
		static vector<shared_ptr<TypeInfo>> _nonBoolPrimitives;

		// Operations
		PreIncrementOperation _preInc = PreIncrementOperation(nullptr);
		PostIncrementOperation _postInc = PostIncrementOperation(nullptr);
		PreDecrementOperation _preDec = PreDecrementOperation(nullptr);
		PostDecrementOperation _postDec = PostDecrementOperation(nullptr);
		static vector<shared_ptr<ArithmeticBinaryOperation>> _arithmeticOperations;
	};

	TEST_CLASS(PrimitiveLogicalBinaryOperationTest)
	{
	public:
		TEST_CLASS_INITIALIZE(Setup)
		{
			_logicalOperations.push_back(make_shared<GreaterThanOrEqualOperation>(nullptr, nullptr));
			_logicalOperations.push_back(make_shared<LessThanOrEqualOperation>(nullptr, nullptr));
			_logicalOperations.push_back(make_shared<GreaterThanOperation>(nullptr, nullptr));
			_logicalOperations.push_back(make_shared<LessThanOperation>(nullptr, nullptr));
			_logicalOperations.push_back(make_shared<EqualToOperation>(nullptr, nullptr));
			_logicalOperations.push_back(make_shared<NotEqualToOperation>(nullptr, nullptr));
		}

		TEST_METHOD(IntComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::IsTrue(BoolTypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), Int64TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
			}
		}

		TEST_METHOD(IntIllegalComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(Int64ComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::IsTrue(BoolTypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
			}
		}

		TEST_METHOD(Int64IllegalComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int64TypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int64TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(UIntComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::IsTrue(BoolTypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), Int64TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
			}
		}

		TEST_METHOD(UIntIllegalComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					UInt32TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(UInt64ComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::IsTrue(BoolTypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
			}
		}

		TEST_METHOD(UInt64IllegalComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					UInt64TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(FloatComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::IsTrue(BoolTypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Float32TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
			}
		}

		TEST_METHOD(FloatIllegalComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Float32TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(Float64ComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::IsTrue(BoolTypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(BoolTypeInfo::Get() == Float64TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
			}
		}

		TEST_METHOD(Float64IllegalComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Float64TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(CharComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::IsTrue(BoolTypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
			}
		}

		TEST_METHOD(CharIllegalComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(WCharIllegalComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					CharTypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					CharTypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

		TEST_METHOD(ByteIllegalComparisonOperators)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					ByteTypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

	private:
		// Operations
		static vector<shared_ptr<LogicalBinaryComparisonOperation>> _logicalOperations;
	};

	TEST_CLASS(PrimitiveLogicalBooleanOperationTest)
	{
	public:
		TEST_CLASS_INITIALIZE(Setup)
		{
			_logicalOperations.push_back(make_shared<LogicalAndOperation>(nullptr, nullptr));
			_logicalOperations.push_back(make_shared<LogicalOrOperation>(nullptr, nullptr));

			_nonBoolTypes.push_back(Int32TypeInfo::Get());
			_nonBoolTypes.push_back(Int64TypeInfo::Get());
			_nonBoolTypes.push_back(UInt32TypeInfo::Get());
			_nonBoolTypes.push_back(UInt64TypeInfo::Get());
			_nonBoolTypes.push_back(Float32TypeInfo::Get());
			_nonBoolTypes.push_back(Float64TypeInfo::Get());
			_nonBoolTypes.push_back(CharByteTypeInfo::Get());
			_nonBoolTypes.push_back(CharTypeInfo::Get());
			_nonBoolTypes.push_back(ByteTypeInfo::Get());
		}

		TEST_METHOD(NonBooleanTypesBooleanComparisonIllegal)
		{
			for (auto& operation : _logicalOperations)
			{
				for (int i = 0; i < _nonBoolTypes.size(); i++)
				{
					for (int j = 0; j < _nonBoolTypes.size(); j++)
					{
						Assert::ExpectException<OperationNotDefinedException>([this, &operation, i, j]()
						{
							_nonBoolTypes[i]->EvaluateOperation(operation.get(), _nonBoolTypes[j]);
						});
					}
				}
			}
		}

		TEST_METHOD(BooleanTtypesBooleanComparison)
		{
			for (auto& operation : _logicalOperations)
			{
				Assert::IsTrue(BoolTypeInfo::Get() == BoolTypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get()));
			}
		}

	private:
		// Primitive TypeInfos
		static vector<shared_ptr<TypeInfo>> _nonBoolTypes;

		// Operations
		static vector<shared_ptr<LogicalBinaryBooleanOperation>> _logicalOperations;
	};

	TEST_CLASS(PrimitiveBitwiseOperationTest)
	{
	public:
		TEST_CLASS_INITIALIZE(Setup)
		{
			_primitives.push_back(Int32TypeInfo::Get());
			_primitives.push_back(Int64TypeInfo::Get());
			_primitives.push_back(UInt32TypeInfo::Get());
			_primitives.push_back(UInt64TypeInfo::Get());
			_primitives.push_back(Float32TypeInfo::Get());
			_primitives.push_back(Float64TypeInfo::Get());
			_primitives.push_back(CharByteTypeInfo::Get());
			_primitives.push_back(CharTypeInfo::Get());
			_primitives.push_back(ByteTypeInfo::Get());
			_primitives.push_back(BoolTypeInfo::Get());

			_bitwiseOperations.push_back(make_shared<BitwiseAndOperation>(nullptr, nullptr));
			_bitwiseOperations.push_back(make_shared<BitwiseOrOperation>(nullptr, nullptr));
			_bitwiseOperations.push_back(make_shared<BitwiseXorOperation>(nullptr, nullptr));
		}

		TEST_METHOD(Int32LegalShiftOperations)
		{
			for (auto& primitive : _primitives)
			{
				// RHS must be implicitly convertable to an int
				if (Int32TypeInfo::Get()->IsImplicitlyAssignableFrom(primitive, nullptr))
				{
					Assert::IsTrue(Int32TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive));
					Assert::IsTrue(Int32TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive));
				}
				else
				{
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						Int32TypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive);
					});
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						Int32TypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive);
					});
				}
			}
		}

		TEST_METHOD(Int64LegalShiftOperations)
		{
			for (auto& primitive : _primitives)
			{
				// RHS must be implicitly convertable to an int
				if (Int32TypeInfo::Get()->IsImplicitlyAssignableFrom(primitive, nullptr))
				{
					Assert::IsTrue(Int64TypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive));
					Assert::IsTrue(Int64TypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive));
				}
				else
				{
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						Int64TypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive);
					});
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						Int64TypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive);
					});
				}
			}
		}

		TEST_METHOD(UInt32LegalShiftOperations)
		{
			for (auto& primitive : _primitives)
			{
				// RHS must be implicitly convertable to an int
				if (Int32TypeInfo::Get()->IsImplicitlyAssignableFrom(primitive, nullptr))
				{
					Assert::IsTrue(UInt32TypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive));
					Assert::IsTrue(UInt32TypeInfo::Get() == UInt32TypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive));
				}
				else
				{
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						UInt32TypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive);
					});
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						UInt32TypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive);
					});
				}
			}
		}

		TEST_METHOD(UInt64LegalShiftOperations)
		{
			for (auto& primitive : _primitives)
			{
				// RHS must be implicitly convertable to an int
				if (Int32TypeInfo::Get()->IsImplicitlyAssignableFrom(primitive, nullptr))
				{
					Assert::IsTrue(UInt64TypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive));
					Assert::IsTrue(UInt64TypeInfo::Get() == UInt64TypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive));
				}
				else
				{
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						UInt64TypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive);
					});
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						UInt64TypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive);
					});
				}
			}
		}

		TEST_METHOD(CharLegalShiftOperations)
		{
			for (auto& primitive : _primitives)
			{
				// RHS must be implicitly convertable to an int
				if (Int32TypeInfo::Get()->IsImplicitlyAssignableFrom(primitive, nullptr))
				{
					Assert::IsTrue(CharByteTypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive));
					Assert::IsTrue(CharByteTypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive));
				}
				else
				{
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						CharByteTypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive);
					});
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						CharByteTypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive);
					});
				}
			}
		}

		TEST_METHOD(WCharLegalShiftOperations)
		{
			for (auto& primitive : _primitives)
			{
				// RHS must be implicitly convertable to an int
				if (Int32TypeInfo::Get()->IsImplicitlyAssignableFrom(primitive, nullptr))
				{
					Assert::IsTrue(CharTypeInfo::Get() == CharTypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive));
					Assert::IsTrue(CharTypeInfo::Get() == CharTypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive));
				}
				else
				{
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						CharTypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive);
					});
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						CharTypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive);
					});
				}
			}
		}

		TEST_METHOD(ByteLegalShiftOperations)
		{
			for (auto& primitive : _primitives)
			{
				// RHS must be implicitly convertable to an int
				if (Int32TypeInfo::Get()->IsImplicitlyAssignableFrom(primitive, nullptr))
				{
					Assert::IsTrue(ByteTypeInfo::Get() == ByteTypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive));
					Assert::IsTrue(ByteTypeInfo::Get() == ByteTypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive));
				}
				else
				{
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						ByteTypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive);
					});
					Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
					{
						ByteTypeInfo::Get()->EvaluateOperation(&_shiftRight, primitive);
					});
				}
			}
		}

		TEST_METHOD(IllegalShiftOperations)
		{
			for (auto& primitive : _primitives)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
				{
					BoolTypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive);
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
				{
					Float32TypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive);
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &primitive]()
				{
					Float64TypeInfo::Get()->EvaluateOperation(&_shiftLeft, primitive);
				});
			}
		}

		TEST_METHOD(IntAndInt64BitwiseOperations)
		{
			for (auto& operation : _bitwiseOperations)
			{
				Assert::IsTrue(Int64TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), Int64TypeInfo::Get()));
				Assert::IsTrue(Int64TypeInfo::Get() == Int64TypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get()));
			}
		}

		TEST_METHOD(IntAndCharBitwiseOperations)
		{
			for (auto& operation : _bitwiseOperations)
			{
				Assert::IsTrue(Int32TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), CharByteTypeInfo::Get()));
				Assert::IsTrue(Int32TypeInfo::Get() == CharByteTypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get()));
			}
		}

		TEST_METHOD(IntAndWCharBitwiseOperations)
		{
			for (auto& operation : _bitwiseOperations)
			{
				Assert::IsTrue(Int32TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), CharTypeInfo::Get()));
				Assert::IsTrue(Int32TypeInfo::Get() == CharTypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get()));
			}
		}

		TEST_METHOD(IntAndByteBitwiseOperations)
		{
			for (auto& operation : _bitwiseOperations)
			{
				Assert::IsTrue(Int32TypeInfo::Get() == Int32TypeInfo::Get()->EvaluateOperation(operation.get(), ByteTypeInfo::Get()));
				Assert::IsTrue(Int32TypeInfo::Get() == ByteTypeInfo::Get()->EvaluateOperation(operation.get(), Int32TypeInfo::Get()));
			}
		}

		TEST_METHOD(IntAndEverythingElseBitwiseOperationsThrowsException)
		{
			for (auto& operation : _bitwiseOperations)
			{
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), UInt32TypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), Float32TypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), Float64TypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), UInt64TypeInfo::Get());
				});
				Assert::ExpectException<OperationNotDefinedException>([this, &operation]()
				{
					Int32TypeInfo::Get()->EvaluateOperation(operation.get(), BoolTypeInfo::Get());
				});
			}
		}

	private:
		// Primitive TypeInfos
		static vector<shared_ptr<TypeInfo>> _primitives;
		static vector<shared_ptr<LogicalBinaryBitwiseOperation>> _bitwiseOperations;
		BitwiseShiftLeftOperation _shiftLeft = BitwiseShiftLeftOperation(nullptr, nullptr);
		BitwiseShiftRightOperation _shiftRight = BitwiseShiftRightOperation(nullptr, nullptr);
	};

	TEST_CLASS(StaticOverflowTests)
	{
		TEST_METHOD(Int64SizedHexConstantOverflowsWhenAssignedToInt32)
		{
			auto tree = ParseTree("class A { int b = 0xF00000001; }");
			// Enable when we make this part of static type checking
			//auto table = std::make_shared<SymbolTable>();
			//Assert::ExpectException<OverflowException>([this, &tree, &table]()
			//{
			//});
			Assert::ExpectException<OverflowException>([this, &tree]()
			{
				auto theClass = dynamic_pointer_cast<ClassDeclaration>(tree->_stmt);
				Assert::IsTrue(theClass != nullptr);
				auto declaration = dynamic_pointer_cast<ClassMemberDeclaration>(theClass->_list->_statement);
				auto constant = dynamic_pointer_cast<IntegerConstant>(declaration->_defaultValue);
				auto asInt = constant->AsInt32();
			});
		}
	};

	TEST_CLASS(BestFitTests)
	{
		TEST_METHOD(AutoIntegerTypeDefaultsToInt32)
		{
			auto tree = ParseTree("class A { fun Foo() { let i = 1; int j = i; } }");
			auto table = std::make_shared<SymbolTable>();
			tree->TypeCheck(table);

			auto theClass = dynamic_pointer_cast<ClassDeclaration>(tree->_stmt);
			Assert::IsTrue(theClass != nullptr);
			auto declaration = dynamic_pointer_cast<FunctionDeclaration>(theClass->_list->_statement);
			auto lineStmts = dynamic_pointer_cast<LineStatements>(declaration->_body);
			auto stmt1 = dynamic_pointer_cast<Assignment>(lineStmts->_statement);
			auto stmt2 = dynamic_pointer_cast<Assignment>(lineStmts->_next->_statement);
			// TODO?
		}
	};

	vector<shared_ptr<TypeInfo>> PrimitivesTest::_primitives;
	vector<shared_ptr<TypeInfo>> PrimitiveArithmeticOperationTest::_nonBoolPrimitives;
	vector<shared_ptr<ArithmeticBinaryOperation>> PrimitiveArithmeticOperationTest::_arithmeticOperations;
	vector<shared_ptr<LogicalBinaryComparisonOperation>> PrimitiveLogicalBinaryOperationTest::_logicalOperations;
	vector<shared_ptr<TypeInfo>> PrimitiveLogicalBooleanOperationTest::_nonBoolTypes;
	vector<shared_ptr<LogicalBinaryBooleanOperation>> PrimitiveLogicalBooleanOperationTest::_logicalOperations;
	vector<shared_ptr<TypeInfo>> PrimitiveBitwiseOperationTest::_primitives;
	vector<shared_ptr<LogicalBinaryBitwiseOperation>> PrimitiveBitwiseOperationTest::_bitwiseOperations;
}