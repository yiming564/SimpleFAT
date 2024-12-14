#include <bits/extc++.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <FAT32.h>

inline bool IsFree(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return Cluster == 0x00000000; }
inline bool IsReserved(uint32_t Cluster) 	{ Cluster &= 0x0fffffff; return Cluster == 0x00000001; }
inline bool IsUsed(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return 0x00000002 <= Cluster && Cluster <= 0x0fffffef; }
inline bool IsUndefined(uint32_t Cluster)	{ Cluster &= 0x0fffffff; return 0x0ffffff0 <= Cluster && Cluster <= 0x0ffffff6; }
inline bool IsBad(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return Cluster == 0x0ffffff7; }
inline bool IsEOF(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return 0x0ffffff8 <= Cluster && Cluster <= 0x0fffffff; }

inline bool FileSystem::FileEntry::IsErased()	{ return SE.FileName[0] == 0xe5; }
inline bool FileSystem::FileEntry::IsNULL()		{ return SE.FileName[0] == 0x00; }
inline bool FileSystem::FileEntry::IsLFN()		{ return LE.MagicNumber == 0x0f; }

inline uint32_t FileSystem::ShortEntry::ClusterNumber()
{
	return (uint32_t)HighClusterNumber << 16 | LowClusterNumber;
}
inline time_t FileSystem::ShortEntry::CreatedTimestamp()
{
	tm t = {
		CreatedTime.HalfSecond * 2 + MilliSecond10 / 100,
		CreatedTime.Minute,
		CreatedTime.Hour,
		CreatedDate.Day,
		CreatedDate.Month - 1,
		CreatedDate.YearSince1980 + 80,
	};
	return timegm(&t);
}
inline time_t FileSystem::ShortEntry::ModifiedTimestamp()
{
	tm t = {
		ModifiedTime.HalfSecond * 2,
		ModifiedTime.Minute,
		ModifiedTime.Hour,
		ModifiedDate.Day,
		ModifiedDate.Month - 1,
		ModifiedDate.YearSince1980 + 80,
	};
	return timegm(&t);
}
inline time_t FileSystem::ShortEntry::VisitedTimestamp()
{
	tm t = {
		0, 0, 0, // Fuck you FAT32
		VisitedDate.Day,
		VisitedDate.Month - 1,
		VisitedDate.YearSince1980 + 80,
	};
	return timegm(&t);
}
inline char16_t *FileSystem::LongEntry::FileName(char16_t *buf)
{
	__builtin_memcpy(&buf[0], FileName1, sizeof(FileName1));
	__builtin_memcpy(&buf[5], FileName2, sizeof(FileName2));
	__builtin_memcpy(&buf[11], FileName3, sizeof(FileName3));
	return buf;
}
inline void *FileSystem::OffsetBySector(uint32_t Sectors)
{
	return (uint8_t *)Data + Sectors * BS->BytesPerSector;
}
inline void *FileSystem::OffsetByCluster(uint32_t Clusters)
{
	return (uint8_t *)Data + (Clusters - 2) * BS->BytesPerSector * BS->SectorsPerCluster;
}
inline void *FileSystem::OffsetBySector(void *Base, uint32_t Sectors)
{
	return (uint8_t *)Base + Sectors * BS->BytesPerSector;
}
inline void *FileSystem::OffsetByCluster(void *Base, uint32_t Clusters)
{
	return (uint8_t *)Base + (Clusters - 2) * BS->BytesPerSector * BS->SectorsPerCluster;
}
FileSystem::FileSystem(const char *FileName)
{
	printf("Loading image file: %s...\n", FileName);
	assert((fd = open(FileName, O_RDWR)) != -1), fstat(fd, &st);
	assert((img = (uint8_t *)mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) != MAP_FAILED);
	BS = (BootSector *)img;
	BytesPerCluster = BS->SectorsPerCluster * BS->BytesPerSector;
	FAT = (uint32_t *)OffsetBySector(img, BS->ReservedSector);
	FATBackup = (uint32_t *)OffsetBySector(img, BS->ReservedSector + BS->SectorsPerFAT);
	Data = (uint8_t *)OffsetBySector(FATBackup, BS->SectorsPerFAT);
	printf("Image size: %.lf MiB.\n", st.st_size / 1048576.0);
}
FileSystem::~FileSystem()
{
	printf("Closing file...\n");
	munmap(img, st.st_size), close(fd);
	printf("Image Closed.\n");
}
FileSystem *DefaultFS = NULL;
struct FileBase
{
protected:
	FileSystem *FS;
	uint8_t *Data;
	uint32_t ClusterSize;
public:
	uint32_t ClusterBase;
	std::u16string FileName;
	uint32_t FileSize;
	time_t CreatedTime, ModifiedTime, VisitedTime;
	FileBase() : FS(DefaultFS), Data(NULL), ClusterSize(0), ClusterBase(0xffffffff), FileSize(0) {}
	~FileBase() { if (Data) free(Data), Data = NULL; }
	FileBase &LoadByCluster(uint32_t Base)
	{
		for (uint32_t i = Base; ; i = FS->FAT[i])
		{
			ClusterSize++;
			if (!IsUsed(FS->FAT[i])) break;
		}
		Data = (uint8_t *)calloc(ClusterSize, FS->BytesPerCluster);
		for (uint32_t i = Base, cnt = 0; ; i = FS->FAT[i], cnt++)
		{
			memcpy(Data + cnt * FS->BytesPerCluster, FS->OffsetByCluster(i), FS->BytesPerCluster);
			if (!IsUsed(FS->FAT[i])) break;
		}
		return *this;
	}
	FileBase &PrintDebugInfo()
	{
		printf(
			"File size: %d Bytes\n"
			"Created time:  %s"
			"Modified time: %s"
			"Visited time:  %s",
			FileSize,
			ctime(&CreatedTime), ctime(&ModifiedTime), ctime(&VisitedTime)
		);
		return *this;
	}
};
struct Directory : public FileBase
{
protected:
	FileSystem::FileEntry *FDT;
public:
	Directory() : FDT(NULL) {};
	Directory(const FileBase &raw) { *this = raw, FDT = (decltype(FDT))Data; };
	std::vector<FileBase> &OpenDir()
	{
		assert(FDT);
		auto sub = new std::vector<FileBase>;
		FileBase Child;
		char16_t filename[MAX_FILENAME];
		memset(filename, 0, sizeof(filename));
		for (auto i = FDT; (void *)i < (void *)(Data + FileSize) && !i->IsNULL(); i++)
		{
			if (i->IsErased()) continue;
			if (i->IsLFN())
			{
				auto &&Entry = i->LE;
				printf("Checksum %d: ID %d\n", Entry.Checksum, Entry.Property.ID);
				Entry.FileName(&filename[(Entry.Property.ID - 1) * 13]);
			}
			else
			{
				auto &&Entry = i->SE;
				Child.ClusterBase = MK_U32(Entry.HighClusterNumber, Entry.LowClusterNumber);
				Child.FileSize = Entry.FileSize;
				Child.CreatedTime = Entry.CreatedTimestamp();
				Child.ModifiedTime = Entry.ModifiedTimestamp();
				Child.VisitedTime = Entry.VisitedTimestamp();
				printf("FileName: %s\n", Entry.FileName);
				sub->emplace_back(Child.PrintDebugInfo());
				memset(filename, 0, sizeof(filename));
			}
		}
		return *sub;
	}
	Directory &LoadRoot()
	{
		LoadByCluster(FS->BS->RootClusterNumber);
		FileSize = ClusterSize * FS->BytesPerCluster;
		FDT = (decltype(FDT))Data;
		return *this;
	}
};