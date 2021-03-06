/**********************************************************
*	fat32.c       //added by mingxuan 2019-5-17
***********************************************************/

#include "fat32.h"
#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "fs_misc.h"
#include "vfs.h"
#include "spinlock.h" // added by ran

void disp_int(int);
//extern DWORD FAT_END;
extern struct file_desc f_desc_table[NR_FILE_DESC];
extern struct super_block super_block[NR_SUPER_BLOCK];	//modified by mingxuan 2020-10-30

//added by ran
//struct spinlock lock;
// deleted by ran
//CHAR VDiskPath[256]={0};
//CHAR cur_path[256]={0};
//u8* buf;
STATE state;
File f_desc_table_fat[NR_FILE_DESC];

PRIVATE void init_super_block();
PRIVATE void mkfs_fat();

STATE DeleteDir(SUPER_BLOCK *psb, PCHAR dirname)
{
	SPIN_LOCK *plock = &psb->lock;
	CHAR fullpath[256]={0};
	CHAR parent[256]={0};
	CHAR name[256]={0};
	DWORD parentCluster=0,startCluster=0;
	UINT tag=0;
	STATE state;
	
	ToFullPath(dirname,fullpath);
	GetParentFromPath(fullpath,parent);
	GetNameFromPath(fullpath,name);
	
	state=IsFile(psb, fullpath,&tag);
	if(state!=OK)
	{
		return state;
	}
	if(tag==F)
	{
		return WRONGPATH;
	}
	acquire(plock); //added by ran
	state=PathToCluster(psb, parent,&parentCluster);
	if(state!=OK)
	{
		release(plock); //added by ran
		return state;
	}
	state=ClearRecord(psb, parentCluster,name,&startCluster);
	if(state!=OK)
	{
		release(plock); //added by ran
		return state;
	}
	DeleteAllRecord(psb, startCluster);
	release(plock); //added by ran
	return OK;
}

STATE CreateDir(SUPER_BLOCK *psb, PCHAR dirname)
{
	SPIN_LOCK *plock = &psb->lock;
    DECLARE_SUPER_BLOCK_VARIABLES(psb);

	acquire(plock); //added by ran
	Record record;
	CHAR ext[4]={0};
	CHAR fullname[256]={0};
	CHAR name[256]={0};
	CHAR parent[256]={0};
	DWORD parentCluster=0,startCluster=0,sectorIndex=0,off_in_sector=0;
	STATE state;
	
	ToFullPath(dirname,fullname);
	GetNameFromPath(fullname,name);
	GetParentFromPath(fullname,parent);
	state=PathToCluster(psb, parent,&parentCluster);
	if(state!=OK)
	{
		release(plock);  //added by ran
		return state;//找不到路径
	}
	state=FindSpaceInDir(psb, parentCluster,name,&sectorIndex,&off_in_sector);
	if(state!=OK)
	{
		release(plock);  //added by ran
		return state;//虚拟磁盘空间不足
	}
	state=FindClusterForDir(psb, &startCluster);
	if(state!=OK)
	{
		release(plock);  //added by ran
		return state;//虚拟磁盘空间不足
	}
	CreateRecord(name,0x10,startCluster,0,&record);
	WriteRecord(psb, record,sectorIndex,off_in_sector);
	WriteFAT(psb, 1,&startCluster);//写FAT
	CreateRecord(".",0x10,startCluster,0,&record);//准备目录项.的数据
	sectorIndex=Reserved_Sector+2*Sectors_Per_FAT+(startCluster-2)*Sectors_Per_Cluster;
	WriteRecord(psb, record,sectorIndex,0);//写.目录项
	CreateRecord("..",0x10,parentCluster,0,&record);//准备目录项..的数据
	WriteRecord(psb, record,sectorIndex,sizeof(Record));//写..目录项
	release(plock); //added by ran
	//fflush(fp);
	return OK;
}

// deleted by ran
// STATE OpenDir(PCHAR dirname)
// {
// 	DWORD parentCluster=0,off=0;
// 	CHAR fullpath[256]={0},parent[256]={0},name[256]={0};
// 	Record record;
// 	STATE state;
	
// 	if(strcmp(dirname,".")==0)
// 	{
// 		return OK;
// 	}else if(strcmp(dirname,"..")==0||strcmp(dirname,"\\")==0){
// 		ChangeCurrentPath(dirname);
// 		return OK;
// 	}else{	
// 		if(IsFullPath(dirname))
// 		{
// 			strcpy(fullpath,dirname);
// 			GetParentFromPath(fullpath,parent);
// 			if(strcmp(parent, "V:")==0)//说明dirname是根目录
// 			{
// 				memset(cur_path,0,sizeof(cur_path));
// 				strcpy(cur_path,fullpath);
// 				return OK;
// 			}
// 			GetNameFromPath(fullpath,name);
// 		}else{
// 			MakeFullPath(cur_path,dirname,fullpath);
// 			strcpy(parent,cur_path);
// 			strcpy(name,dirname);
// 		}
// 		state=PathToCluster(parent,&parentCluster);
// 		if(state!=OK)
// 		{
// 			return state;
// 		}
// 		state=ReadRecord(parentCluster,name,&record,NULL,NULL);
// 		if(state!=OK)
// 		{
// 			return state;
// 		}
// 		if(record.proByte==(BYTE)0x10)
// 		{
// 			strcpy(cur_path,fullpath);
// 			return OK;
// 		}else{
// 			return WRONGPATH;
// 		}
// 	}
// 	return OK;
// }

