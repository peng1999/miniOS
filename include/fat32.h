/**********************************************************
*	fat32.h       //added by mingxuan 2019-5-17
***********************************************************/

#ifndef FAT32_H
#define FAT32_H

#include "type.h"

#define TRUE 1//是
#define FALSE 0//否

#define OK 1//正常返回
#define SYSERROR 2//系统错误
#define VDISKERROR 3//虚拟磁盘错误
#define INSUFFICIENTSPACE 4//虚拟磁盘空间不足
#define WRONGPATH 5//路径有误
#define NAMEEXIST 6//文件或目录名已存在
#define ACCESSDENIED 7//读写权限不对拒绝访问
#define END_OF_DIR 8 // 已读到目录结尾

#define C 1//创建	//added by mingxuan 2019-5-18
#define R 5//读
#define W 6//写
#define RW 2 //读写	//added by mingxuan 2019-5-18

#define F 1//文件
#define D 0//目录

#define FAT_END 0x0FFFFFFF

typedef int STATE;//函数返回状态

// typedef unsigned char   BYTE;//字节
// typedef unsigned short  WORD;//双字节
// typedef unsigned long   DWORD;//四字节
// typedef unsigned int    UINT;//无符号整型
// typedef char		    CHAR;//字符类型

// typedef unsigned char*  PBYTE;
// typedef unsigned short* PWORD;
// typedef unsigned long*  PDWORD;//四字节指针
// typedef unsigned int*   PUINT;//无符号整型指针
// typedef char*           PCHAR;//字符指针

// typedef struct super_block SUPER_BLOCK;
// typedef struct spinlock SPIN_LOCK;

typedef struct//定义目录项：占32个字节
{
	BYTE filename[8];//文件名：占8个字节
	BYTE extension[3];//扩展名：占3个字节
	BYTE proByte;//属性字节：占1个字节
	BYTE sysReserved;//系统保留：占1个字节
	BYTE createMsecond;//创建时间的10毫秒位：占1个字节
	WORD createTime;//文件创建时间：占2个字节
	WORD createDate;//文件创建日期：占2个字节
	WORD lastAccessDate;//文件最后访问日期：占2个字节
	WORD highClusterNum;//文件的起始簇号的高16位：占2个字节
	WORD lastModifiedTime;//文件的最近修改时间
	WORD lastModifiedDate;//文件的最近修改日期
	WORD lowClusterNum;//文件的起始簇号的低16位：占2个字节
	DWORD filelength;//文件的长度：占4个字节
}Record,*PRecord;

typedef struct//定义长文件名目录项：占32个字节
{
	BYTE proByte;//属性字节
	BYTE name1[10];//长文件名第1段
	BYTE longNameFlag;//长文件名目录项标志，取值0FH
	BYTE sysReserved;//系统保留
	BYTE name2[19];//长文件名第2段
}LONGNAME;

typedef struct{//文件类型
	CHAR parent[256];//父路径
	CHAR name[256];//文件名
	DWORD start;//起始地址
	DWORD off;//总偏移量,以字节为单位
	DWORD size;//文件的大小，以字节为单位
	UINT flag;//文件读写标志
}File,*PFile;

typedef struct{//动态数组元素类型，用于存储文件或目录的基本信息
	CHAR fullpath[256];//绝对路径
	CHAR name[256];//文件名或目录名
	UINT tag;//1表示文件，0表示目录
}DArrayElem;

typedef struct{//动态数组类型
	DArrayElem *base;//数组基地址
	UINT offset;//读取数组时的偏移量
	UINT used;//数组当前已使用的容量
	UINT capacity;//数组的总容量
	UINT increment;//当数组容量不足时，动态增长的步长
}DArray;

typedef struct{//文件或目录属性类型
	BYTE  type;//0x10表示目录，否则表示文件
	CHAR  name[256];//文件或目录名称
	CHAR  location[256];//文件或目录的位置，绝对路径
	DWORD size;//文件的大小或整个目录中(包括子目录中)的文件总大小
	CHAR  createTime[20];//创建时间型如：yyyy-MM-dd hh:mm:ss类型
	CHAR  lastModifiedTime[20];//最后修改时间
	union{
		CHAR lastAccessDate[11];//最后访问时间，当type为文件值有效
		UINT contain[2];//目录中的文件个数和子目录的个数，当type为目录时有效
	}share;
}Properties;

