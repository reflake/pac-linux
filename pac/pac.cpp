#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <unistd.h>
#include <filesystem>

namespace fs = std::filesystem;

#if NDEBUG
constexpr bool debug = false;
#else
constexpr bool debug = true;
#endif
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long      uint64;

typedef unsigned char      byte8;
typedef unsigned short     byte16;
typedef unsigned int       byte32;
typedef unsigned long      byte64;

template <typename T>
constexpr uint32 countof(T & var)
{
	return static_cast<uint32>(sizeof(var) / sizeof(var[0]));
}

#define PrintFunction() printf("%s\n", __FUNCTION__)

char directory[256] = {};

byte8 signature1[] = { 'P', 'A', 'C' };
byte8 signature2[] = { 'P', 'N', 'S','T' };

byte8 * signature[] =
{
	signature1,
	signature2,
};

uint8 signatureCount[] =
{
	(uint8)countof(signature1),
	(uint8)countof(signature2),
};

const char * signatureString[] =
{
	"PAC",
	"PNST",
};

static int lastError = 0;

void SetLastError(int number) {

	throw std::runtime_error("Error");
}

int GetLastError() {

	return lastError;
}

template <typename T>
void Align(T & pos, T boundary, byte8 * addr = 0, byte8 pad = 0)
{
	T remainder = (pos % boundary);
	if (remainder)
	{
		T size = (boundary - remainder);
		if (addr)
		{
			memset((addr + pos), pad, size);
		}
		pos += size;
	}
}

byte8 * Alloc(uint32 size)
{
	// TODO: instead of exception return null and print error
	byte8 * addr = 0;
	byte32 error = 0;

	addr = reinterpret_cast<byte8*>(malloc(size));
	if (!addr)
	{
		throw std::runtime_error("Allocation failed");
	}

	memset(addr, 0, size);

	return addr;
}

byte8 * LoadFile(const char * fileName, uint32 * size, byte8 * dest = 0)
{
	// TODO: instead of exception return null and print error
	byte8 * addr = dest;

	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("CreateFile failed");
	}

	int fileSize = file.tellg();
	if (fileSize == 0)
	{
		// throw std::runtime_error("File exists, but is empty");
		return nullptr;
	}

	if (!addr)
	{
		addr = reinterpret_cast<byte8*>(malloc(fileSize));
		if (!addr)
		{
			throw std::runtime_error("Alloc failed");
		}
	}

	file.seekg(std::ios::beg);
	file.read(reinterpret_cast<char*>(addr), fileSize);
	file.close();

	if (size)
	{
		*size = fileSize;
	}

	return addr;
}

bool SignatureMatch(byte8 * addr, const byte8 * signature, uint8 count)
{
	for (uint8 index = 0; index < count; index++)
	{
		if (addr[index] != signature[index])
		{
			return false;
		}
	}
	return true;
}

#define MATCH_SIGNATURE_OF_FILE(sign, size) fileSize >= size && SignatureMatch(addr, reinterpret_cast<const byte8 *>(sign), size)

bool SaveFile(const char * fileName, byte8 * addr, uint32 fileSize)
{
	const char* MOD_SIGNATURE = "MOD ";
	const char* MOT_SIGNATURE = "P\0\0\0MOT\0";

	std::string newFileName(fileName);

	if (MATCH_SIGNATURE_OF_FILE(MOD_SIGNATURE, 4))
	{
		// Model file??
		newFileName.append(".mod");
	} else if (MATCH_SIGNATURE_OF_FILE(MOT_SIGNATURE, 8))
	{
		// Motion (animation) file??
		newFileName.append(".mot");
	}

	// TODO: instead of exception return false and print error
	std::ofstream file(newFileName.c_str(), std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to save. Couldn't create file");
	}

	file.write(reinterpret_cast<char*>(addr), fileSize);
	file.close();

	return true;
}

bool CreateEmptyFile(const char * fileName)
{
	// TODO: instead of exception return false and print error
	std::ofstream file(fileName);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to create empty file");
	}

	file.close();

	return true;
}

bool ChangeDirectory(const char * dest)
{
	if (chdir(dest) == 1)
	{
		printf("ChangeDirectory failed. %X\n", errno);
		return false;
	}

	return true;
}

const char * CheckSignature(byte8 * addr)
{
	for (uint8 index = 0; index < (uint8)countof(signature); index++)
	{
		if (SignatureMatch(addr, signature[index], signatureCount[index]))
		{
			return signatureString[index];
		}
	}
	return 0;
}

#pragma endregion

bool CheckArchive
(
	byte8 * archive,
	uint32 archiveSize,
	const char * directoryName,
	bool nonRecursive
);

void ExtractFiles
(
	byte8 * archive,
	uint32 archiveSize,
	bool nonRecursive
);