//added by ran
STATE OpenDir(PCHAR dirname)
{
	disp_str("opendir is deprecated, use chdir instead\n");
	return OK;
}


//added by ran
int fat32_chdir(SUPER_BLOCK *psb, const char *dirname)
{
	DWORD parentCluster=0,off=0;
	CHAR fullpath[256]={0},parent[256]={0},name[256]={0};
	Record record;
	STATE state;
	PROCESS_0 *cur_proc = p_proc_current; //added by ran
	char cwd[MAX_PATH];
	
	if(strcmp(dirname,".")==0)
	{
		return OK;
	}
	else if(strcmp(dirname,"..")==0||strcmp(dirname,"\\")==0)
	{
		ChangeCurrentPath(dirname);
		return OK;
	}
	else
	{	
		if(IsFullPath(dirname))
		{
			strcpy(fullpath,dirname);
			GetParentFromPath(fullpath,parent);
			if(strcmp(parent, "V:")==0)//说明dirname是根目录
			{
				memset(cwd,0,sizeof(cwd));
				strncpy(cwd,fullpath, MAX_PATH);
				strncpy(cur_proc->cwd, cwd, MAX_PATH); // modified by ran
				return OK;
			}
			GetNameFromPath(fullpath,name);
		}
		else
		{
			strncpy(cwd, cur_proc->cwd, MAX_PATH); //modified by ran
			MakeFullPath(cwd, dirname, fullpath);
			strcpy(parent, cwd);
			strcpy(name, dirname);
		}
		state=PathToCluster(psb, parent,&parentCluster);
		if(state!=OK)
		{
			return state;
		}
		state=ReadRecord(psb, parentCluster,name,&record,NULL,NULL);
		if(state!=OK)
		{
			return state;
		}
		if(record.proByte==(BYTE)0x10)
		{
			strncpy(cwd, fullpath, MAX_PATH);
			strncpy(cur_proc->cwd, cwd, MAX_PATH); //modified by ran
			return OK;
		}else{
			return WRONGPATH;
		}
	}
	return OK;
}

// added by pg999w, 2021
STATE ReadDir(SUPER_BLOCK *psb, PCHAR dirname, DWORD dir[3], char* filename)
{
  Record record;
  CHAR fullname[256]={0};
  STATE state;
  DirEntry * dir_entry = (DirEntry*)dir;

  while(1) {
      if (dir_entry->sectorIndex == 0) {
          ToFullPath(dirname, fullname);
          state = PathToCluster(psb, fullname, &dir_entry->clusterIndex);
          if (state != OK) {
              return state;//找不到路径
          }
      }
      state = ReadNextRecord(psb, dir_entry->clusterIndex, &dir_entry->sectorIndex, &dir_entry->offset, &record);
      if (state != OK) {
          return state;//目录读完了
      }
      if (record.proByte & (0x0f | 0x08)) {
          continue; //卷标或长文件名
      }
	  if (record.filename[0] == 0xe5)
	  {
		  continue;
	  }
      GetNameFromRecord(record, filename);
      return OK;
  }
}


