#include "stdafx.h"
#include "ArchiveSerialization.h"
#include <archive.h>
#include <archive_entry.h>

#include <boost\property_tree\ptree_serialization.hpp>
#include <boost\property_tree\json_parser.hpp>
#include <boost\asio\streambuf.hpp>
#include <boost\algorithm\string.hpp>

#include <cstdio>

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

typedef struct archive Archive;
typedef struct archive_entry ArchiveEntry;

#define CheckLibArchiveResult(x, a) \
if (x != 0)									\
{												\
	std::stringstream strstr;				\
	strstr << archive_errno(a); \
	throw Ruddy::UnexpectedArchiveException(strstr.str().c_str()); \
}

// TODO: we should wrap archive_read_data_block in a stream
// in order to avoid doing a double copy here to memory
void GetStreamBufferFromArchiveEntry(Archive* a, std::ostream& ostream)
{
	const char* buff;
	size_t size;
	int64_t offset;
	while (archive_read_data_block(a, reinterpret_cast<const void**>(&buff), &size, &offset) == ARCHIVE_OK)
	{
		ostream.write(buff, size);
	}
}

Ruddy::VersionNumber Ruddy::VersionNumber::CurrentVersion = Ruddy::VersionNumber(1, 0);

Ruddy::VersionNumber::VersionNumber(unsigned int major, unsigned int minor) : MajorVersion(major), MinorVersion(minor)
{
}

std::string Ruddy::VersionNumber::Serialize() const
{
	std::stringstream ss;
	ss << MajorVersion << "." << MinorVersion;
	return ss.str();
}

Ruddy::LibraryMetadata::LibraryMetadata(const std::string & libName, VersionNumber version) : Name(libName), Version(version)
{
}

Ruddy::LibraryMetadata::LibraryMetadata(std::istream& input, const std::string& fileName) : Version(0,0)
{
	ptree root;
	try
	{
		boost::property_tree::read_json(input, root);
	}
	catch (boost::property_tree::json_parser::json_parser_error& err)
	{
		throw ArchiveReadException(fileName.c_str(), err.message().c_str());
	}
	Name = root.get<std::string>("Name");
	auto versionNum = root.get<std::string>("Version");
	std::vector<std::string> versionNums;
	boost::split(versionNums, versionNum, boost::is_any_of("."));
	if (versionNums.size() < 2)
		throw ArchiveReadException(fileName.c_str(), "Could not find version number");

	Version.MajorVersion = std::atoi(versionNums[0].c_str());
	Version.MinorVersion = std::atoi(versionNums[1].c_str());
}

void Ruddy::LibraryMetadata::Serialize(const char* filePath)
{
	ptree root;
	root.put("Name", Name);
	root.put("Version", Version.Serialize());

	std::ofstream tempFile(filePath, std::ios_base::trunc | std::ios_base::out);
	boost::property_tree::write_json(tempFile, root);
	tempFile.close();
}

/* Rinc */
Ruddy::Rinc::Rinc(std::shared_ptr<Ast::SymbolTable> table, LibraryMetadata & metadata, const std::string& outputDirectory) : SymbolTable(table), Metadata(metadata), OutputDir(outputDirectory)
{
}

void WriteDataFromFile(Archive *a, const char* symbolsFileName, ArchiveEntry* symbolEntry)
{
	struct stat st;
	stat(symbolsFileName, &st);
	archive_entry_copy_stat(symbolEntry, &st);
	archive_write_header(a, symbolEntry);

	{
		std::ifstream tempFile(symbolsFileName, std::ios_base::binary | std::ios_base::binary);
		char buff[256];
		while (!tempFile.eof())
		{
			tempFile.read(buff, _countof(buff));
			archive_write_data(a, buff, tempFile.gcount());
		}
	}
}

