#include <bits/extc++.h>
#include <FAT32.h>

const char raw_img[] = {"sandbox/raw.img"};
FileSystem FS(raw_img);
int main()
{
	auto Root = FS.OpenRoot();
	auto sub = Directory(Root.Load().Child["SubDataLongName"]);
	sub.Load();
	return 0;
}