#ifndef __FAT32_H
#define __FAT32_H

#include <stdint.h>
#include <sys/stat.h>

template <typename K, typename V> using hashmap = std::map<K, V>;

#define MK_U32(x, y)	((x) << 16 | (y))

inline bool IsFree(uint32_t Cluster);
inline bool IsReserved(uint32_t Cluster);
inline bool IsUsed(uint32_t Cluster);
inline bool IsUndefined(uint32_t Cluster);
inline bool IsBad(uint32_t Cluster);
inline bool IsEOF(uint32_t Cluster);

struct FileSystem
{
private:
	int fd; struct stat st;
public:
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
		char Filename1[8];
		char Filename2[3];
		ShortEntryProperty Property;
		uint8_t NTReserved;
		uint8_t MilliSecond10;
		Time CreatedTime;
		Date CreatedDate;
		Date VisitedDate;
		uint16_t HighClusterNumber;
		Time ModifiedTime;
		Date ModifiedDate;
		uint16_t LowClusterNumber;
		uint32_t FileSize;
		inline uint32_t ClusterNumber()
		{
			return (uint32_t)HighClusterNumber << 16 | LowClusterNumber;
		}
		inline time_t CreatedTimestamp()
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
		inline time_t ModifiedTimestamp()
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
		inline time_t VisitedTimestamp()
		{
			tm t = {
				0, 0, 0, 			// FAT32 不记录访问的时间
				VisitedDate.Day,
				VisitedDate.Month - 1,
				VisitedDate.YearSince1980 + 80,
			};
			return timegm(&t);
		}
		inline std::string &Filename()
		{
			auto s = new std::string();
			int i = 10;
			for (; i >= 8 && Filename1[i] == ' '; i--);
			for (; i >= 8; i--)	*s = Filename1[i] + *s;
			if (!s->empty())	*s = '.' + *s;
			for (; i >= 0 && Filename1[i] == ' '; i--);
			for (; i >= 0; i--)	*s = Filename1[i] + *s;
			return *s;
		}
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
		char16_t Filename1[5];
		uint8_t MagicNumber; 		// 0x0f
	private:
		uint8_t Reserved1;
	public:
		uint8_t Checksum;
		char16_t Filename2[6];
		uint16_t LowClusterNumber;	// Always be 0x0000.
		char16_t Filename3[2];
		inline char16_t *Filename(char16_t *buf)
		{
			__builtin_memcpy(&buf[0], Filename1, sizeof(Filename1));
			__builtin_memcpy(&buf[5], Filename2, sizeof(Filename2));
			__builtin_memcpy(&buf[11], Filename3, sizeof(Filename3));
			return buf;
		}
	} __attribute__((packed));
	union FileEntry
	{
		ShortEntry SE;
		LongEntry LE;
		inline bool IsDirLink()	{ return SE.Filename1[0] == 0x2e; }
		inline bool IsErased()	{ return SE.Filename1[0] == 0xe5; }
		inline bool IsNULL()	{ return SE.Filename1[0] == 0x00; }
		inline bool IsLFN()		{ return LE.MagicNumber == 0x0f; }
	} __attribute__((packed));

	uint8_t *img, *Data;
	uint32_t *FAT, *FATBackup;
	uint32_t BytesPerCluster;
	FileSystem(const char *Filename);
	~FileSystem();
	inline void *OffsetBySector(uint32_t Sectors)				{ return (uint8_t *)Data + Sectors * BS->BytesPerSector; }
	inline void *OffsetByCluster(uint32_t Clusters)				{ return (uint8_t *)Data + (Clusters - 2) * BS->BytesPerSector * BS->SectorsPerCluster;	}
	inline void *OffsetBySector(void *Base, uint32_t Sectors)	{ return (uint8_t *)Base + Sectors * BS->BytesPerSector; }
	inline void *OffsetByCluster(void *Base, uint32_t Clusters)	{ return (uint8_t *)Base + (Clusters - 2) * BS->BytesPerSector * BS->SectorsPerCluster; }
	struct FileBase &OpenRoot();
};

struct FileBase
{
protected:
	FileSystem *FS;
	uint8_t *Data;
	FileSystem::FileEntry *FDT;
public:
	FileSystem::ShortEntry::ShortEntryProperty Property;
	uint32_t ClusterBase;
	uint32_t FileSize;
	std::string Filename;	// 微软你 TM 用什么 UTF-16，害得我还要转码。
	time_t CreatedTime, ModifiedTime, VisitedTime;
	hashmap<std::string, FileBase> Child;
	FileBase();
	FileBase(FileSystem *fs);
	~FileBase();
	FileBase &Load();
	FileBase &PrintDebugInfo();
	inline const uint8_t *Raw() const { return Data; }
};

#endif