void WriteMetadataEntry(Archive *a, Ruddy::LibraryMetadata& metadata, const std::string& outputDir)
{
	auto filename = outputDir.empty() ? LibraryMetadataPathName : outputDir + "\\" + LibraryMetadataPathName;
	metadata.Serialize(filename.c_str());

	ArchiveEntry* metadataEntry = archive_entry_new();
	archive_entry_set_pathname(metadataEntry, LibraryMetadataPathName);
	archive_entry_set_filetype(metadataEntry, AE_IFREG);

	WriteDataFromFile(a, filename.c_str(), metadataEntry);

	CheckLibArchiveResult(archive_write_finish_entry(a),a)
	archive_entry_free(metadataEntry);
}

void WriteSymbols(Archive *a, Ruddy::LibraryMetadata& metadata, std::shared_ptr<Ast::SymbolTable> table, const std::string& outputDir)
{
	ArchiveEntry* symbolEntry = archive_entry_new();
	archive_entry_set_pathname(symbolEntry, LibrarySymbolsPathName);
	archive_entry_set_filetype(symbolEntry, AE_IFREG);

	auto filename = outputDir.empty() ? LibrarySymbolsPathName : outputDir + "\\" + LibrarySymbolsPathName;
	std::ofstream tempFile(filename, std::ios_base::trunc | std::ios_base::out);
	table->Serialize(tempFile, metadata.Name);
	tempFile.close();

	WriteDataFromFile(a, filename.c_str(), symbolEntry);

	CheckLibArchiveResult(archive_write_finish_entry(a), a)
	archive_entry_free(symbolEntry);
}

std::shared_ptr<Ast::SymbolTable> Ruddy::Rinc::GetSymbolTable()
{
	if (_rincFile.empty())
	{
		return SymbolTable;
	}
	else if (SymbolTable == nullptr)
	{
		// Deserialize the symbol table
		Archive* a = archive_read_new();
		CheckLibArchiveResult(archive_read_support_compression_all(a), a)
		CheckLibArchiveResult(archive_read_support_format_all(a), a)
		CheckLibArchiveResult(archive_read_open_filename(a, _rincFile.c_str(), 16384), a)
		ArchiveEntry* entry;
		while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
		{
			auto entryName = std::string(archive_entry_pathname(entry));
			if (entryName.compare(LibrarySymbolsPathName) == 0)
			{
				boost::asio::streambuf buf;
				std::iostream str(&buf);
				GetStreamBufferFromArchiveEntry(a, str);
				str.seekg(0);
				SymbolTable = std::make_shared<Ast::SymbolTable>();
				SymbolTable->LoadFrom(str);
				break;
			}
			else
			{
				archive_read_data_skip(a);
			}
		}
		archive_read_finish(a);
		if (SymbolTable == nullptr)
			throw ArchiveReadException(_rincFile.c_str(), "Symbols not found");
	}
	return SymbolTable;
}

void Ruddy::Rinc::Serialize(const char *file)
{
	Archive *a = archive_write_new();
	CheckLibArchiveResult(archive_write_set_compression_bzip2(a), a)
	CheckLibArchiveResult(archive_write_add_filter_bzip2(a), a)
	CheckLibArchiveResult(archive_write_open_filename(a, file), a)

	// LibraryMetadata
	WriteMetadataEntry(a, Metadata, OutputDir);

	// Symbols
	WriteSymbols(a, Metadata, SymbolTable, OutputDir);

	// Cleanup
	CheckLibArchiveResult(archive_write_close(a), a)
	CheckLibArchiveResult(archive_write_free(a), a)
}

bool Ruddy::Rinc::WasAccessed()
{
	return SymbolTable != nullptr;
}

/* Rib */
void WriteIrCode(Archive *a, Ruddy::LibraryMetadata& metadata, const std::string& irCodeFile)
{
	ArchiveEntry* irEntry = archive_entry_new();
	archive_entry_set_pathname(irEntry, LibraryIRPathName);
	archive_entry_set_filetype(irEntry, AE_IFREG);

	WriteDataFromFile(a, irCodeFile.c_str(), irEntry);

	CheckLibArchiveResult(archive_write_finish_entry(a), a)
	archive_entry_free(irEntry);
}

