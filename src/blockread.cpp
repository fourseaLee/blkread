#include<iostream>
#include "blockread.h"
#include "logging.h"
#include "streams.h"
#include "clientversion.h"

static const uint64_t ONE_KILOBYTE = 1000;
static const uint64_t ONE_MEGABYTE = ONE_KILOBYTE * 1000;
static const uint64_t ONE_GIGABYTE = ONE_MEGABYTE * 1000;
static const uint64_t MAX_TX_SIZE_CONSENSUS_BEFORE_GENESIS = ONE_MEGABYTE;
static const uint64_t LEGACY_MAX_BLOCK_SIZE = ONE_MEGABYTE;

/*static FILE *OpenDiskFile(const CDiskBlockPos &pos, const char *prefix, bool fReadOnly)
{
    if (pos.IsNull())
   	{
        return nullptr;
    }

    fs::path path = GetBlockPosFilename(pos, prefix);
    fs::create_directories(path.parent_path());
    FILE *file = fsbridge::fopen(path, "rb+");
    if (!file && !fReadOnly)
   	{
        file = fsbridge::fopen(path, "wb+");
    }

    if (!file)
   	{
        LogPrintf("Unable to open file %s\n", path.string());
        return nullptr;
    }

    if (pos.nPos)
   	{
        if (fseek(file, pos.nPos, SEEK_SET))
	   	{
            LogPrintf("Unable to seek to position %u of %s\n", pos.nPos,
                      path.string());
            fclose(file);
            return nullptr;
        }
    }
    return file;
}*/

static FILE *OpenBlockFile(const std::string& file_path, const CDiskBlockPos &pos, bool fReadOnly) 
{
    if (pos.IsNull())
	{
		return nullptr;
	}
	fs::path path(file_path);
	path = path / "blocks" / strprintf("%s%05u.dat", "blk", pos.nFile);
	fs::create_directories(path.parent_path());
    FILE *file = fsbridge::fopen(path, "rb+");
    if (!file && !fReadOnly)
    {
        file = fsbridge::fopen(path, "wb+");
    }

    if (!file)
    {
        LogPrintf("Unable to open file %s\n", path.string());
        return nullptr;
    }

    if (pos.nPos)
    {
        if (fseek(file, pos.nPos, SEEK_SET))
        {
            LogPrintf("Unable to seek to position %u of %s\n", pos.nPos,
                      path.string());
            fclose(file);
            return nullptr;
        }
    }
    return file;
}

bool BlockRead::openBlockFile(const std::string& file_path, int nFile)
{
	CDiskBlockPos pos(nFile, 0);
	file_ = OpenBlockFile(file_path, pos, true);
	if (!file_) 
	{
		// This error is logged in OpenBlockFile
		return false;;
	}
	pos_ = pos;

	return true;
}
bool BlockRead::loadBlock()
{
	CDiskBlockPos *dbp = &pos_;
	int nLoaded = 0;
	CBufferedFile blkdat(file_, 2 * ONE_MEGABYTE, ONE_MEGABYTE + 8, SER_DISK, CLIENT_VERSION);
	uint64_t nRewind = blkdat.GetPos();
	while (!blkdat.eof())
	{
		//boost::this_thread::interruption_point();
		blkdat.SetPos(nRewind);
		nRewind++;
		blkdat.SetLimit();
		uint64_t nSize = 0;
		uint32_t nSizeLegacy = 0;
		
		uint8_t buf[MESSAGE_START_SIZE];
		blkdat.FindByte(disk_magic_[0]);
		nRewind = blkdat.GetPos() + 1;
		blkdat >> FLATDATA(buf);
		if (memcmp(buf, disk_magic_.data(), MESSAGE_START_SIZE))
		{
			continue;
		}
		blkdat >> nSizeLegacy;
		if (nSizeLegacy == std::numeric_limits<uint32_t>::max())
		{
			blkdat >> nSize;
		}
		else
		{
			nSize = nSizeLegacy;
		}
		if (nSize < 80)
		{
			continue;
		}

		uint64_t nBlockPos = blkdat.GetPos();
		if (dbp)
		{
			dbp->nPos = nBlockPos;
		}
		blkdat.SetLimit(nBlockPos + nSize);
		blkdat.SetPos(nBlockPos);
		std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
		CBlock &block = *pblock;
		blkdat >> block;
		nLoaded ++;
		nRewind = blkdat.GetPos();
		uint256 hash = block.GetHash();

		std::deque<uint256> queue;
	}

	return nRewind > 0;
}

