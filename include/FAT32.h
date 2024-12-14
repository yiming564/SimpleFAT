#ifndef __FAT32_H
#define __FAT32_H

#include <stdint.h>
#include <sys/stat.h>
#include <config.h>

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
		struct alignas(1) Time
		{
			uint8_t HalfSecond:5;
			uint8_t Minute:6;
			uint8_t Hour:5;
		};
		struct alignas(1) Date
		{
			uint8_t Day:5;
			uint8_t Month:4;
			uint8_t YearSince1980:7;
		};
		char FileName[8];
		char ExtName[3];
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
		inline uint32_t ClusterNumber();
		inline time_t CreatedTimestamp();
		inline time_t ModifiedTimestamp();
		inline time_t VisitedTimestamp();
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
		inline char16_t *FileName(char16_t *buf);
	} __attribute__((packed));
	union FileEntry
	{
		ShortEntry SE;
		LongEntry LE;
		inline bool IsErased();
		inline bool IsNULL();
		inline bool IsLFN();
	} __attribute__((packed));
	uint32_t *FAT, *FATBackup;
	uint32_t BytesPerCluster;
	inline void *OffsetBySector(uint32_t Sectors);
	inline void *OffsetByCluster(uint32_t Clusters);
	inline void *OffsetBySector(void *Base, uint32_t Sectors);
	inline void *OffsetByCluster(void *Base, uint32_t Clusters);
	FileSystem(const char *FileName);
	~FileSystem();
};
extern FileSystem *DefaultFS;

#endif