Ruddy::Rib::Rib(std::shared_ptr<Ruddy::Rinc> rinc, const std::string& ircodeFilename) :
	Rinc(rinc), IrFileName(ircodeFilename)
{
}

Ruddy::Rib::Rib(const std::string& ribFile) :
	Rinc(std::make_shared<Ruddy::Rinc>(ribFile)), RibFileName(ribFile)
{
	Archive* a = archive_read_new();
	CheckLibArchiveResult(archive_read_support_compression_all(a), a)
	CheckLibArchiveResult(archive_read_support_format_all(a), a)
	CheckLibArchiveResult(archive_read_open_filename(a, ribFile.c_str(), 16384), a)
	ArchiveEntry* entry;
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
	{
		auto entryName = std::string(archive_entry_pathname(entry));
		if (entryName.compare(LibraryIRPathName) == 0)
		{
			archive_read_data_skip(a);
			// To be delay loaded
		}
		else if (entryName.compare(LibrarySymbolsPathName) == 0)
		{
			archive_read_data_skip(a);
			// To be delay loaded
		}
		else if (entryName.compare(LibraryMetadataPathName) == 0)
		{
			boost::asio::streambuf buf;
			std::iostream ostr(&buf);
			GetStreamBufferFromArchiveEntry(a, ostr);
			ostr.seekg(0);
			Rinc->Metadata = Ruddy::LibraryMetadata(ostr, ribFile);
		}
		else
		{
			archive_read_data_skip(a);
		}
	}
	archive_read_finish(a);
	if (Rinc->Metadata.Version.MajorVersion == 0)
		throw ArchiveReadException(ribFile.c_str(), "Library metadata not found");
}

void Ruddy::Rib::Serialize(const char * file)
{
	Archive* a = archive_write_new();
	CheckLibArchiveResult(archive_write_add_filter_gzip(a), a)
	CheckLibArchiveResult(archive_write_set_format_pax_restricted(a), a)
	CheckLibArchiveResult(archive_write_set_compression_gzip(a), a)
	CheckLibArchiveResult(archive_write_open_filename(a, file), a)

	// LibraryMetadata
	WriteMetadataEntry(a, Rinc->Metadata, Rinc->OutputDir);

	// Symbols
	WriteSymbols(a, Rinc->Metadata, Rinc->GetSymbolTable(), Rinc->OutputDir);

	// ir
	WriteIrCode(a, Rinc->Metadata, IrFileName);

	// Cleanup
	CheckLibArchiveResult(archive_write_close(a), a)
	CheckLibArchiveResult(archive_write_free(a), a)
}

std::string Ruddy::Rib::WriteIRTo(const std::string & path)
{
	if (RibFileName.empty())
		throw std::exception();

	std::string irPath(path);
	Archive* a = archive_read_new();
	CheckLibArchiveResult(archive_read_support_compression_all(a), a)
	CheckLibArchiveResult(archive_read_support_format_all(a), a)
	CheckLibArchiveResult(archive_read_open_filename(a, RibFileName.c_str(), 16384), a)
	ArchiveEntry* entry;
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
	{
		auto entryName = std::string(archive_entry_pathname(entry));
		if (entryName.compare(LibraryIRPathName) == 0)
		{
			if (!irPath.empty())
				irPath.append("\\");
			irPath.append(Rinc->Metadata.Name);
			irPath.append(".ll");
			std::ofstream output(irPath);
			GetStreamBufferFromArchiveEntry(a, output);
			output.close();
			break;
		}
		else
		{
			archive_read_data_skip(a);
		}
	}
	archive_read_finish(a);
	return irPath;
}

bool Ruddy::Rib::WasAccessed()
{
	return Rinc != nullptr && Rinc->WasAccessed();
}
