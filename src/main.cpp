#include <bits/extc++.h>
#include <FAT32.h>

const char raw_img[] = {"sandbox/raw.img"};
int main()
{
	DefaultFS = new FileSystem(raw_img);
	return 0;
}