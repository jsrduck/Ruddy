#include "stdafx.h"
#include "FileLocationContext.h"
#include <thread>
#include <mutex>
#include <stack>

class FileLocationStack
{
public:
	void Push(Ast::FileLocation& location)
	{
		_stack.push(location);
	}

	void Pop()
	{
		_stack.pop();
	}

	Ast::FileLocation Top()
	{
		return _stack.top();
	}

	bool Empty()
	{
		return _stack.empty();
	}

private:
	std::stack<Ast::FileLocation> _stack;
};

thread_local std::unique_ptr<FileLocationStack> _fileLocationStack;
std::mutex _locationMutex;

Ast::FileLocationContext::FileLocationContext(FileLocation& location)
{
	std::lock_guard<std::mutex> lock(_locationMutex);
	if (_fileLocationStack == nullptr)
	{
		_fileLocationStack = std::make_unique<FileLocationStack>();
	}
	_fileLocationStack->Push(location);
}

Ast::FileLocationContext::~FileLocationContext()
{
	std::lock_guard<std::mutex> lock(_locationMutex);
	_fileLocationStack->Pop();
}

Ast::FileLocation Ast::FileLocationContext::CurrentLocation()
{
	std::lock_guard<std::mutex> lock(_locationMutex);
	if (_fileLocationStack != nullptr && !_fileLocationStack->Empty())
		return _fileLocationStack->Top();
	else
		return FileLocation(-1, -1);
}