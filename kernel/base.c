/**********************************************************
*	base.c       //added by mingxuan 2019-5-17
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

//DWORD FAT_END=268435455;//文件簇号结束标记

//deleted by ran
//extern CHAR cur_path[256];
//BYTE FATBuf[1024]={0};
//DWORD globalSectorIndex=-1;

int strcmp(const char * s1, const char *s2);

void ReadSector(int fat32_dev, BYTE* buf,DWORD sectorIndex)
{
	//int fat32_dev = get_fs_dev(PRIMARY_MASTER, FAT32_TYPE);	//added by mingxuan 2020-10-27

    //RD_SECT_SCHED_FAT(buf, sectorIndex);	// deleted by mingxuan 2020-10-27
	RD_SECT_SCHED_FAT(fat32_dev, buf, sectorIndex);	// modified by mingxuan 2020-10-27
}

//deleted by ran
// void WriteSector(BYTE* buf,DWORD sectorIndex)
// {
// 	if(sectorIndex==globalSectorIndex)
// 	{
// 		memcpy(FATBuf,buf,512);//写FAT表的缓冲区，保持数据同步
// 	}
// 	int fat32_dev = get_fs_dev(PRIMARY_MASTER, FAT32_TYPE);	//added by mingxuan 2020-10-27
	
//     //WR_SECT_SCHED_FAT(buf, sectorIndex);	// deleted by mingxuan 2020-10-27
// 	WR_SECT_SCHED_FAT(fat32_dev, buf, sectorIndex);	// modified by mingxuan 2020-10-27
// }

//added by ran
void WriteSector(int fat32_dev, BYTE* buf,DWORD sectorIndex)
{
	//int fat32_dev = get_fs_dev(PRIMARY_MASTER, FAT32_TYPE);	//added by mingxuan 2020-10-27
	
    //WR_SECT_SCHED_FAT(buf, sectorIndex);	// deleted by mingxuan 2020-10-27
	WR_SECT_SCHED_FAT(fat32_dev, buf, sectorIndex);	// modified by mingxuan 2020-10-27
}

void DeleteAllRecord(SUPER_BLOCK *psb, DWORD startCluster)
{
	DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;

	PBYTE buf=NULL;
	Record record;
	DWORD off=0;
	DWORD curClusterIndex=startCluster,nextClusterIndex=0,start=0;
	DWORD curSectorIndex=0,preSectorIndex=0,off_in_sector=0,last=0;

	if(startCluster==2)
	{
		off=Position_Of_RootDir+(startCluster-2)*Sectors_Per_Cluster*Bytes_Per_Sector+sizeof(Record);
	}else{
		off=Position_Of_RootDir+(startCluster-2)*Sectors_Per_Cluster*Bytes_Per_Sector+2*sizeof(Record);
	}
	buf = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));	
	last=Reserved_Sector+2*Sectors_Per_FAT+(startCluster-2)*Sectors_Per_Cluster+Sectors_Per_Cluster;
	do
	{
		curSectorIndex=off/Bytes_Per_Sector;
		off_in_sector=off%Bytes_Per_Sector;
		if(curSectorIndex!=preSectorIndex)
		{
			if(preSectorIndex!=0)
			{
				WriteSector(dev, buf,preSectorIndex);
			}
			ReadSector(dev, buf,curSectorIndex);
			preSectorIndex=curSectorIndex;
		}
		memcpy(&record,buf+off_in_sector,sizeof(Record));
		if(record.filename[0]==0)
		{
			WriteSector(dev, buf,curSectorIndex);
			break;
		}
		if(record.filename[0]!=(BYTE)0xE5&&record.filename[0]!=0)
		{
			record.filename[0]=(BYTE)0xE5;
			memcpy(buf+off_in_sector,&record,sizeof(Record));
			start=(DWORD)(record.highClusterNum<<16)+(DWORD)record.lowClusterNum;
			if(record.proByte==(BYTE)0x10)//是子目录
			{
				DeleteAllRecord(psb, start);//删除此子目录
			}else{
				ClearFATs(psb, start);
			}
		}
		preSectorIndex=curSectorIndex;
		if(curSectorIndex>=last)
		{
			GetNextCluster(psb, curClusterIndex,&nextClusterIndex);
			if(nextClusterIndex==FAT_END)
			{
				WriteSector(dev, buf,curSectorIndex);
				break;
			}else{
				curClusterIndex=nextClusterIndex;
				off=Position_Of_RootDir+(curClusterIndex-2)*Sectors_Per_Cluster*Bytes_Per_Sector;
			}
		}else{
			off+=sizeof(Record);
		}
	}while(1);
	sys_free(buf);
	ClearFATs(psb, startCluster);
}

STATE FindClusterForDir(SUPER_BLOCK *psb, PDWORD pcluster)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;

	PBYTE buf=NULL;
	DWORD clusterIndex=2,nextClusterIndex=0;
	UINT index=0,i=0;
	DWORD curSectorIndex=0,last=0,offset=0;

	buf = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));	
	if(buf==NULL)
	{
		return SYSERROR;
	}
	curSectorIndex=Reserved_Sector;
	last=curSectorIndex+Sectors_Per_FAT;
	offset=8;
	for(;curSectorIndex<last;curSectorIndex++)
	{
		ReadSector(dev, buf,curSectorIndex);
		for(;offset<Bytes_Per_Sector;offset+=sizeof(DWORD),clusterIndex++)
		{
			memcpy(&nextClusterIndex,buf+offset,sizeof(DWORD));
			if(nextClusterIndex==0)
			{
				sys_free(buf);
				*pcluster=clusterIndex;
				ClearClusters(psb, clusterIndex);
				return OK;
			}
		}
		offset=0;
	}
	sys_free(buf);
	return INSUFFICIENTSPACE;
}

void GetFileOffset(SUPER_BLOCK *psb, PFile pfile,PDWORD sectorIndex,PDWORD off_in_sector,PUINT isLastSector)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);

	DWORD curSectorIndex=0,nextSectorIndex=0,sectorNum=0,totalSectors=0,counter=0;
	
	if(pfile->size%Bytes_Per_Sector==0)
	{
		totalSectors=pfile->size/Bytes_Per_Sector;
	}else{
		totalSectors=pfile->size/Bytes_Per_Sector+1;
	}
	sectorNum=pfile->off/Bytes_Per_Sector+1;
	if(off_in_sector!=NULL)
	{
		(*off_in_sector)=pfile->off%Bytes_Per_Sector;
	}
	curSectorIndex=Reserved_Sector+2*Sectors_Per_FAT+(pfile->start-2)*Sectors_Per_Cluster;
	do
	{
		counter++;
		if(counter==sectorNum)
		{
			// if(totalSectors==1) // deleted by ran
			if(totalSectors==sectorNum) // modified by ran
			{
				*sectorIndex=curSectorIndex;
				*isLastSector=1;
			}else{
				*sectorIndex=curSectorIndex;
			}
			break;
		}
		GetNextSector(psb, pfile,curSectorIndex,&nextSectorIndex,isLastSector);
		curSectorIndex=nextSectorIndex;
	}while(1);
}

void GetNextSector(SUPER_BLOCK *psb, PFile pfile,DWORD curSectorIndex,PDWORD nextSectorIndex,PUINT isLastSector)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);

//	debug("nextsector");
	DWORD temp=0;
	DWORD curClusterIndex=0,nextClusterIndex=0,last=0;
	BYTE off_in_cluster=0;
	
	temp=curSectorIndex-Reserved_Sector-2*Sectors_Per_FAT;
	curClusterIndex=temp/Sectors_Per_Cluster+2;//此扇区所在的簇
	off_in_cluster=(BYTE)(temp%Sectors_Per_Cluster);//此扇区是所在簇的的几个扇区，从零开始
	
	GetNextCluster(psb, curClusterIndex,&nextClusterIndex);
	if(nextClusterIndex==FAT_END)//此扇区所在簇是该文件的最后一个簇
	{
		if(pfile->flag==R)
		{
			temp=pfile->size%(Sectors_Per_Cluster*Bytes_Per_Sector);
			if(temp==0)//此文件实际占用了整数个簇
			{
				last=Sectors_Per_Cluster;
			}else{
				if(temp%Bytes_Per_Sector==0)
				{
					last=temp/Bytes_Per_Sector;
				}else{
					last=temp/Bytes_Per_Sector+1;//最后一簇实际占用的扇区数
				}
			}
		}else{
			last=Sectors_Per_Cluster;
		}
		if(off_in_cluster<last-1)
		{
			(*nextSectorIndex)=Reserved_Sector+2*Sectors_Per_FAT+(curClusterIndex-2)*Sectors_Per_Cluster+off_in_cluster+1;
			if(off_in_cluster==last-2)//返回的扇区号是该文件的最后一个扇区
			{
				*isLastSector=1;
			}else{
				*isLastSector=0;
			}
		}
	}else{
		if(off_in_cluster<Sectors_Per_Cluster-1)
		{
			(*nextSectorIndex)=Reserved_Sector+2*Sectors_Per_FAT+(curClusterIndex-2)*Sectors_Per_Cluster+off_in_cluster+1;
		}else{
			curClusterIndex=nextClusterIndex;
			(*nextSectorIndex)=Reserved_Sector+2*Sectors_Per_FAT+(curClusterIndex-2)*Sectors_Per_Cluster;
		}
		GetNextCluster(psb, curClusterIndex,&nextClusterIndex);
		if(nextClusterIndex==FAT_END)
		{
			if(pfile->flag==R)
			{
				temp=pfile->size%(Sectors_Per_Cluster*Bytes_Per_Sector);
				if(temp!=0)//此文件实际占用了整数个簇
				{
					last=temp/Bytes_Per_Sector+1;//最后一簇实际占用的扇区数
					if(last==1)//最后一簇实际上只占用了一个扇区
					{
						*isLastSector=1;//那么当前返回的扇区就是此文件的最后一个扇区
					}else{
						*isLastSector=0;
					}
				}
			}
		}
	}
}

//deleted by ran
// STATE GetNextCluster(DWORD clusterIndex,PDWORD nextCluster)
// {
// 	debug("nextcluster");
// 	DWORD sectorIndex=0,offset=0,off_in_sector=0;
	
// 	offset=8+(clusterIndex-2)*sizeof(DWORD);
// 	sectorIndex=Reserved_Sector+offset/Bytes_Per_Sector;
// 	off_in_sector=offset%Bytes_Per_Sector;
// 	if(sectorIndex!=globalSectorIndex)
// 	{	
// 		ReadSector(FATBuf,sectorIndex);
// 		globalSectorIndex=sectorIndex;
// 	}
// 	memcpy(nextCluster,FATBuf+off_in_sector,sizeof(DWORD));
// 	return OK;
// }

//added by ran
STATE GetNextCluster(SUPER_BLOCK *psb, DWORD clusterIndex,PDWORD nextCluster)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
    int dev = psb->sb_dev;

//	debug("nextcluster");
	DWORD sectorIndex=0,offset=0,off_in_sector=0;
	
	offset=8+(clusterIndex-2)*sizeof(DWORD);
	sectorIndex=Reserved_Sector+offset/Bytes_Per_Sector;
	off_in_sector=offset%Bytes_Per_Sector;
	char buf[1024];
	ReadSector(dev, buf, sectorIndex);
	memcpy(nextCluster,buf+off_in_sector,sizeof(DWORD));
	return OK;
}

/// 在簇当中读取某个特定的目录项
/// \param parentCluster 读取的簇
/// \param name 目录项名称
/// \param record 输出的目录项结构体
/// \param sectorIndex
/// \param off_in_sector
/// \return
STATE ReadRecord(SUPER_BLOCK *psb, DWORD parentCluster,PCHAR name,PRecord record,PDWORD sectorIndex,PDWORD off_in_sector)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;
	CHAR temp[256]={0};
	DWORD curSectorIndex=0,curClusterIndex=parentCluster,nextClusterIndex=0,off=0,size_of_Record;
	BYTE *buf;
	UINT last=0;
	LONGNAME lrecord;
	UINT offinlongname=0;

	size_of_Record=sizeof(Record);
	buf = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));
	do
	{
		curSectorIndex=Reserved_Sector+2*Sectors_Per_FAT+(curClusterIndex-2)*Sectors_Per_Cluster;
		last=curSectorIndex+Sectors_Per_Cluster;
		for(;curSectorIndex<last;curSectorIndex++)//扇区号循环
		{
			ReadSector(dev, buf,curSectorIndex);
			for(off=0;off<Bytes_Per_Sector;off+=size_of_Record)//目录项号循环
			{
				memcpy(record,buf+off,size_of_Record);
				if(record->filename[0]==0)//没有此目录项
				{
					sys_free(buf);
					return WRONGPATH;
				}
				memset(temp,0,sizeof(temp));
				// if(record->sysReserved==0x18)//是长文件名文件的短目录项
				// {
				// 	do
				// 	{
				// 		memcpy(&lrecord,buf+off,size_of_Record);
				// 		strcpy(temp+offinlongname,lrecord.name1);
				// 		offinlongname+=10;
				// 		strcpy(temp+offinlongname,lrecord.name2);
				// 		offinlongname+=19;
				// 		if(lrecord.proByte&0x40!=0)
				// 		{
				// 			break;
				// 		}
				// 		if(off>=Bytes_Per_Sector)
				// 		{
				// 			ReadSector(buf,curSectorIndex);
				// 			off=0;
				// 		}
				// 	}while(1);
				// }else{
				// 	GetNameFromRecord(*record,temp);
				// }
				GetNameFromRecord(*record,temp);
				if(strcmp(temp,name)==0)
				{
					if(sectorIndex!=NULL)
					{
						*sectorIndex=curSectorIndex;
					}
					if(off_in_sector!=NULL)
					{
						*off_in_sector=off;
					}
					sys_free(buf);
					return OK;
				}
			}
		}
		// GetNextCluster(curClusterIndex,&nextClusterIndex);
		// if(nextClusterIndex==FAT_END)
		// {
		// 	sys_free(buf);
		// 	return WRONGPATH;
		// }else{
		// 	curClusterIndex=nextClusterIndex;
		// }
	}while(1);
}

/// 获取簇中的项目并设置下一个项目的参数，如果*sectorIndex和*off_in_sector为0则返回第一个项目，如果没有项目，则返回state=END_OF_DIR
/// \param parentCluster 查询的簇
/// \param sectorIndex
/// \param off_in_sector
/// \param record
/// \return
STATE ReadNextRecord(SUPER_BLOCK *psb, DWORD parentCluster,PDWORD sectorIndex,PDWORD off_in_sector,PRecord record)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;

  CHAR temp[256]={0};
  DWORD curSectorIndex=0,curClusterIndex=parentCluster,nextClusterIndex=0,off=0,size_of_Record;
  BYTE *buf;
  UINT last=0;
  LONGNAME lrecord;
  UINT offinlongname=0;

  size_of_Record=sizeof(Record);
  buf = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));

  DWORD beginSectorIndex = Reserved_Sector+2*Sectors_Per_FAT+(curClusterIndex-2)*Sectors_Per_Cluster;

  if (*off_in_sector == 0 && *sectorIndex == 0) {
    curSectorIndex= beginSectorIndex;
    off = 0;
  } else {
    curSectorIndex = *sectorIndex;
    off = *off_in_sector;
  }

  last=beginSectorIndex+Sectors_Per_Cluster;
  if (last == curSectorIndex) {
    // 已经读完整个簇
    sys_free(buf);
    return END_OF_DIR;
  }
  // 读取目录项
  ReadSector(dev, buf,curSectorIndex);
  memcpy(record,buf+off,size_of_Record);
  if(record->filename[0]==0)//已经读到目录结尾了
  {
    sys_free(buf);
    return END_OF_DIR;
  }
  // 计算下一项
  off += size_of_Record;
  if (off >= Bytes_Per_Sector) {
    off = 0;
    curSectorIndex++;
  }

  // 写回参数
  *sectorIndex=curSectorIndex;
  *off_in_sector=off;

  sys_free(buf);
  return OK;
}

void ClearFATs(SUPER_BLOCK *psb, DWORD startClusterIndex)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;

	PBYTE buf=NULL;
	DWORD curClusterIndex=startClusterIndex,nextClusterIndex=0;
	DWORD curSectorIndex=0,preSectorIndex=0,temp=0,offset=0;
	DWORD clear=0;
	
	if(startClusterIndex==0)
	{
		return;
	}
	buf = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));
	do 
	{
		temp=(curClusterIndex-2)*sizeof(DWORD)+8;
		curSectorIndex=Reserved_Sector+temp/Bytes_Per_Sector;
		offset=temp%Bytes_Per_Sector;
		if(curSectorIndex!=preSectorIndex)
		{	
			if(preSectorIndex!=0)//不是第一个扇区
			{
				WriteSector(dev, buf,preSectorIndex);
			}
			preSectorIndex=curSectorIndex;
			ReadSector(dev, buf,curSectorIndex);
		}
		memcpy(&nextClusterIndex,buf+offset,sizeof(DWORD));
		curClusterIndex=nextClusterIndex;
		memcpy(buf+offset,&clear,sizeof(DWORD));
		preSectorIndex=curSectorIndex;
	}while(curClusterIndex!=FAT_END);
	WriteSector(dev, buf,curSectorIndex);
	sys_free(buf);
}

STATE ClearRecord(SUPER_BLOCK *psb, DWORD parentCluster,PCHAR name,PDWORD startCluster)
{
	int dev = psb->sb_dev;

	Record record;
	DWORD startClusterIndex=0,sectorIndex=0,off_in_sector=0;
	STATE state;
	
	state=ReadRecord(dev, parentCluster,name,&record,&sectorIndex,&off_in_sector);
	if(state!=OK)
	{
		return state;
	}
	record.filename[0]=(BYTE)0xE5;
	startClusterIndex=((DWORD)record.highClusterNum<<16)+(DWORD)record.lowClusterNum;
	WriteRecord(dev, record,sectorIndex,off_in_sector);
	*startCluster=startClusterIndex;
	return OK;
}

/// 获取路径所对应的簇号
/// \param path 文件夹路径
/// \param cluster 输出：簇号
/// \return
STATE PathToCluster(SUPER_BLOCK *psb, PCHAR path, PDWORD cluster)
{
	UINT i=0,j=0,k=0,n=0,len=0;
	CHAR name[256]={0};
	CHAR fullpath[256]={0};
	Record record;
	DWORD parentCluster=2,off=0,counter=0;
	STATE state;
	
	ToFullPath(path,fullpath);
	len=strlen(fullpath);
	for(i=0;i<len;i++)
	{
	    if(fullpath[i]=='\\')
		{
			break;
		}
	}
	if(i>=len)//说明是根目录
	{
		*cluster=2;
		return OK;
	}
	j=0;
	for(i=i+1;i<len;i++) 
	{
		if(fullpath[i]=='\\'||i==len-1)
		{
			if(i==len-1)
			{
				name[j]=fullpath[i];
			}
			state=ReadRecord(psb, parentCluster,name,&record,NULL,NULL);
			if(state==OK)
			{
				parentCluster=(record.highClusterNum<<16)+record.lowClusterNum;
				memset(name,0,sizeof(name));
			}else{
				return state;
			}
			j=0;
		}else{
			name[j++]=fullpath[i];
		}
	}
	*cluster=parentCluster;
	return OK;
}

STATE FindSpaceInDir(SUPER_BLOCK *psb, DWORD parentClusterIndex,PCHAR name,PDWORD sectorIndex,PDWORD off_in_sector)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;

	PBYTE buf=NULL;
	Record record;
	CHAR fullname[256]={0};
	DWORD curClusterIndex=parentClusterIndex,nextClusterIndex=0;
	DWORD curSectorIndex=0,offset=0,last=0;
	STATE state;
	UINT recordsnum=1,curnum=0;

	buf = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));
	if(strlen(name)%29==0)
	{
		recordsnum=strlen(name)/29;
	}else{
		recordsnum=strlen(name)/29+1;
	}
	do//簇号循环
	{
	    curSectorIndex=Reserved_Sector+2*Sectors_Per_FAT+(curClusterIndex-2)*Sectors_Per_Cluster;
	    last=curSectorIndex+Sectors_Per_Cluster;
		for(;curSectorIndex<last;curSectorIndex++)//扇区号循环
		{
			ReadSector(dev, buf,curSectorIndex);
			for(offset=0;offset<Bytes_Per_Sector;offset+=sizeof(Record))//目录项号循环
			{
				memcpy(&record,buf+offset,sizeof(Record));
				if(record.filename[0]==0||record.filename[0]==(BYTE)0xE5)
				{
					do
					{
						curnum++;
						if(curnum==recordsnum)
						{
							*sectorIndex=curSectorIndex;
							*off_in_sector=offset;
							sys_free(buf);
							return OK;//为目录项找到位置了
						}
						memcpy(&record,buf+offset,sizeof(Record));
						offset+=sizeof(Record);
						if(record.filename[0]!=0&&record.filename[0]!=(BYTE)0xE5)
						{
							curnum=0;
							break;
						}
						if(offset>=Bytes_Per_Sector)
						{
							ReadSector(dev, buf,curSectorIndex);
							offset=0;
						}
					}while(1);
				}
                else
                {
					if(record.proByte!=(BYTE)0x08)//不是卷标
					{
						GetNameFromRecord(record,fullname);
						if(strcmp(name,fullname)==0)//有重名的文件或目录
						{
							sys_free(buf);
							return NAMEEXIST;
						}
					}
				}
			}
		}
		// GetNextCluster(curClusterIndex,&nextClusterIndex);
		// if(nextClusterIndex==FAT_END)//已经是最后一簇了
		// {
		// 	state=AddCluster(parentClusterIndex,1);//需要为此目录增加一个簇
		// 	if(state!=OK)
		// 	{
		// 		free(buf);
		// 		return state;
		// 	}else{
		// 		GetNextCluster(curClusterIndex,&nextClusterIndex);
		// 	}
		// }else{
		// 	curClusterIndex=nextClusterIndex;
		// }
	}while(1);
}

void GetNameFromRecord(Record record,PCHAR fullname)
{
	UINT i=0,j=0,point=0;
	for(i=0;i<8;i++)
	{
		if(record.filename[i]==' ')
		{	
			break;
		}
		if(record.filename[i]>='A'&&record.filename[i]<='Z')
		{
			fullname[j++]=record.filename[i]+32;
		}else{
			fullname[j++]=record.filename[i];
		}
	}
	point=j;
	fullname[j++]='.';
	for(i=0;i<3;i++)
	{
		if(record.extension[i]>='A'&&record.extension[i]<='Z')
		{
			fullname[j++]=record.extension[i]+32;
		}else{
			fullname[j++]=record.extension[i];
		}
	}
	fullname[j] = 0;
	for(j=j-1;j>=0;j--)//去掉后面的空格
	{
		if(fullname[j]==' '||(fullname[j]=='.'&&j==point))
		{
			fullname[j]=0;
		}else{
			break;
		}
	}
}

void CreateRecord(PCHAR filename,BYTE type,DWORD startCluster,DWORD size,PRecord precord)
{
	WORD time[2];
	CHAR name[256]={0};
	CHAR ext[256]={0};
	if(type==(BYTE)0x08||type==(BYTE)0x10)
	{
		FormatDirNameAndExt(filename,name,ext);
		precord->sysReserved=0x08;
	}else{
		FormatFileNameAndExt(filename,name,ext);
		precord->sysReserved=0x18;
	}
	TimeToBytes(time);//获取当前时间
	strcpy(precord->filename,name);
	strcpy(precord->extension,ext);
	precord->proByte=type;
	precord->createMsecond=0;
	precord->createTime=time[1];
	precord->createDate=time[0];
	precord->lastAccessDate=time[0];
	precord->highClusterNum=(WORD)(startCluster>>16);
	precord->lastModifiedTime=time[1];
	precord->lastModifiedDate=time[0];
	precord->lowClusterNum=(WORD)(startCluster&0x0000ffff);
	precord->filelength=size;
}

STATE WriteRecord(SUPER_BLOCK *psb, Record record,DWORD sectorIndex,DWORD off_in_sector)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;

	BYTE *buf=NULL;

	buf = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));
	ReadSector(dev, buf,sectorIndex);
	memcpy(buf+off_in_sector,&record,sizeof(Record));
	WriteSector(dev, buf,sectorIndex);
    sys_free(buf);
	return OK;
}

STATE WriteFAT(SUPER_BLOCK *psb, DWORD totalclusters,PDWORD clusters)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;

	PBYTE buf=NULL;
	DWORD i=0,curSectorIndex=0,preSectorIndex=0,offset=0,off_in_sector=0;
	
	buf = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));
	if(buf==NULL)
	{
		return SYSERROR;
	}
	for(i=0;i<totalclusters;i++)
	{
		offset=8+(clusters[i]-2)*sizeof(DWORD);
		curSectorIndex=Reserved_Sector+offset/Bytes_Per_Sector;
		off_in_sector=offset%Bytes_Per_Sector;
		if(curSectorIndex!=preSectorIndex)//两个簇号不在同一个扇区。
		{
			if(preSectorIndex!=0)
			{
				WriteSector(dev, buf,preSectorIndex);//先把上一个扇区的内容写入FAT1中
			}
			ReadSector(dev, buf,curSectorIndex);
			preSectorIndex=curSectorIndex;
		}
		if(i<totalclusters-1)
		{
			memcpy(buf+off_in_sector,clusters+i+1,sizeof(DWORD));
		}else{
			DWORD dword = FAT_END;
			memcpy(buf+off_in_sector,&dword,sizeof(DWORD));
		}
		preSectorIndex=curSectorIndex;
	}
	WriteSector(dev, buf,curSectorIndex);
	//暂时用不上FAT2所以不写FAT2
	sys_free(buf);
	return OK;
}

STATE AllotClustersForEmptyFile(SUPER_BLOCK *psb, PFile pfile,DWORD size)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);

	DWORD n=0,bytes_per_cluster=0;
	PDWORD clusters=NULL;

	bytes_per_cluster=Sectors_Per_Cluster*Bytes_Per_Sector;
	if(size%bytes_per_cluster==0)
	{
		n=size/bytes_per_cluster;//n表示实际要占用的簇的个数
	}else{
		n=size/bytes_per_cluster+1;
	}
	clusters = (PDWORD)K_PHY2LIN(sys_kmalloc(n*sizeof(DWORD)));
	if(clusters==NULL)
	{
		return SYSERROR;
	}
	if(!FindClusterForFile(psb, n,clusters))//空间不足
	{
		return INSUFFICIENTSPACE;
	}
	pfile->start=clusters[0];
	WriteFAT(psb, n,clusters);
	sys_free(clusters);
	return OK;
}

STATE AddCluster(SUPER_BLOCK *psb, DWORD startClusterIndex,DWORD num)//cluster表示该文件或目录所在目录的簇,num表示增加几个簇
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	PDWORD clusters=NULL;
	DWORD curClusterIndex=startClusterIndex,nextClusterIndex=0;
	DWORD bytes_per_sector=Sectors_Per_FAT*Bytes_Per_Sector;
	STATE state;
	clusters = (PDWORD)K_PHY2LIN(sys_kmalloc((num+1)*sizeof(DWORD)));
	if(clusters==NULL)
	{
		return SYSERROR;
	}
	state=FindClusterForFile(psb, num,clusters+1);
	if(state!=OK)
	{
		return state;
	}
	do
	{
		GetNextCluster(psb, curClusterIndex,&nextClusterIndex);
		if(nextClusterIndex==FAT_END)
		{
			clusters[0]=curClusterIndex;
			break;
		}else{
			curClusterIndex=nextClusterIndex;
		}
	}while(1);
	WriteFAT(psb, num+1,clusters);
	sys_free(clusters);
	return OK;
}

STATE NeedMoreCluster(SUPER_BLOCK *psb, PFile pfile,DWORD size,PDWORD number)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);

	DWORD n=0,clusterNum,bytes_per_cluster=0;

	bytes_per_cluster=Sectors_Per_Cluster*Bytes_Per_Sector;
	if(pfile->off == 0) {
		return FALSE;
	}
	if(pfile->off%bytes_per_cluster==0)//clusterNum是实际占用的簇的个数
	{
		clusterNum=pfile->off/bytes_per_cluster;
	}else{
		clusterNum=pfile->off/bytes_per_cluster+1;
	}
	if(size>clusterNum*bytes_per_cluster-pfile->off)//空间不足需要追加更多的簇
	{
		if((size-(clusterNum*bytes_per_cluster-pfile->off))%bytes_per_cluster==0)
		{
			n=(size-(clusterNum*bytes_per_cluster-pfile->off))/bytes_per_cluster;
		}else{
			n=(size-(clusterNum*bytes_per_cluster-pfile->off))/bytes_per_cluster+1;
		}
		*number=n;
		return TRUE;//表示还需要n簇
	}
	return FALSE;//表示目前空间够用，不需要更多的簇
}

STATE FindClusterForFile(SUPER_BLOCK *psb, DWORD totalClusters,PDWORD clusters)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;

	PBYTE buf=NULL;
	DWORD clusterIndex=1,nextClusterIndex=0;
	UINT index=0,i=0,j=0;
	DWORD curSectorIndex=0,off_in_sector=0;

	buf = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));	
	if(buf==NULL)
	{
		return SYSERROR;
	}
	curSectorIndex=Reserved_Sector;
	off_in_sector=8;
	do
	{
		ReadSector(dev, buf,curSectorIndex);
		for(i=off_in_sector;i<Bytes_Per_Sector;i+=sizeof(DWORD))
		{
			memcpy(&nextClusterIndex,buf+i,sizeof(DWORD));
			clusterIndex++;
			if(nextClusterIndex==0)
			{
				clusters[index++]=clusterIndex;
				if(index>=totalClusters)//找够了
				{
					sys_free(buf);
					for(j=0;j<totalClusters;j++)
					{
						ClearClusters(psb, clusters[j]);//清空这些簇
					}
					return OK;
				}
			}
		}
		if(curSectorIndex*Bytes_Per_Sector>=Position_Of_FAT2)
		{
			sys_free(buf);
			return INSUFFICIENTSPACE;
		}else{
			curSectorIndex++;
			off_in_sector=0;
		}
	}while(1);
}

void ClearClusters(SUPER_BLOCK *psb, DWORD cluster)
{
    DECLARE_SUPER_BLOCK_VARIABLES(psb);
	int dev = psb->sb_dev;

	BYTE buf[512]={0};
	UINT sectorIndex=0;
	UINT first=0,last=0;

	first=Reserved_Sector+2*Sectors_Per_FAT+(cluster-2)*Sectors_Per_Cluster;
	last=first+8;
	
	for(sectorIndex=first;sectorIndex<last;sectorIndex++)
	{
		WriteSector(psb, buf,sectorIndex);
	}
}

int strcmp(const char * s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = s1;
	const char * p2 = s2;

	for (; *p1 && *p2; p1++,p2++) {
		if (*p1 != *p2) {
			break;
		}
	}

	return (*p1 - *p2);
}
