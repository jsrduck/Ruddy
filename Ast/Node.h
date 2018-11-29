#pragma once
#include <string>
#include <memory>

namespace Ast
{
	class Node
	{
	public:
		virtual std::string ToString() { return "NYI"; }
	};

	// DO NOT DELETE ENTRIES: reordering breaks our symbol table serialization
	enum Visibility
	{
		PUBLIC,
		PRIVATE,
		PROTECTED
	};

	class Modifier : public Node
	{
	public:
		enum class Modifiers
		{
			NONE = 0x0,
			STATIC = 0x1,
			UNSAFE = 0x2
		};

		Modifier(Modifiers mods) : _mods(mods)
		{
		}

		Modifiers Get()
		{
			return _mods;
		}

		bool IsStatic()
		{
			return (int) _mods & (int) Modifiers::STATIC;
		}

		bool IsUnsafe()
		{
			return (int) _mods & (int) Modifiers::UNSAFE;
		}

	private:
		Modifiers _mods;
	};

	class PeriodSeparatedId : public Node
	{
	public:
		PeriodSeparatedId(const std::string& id) : _id(id)
		{
		}

		PeriodSeparatedId(const std::string& lhs, const std::string& rhs) : _id(lhs + "." + rhs)
		{
		}

		std::string Id()
		{
			return _id;
		}
	private:
		const std::string _id;
	};

	enum TypeCheckPass
	{
		TYPE_CHECK_ALL = 0,

		TYPE_CHECK_START = 1,
		CLASS_AND_NAMESPACE_DECLARATIONS = 1,
		METHOD_DECLARATIONS = 2,
		CLASS_VARIABLES = 3,
		METHOD_BODIES = 4,
		TYPE_CHECK_END = 4
	};

}