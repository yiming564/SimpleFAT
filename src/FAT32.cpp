#include <bits/extc++.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <FAT32.h>
#include <config.h>

inline bool IsFree(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return Cluster == 0x00000000; }
inline bool IsReserved(uint32_t Cluster) 	{ Cluster &= 0x0fffffff; return Cluster == 0x00000001; }
inline bool IsUsed(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return 0x00000002 <= Cluster && Cluster <= 0x0fffffef; }
inline bool IsUndefined(uint32_t Cluster)	{ Cluster &= 0x0fffffff; return 0x0ffffff0 <= Cluster && Cluster <= 0x0ffffff6; }
inline bool IsBad(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return Cluster == 0x0ffffff7; }
inline bool IsEOF(uint32_t Cluster) 		{ Cluster &= 0x0fffffff; return 0x0ffffff8 <= Cluster && Cluster <= 0x0fffffff; }

FileSystem::FileSystem(const char *Filename)
{
	printf("Loading image file: %s...\n", Filename);
	assert((fd = open(Filename, O_RDWR)) != -1), fstat(fd, &st);
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
FileBase &FileSystem::OpenRoot()
{
	auto Root = new FileBase(this);
	Root->ClusterBase = BS->RootClusterNumber;
	Root->Property.Directory = 1;
	return *Root;
}

inline uint8_t LFNChksum(const uint8_t *Filename)
{
	uint8_t hash = 0;
	for (int i = 0; i < 11; i++)
		hash = (hash & 1 ? 0x80 : 0) + (hash >> 1) + *Filename++;
	return hash;
}
std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8_utf16_cvt;
FileBase::FileBase() : Data(NULL), ClusterBase(2), FileSize(0) {}
FileBase::FileBase(FileSystem *fs) : FS(fs) { FileBase(); }
FileBase::~FileBase() {}
FileBase &FileBase::Load()
{
	uint32_t cnt = 0;

	// Load file to memory
	for (uint32_t i = ClusterBase; ; i = FS->FAT[i])
	{
		cnt++;
		if (!IsUsed(FS->FAT[i])) break;
	}
	Data = (uint8_t *)calloc(cnt, FS->BytesPerCluster);
	for (uint32_t i = ClusterBase, cnt = 0; ; i = FS->FAT[i], cnt++)
	{
		memcpy(Data + cnt * FS->BytesPerCluster, FS->OffsetByCluster(i), FS->BytesPerCluster);
		if (!IsUsed(FS->FAT[i])) break;
	}
	if (!Property.Directory) return *this;

	// Analyzing directory
	FileSize = cnt * FS->BytesPerCluster;
	FDT = (decltype(FDT))Data;
	FileBase File(FS);
	char16_t buf[MAX_FILENAME]; uint8_t hash = 0;
	memset(buf, 0, sizeof(buf));
	for (auto i = FDT; (void *)i < (void *)(Data + FileSize) && !i->IsNULL(); i++)
	{
		if (i->IsErased()) continue;
		if (i->IsLFN())
		{
			auto &&Entry = i->LE;
			hash = Entry.Checksum;
			Entry.Filename(&buf[(Entry.Property.ID - 1) * 13]);
		}
		else
		{
			auto &&Entry = i->SE;
			auto s = std::u16string(buf);
			File.Property = Entry.Property;
			File.Filename = (hash == LFNChksum((uint8_t *)Entry.Filename1)) ? utf8_utf16_cvt.to_bytes(s) : Entry.Filename();
			File.FileSize = Entry.FileSize;
			File.ClusterBase = MK_U32(Entry.HighClusterNumber, Entry.LowClusterNumber);
			File.CreatedTime = Entry.CreatedTimestamp();
			File.ModifiedTime = Entry.ModifiedTimestamp();
			File.VisitedTime = Entry.VisitedTimestamp();
			Child[File.Filename] = File;
			memset(buf, 0, sizeof(buf)), hash = 0;
		}
	}
	return *this;
}
FileBase &FileBase::PrintDebugInfo()
{
	printf(
		"Filename: %s\n"
		"File size: %d Bytes\n"
		"Created time:  %s"
		"Modified time: %s"
		"Visited time:  %s",
		Filename.c_str(),
		FileSize,
		ctime(&CreatedTime),
		ctime(&ModifiedTime),
		ctime(&VisitedTime)
	);
	return *this;
}