bool CheckArchive
(
	byte8 * archive,
	uint32 archiveSize,
	const char * directoryName,
	bool nonRecursive
)
{
	if constexpr (debug)
	{
		PrintFunction();
	}

	const char * match = CheckSignature(archive);
	if (!match)
	{
		return false;
	}

	fs::create_directory(directoryName);
	ChangeDirectory(directoryName);


	if (!CreateEmptyFile(match))
	{
		printf("CreateEmptyFile failed.\n");
		ChangeDirectory("..");
		return false;
	}

	ExtractFiles(archive, archiveSize, nonRecursive);

	ChangeDirectory("..");

	return true;
}

void ExtractFiles
(
	byte8 * archive,
	uint32 archiveSize,
	bool nonRecursive
)
{
	if constexpr (debug)
	{
		PrintFunction();
	}

	auto & fileCount = *(uint32 *)(archive + 4);

	for (uint32 fileIndex = 0; fileIndex < fileCount; fileIndex++)
	{
		char dest[64];

		uint32   fileOff = 0;
		uint32   nextFileOff = 0;
		byte8  * file = 0;
		uint32   fileSize = 0;

		snprintf(dest, sizeof(dest), "%.4u", fileIndex);

		fileOff = ((uint32 *)(archive + 8))[fileIndex];
		if (!fileOff)
		{
			CreateEmptyFile(dest);
			continue;
		}

		{
			uint32 index = fileIndex;
			do
			{
				if (index == (fileCount - 1))
				{
					nextFileOff = archiveSize;
					break;
				}
				nextFileOff = ((uint32 *)(archive + 8))[(index + 1)];
				if (nextFileOff)
				{
					break;
				}
				else
				{
					index++;
					continue;
				}
			}
			while (index < fileCount);
		}

		file = (archive + fileOff);

		fileSize = (nextFileOff - fileOff);

		if constexpr (debug)
		{
			printf("file     %llX\n", file);
			printf("fileOff  %X\n", fileOff);
			printf("fileSize %u\n", fileSize);
		}

		if (!nonRecursive && CheckArchive(file, fileSize, dest, nonRecursive))
		{
			continue;
		}

		SaveFile(dest, file, fileSize);
	}
}

int ExtractFileIndex(const fs::path& fileName) {

	std::string fileStem = fileName.stem().string();

	try {
		return std::stoi( fileStem );
	} catch (const std::invalid_argument& e) {
		std::cerr << "Invalid filename format: " << fileName << std::endl;
		return -1;
	}
}

byte8 * CreateArchive(uint32 * saveSize = 0)
{
	if constexpr (debug)
	{
		PrintFunction();
	}

	byte32 error = 0;

	byte8 * head = 0;
	uint32 headPos = 0;

	byte8 * data = 0;
	uint32 dataPos = 0;

	head = Alloc(4096);
	if (!head)
	{
		printf("Alloc failed.\n");
		return 0;
	}

	data = Alloc((8 * 1024 * 1024));
	if (!data)
	{
		printf("Alloc failed.\n");
		return 0;
	}

	auto & fileCount = *(uint32 *)(head + 4);
	auto   fileOff   =  (uint32 *)(head + 8);

	fs::path current_path = fs::current_path();
	bool foundAny = false;

	std::vector<fs::directory_entry> sortedEntries;

	for ( const auto& entry : fs::directory_iterator(current_path) ) {

		sortedEntries.push_back( entry );
	};

	// Hack: directories considered as files too
	std::sort(sortedEntries.begin(), sortedEntries.end(), [](const fs::path& a, const fs::path& b) {

		return ExtractFileIndex( a ) < ExtractFileIndex( b );
	});

	for( const auto& directoryEntry : sortedEntries ) {

		foundAny = true;

		auto entryPath = directoryEntry.path();
		auto entryStem = entryPath.stem().c_str();

		if (strcmp(entryStem, ".") == 0)
		{
			continue;
		}
		if (strcmp(entryStem, "..") == 0)
		{
			continue;
		}
		if (strcmp(entryStem, "PAC") == 0)
		{
			head[0] = 'P';
			head[1] = 'A';
			head[2] = 'C';
			continue;
		}
		if (strcmp(entryStem, "PNST") == 0)
		{
			head[0] = 'P';
			head[1] = 'N';
			head[2] = 'S';
			head[3] = 'T';
			continue;
		}
		if (directoryEntry.is_directory()) // @Todo: Add empty check.
		{
			ChangeDirectory(entryPath.c_str());

			byte8 * archive = 0;
			uint32 archiveSize = 0;

			archive = CreateArchive(&archiveSize);
			if (!archive)
			{
				printf("CreateArchive failed.\n");
				ChangeDirectory("..");
				continue;
			}

			fileOff[fileCount] = dataPos;

			if constexpr (debug)
			{
				printf("directory fileOff[%u] = %X\n", fileCount, dataPos);
			}

			fileCount++;

			memcpy((data + dataPos), archive, archiveSize);
			dataPos += archiveSize;
			Align<uint32>(dataPos, 0x10);

			free(archive);

			ChangeDirectory("..");
			continue;
		}

		byte8 * file = 0;
		uint32 fileSize = 0;

		file = LoadFile(entryPath.c_str(), &fileSize);
		fileOff[fileCount] = 0xFFFFFFFF;

		if (file)
		{
			fileOff[fileCount] = dataPos;

			if constexpr (debug)
			{
				printf("fileOff[%u] = %X\n", fileCount, dataPos);
			}

			memcpy((data + dataPos), file, fileSize);
			dataPos += fileSize;
			Align<uint32>(dataPos, 0x10);

			free(file);
		}

		fileCount++;
	}

	if (!foundAny)
		return 0;

	// All files compiled.

	byte8 * archive = 0;
	uint32 archivePos = 0;

	archive = Alloc((8 * 1024 * 1024));
	if (!archive)
	{
		printf("Alloc failed.\n");
		return 0;
	}

	headPos = (8 + (fileCount * 4));
	Align<uint32>(headPos, 0x10);

	for (uint32 index = 0; index < fileCount; index++)
	{
		auto & off = fileOff[index];

		if (off == 0xFFFFFFFF)
		{
			off = 0;
			continue;
		}

		off += headPos;
	}

	memcpy(archive, head, headPos);
	archivePos += headPos;
	Align<uint32>(archivePos, 0x10);

	memcpy((archive + archivePos), data, dataPos);
	archivePos += dataPos;
	Align<uint32>(archivePos, 0x10);

	free(head);
	free(data);

	if (saveSize)
	{
		*saveSize = archivePos;
	}

	return archive;
}