typedef struct{
  DWORD clusterIndex;
  DWORD sectorIndex;
  DWORD offset;
} DirEntry;

STATE CreateVDisk(DWORD size);
STATE FormatVDisk(PCHAR path,PCHAR volumelabel);
STATE LoadVDisk(PCHAR path);
STATE CreateDir(SUPER_BLOCK *psb, PCHAR dirname);
STATE CreateFile(SUPER_BLOCK *psb, PCHAR filename);
STATE OpenFile(SUPER_BLOCK *psb, PCHAR filename,UINT mode);
STATE CloseFile(SUPER_BLOCK *psb, int fd);
STATE OpenDir(PCHAR dirname);
STATE ReadDir(SUPER_BLOCK *psb, PCHAR dirname, DWORD dir[3], char* filename);
//added by ran
int fat32_chdir(SUPER_BLOCK *psb, const char *path);

STATE ReadFile(SUPER_BLOCK *psb, int fd,BYTE buf[], DWORD length);
STATE WriteFile(SUPER_BLOCK *psb, int fd,BYTE buf[],DWORD length);
STATE LSeek(int, int, int);
STATE CopyFileIn(PCHAR sfilename,PCHAR dfilename);
STATE CopyFileOut(PCHAR sfilename,PCHAR dfilename);
STATE DeleteFile(SUPER_BLOCK *psb, PCHAR filename);
STATE DeleteDir(SUPER_BLOCK *psb, PCHAR dirname);
STATE ListAll(PCHAR dirname,DArray *array);
STATE IsFile(SUPER_BLOCK *psb, PCHAR path,PUINT tag);
STATE GetFileLength(PCHAR filename,PDWORD length);
STATE Rename(PCHAR path,PCHAR newname);
STATE CopyFile(PCHAR sfilename,PCHAR dpath);
STATE CopyDir(PCHAR sdirname,PCHAR dpath);
STATE Move(PCHAR sfilename,PCHAR dpath);
STATE GetProperty(PCHAR filename,Properties *properties);
STATE GetParenetDir(PCHAR name,PCHAR parentDir);
STATE GetVDiskFreeSpace(PDWORD left);
STATE GetVDiskSize(PDWORD totalsize);
STATE GetCurrentPath(PCHAR path);

DArray* InitDArray(UINT initialCapacity,UINT capacityIncrement);
void AddElement(DArray *array,DArrayElem element);
DArrayElem* NextElement(DArray *array);
void DestroyDArray(DArray *array);

void TimeToBytes(WORD result[]);//t是一个WORD类型的数组，长度为2，第0个元素为日期，第1个元素为时间
void BytesToTime(WORD date,WORD time,WORD result[]);//t是一个表示时间的WORD数组，长度为6，分别表示年、月、日、时、分、秒
void TimeToString(WORD result[],PCHAR timestring);
STATE IsFullPath(PCHAR path);//判断path是否是一个绝对路径
void ToFullPath(PCHAR path,PCHAR fullpath);//将path转换成绝对路径，用fullpath返回
void GetParentFromPath(PCHAR fullpath,PCHAR parent);//从一个绝对路径中得到它的父路径(父目录)
void GetNameFromPath(PCHAR path,PCHAR name);//从一个路径中得到文件或目录的名称
void MakeFullPath(PCHAR parent,PCHAR name,PCHAR fullpath);//将父路径和文件名组合成一个绝对路径
void FormatFileNameAndExt(PCHAR filename,PCHAR name,PCHAR ext);//将一个文件名转换成带空格的文件名和后缀名的形式，为了写入目录项。
void FormatDirNameAndExt(PCHAR dirname,PCHAR name,PCHAR ext);//将一个目录名转换成带空格的目录名和后缀的形式，为了写入目录项。
void ChangeCurrentPath(PCHAR addpath);