STATE ReadFile(SUPER_BLOCK *psb, int fd,BYTE buf[], DWORD length)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;
	int size = 0;
	PBYTE sector=NULL;
	DWORD curSectorIndex=0,nextSectorIndex=0,off_in_sector=0,free_in_sector=0,readsize=0;
	UINT isLastSector=0,tag=0;
	//PFile pfile = p_proc_current->task.filp_fat[fd];
	PFile pfile = p_proc_current->task.filp[fd] ->fd_node.fd_file;
	// deint(pfile->flag);
	//pfile->off = 0;

	//if(pfile->flag!=R) //deleted by mingxuan 2019-5-18
	if(pfile->flag!=R && pfile->flag!=RW && pfile->flag!=(RW|C)) //modified by mingxuan 2019-5-18
	{
		return ACCESSDENIED;
	}
	
	//disp_str("read:");
	if(pfile->off>=pfile->size)
	{
		return 0;
	}
	sector = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));
	if(sector==NULL)
	{
		return SYSERROR;
	}
	GetFileOffset(psb, pfile,&curSectorIndex,&off_in_sector,&isLastSector);
	do 
	{	
		if(isLastSector)//当前的扇区是该文件的最后一个扇区
		{
			if(pfile->size%Bytes_Per_Sector==0)
			{
				free_in_sector=Bytes_Per_Sector-off_in_sector;
			}else{
				free_in_sector=pfile->size%Bytes_Per_Sector-off_in_sector;//最后一个扇区的剩余量
			}
			tag=1;//置跳出标志
		}else{
			free_in_sector=Bytes_Per_Sector-off_in_sector;//本扇区的剩余量
		}
		if(free_in_sector<length-(size))//缓冲区装不满
		{
			readsize=free_in_sector;
		}else{//缓冲区能装满
			readsize=length-(size);
			tag=1;//置跳出标志
		}
		ReadSector(dev, sector,curSectorIndex);
		memcpy(buf+(size),sector+off_in_sector,readsize);
		(size)+=readsize;
		pfile->off+=readsize;
		if(tag==1)//最后一个扇区或缓冲区装满了
		{
			break;
		}else{//缓冲区还没装满并且还没到最后一个扇区
			GetNextSector(psb, pfile,curSectorIndex,&nextSectorIndex,&isLastSector);
			curSectorIndex=nextSectorIndex;
			off_in_sector=0;
		}
	}while(1);
	sys_free(sector);
	//pfile->off = 0;
	return size;
}

// added by pg999w, 2020
STATE LSeek(int fd, int offset, int whence)
{
    //PFile pfile = p_proc_current->task.filp_fat[fd];
    PFile pfile = p_proc_current->task.filp[fd] ->fd_node.fd_file;
    switch (whence) {
        case SEEK_SET:
            pfile->off = 0;
            break;
        case SEEK_CUR:
            break;
        case SEEK_END:
            pfile->off = pfile->size;
            break;
        default:
            udisp_str("error: invalid whence");
            break;
    }

    pfile->off += offset;
    return OK;
}

STATE WriteFile(SUPER_BLOCK *psb, int fd,BYTE buf[],DWORD length)
{
	SPIN_LOCK *plock = &psb->lock;
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;
	PBYTE sector=NULL;
	DWORD clusterNum=0,bytes_per_cluster=0,clusterIndex=0;
	DWORD curSectorIndex=0,nextSectorIndex=0,off_in_sector=0,free_in_sector=0,off_in_buf=0;
	CHAR fullpath[256]={0};
	UINT isLastSector=0;
	STATE state;
	PFile pfile = p_proc_current->task.filp[fd] ->fd_node.fd_file;
	//PFile pfile = &f_desc_table_fat[0];

	//if(pfile->flag!=W) //deleted by mingxuan 2019-5-18
	if(pfile->flag!=W && pfile->flag!=RW && pfile->flag!=(RW|C) ) //modified by mingxuan 2019-5-18
	{
		return ACCESSDENIED;
	}

	bytes_per_cluster=Sectors_Per_Cluster*Bytes_Per_Sector;
	sector = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));
	if(sector==NULL)
	{
		return SYSERROR;
	}

	acquire(plock); //added by ran

	if(pfile->start==0)//此文件是个空文件原来没有分配簇
	{
		state=AllotClustersForEmptyFile(psb, pfile,length);//空间不足无法分配
		if(state!=OK)
		{
			sys_free(sector);
			release(plock); //added by ran
			return state;//虚拟磁盘空间不足
		}
	}else{
		if(NeedMoreCluster(psb, pfile,length,&clusterNum))
		{
			state=AddCluster(psb, pfile->start,clusterNum);//空间不足
			if(state!=OK)
			{
				sys_free(sector);
				release(plock); //added by ran
				return state;//虚拟磁盘空间不足
			}
		}
	}
	GetFileOffset(psb, pfile,&curSectorIndex,&off_in_sector,&isLastSector);
	free_in_sector=Bytes_Per_Sector-off_in_sector;
	while(free_in_sector<length-off_in_buf)//当前扇区的空闲空间放不下本次要写入的内容
	{
		ReadSector(dev, sector,curSectorIndex);
		memcpy(sector+off_in_sector,buf+off_in_buf,free_in_sector);
		WriteSector(dev, sector,curSectorIndex);
		off_in_buf+=free_in_sector;
		pfile->off+=free_in_sector;
		GetNextSector(psb, pfile,curSectorIndex,&nextSectorIndex,&isLastSector);
		curSectorIndex=nextSectorIndex;
		free_in_sector=Bytes_Per_Sector;
		off_in_sector=0;
	}
	ReadSector(dev,sector,curSectorIndex);
	memcpy(sector+off_in_sector,buf+off_in_buf,length-off_in_buf);
	WriteSector(dev, sector,curSectorIndex);
	pfile->off+=length-off_in_buf;
	sys_free(sector);
	//fflush(fp);
	release(plock); //added by ran
	return OK;
}

