#ifndef __FILE_H
#define __FILE_H

#include <stdint.h>
#include <sys/stat.h>

struct property_t
{
	uint8_t type;
	struct rwx
	{
		uint8_t r:1;
		uint8_t w:1;
		uint8_t x:1;
	} __attribute__((packed));
	rwx owner, group, others;
	time_t CreatedTime, ModifiedTime, VisitedTime;
} __attribute__((packed));

#endif