void GetNameFromRecord(Record record,PCHAR fullname);//从目录项中得到文件或目录的全名
STATE PathToCluster(SUPER_BLOCK *psb, PCHAR path, PDWORD cluster);//将抽象的路径名转换成簇号
STATE FindSpaceInDir(SUPER_BLOCK *psb, DWORD parentCluster,PCHAR name,PDWORD sectorIndex,PDWORD off_in_sector);//在指定的目录中寻找空的目录项
STATE FindClusterForDir(SUPER_BLOCK *psb, PDWORD pcluster);//为目录分配簇
STATE ReadRecord(SUPER_BLOCK *psb, DWORD parentCluster,PCHAR name,PRecord record,PDWORD sectorIndex,PDWORD off_in_sector);//获得指定的目录项的位置(偏移量)
STATE ReadNextRecord(SUPER_BLOCK *psb, DWORD parentCluster,PDWORD sectorIndex,PDWORD off_in_sector,PRecord record);
STATE WriteRecord(SUPER_BLOCK *psb, Record record,DWORD sectorIndex,DWORD off_in_sector);
STATE FindClusterForFile(SUPER_BLOCK *psb, DWORD totalClusters,PDWORD clusters);//为一个文件分配totalClusters个簇
STATE WriteFAT(SUPER_BLOCK *psb, DWORD totalClusters,PDWORD clusters);//写FAT1和FAT2
STATE GetNextCluster(SUPER_BLOCK *psb, DWORD curClusterIndex,PDWORD nextClusterIndex);
STATE AddCluster(SUPER_BLOCK *psb, DWORD startCluster,DWORD num);//cluster表示该文件或目录所在目录的簇,num表示增加几个簇
void CreateRecord(PCHAR filename,BYTE type,DWORD startCluster,DWORD size,PRecord precord);//创建一个目录项
void GetFileOffset(SUPER_BLOCK *psb, PFile pfile,PDWORD sectorIndex,PDWORD off_in_sector,PUINT isLastSector);//将文件当前位置转换成相对虚拟磁盘文件起始地址的偏移量，便于读写。
STATE AllotClustersForEmptyFile(SUPER_BLOCK *psb, PFile pfile,DWORD size);//为一个打开的空文件分配簇。
STATE NeedMoreCluster(SUPER_BLOCK *psb, PFile pfile,DWORD size,PDWORD number);//判断是否需要更多的簇，如果需要就返回需要的簇的个数
STATE ClearRecord(SUPER_BLOCK *psb, DWORD parentCluster,PCHAR name,PDWORD startCluster);//删除记录项
void ClearFATs(SUPER_BLOCK *psb, DWORD startcluster);//删除簇号
STATE FindNextRecord(SUPER_BLOCK *psb, PDWORD cluster,PDWORD off,PCHAR name,PUINT tag);
void DeleteAllRecord(SUPER_BLOCK *psb, DWORD startCluster);
void GetContent(DWORD startCluster,PDWORD size,PDWORD files,PDWORD folders);
void ClearClusters(SUPER_BLOCK *psb, DWORD cluster);
void ReadSector(int fat32_dev, BYTE*,DWORD sectorIndex);
void WriteSector(int fat32_dev, BYTE*,DWORD sectorIndex);
void GetNextSector(SUPER_BLOCK *psb, PFile pfile,DWORD curSectorIndex,PDWORD nextSectorIndex,PUINT isLastSector);

int rw_sector_fat(int io_type, int dev, unsigned long long pos, int bytes, int proc_nr, void* buf);
int rw_sector_sched_fat(int io_type, int dev, int pos, int bytes, int proc_nr, void* buf);

void debug(char * s);
void deint(int t);
void DisErrorInfo(STATE state);

int sys_CreateFile(void *uesp);
int sys_DeleteFile(void *uesp);
int sys_OpenFile(void *uesp);
int sys_CloseFile(void *uesp);
int sys_WriteFile(void *uesp);
int sys_ReadFile(void *uesp);
int sys_OpenDir(void *uesp);
int sys_CreateDir(void *uesp);
int sys_DeleteDir(void *uesp);
int sys_ListDir(void *uesp);
#endif