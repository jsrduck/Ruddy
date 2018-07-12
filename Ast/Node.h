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

	class Modifier : public Node
	{
	public:
		enum class Modifiers
		{
			NONE = 0x0,
			STATIC = 0x1
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

	private:
		Modifiers _mods;
	};

}