STATE CloseFile(SUPER_BLOCK *psb, int fd)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);

	//debug("close");
	PFile pfile;
	pfile = p_proc_current->task.filp[fd] ->fd_node.fd_file;
	DWORD curSectorIndex=0,curClusterIndex=0,nextClusterIndex=0,parentCluster=0;
	UINT isLastSector=0;
	Record record;
	DWORD sectorIndex=0,off_in_sector=0;
	struct super_block* cur_sb = get_super_block_by_fd(fd);

	//p_proc_current->task.filp_fat[fd] = 0;
	f_desc_table_fat[fd].flag = 0;
	p_proc_current->task.filp[fd]->flag = 0;
	p_proc_current->task.filp[fd] = 0;
	if(pfile->flag==R)
	{
		return OK;
	}else{
		if(pfile->off<pfile->size)
		{
			GetFileOffset(psb, pfile,&curSectorIndex,NULL,&isLastSector);
			if(isLastSector==0)
			{
				curSectorIndex=(curClusterIndex-Reserved_Sector-2*Sectors_Per_FAT)/Sectors_Per_Cluster+2;
				GetNextCluster(psb, curClusterIndex,&nextClusterIndex);
				if(nextClusterIndex!=FAT_END)//说明当前簇不是此文件的最后一簇
				{
					WriteFAT(psb, 1,&curClusterIndex);//把当前簇设置为此文件的最后一簇
					ClearFATs(psb, nextClusterIndex);//清除此文件的多余簇
				}
			}
		}
		PathToCluster(psb, pfile->parent,&parentCluster);
		ReadRecord(psb, parentCluster,pfile->name,&record,&sectorIndex,&off_in_sector);
		record.highClusterNum=(WORD)(pfile->start>>16);
		record.lowClusterNum=(WORD)(pfile->start&0x0000FFFF);
		record.filelength=pfile->off;
		WriteRecord(psb, record,sectorIndex,off_in_sector);
	}
	return OK;
}

STATE OpenFile(SUPER_BLOCK *psb, PCHAR filename,UINT mode)
{

	CHAR fullpath[256]={0};
	CHAR parent[256]={0};
	CHAR name[256]={0};
	Record record;
	DWORD parentCluster;
	STATE state;

	DWORD sectorIndex=0,off_in_sector=0; //added by mingxuan 2019-5-19

	ToFullPath(filename,fullpath);
	GetParentFromPath(fullpath,parent);
	GetNameFromPath(fullpath,name);

	state=PathToCluster(psb, parent,&parentCluster);
	//disp_str("\nstate=");
	//disp_int(state);
	if(state!=OK)
	{
		return -1;
	}
	
	//added by mingxuan 2019-5-19
	state=FindSpaceInDir(psb, parentCluster,name,&sectorIndex,&off_in_sector); //检测文件名是否存在
	if(mode & O_CREAT) //如果用户使用了O_CREAT
	{
		if(state == NAMEEXIST) //文件存在，使用O_CREAT是多余的，继续执行OpenFile即可
		{
			disp_str("file exists, O_CREAT is no use!");
		}
		else //文件不存在，需要使用O_CREAT，先创建文件，再执行OpenFile
		{
			CreateRecord(name,0x20,0,0,&record);
			WriteRecord(psb, record,sectorIndex,off_in_sector);//写目录项
		}
	}
	else //用户没有使用O_CREAT
	{
		if(state != NAMEEXIST) //文件不存在，需要使用O_CREAT，用户没有使用，则报错并返回-1，表示路径有误
		{
			disp_str("no file, use O_CREAT!");
			return -1;
		}
		else{} //文件存在，使用O_CREAT是多余的，继续执行OpenFile即可
	}
	//~mingxuan 2019-5-19

	state=ReadRecord(psb, parentCluster,name,&record,NULL,NULL);
	//disp_str("state=");
	//disp_int(state);
	if(state!=OK)
	{
		disp_str("ReadRecord Fail!");
		return -1;
	}
	
	int i;
	int fd = -1;
	for (i = 3; i < NR_FILES; i++) {
		if (p_proc_current->task.filp[i] == 0) {
			fd = i;
			break;
		}
	}

    if ((fd < 0) || (fd >= NR_FILES)) {
		// panic("filp[] is full (PID:%d)", proc2pid(p_proc_current));
		disp_str("filp[] is full (PID:");
		disp_int(proc2pid(p_proc_current));
		disp_str(")\n");
		return -1;
    }

	//找一个未用的文件描述符
	for (i = 0; i < NR_FILE_DESC; i++)
		if ((f_desc_table[i].flag == 0))
			break;
	if (i >= NR_FILE_DESC) {
		disp_str("f_desc_table[] is full (PID:");
		disp_int(proc2pid(p_proc_current));
		disp_str(")\n");
		return -1;
	}
	
	p_proc_current->task.filp[fd] = &f_desc_table[i];
	f_desc_table[i].flag = 1;
	
	//找一个未用的FILE
	for (i = 0; i < NR_FILE_DESC; i++)
		if (f_desc_table_fat[i].flag == 0)
			break;
	if (i >= NR_FILE_DESC) {
		disp_str("f_desc_table[] is full (PID:");
		disp_int(proc2pid(p_proc_current));
		disp_str(")\n");
	}

	//以下是给File结构体赋值
	memset(f_desc_table_fat[i].parent,0,sizeof(f_desc_table_fat[i].parent));//初始化parent字段
	memset(f_desc_table_fat[i].name,0,sizeof(f_desc_table_fat[i].name));//初始化name字段
	strcpy(f_desc_table_fat[i].parent,parent);
	strcpy(f_desc_table_fat[i].name,name);
	f_desc_table_fat[i].start=(record.highClusterNum<<16)+record.lowClusterNum;
	f_desc_table_fat[i].off=0;
	f_desc_table_fat[i].size=record.filelength;
	f_desc_table_fat[i].flag=mode;
	//disp_str("flag:");
	//deint(f_desc_table_fat[i].flag);
	//disp_str("index:");
	//deint(i);
	p_proc_current->task.filp[fd] ->fd_node.fd_file = &f_desc_table_fat[i];
	
	return fd;
}

