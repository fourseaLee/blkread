#ifndef BLOCK_READ_H
#define BLOCK_READ_H
#include <string>
#ifndef MESSAGE_START_SIZE 
#define MESSAGE_START_SIZE 4
#endif
#include <array>
#include "chain.h"
#include "fs.h"

class BlockRead
{
public:
	BlockRead() 
	{}
	~BlockRead()
	{}
public:
	bool openBlockFile(const std::string& file_path, int nFile);

	bool loadBlock();	
protected:
	FILE* file_;
	CDiskBlockPos pos_;	
	std::array<uint8_t, MESSAGE_START_SIZE> disk_magic_{{0xf9, 0xbe, 0xb4,0xd9}};
};


#endif
