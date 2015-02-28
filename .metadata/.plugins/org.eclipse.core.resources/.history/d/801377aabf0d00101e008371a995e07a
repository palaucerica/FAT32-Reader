#define FUSE_USE_VERSION 25
#define D_FILE_OFFSET_BITS 64
#define _FILE_OFFSET_BITS 64

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_LONG_NAME		(ATTR_READ_ONLY |ATTR_HIDDEN |ATTR_SYSTEM |ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK	(ATTR_READ_ONLY |ATTR_HIDDEN |ATTR_SYSTEM |ATTR_VOLUME_ID |ATTR_DIRECTORY |ATTR_ARCHIVE)


//-------------------Clusters--------------------
#define FreeClust      0x00000000
#define RervClust      0x00000001
#define ResvClust1     0xFFFFFFF0
#define ResvClust2     0xFFFFFFF6
#define WrongClust     0XFFFFFFF7
#define FirstClust     0xFFFFFFF8
#define LastCLust      0xFFFFFFFF
//------------------Liberar los primero cuatro ceros----------------------
#define FourReserved  0x0FFFFFFF


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include <time.h>
#include <ctype.h>
#include <fuse.h>
#include <errno.h>
#include <fcntl.h>



//estructura BPP
typedef struct BPB
{
	unsigned char BS_jmpBoot[3];
	unsigned char BS_OEMName[8];
	unsigned short BPB_BytsPerSec;
	unsigned char BPB_SecPerClus;
	unsigned short BPB_RsvdSecCnt;
	unsigned char BPB_NumFATs;
	unsigned short BPB_RootEntCnt;
	unsigned short BPB_TotSec16;
	unsigned char BPB_Media;
	unsigned short BPB_FATSz16;
	unsigned short BPB_SecPerTrk;
	unsigned short BPB_NumHeads;
	unsigned int BPB_HiddSec;
	unsigned int BPB_TotSec32;
	unsigned int BPB_FATSz32;
	unsigned short BPB_ExtFlags;
	unsigned short BPB_FSVer;
	unsigned int BPB_RootClus;
	unsigned short BPB_FSInfo;
	unsigned short BPB_BkBootSec;
	unsigned char BPB_Reserved[12];
	unsigned char BS_DrvNum;
	unsigned char BS_Reserved1;
	unsigned char BS_BootSig;
	unsigned int BS_VolID;
	unsigned char BS_VolLab[11];
	unsigned char BS_FilSysType[8];
}__attribute__((packed)) BPB;

typedef union DirEntry
{

	struct
	{
		unsigned char DIR_Name[11];			//Short name.
		unsigned char DIR_Attr;				//File attributes:
		unsigned char DIR_NTRes;			//Reserved for us e by Windows NT.
		unsigned char DIR_CrtTimeTenth;		//Millisecond s tamp at file creation time.
		unsigned short DIR_CrtTime;			//Time file was created.
		unsigned short DIR_CrtDate;			//Date file was created.
		unsigned short DIR_LstAccDate;		//Last access date.
		unsigned short DIR_FstClusHI;		//High word of this entrys first cluster number
		unsigned short DIR_WrtTime;			//Time of last write.
		unsigned short DIR_WrtDate;			//Date of last write.
		unsigned short DIR_FstClusLO;		//Low word of this entrys first cluster number.
		unsigned int DIR_FileSize;			//32-bit DWORD holding this files size in bytes.
	}__attribute__((packed)) ShortDirEntry;


	struct
	{
        	unsigned char LDIR_Ord;			//The order of this entry in the sequence of long dir
										//entries associated with the short dir entry at the end
										//of the long dir set.
		unsigned char LDIR_Name1[10];	//Characters 1-5 of the long-name sub-component in
										//this dir entry.
		unsigned char LDIR_Attr;		//Attributes
		unsigned char LDIR_Type;
		unsigned char LDIR_Chksum;		//Checksum
		unsigned char LDIR_Name2[12];	//Characters 6-11
		unsigned short LDIR_FstClusLO;	//Must be ZERO.
		unsigned char LDIR_Name3[4];	//Characters 12-13

	}__attribute__((packed)) LongDirEntry;
}__attribute__((packed)) DirEntry;

typedef struct MyEntry
	{
		union DirEntry data;
		char name[255];
	}__attribute__((packed)) MyEntry;

FILE* file;
struct BPB boot;
int FirstDataSector;
char myChksum;
int currentCluster;
int primVez;