STATE CreateFile(SUPER_BLOCK *psb, PCHAR filename)
{
	SPIN_LOCK *plock = &psb->lock;
	UINT i=0,j=0;
	Record record;
	CHAR fullpath[256]={0};
	CHAR parent[256]={0};
	CHAR name[256]={0};
	DWORD parentCluster=0,sectorIndex=0,off_in_sector=0;
	STATE state;
	
	acquire(plock); //added by ran

	ToFullPath(filename,fullpath);
	GetParentFromPath(fullpath,parent);
	GetNameFromPath(filename,name);
	state=PathToCluster(psb, parent,&parentCluster);
	if(state!=OK)
	{
		release(plock); //added by ran
		return state;//找不到路径
	}

	state=FindSpaceInDir(psb, parentCluster,name,&sectorIndex,&off_in_sector);
	if(state != OK) {
		release(plock); //added by ran
		return state;
	}

	CreateRecord(name,0x20,0,0,&record);
	WriteRecord(psb, record,sectorIndex,off_in_sector);//写目录项
	release(plock); //added by ran
	return OK;
}

STATE DeleteFile(SUPER_BLOCK *psb, PCHAR filename)
{
	SPIN_LOCK *plock = &psb->lock;
	CHAR fullpath[256]={0};
	CHAR parent[256]={0};
	CHAR name[256]={0};
	DWORD parentCluster=0;
	DWORD startCluster=0;
	UINT tag=0;
	STATE state;
	
	ToFullPath(filename,fullpath);
	GetParentFromPath(fullpath,parent);
	GetNameFromPath(fullpath,name);
	
	state=IsFile(psb, fullpath,&tag);
	if(state!=OK)
	{
		return state;
	}
	if(tag==D)
	{
		return WRONGPATH;
	}
	acquire(plock); //added by ran
	state=PathToCluster(psb, parent,&parentCluster);
	if(state!=OK)
	{
		release(plock); //added by ran
		return state;
	}
	state=ClearRecord(psb, parentCluster,name,&startCluster);
	if(state!=OK)
	{
		release(plock); //added by ran
		return state;
	}
	if(startCluster!=0)
	{
		ClearFATs(psb, startCluster);
	}
	release(plock); //added by ran
	return OK;
}

STATE IsFile(SUPER_BLOCK *psb, PCHAR path,PUINT tag)
{
	CHAR fullpath[256]={0};
	CHAR parent[256]={0};
	CHAR name[256]={0};
	DWORD parentCluster=0,off=0;
	Record record;
	STATE state;
	
	ToFullPath(path,fullpath);
	GetParentFromPath(fullpath,parent);
	GetNameFromPath(fullpath,name);	
	state=PathToCluster(psb, parent,&parentCluster);
	if(state!=OK)
	{
		return state;//找不到路径
	}
	state=ReadRecord(psb, parentCluster,name,&record,NULL,NULL);
	if(state!=OK)
	{
		return state;//找不到路径
	}
	if(record.proByte==0x10)
	{
		*tag=D;
	}else{
		*tag=F;
	}
	return OK;
}

PUBLIC void init_all_fat(int drive)
{
	int i;
	for(i = 0; i < NR_PRIM_PER_DRIVE; i++)
	{
		if(hd_info[drive].primary[i].fs_type == FAT32_TYPE)
		init_fs_fat((DEV_HD << MAJOR_SHIFT) | i);
	}

	//added by mingxuan 2020-10-29
	for(i = 0; i < NR_SUB_PER_DRIVE; i++)
	{
		if(hd_info[drive].logical[i].fs_type == FAT32_TYPE)
		init_fs_fat((DEV_HD << MAJOR_SHIFT) | (i + MINOR_hd1a)); // logic的下标i加上hd1a才是该逻辑分区的次设备号
	}

	for (i = 0; i < NR_FILE_DESC; ++i) {
		f_desc_table_fat[i].flag = 0;
	}
}