void PrintHelp()
{
	printf("Extract\n");
	printf("\n");
	printf("  pac e archive\n");
	printf("\n");
	printf("Repack\n");
	printf("\n");
	printf("  pac r directory archive\n");
	printf("\n");
	printf("Examples\n");
	printf("\n");
	printf("  pac e plwp_newvergilsword.pac\n");
	printf("  pac r plwp_sword2 new_plwp_sword2.pac");
	printf("\n");
}

int main(int argc, char ** argv)
{
	fs::path current_path = fs::current_path();

	strcpy(directory, current_path.c_str());

	if (argc < 2)
	{
		PrintHelp();
		return 0;
	}

	std::vector<std::string> args;

	for(int i = 1; i < argc; i++) {

		args.push_back( std::string( argv[i] ) );
	}

	auto contains_argument = [ args ](std::string a) { 
		return std::find(args.begin(), args.end(), a) != args.end();
	};

	if ( contains_argument( "e" ) )
	{
		if (argc < 3)
		{
			PrintHelp();
			return 0;
		}

		char   * fileName          = nullptr;
		byte8  * file              = 0;
		uint32   fileSize          = 0;
		char     directoryName[64] = {};
		bool     nonRecursive      = false;

		for(int i = 2; i < argc; i++) {

			if (strcmp(argv[i], "norec") == 0) {

				nonRecursive = true;
			} else if ( !fileName ) {

				fileName = argv[i];
			} else {

				printf("Too many arguments.\n");
			}
		}

		file = LoadFile(fileName, &fileSize);
		if (!file)
		{
			printf("LoadFile failed.\n");
			return 0;
		}

		memcpy(directoryName, fileName, (strlen(fileName) - 4)); // @Todo: A bit too optimistic. Add length check.

		CheckArchive(file, fileSize, directoryName, nonRecursive);
	}
	else if ( contains_argument( "r" ))
	{
		if (argc < 4)
		{
			PrintHelp();
			return 0;
		}

		char * directoryName = nullptr;
		char * fileName = nullptr;
		byte8 * archive = 0;
		uint32 archiveSize = 0;
		bool result = false;

		for (int i = 2; i < argc; i++) {

			if (!directoryName) {

				directoryName = argv[i];
			} else if (!fileName) {

				fileName = argv[i];
			} else {

				printf("Too many arguments.\n");
				return 0;
			}
		}

		if ( !directoryName || !fileName ) {
			printf("Invalid argument(s).\n");
			return 0;
		}

		if (!ChangeDirectory(directoryName))
		{
			printf("ChangeDirectory failed.\n");
			return 0;
		}

		archive = CreateArchive(&archiveSize);
		if (!archive)
		{
			printf("CreateArchive failed.\n");
			ChangeDirectory("..");
			return 0;
		}

		if constexpr (debug)
		{
			printf("archive     %llX\n", archive);
			printf("archiveSize %u\n", archiveSize);
		}

		ChangeDirectory("..");

		result = SaveFile(fileName, archive, archiveSize);
		if (!result)
		{
			printf("SaveFile failed.\n");
			return 0;
		}
	}
	else
	{
		PrintHelp();
		return 0;
	}

	return 1;
}
