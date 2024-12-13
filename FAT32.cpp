#include <bits/extc++.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

const char FileName[] = {"sandbox/raw.img"};
inline bool IsFree(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return Cluster == 0x00000000; }
inline bool IsReserved(uint32_t Cluster) 	{ Cluster &= 0x0fffffff; return Cluster == 0x00000001; }
inline bool IsUsed(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return 0x00000002 <= Cluster && Cluster <= 0x0fffffef; }
inline bool IsUndefined(uint32_t Cluster)	{ Cluster &= 0x0fffffff; return 0x0ffffff0 <= Cluster && Cluster <= 0x0ffffff6; }
inline bool IsBad(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return Cluster == 0x0ffffff7; }
inline bool IsEOF(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return 0x0ffffff8 <= Cluster && Cluster <= 0x0fffffff; }
//// inline bool IsValid(uint32_t Cluster) 		{ return Cluster != 0x00000001 && Cluster <= 0x0fffffef; }
struct FileSystem
{
private:
	int fd; struct stat st;
public:
	uint8_t *img, *Data;
	struct BootSector
	{
		uint8_t jmp[3];
		char oem_id[8];
		uint16_t BytesPerSector;
		uint8_t SectorsPerCluster;
		uint16_t ReservedSector;
		uint8_t NumberOfFAT;
	private:
		uint16_t RootEntries16;
		uint16_t SmallSector16;
	public:
		uint8_t MediaDescriptor;
	private:
		uint16_t SectorsPerFAT16;
	public:
		uint16_t SectorsPerTrack;
		uint16_t NumberOfHead;
		uint32_t HiddenSector;
		uint32_t LargeSector;
		uint32_t SectorsPerFAT;
		uint16_t ExtendedFlag;
		uint16_t FileSystemVersion;
		uint32_t RootClusterNumber;
		uint16_t FSINFOSectorNumber;
		uint16_t BackupBootSectorNumber;
	private:
		uint8_t Reserved1[12];
	public:
		uint8_t PhysicalDriveNumber;
		uint8_t Reserved2;
		uint8_t ExtendedBootSignature;
		uint32_t VolumeSerialNumber;
		char VolumeLabel[11];
		char SystemID[8];
		uint8_t BootLoader[420];
		uint16_t MagicNumber;
	} __attribute__((packed)) *BS;
	struct ShortEntry
	{
		struct ShortEntryProperty
		{
			uint8_t ReadOnly:1;
			uint8_t Hide:1;
			uint8_t SystemFile:1;
			uint8_t Volume:1;
			uint8_t Directory:1;
			uint8_t Archive:1;
		} __attribute__((packed));
		struct Time
		{
			uint8_t HalfSecond:5;
			uint8_t Minute:6;
			uint8_t Hour:5;
		} __attribute__((packed));
		struct Date
		{
			uint8_t Day:5;
			uint8_t Month:4;
			uint8_t YearSince1980:7;
		} __attribute__((packed));
		char FileName[8];
		char ExtName[3];
		ShortEntryProperty Property;
	private:
		uint8_t Reserved1;
	public:
		uint8_t MilliSecond10;
		Time CreatedTime;
		Date CreatedDate;
		Date VisitedDate;
		uint16_t HighClusterNumber;
		Time ModifiedTime;
		Date ModifiedDate;
		uint16_t LowClusterNumber;
		uint32_t FileSize;
		inline uint32_t ClusterNumber() { return (uint32_t)HighClusterNumber << 16 | LowClusterNumber; }
	} __attribute__((packed));
	struct LongEntry
	{
		struct LongEntryProperty
		{
			uint8_t ID:5;
		private:
			uint8_t Reserved1:1;
		public:
			uint8_t IsLastEntry:1;
		private:
			uint8_t Reserved2:1;
		} __attribute__((packed));
		LongEntryProperty Property;
		char16_t FileName1[5];
		uint8_t MagicNumber; 		// 0x0f
	private:
		uint8_t Reserved1;
	public:
		uint8_t Checksum;
		char16_t FileName2[6];
		uint16_t LowClusterNumber;	// Always be 0x0000.
		char16_t FileName3[2];
	} __attribute__((packed));
	union FileEntry
	{
		ShortEntry SE;
		LongEntry LE;
	} __attribute__((packed));
	uint32_t *FAT, *FATBackup;
	uint32_t BytesPerCluster;
	inline void *OffsetBySector(uint32_t Sectors) { return (uint8_t *)Data + Sectors * BS->BytesPerSector; }
	inline void *OffsetByCluster(uint32_t Clusters) { return (uint8_t *)Data + Clusters * BS->BytesPerSector * BS->SectorsPerCluster; }
	inline void *OffsetBySector(void *Base, uint32_t Sectors) { return (uint8_t *)Base + Sectors * BS->BytesPerSector; }
	inline void *OffsetByCluster(void *Base, uint32_t Clusters) { return (uint8_t *)Base + Clusters * BS->BytesPerSector * BS->SectorsPerCluster; }
	FileSystem(const char *FileName)
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
	~FileSystem()
	{
		printf("Closing file...\n");
		munmap(img, st.st_size), close(fd);
		printf("Image Closed.\n");
	}
};
FileSystem DefaultFS(FileName);
struct FileBase
{
private:
	FileSystem *FS;
	uint8_t *Data;
	uint32_t ClusterSize, ClusterBase;
	FileSystem::FileEntry *FDT;
public:
	uint32_t FileSize;
	FileBase() : FS(&DefaultFS), Data(NULL), FileSize(0), ClusterSize(0) {}
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
		FDT = (FileSystem::FileEntry *)Data;
		return *this;
	}
} Root;
struct Directory : public FileBase
{
private:
	FileSystem::FileEntry *FDT;
public:
	Directory() : FDT(NULL) {};
	Directory(const FileBase &raw) { *this = raw; };
	std::vector<FileBase> OpenDir()
	{
		
	}
};

int main()
{
	Root.LoadByCluster(0);
	return 0;
}