PUBLIC void init_fs_fat(int fat32_dev) {
	struct vfs* pvfs = vfs_alloc_vfs_entity();

	init_super_block(pvfs->sb, fat32_dev);

	pvfs->used = 1;
}

PRIVATE void init_super_block(SUPER_BLOCK *psb, int dev) {
	MESSAGE driver_msg;
	char buf[512]; //added by ran
	PCHAR cur="V:\\";

	driver_msg.type		= DEV_READ;
	driver_msg.DEVICE	= MINOR(dev);
	//driver_msg.POSITION	= SECTOR_SIZE * 1;
	driver_msg.POSITION	= 0;
	driver_msg.BUF		= buf;
	driver_msg.CNT		= SECTOR_SIZE;
	driver_msg.PROC_NR	= proc2pid(p_proc_current);///TASK_A

	hd_rdwt(&driver_msg);

	DWORD TotalSectors;
	WORD  Bytes_Per_Sector;
	BYTE  Sectors_Per_Cluster;
	WORD  Reserved_Sector;
	DWORD Sectors_Per_FAT;
	UINT Position_Of_RootDir;
	UINT Position_Of_FAT1;
	UINT Position_Of_FAT2;

	memcpy(Bytes_Per_Sector, buf+0x0b, 2);
    memcpy(Sectors_Per_Cluster,buf+0x0d,1);
    memcpy(Reserved_Sector,buf+0x0e,2);
    // deleted by ran
    //memcpy(&TotalSectors,buf+32,4);
    //Total logical sectors (if greater than 65535; otherwise, see offset 0x013).
    // added by ran
    TotalSectors = 0;
    memcpy(&TotalSectors,buf+0x13, 2);
    if (!TotalSectors)
    {
        memcpy(&TotalSectors,buf+0x20,4);
    }
    TotalSectors = 0;
    memcpy(&TotalSectors,buf+0x13, 2);
    memcpy(&Sectors_Per_FAT,buf+36,4);
    Position_Of_RootDir=(Reserved_Sector+Sectors_Per_FAT*2)*Bytes_Per_Sector;
    Position_Of_FAT1=Reserved_Sector*Bytes_Per_Sector;
    Position_Of_FAT2=(Reserved_Sector+Sectors_Per_FAT)*Bytes_Per_Sector;
    disp_str("FAT32 Sector Per Cluster: ");
    disp_int(Sectors_Per_Cluster);
    disp_str("\n");

	psb->Bytes_Per_Sector = Bytes_Per_Sector;
	psb->Sectors_Per_Cluster = Sectors_Per_Cluster;
	psb->Reserved_Sector = Reserved_Sector;
	psb->TotalSectors = TotalSectors;
	psb->Sectors_Per_FAT = Sectors_Per_FAT;
	psb->Position_Of_RootDir = Position_Of_RootDir;
	psb->Position_Of_FAT1 = Position_Of_FAT1;
	psb->Position_Of_FAT2 = Position_Of_FAT2;

	psb->sb_dev = dev;
	psb->fs_type = FAT32_TYPE;
	psb->used = 1;

	initlock(&psb->lock, 0);

    //deleted by pg999w, 2021
    //memcpy(&Bytes_Per_Sector,buf+0x0b,2);
	//memcpy(&Sectors_Per_Cluster,buf+0x0d,1);
	//memcpy(&Reserved_Sector,buf+0x0e,2);
	////memcpy(&TotalSectors,buf+32,4);
	////Total logical sectors (if greater than 65535; otherwise, see offset 0x013).
	//TotalSectors = 0;
	//memcpy(&TotalSectors,buf+0x13, 2);
	//memcpy(&Sectors_Per_FAT,buf+36,4);
	//Position_Of_RootDir=(Reserved_Sector+Sectors_Per_FAT*2)*Bytes_Per_Sector;
	//Position_Of_FAT1=Reserved_Sector*Bytes_Per_Sector;
	//Position_Of_FAT2=(Reserved_Sector+Sectors_Per_FAT)*Bytes_Per_Sector;
	//disp_str("FAT32 Sector Per Cluster: ");
	//disp_int(Sectors_Per_Cluster);
	//disp_str("\n");

	//deleted by ran
	//strcpy(cur_path,cur);
}

