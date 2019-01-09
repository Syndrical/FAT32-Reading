#ifndef DIR_H
#define DIR_H

#include <inttypes.h>

#pragma pack(push)
#pragma pack(1)
struct dir_struct {
	unsigned char DIR_Name[11];
	unsigned char DIR_Attr;
	unsigned char DIR_NTRes;
	unsigned char DIR_CrtTimeTenth;
	unsigned short DIR_CrtTime;
	unsigned short DIR_CrtDate;
	unsigned short DIR_LstAccDate;
	unsigned short DIR_FstClusHI;
	unsigned short DIR_WrtTime;
	unsigned short DIR_WrtDate;
	unsigned short DIR_FstClusLO;
    unsigned long DIR_FileSize;
};
#pragma pack(pop)

typedef struct dir_struct DIR;

#endif