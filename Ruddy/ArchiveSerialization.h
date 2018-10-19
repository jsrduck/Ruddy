#pragma once

#include <SymbolTable.h>
#include <sstream>

namespace Ruddy 
{

struct VersionNumber
{
public:
	unsigned int MajorVersion;
	unsigned int MinorVersion;
	VersionNumber(unsigned int major, unsigned int minor);
	std::string Serialize() const;
	static VersionNumber CurrentVersion;
};

#define LibraryMetadataPathName "Meta"
class LibraryMetadata
{
public:
	LibraryMetadata(const std::string& libName, VersionNumber version);
	LibraryMetadata(std::istream& input, const std::string& fileName);
	LibraryMetadata() : Version(0,0) {}
	std::string Name;
	VersionNumber Version;
	void Serialize(const char* filePath);
};

/*
 * The "Rinc" is a ruddy include library. It has all the symbol and type information for the library,
 * but lacks the implementation. It can be used for compilation, but not linking.
 */
#define LibrarySymbolsPathName "Symbols"
class Rinc
{
public:
	Rinc(std::shared_ptr<Ast::SymbolTable> table, LibraryMetadata& metadata, const std::string& outputDirectory);
	Rinc(const std::string& rincFile) :_rincFile(rincFile) { }

	std::shared_ptr<Ast::SymbolTable> GetSymbolTable();
	void Serialize(const char *file);
	bool WasAccessed();
	
	LibraryMetadata Metadata;
	const std::string OutputDir;
private:
	std::shared_ptr<Ast::SymbolTable> SymbolTable;
	std::string _rincFile;
};

/*
* The "Rib" is a ruddy static library. It has all the symbol and type information for the library,
* and also the implementation. It can be used for compilation and static linking.
*/
#define LibraryIRPathName "IR"
class Rib
{
public:
	Rib(std::shared_ptr<Rinc> rinc, const std::string& ircodeFilename);
	Rib(const std::string& ribFile);

	void Serialize(const char *file);
	std::string WriteIRTo(const std::string& path);
	bool WasAccessed();

	std::shared_ptr<Rinc> Rinc;
	std::string IrFileName;
	std::string RibFileName;
};

class CompilerException : public std::exception
{
public:
	std::string Message()
	{
		return _message;
	}

protected:
	std::string _message;
};
class UnexpectedArchiveException : public CompilerException
{
public:
	UnexpectedArchiveException(const char * error)
	{
		_message = "Unexpected libarchive exception: ";
		_message.append(error);
	}
};
class ArchiveReadException : public CompilerException
{
public:
	ArchiveReadException(const char * archiveName)
	{
		_message = "Corrupt file detected: ";
		_message.append(archiveName);
	}

	ArchiveReadException(const char * archiveName, const char * extendedError)
	{
		std::stringstream ss;
		ss << "Corrupt file detected: ";
		ss << archiveName;
		ss << std::endl;
		ss << "\t";
		ss << extendedError;
		ss << std::endl;
		_message = ss.str();
	}
};
}