PRIVATE void mkfs_fat() {
    //deleted by pg999w, 2021
    //MESSAGE driver_msg;
	//char buf[512];  //added by ran
	//int fat32_dev = get_fs_dev(PRIMARY_MASTER, FAT32_TYPE);	//added by mingxuan 2020-10-27

	///* get the geometry of ROOTDEV */
	//struct part_info geo;
	//driver_msg.type		= DEV_IOCTL;
	////driver_msg.DEVICE	= MINOR(FAT_DEV);	//deleted by mingxuan 2020-10-27
	//driver_msg.DEVICE	= MINOR(fat32_dev);	//modified by mingxuan 2020-10-27

	//driver_msg.REQUEST	= DIOCTL_GET_GEO;
	//driver_msg.BUF		= &geo;
	//driver_msg.PROC_NR	= proc2pid(p_proc_current);
	//hd_ioctl(&driver_msg);

	//disp_str("dev size: ");
	//disp_int(geo.size);
	//disp_str(" sectors\n");

    //TotalSectors = geo.size;

	//DWORD jump=0x009058eb;//跳转指令：占3个字节
	//DWORD oem[2]={0x4f44534d,0x302e3553};//厂商标志，OS版本号:占8个字节
	////以下是BPB的内容
	//WORD bytes_per_sector=512;//每扇区字节数：占2个字节
	//WORD sectors_per_cluster=8;//每簇扇区数：占1个字节
	//WORD reserved_sector=32;//保留扇区数：占2个字节
	//WORD number_of_FAT=2;//FAT数：占1个字节
	//BYTE mediaDescriptor=0xF8;
	//DWORD sectors_per_FAT=(TotalSectors*512-8192)/525312+1;//每FAT所占扇区数，用此公式可以算出来：占4个字节
	//DWORD root_cluster_number=2;//根目录簇号：占4个字节
	////以下是扩展BPB内容
	//CHAR volumeLabel[11]={'N','O',' ','N','A','M','E',' ',' ',' ',' '};//卷标：占11个字节
	//CHAR systemID[8]={'F','A','T','3','2',' ',' ',' '};//系统ID，FAT32系统中一般取为“FAT32”：占8个字节
	////以下是有效结束标志
	//DWORD end=0xaa55;
	//
	//DWORD media_descriptor[2]={0x0ffffff8,0xffffffff};//FAT介质描述符
	//DWORD cluster_tag=0x0fffffff;//文件簇的结束单元标记
	//
	//Record vLabel;//卷标的记录项。
	//DWORD clearSize=sectors_per_cluster*bytes_per_sector-sizeof(Record);
	//char volumelabel[3] = "MZY";

	//memcpy(buf,&jump,3);//写入跳转指令:占3个字节(其实没有用)
	//memcpy(buf+3,oem,8);//厂商标志，OS版本号:占8个字节
	////以下是写 BPB
	//memcpy(buf+11,&bytes_per_sector,2);//每扇区字节数：占2个字节
	//memcpy(buf+13,&sectors_per_cluster,1);//写入每簇扇区数：占1个字节
	//memcpy(buf+14,&reserved_sector,2);//写入保留扇区数：占2个字节
	//memcpy(buf+16,&number_of_FAT,1);//写入FAT数：占1个字节
	//memcpy(buf+21,&mediaDescriptor,1);//写入媒体描述符
	//memcpy(buf+32,&TotalSectors,4);//写入总扇区数
	//memcpy(buf+36,&sectors_per_FAT,4);//写入每FAT所占扇区数：占4个字节
	//memcpy(buf+44,&root_cluster_number,4);//写入根目录簇号：占4个字节
	////以下是写 扩展BPB
	//memcpy(buf+71,volumeLabel,11);//写卷标：占11个字节
	//memcpy(buf+82,systemID,8);//系统ID，FAT32系统中一般取为“FAT32”：占8个字节
	////由于引导代码对于本虚拟系统没有用，故省略
	//memcpy(buf+510,&end,2);
    ////WR_SECT_FAT(buf, 1);			//deleted by mingxuan 2020-10-27
	//WR_SECT_FAT(fat32_dev, buf, 1);	//modified by mingxuan 2020-10-27

	////初始化FAT
	//memset(buf,0,SECTOR_SIZE);//写介质描述单元
	//memcpy(buf,media_descriptor,8);
	//memcpy(buf+8,&cluster_tag,4);//写根目录的簇号
    ////WR_SECT_FAT(buf, reserved_sector);	// deleted by mingxuan 2020-10-27
	//WR_SECT_FAT(fat32_dev, buf, reserved_sector);	// modified by mingxuan 2020-10-27

	////初始化根目录
	//CreateRecord(volumelabel,0x08,0,0,&vLabel);//准备卷标的目录项的数据
	//memset(buf,0,SECTOR_SIZE);//将准备好的记录项数据写入虚拟硬盘
	//memcpy(buf,&vLabel,sizeof(Record));
    ////WR_SECT_FAT(buf, reserved_sector+2*sectors_per_FAT);	// deleted by mingxuan 2020-10-27
	//WR_SECT_FAT(fat32_dev, buf, reserved_sector+2*sectors_per_FAT);	// modified by mingxuan 2020-10-27
}

