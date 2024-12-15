#include <bits/extc++.h>
#include <FAT32.h>

const char raw_img[] = {"sandbox/raw.img"};
FileSystem FS(raw_img);

inline FileBase &PrintFile(FileBase &File, std::string path)
{
	printf("[%s]: \n", (path + "/" + File.Filename).c_str());
	auto raw = File.Raw();
	for (uint32_t i = 0; i < File.FileSize; i++)
		putchar(raw[i]);
	printf("\n===EOF===\n\n");
	return File;
}

void dfs(FileBase &File, std::string path = "")
{
	File.Load();
	if (File.Property.Directory)
	{
		for (auto &&[Filename, Child] : File.Child)
			if (Filename != "." && Filename != "..") dfs(Child, path + "/" + File.Filename);
	}
	else PrintFile(File, path);
}

int main()
{
	dfs(FS.OpenRoot());
	return 0;
}