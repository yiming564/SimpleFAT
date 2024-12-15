#include <bits/extc++.h>
#include <FAT32.h>

const char raw_img[] = {"sandbox/raw.img"};
FileSystem FS(raw_img);
int main()
{
	auto Root = FS.OpenRoot();
	Root.Load();
	auto sub = Directory(Root.Child["SubDataLongName"]);
	sub.Load();
	auto Text = File(Root.Child["test.txt"]);
	Text.Load();
	printf("%s", Text.Raw());
	return 0;
}