PUBLIC int rw_sector_fat(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
{
	MESSAGE driver_msg;
	
	driver_msg.type		= io_type;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= pos;
	driver_msg.CNT		= bytes;	/// hu is: 512
	driver_msg.PROC_NR	= proc_nr;
	driver_msg.BUF		= buf;

	hd_rdwt(&driver_msg);
	return 0;
}

PUBLIC int rw_sector_sched_fat(int io_type, int dev, int pos, int bytes, int proc_nr, void* buf)
{
	MESSAGE driver_msg;
	
	driver_msg.type		= io_type;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= pos;
	driver_msg.CNT		= bytes;	/// hu is: 512
	driver_msg.PROC_NR	= proc_nr;
	driver_msg.BUF		= buf;

	hd_rdwt_sched(&driver_msg);
	return 0;
}

void debug(char * s) {
    disp_str(s);
    disp_str("\n");
}

void deint(int t) {
    disp_int(t);
    disp_str("\n");
}

int sys_CreateFile(void *uesp)
{
	// state=CreateFile(get_arg(uesp, 1)); 
	// if(state==OK)
	// {
	// 	//debug("create file success");
	// 	debug("           create file success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }

	// return state;
}

int sys_DeleteFile(void *uesp)
{
	// state=DeleteFile(get_arg(uesp, 1));
	// if(state==OK)
	// {
	// 	//debug("delete file success");
	// 	debug("           delete file success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
}

int sys_OpenFile(void *uesp)
{
/*	// state=OpenFile(get_arg(uesp, 1),
	// 				get_arg(uesp, 2));
	// if(state==OK)
	// {
	// 	debug("open file success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
	state=OpenFile(get_arg(uesp, 1),
					get_arg(uesp, 2));
	//debug("open file success");
	debug("           open file success");
	return state;	*/
}

int sys_CloseFile(void *uesp)
{
	// state=CloseFile(get_arg(uesp, 1));
	// if(state==OK)
	// {
	// 	//debug("close file success");
	// 	debug("           close file success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
}

int sys_WriteFile(void *uesp)
{
	// state=WriteFile(get_arg(uesp, 1),
	// 				get_arg(uesp, 2),
	// 				get_arg(uesp, 3));
	// if(state==OK)
	// {
	// 	//debug("write file success");
	// 	debug("           write file success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
}

int sys_ReadFile(void *uesp)
{
	// state=ReadFile(get_arg(uesp, 1),
	// 				get_arg(uesp, 2),
	// 				get_arg(uesp, 3),
	// 				get_arg(uesp, 4));
	// if(state==OK)
	// {
	// 	//debug("read file success");
	// 	debug("           read file success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
}

int sys_OpenDir(void *uesp)
{
	// state=OpenDir(get_arg(uesp, 1));
	// if(state==OK)
	// {
	// 	//debug("open dir success");
	// 	debug("           open dir success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
}

int sys_CreateDir(void *uesp)
{
	// state=CreateDir(get_arg(uesp, 1));
	// if(state==OK)
	// {
	// 	//debug("create dir success");
	// 	debug("           create dir success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
}

int sys_DeleteDir(void *uesp)
{
	// state=DeleteDir(get_arg(uesp, 1));
	// if(state==OK)
	// {
	// 	debug("delete dir success");
	// 	debug("           delete dir success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
}

int sys_ListDir(void *uesp) {
		
	// DArray *array=NULL;
	// char *s = get_arg(uesp, 1);
	// CHAR temp[256]={0};
	// UINT tag=0;

	// array = InitDArray(10, 10);
	// memset(temp, 0, sizeof(temp));
	// if (strlen(s) != 0)
	// {
	// 	strcpy(temp,s);
	// 	if(IsFile(temp,&tag))
	// 	{
	// 		if(tag==1)
	// 		{
	// 			printf("不是目录的路径\n\n");
	// 		}
	// 	}
	// }
	// else {
	// 	GetCurrentPath(temp);
	// }
	// state=ListAll(temp, array);
	// if(state==OK)
	// {
	// 	DirCheckup(array);
	// }else {
	// 	DisErrorInfo(state);
	// 	disp_str("\n");
	// }
	// DestroyDArray(array);
}

void DisErrorInfo(STATE state)
{
	if(state==SYSERROR)
	{
		disp_str("          system error\n");
	}
	else if(state==VDISKERROR)
	{
		disp_str("          disk error\n");
	}
	else if(state==INSUFFICIENTSPACE)
	{
		disp_str("          no much space\n");
	}
	else if(state==WRONGPATH)
	{
		disp_str("          path error\n");
	}
	else if(state==NAMEEXIST)
	{
		disp_str("          name exists\n");
	}
	else if(state==ACCESSDENIED)
	{
		disp_str("          deny access\n");
	}
	else
	{
		disp_str("          unknown error\n");
	}
}

