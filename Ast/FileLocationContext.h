#pragma once

namespace Ast {

struct FileLocation
{
public:
	FileLocation(const int lineNumber, const int columnNumber) :
		LineNumber(lineNumber), ColumnNumber(columnNumber)
	{
	}

	int LineNumber;
	int ColumnNumber;
};

class FileLocationContext
{
public:
	FileLocationContext(FileLocation& location);
	~FileLocationContext();

	static FileLocation CurrentLocation();
};

}