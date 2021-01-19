/**********************************************************
*	vfs.c       //added by mingxuan 2019-5-17
***********************************************************/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "fs_misc.h"
#include "vfs.h"
#include "fat32.h"

//PRIVATE struct device  device_table[NR_DEV];  //deleted by mingxuan 2020-10-18
PRIVATE struct vfs  vfs_table[NR_FS];   //modified by mingxuan 2020-10-18

PUBLIC struct file_desc f_desc_table[NR_FILE_DESC];
PUBLIC struct super_block super_block[NR_SUPER_BLOCK]; //added by mingxuan 2020-10-30

//PRIVATE struct file_op f_op_table[NR_fs]; //文件系统操作表
PRIVATE struct file_op f_op_table[NR_FS_OP]; //modified by mingxuan 2020-10-18
PRIVATE struct sb_op   sb_op_table[NR_SB_OP];   //added by mingxuan 2020-10-30

PRIVATE int strcmp(const char *s1, const char *s2);

//PRIVATE void init_dev_table();//deleted by mingxuan 2020-10-30
PRIVATE void init_vfs_table();  //modified by mingxuan 2020-10-30
PUBLIC void init_file_desc_table();   //added by mingxuan 2020-10-30
PUBLIC void init_fileop_table();
PUBLIC void init_super_block_table();  //added by mingxuan 2020-10-30

PUBLIC struct vfs* vfs_alloc_vfs_entity()
{
    int i;
    for (int i = 0; i < NR_FS; i++)
    {
        if (!vfs_table[i].used)
        {
            return &vfs_table[i];
        }
    }
    return 0;
}

PRIVATE int get_index(char path[]);

PUBLIC void init_vfs(){

    init_file_desc_table();
    init_fileop_table();

    init_super_block_table();
    init_sb_op_table(); //added by mingxuan 2020-10-30

    //init_dev_table(); //deleted by mingxuan 2020-10-30
    init_vfs_table();   //modified by mingxuan 2020-10-30
}

//added by mingxuan 2020-10-30
PUBLIC void init_file_desc_table(){
    int i;
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));
}

PUBLIC void init_fileop_table(){
    // table[0] for tty
    f_op_table[0].open = real_open;
    f_op_table[0].close = real_close;
    f_op_table[0].write = real_write;
    f_op_table[0].lseek = real_lseek;
    f_op_table[0].unlink = real_unlink;
    f_op_table[0].read = real_read;

    // table[1] for orange
    f_op_table[1].open = real_open;
    f_op_table[1].close = real_close;
    f_op_table[1].write = real_write;
    f_op_table[1].lseek = real_lseek;
    f_op_table[1].unlink = real_unlink;
    f_op_table[1].read = real_read;

    // table[2] for fat32
    // f_op_table[2].create = CreateFile;
    // f_op_table[2].delete = DeleteFile;
    // f_op_table[2].open = OpenFile;
    // f_op_table[2].close = CloseFile;
    // f_op_table[2].write = WriteFile;
    // f_op_table[2].read = ReadFile;
    // f_op_table[2].lseek = LSeek;
    // f_op_table[2].opendir = OpenDir;
    // f_op_table[2].createdir = CreateDir;
    // f_op_table[2].deletedir = DeleteDir;
    // f_op_table[2].chdir = fat32_chdir;

    // table[2] for fat32
    f_op_table[2].CreateFile = CreateFile;
    f_op_table[2].DeleteFile = DeleteFile;
    f_op_table[2].OpenFile = OpenFile;
    f_op_table[2].CloseFile = CloseFile;
    f_op_table[2].WriteFile = WriteFile;
    f_op_table[2].ReadFile = ReadFile;
    f_op_table[2].LSeek = LSeek;
    f_op_table[2].OpenDir = OpenDir;
    f_op_table[2].CreateDir = CreateDir;
    f_op_table[2].DeleteDir = DeleteDir;
    f_op_table[2].ChangeDir = fat32_chdir;

}

//added by mingxuan 2020-10-30
PUBLIC void init_super_block_table(){
    struct super_block * sb = super_block;						//deleted by mingxuan 2020-10-30

    //super_block[0] is tty0, super_block[1] is tty1, uper_block[2] is tty2
    for(; sb < &super_block[3]; sb++) {
        sb->sb_dev =  DEV_CHAR_TTY;
        sb->fs_type = TTY_FS_TYPE;
        sb->used = 1;
    }

    //super_block[3] is orange's superblock
    sb->sb_dev = DEV_HD;
    sb->fs_type = ORANGE_TYPE;
    sb->used = 1;
    sb++;

    //super_block[4] is fat32's superblock
    // sb->sb_dev = DEV_HD;
    // sb->fs_type = FAT32_TYPE;
    // sb++;

    //another super_block are free
    for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)			//deleted by mingxuan 2020-10-30
	{
    	sb->sb_dev = NO_DEV;
        sb->fs_type = NO_FS_TYPE;
        sb->used = 0;
    }
}

//added by mingxuan 2020-10-30
PUBLIC void init_sb_op_table(){
    //orange
    sb_op_table[0].read_super_block = read_super_block;
    sb_op_table[0].get_super_block = get_super_block;

    //fat32 and tty
    sb_op_table[1].read_super_block = NULL;
    sb_op_table[1].get_super_block = NULL;
}

//PRIVATE void init_dev_table(){
PRIVATE void init_vfs_table(){  // modified by mingxuan 2020-10-30

    // 我们假设每个tty就是一个文件系统
    // tty0
    // device_table[0].dev_name="dev_tty0";
    // device_table[0].op = &f_op_table[0];
    vfs_table[0].fs_name = "dev_tty0"; //modifed by mingxuan 2020-10-18
    vfs_table[0].op = &f_op_table[0];
    vfs_table[0].sb = &super_block[0];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[0].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30
    vfs_table[0].used = 1;

    // tty1
    //device_table[1].dev_name="dev_tty1";
    //device_table[1].op =&f_op_table[0];
    vfs_table[1].fs_name = "dev_tty1"; //modifed by mingxuan 2020-10-18
    vfs_table[1].op = &f_op_table[0];
    vfs_table[1].sb = &super_block[1];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[1].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30
    vfs_table[1].used = 1;

    // tty2
    //device_table[2].dev_name="dev_tty2";
    //device_table[2].op=&f_op_table[0];
    vfs_table[2].fs_name = "dev_tty2"; //modifed by mingxuan 2020-10-18
    vfs_table[2].op = &f_op_table[0];
    vfs_table[2].sb = &super_block[2];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[2].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30
    vfs_table[2].used = 1;

    // orangefs
    vfs_table[3].fs_name = "orange";
    vfs_table[3].op = &f_op_table[1];
    vfs_table[3].sb = &super_block[3];  
    vfs_table[3].s_op = &sb_op_table[0];
    vfs_table[3].used = 1;

    // fat32
    vfs_table[4].fs_name = "fat0";
    vfs_table[4].op = &f_op_table[2];
    vfs_table[4].sb = &super_block[4];
    vfs_table[4].s_op = &sb_op_table[1];
    vfs_table[4].used = 0; //动态绑定

    // fat32
    vfs_table[5].fs_name = "fat1";
    vfs_table[5].op = &f_op_table[2];
    vfs_table[5].sb = &super_block[5];
    vfs_table[5].s_op = &sb_op_table[1];
    vfs_table[5].used = 0; //动态绑定 
}

PRIVATE int get_fs_len(const char *path) {
  int pathlen = strlen(path);
  //char dev_name[DEV_NAME_LEN];
  char fs_name[DEV_NAME_LEN] = {0};   //modified by mingxuan 2020-10-18
  int len = (pathlen < DEV_NAME_LEN) ? pathlen : DEV_NAME_LEN;
  int i,a=0;
  for(i=0;i<len;i++){
    if( path[i] == '/'){
      a=i;
      a++;
      break;
    }
    else {
      //dev_name[i] = path[i];
      fs_name[i] = path[i];   //modified by mingxuan 2020-10-18
    }
  }
  return strlen(fs_name);
}

PRIVATE int get_index(char path[]){

  int pathlen = strlen(path);
    //char dev_name[DEV_NAME_LEN];
    char fs_name[DEV_NAME_LEN];   //modified by mingxuan 2020-10-18
    int len = (pathlen < DEV_NAME_LEN) ? pathlen : DEV_NAME_LEN;

    int i,a=0;
    for(i=0;i<len;i++){
        if( path[i] == '/'){
            a=i;
            a++;
            break;
        }
        else {
            //dev_name[i] = path[i];
            fs_name[i] = path[i];   //modified by mingxuan 2020-10-18
        }
    }
    //dev_name[i] = '\0';
    fs_name[i] = '\0';  //modified by mingxuan 2020-10-18
    for(i=0;i<pathlen-a;i++)
        path[i] = path[i+a];
    path[pathlen-a] = '\0';

    //for(i=0;i<NR_DEV;i++)
    for(i=0;i<NR_FS;i++)    //modified by mingxuan 2020-10-29
    {
        // if(!strcmp(dev_name, device_table[i].dev_name))
        if(!strcmp(fs_name, vfs_table[i].fs_name)) //modified by mingxuan 2020-10-18
            return i;
    }

    return -1;
}


/*======================================================================*
                              sys_* 系列函数
 *======================================================================*/

PUBLIC int sys_open(void *uesp)
{
    return do_vopen(get_arg(uesp, 1), get_arg(uesp, 2));
}

PUBLIC int sys_close(void *uesp)
{
    return do_vclose(get_arg(uesp, 1));
}

PUBLIC int sys_read(void *uesp)
{
    return do_vread(get_arg(uesp, 1), get_arg(uesp, 2), get_arg(uesp, 3));
}

PUBLIC int sys_write(void *uesp)
{
    return do_vwrite(get_arg(uesp, 1), get_arg(uesp, 2), get_arg(uesp, 3));
}

PUBLIC int sys_lseek(void *uesp)
{
    return do_vlseek(get_arg(uesp, 1), get_arg(uesp, 2), get_arg(uesp, 3));
}

PUBLIC int sys_unlink(void *uesp) {
    return do_vunlink(get_arg(uesp, 1));
}

PUBLIC int sys_create(void *uesp) {
    return do_vcreate(get_arg(uesp, 1));
}

PUBLIC int sys_delete(void *uesp) {
    return do_vdelete(get_arg(uesp, 1));
}

PUBLIC int sys_opendir(void *uesp) {
    return do_vopendir(get_arg(uesp, 1));
}

PUBLIC int sys_createdir(void *uesp) {
    return do_vcreatedir(get_arg(uesp, 1));
}

PUBLIC int sys_deletedir(void *uesp) {
    return do_vdeletedir(get_arg(uesp, 1));
}

PUBLIC int sys_readdir(void *uesp) {
    return do_vreaddir(get_arg(uesp, 1), get_arg(uesp, 2), get_arg(uesp, 3));
    //return ReadDir((PCHAR)get_arg(uesp, 1), (PDWORD)get_arg(uesp, 2), (PCHAR)get_arg(uesp, 3));
}

//added by ran
PUBLIC int sys_chdir(void *uesp) {
  return do_vchdir((PCHAR)get_arg(uesp, 1));
}

//added by ran
PUBLIC int sys_getcwd(void *uesp) {
  return (int)do_vgetcwd((PCHAR)get_arg(uesp, 1), (PDWORD)get_arg(uesp, 2));
}


/*======================================================================*
                              do_v* 系列函数
 *======================================================================*/

PUBLIC int do_vopen(const char *path, int flags) {

    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index,i;
    int fd = -1;
    index = get_index(pathname);
    if(index == -1){
        disp_str("pathname error!\n");
        disp_str(path);
        return -1;
    }
    struct vfs *pvfs = &vfs_table[index];
    if (pvfs->sb->fs_type == FAT32_TYPE)
    {
        fd = pvfs->op->OpenFile(pvfs->sb, pathname, flags);
    }
    else
    {
        fd = vfs_table[index].op->open(pathname, flags);
    }

    if(fd != -1)
    {
        p_proc_current -> task.filp[fd] -> dev_index = index;
        //disp_str("          open file success!\n");   //deleted by mingxuan 2019-5-22
    } 
    else {
        disp_str("          error!\n");
    }

    return fd;
}


PUBLIC int do_vclose(int fd) {
    int index = p_proc_current->task.filp[fd]->dev_index;
    struct vfs* pvfs = &vfs_table[index];
    
    int result;
    if (pvfs->sb->fs_type == FAT32_TYPE)
    {
        result = pvfs->op->CloseFile(pvfs->sb, fd);
    }
    else
    {
        result = pvfs->op->close(fd);
    }

    return result;
    
    //return device_table[index].op->close(fd);
    //return vfs_table[index].op->close(fd);  //modified by mingxuan 2020-10-18
    // if (state == 1) {
    //     debug("          close file success!");
    // } else {
	// 	DisErrorInfo(state);
    // }
}

PUBLIC int do_vread(int fd, char *buf, int count) {
    //disp_int(fd);
    int index = p_proc_current->task.filp[fd]->dev_index;
    struct vfs* pvfs = &vfs_table[index];
    
    int result;
    if (pvfs->sb->fs_type == FAT32_TYPE)
    {
        result = pvfs->op->ReadFile(pvfs->sb, fd, buf, count);
    }
    else
    {
        result = pvfs->op->read(fd, buf, count);
    }

    return result;

    //disp_int(index);
    //return device_table[index].op->read(fd, buf, count);
    //return vfs_table[index].op->read(fd, buf, count);   //modified by mingxuan 2020-10-18
    // if (size >= 0) {
    //     debug("          read file success!");
    // } else {
	// 	DisErrorInfo(size);
    // }
    // return size;
}

PUBLIC int do_vwrite(int fd, const char *buf, int count) {
    //char s[256];  //deleted by mingxuan 2019-5-19
    /*  //deleted by mingxuan 2019-5-23
    char s[FILE_MAX_LEN]; //modified by mingxuan 2019-5-19

    strcpy(s, buf);

    int index = p_proc_current->task.filp[fd]->dev_index;
    return device_table[index].op->write(fd,s,strlen(s));
    */

    //modified by mingxuan 2019-5-23
    char s[512];
    int index = p_proc_current->task.filp[fd]->dev_index;
    struct vfs* pvfs = &vfs_table[index];
    int fs_type = pvfs->sb->fs_type;
    char *fsbuf = buf;
    int f_len = count;
    int bytes;
    while(f_len)
    {
        int iobytes = min(512, f_len);
        int i=0;
        for(i=0; i<iobytes; i++)
        {
            s[i] = *fsbuf;
            fsbuf++;
        }
        if (fs_type == FAT32_TYPE)
        {
            bytes = pvfs->op->WriteFile(pvfs->sb, fd, s, iobytes);
        }
        else
        {
            bytes = pvfs->op->write(fd, s, iobytes);
        }
        
        //bytes = device_table[index].op->write(fd,s,iobytes);
        //bytes = vfs_table[index].op->write(fd,s,iobytes);   //modified by mingxuan 2020-10-18
        if(bytes != iobytes)
        {
            return bytes;
        }
        f_len -= bytes;
    }
    return count;

    // int fd = get_arg(uesp,1);
    // int index = p_proc_current->task.vfs_filp[fd]->index_of_optable;
    // int d = p_proc_current -> task.vfs_filp[fd] -> fd;
    // int state = f_op_table[index].write(get_arg(uesp,2), get_arg(uesp,3), d);
    // if (state == 1) {
    //     debug("          write file success!");
    // } else {
	// 	DisErrorInfo(state);
    // }
}

PUBLIC int do_vunlink(const char *path) {
    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    index = get_index(pathname);
    if(index==-1){
        disp_str("pathname error!\n");
        return -1;
    }

    //return device_table[index].op->unlink(pathname);
    return vfs_table[index].op->unlink(pathname);   //modified by mingxuan 2020-10-18
}

PUBLIC int do_vlseek(int fd, int offset, int whence) {
    int index = p_proc_current->task.filp[fd]->dev_index;
    struct vfs *pvfs = &vfs_table[index];

    int result;
    if (pvfs->sb->fs_type == FAT32_TYPE)
    {
        result = pvfs->op->LSeek(fd, offset, whence);
    }
    else
    {
        result = pvfs->op->lseek(fd, offset, whence);
    }

    return result;

    //return device_table[index].op->lseek(fd, offset, whence);
    //return vfs_table[index].op->lseek(fd, offset, whence);  //modified by mingxuan 2020-10-18

}

//PUBLIC int do_vcreate(char *pathname) {
PUBLIC int do_vcreate(char *filepath) { //modified by mingxuan 2019-5-17
    // disp_str("hhh");
    // const char *path = get_arg(uesp,1);

    // int pathlen = strlen(path);
    // char pathname[MAX_PATH];

    // strcpy(pathname,path);
    // pathname[pathlen] = '\0';

    // int index;
    // index = get_index(pathname);
    // disp_int(index);
    // if(index==-1){
    //     disp_str("          pathname error!\n");
    //     return -1;
    // }


    // int state = 0;

    // //int state = device_table[index].op -> create(pathname);
    // if (state == 1) {
    //     debug("          create file success!");
    // } else {
	// 	DisErrorInfo(state);
    // }
    // return state;

    //added by mingxuan 2019-5-17
    int state;
    const char *path = filepath;

    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    //index = (int)(pathname[1]-'0');   //deleted by mingxuan 2019-5-17
    //disp_int(index);  //deleted by mingxuan 2019-5-17
    index = get_index(pathname);
    if(index == -1){
        disp_str("pathname error!\n");
        disp_str(path);
        return -1;
    }

    struct vfs *pvfs = &vfs_table[index];
    
    if (pvfs->sb->fs_type == FAT32_TYPE)
    {
        state = pvfs->op->CreateFile(pvfs->sb, pathname);
    }
    else
    {
        state = pvfs->op->create(pathname);
    }

    /*
    for(int j=0;j<= pathlen-3;j++)
    {
        pathname[j] = pathname[j+3];
    }
    */
    //state = f_op_table[index].create(pathname);
    //state = device_table[index].op->create(pathname);
    //state = vfs_table[index].op->create(pathname); //modified by mingxuan 2020-10-18
//    if (state == 1) {
//        debug("          create file success!");
//    } else {
//		DisErrorInfo(state);
//    }
    return state;
}

PUBLIC int do_vdelete(char *path) {

    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    index = get_index(pathname);
    if(index==-1){
        disp_str("pathname error!\n");
        return -1;
    }

    struct vfs *pvfs = &vfs_table[index];

    int result;
    if (pvfs->sb->fs_type == FAT32_TYPE)
    {
        result = pvfs->op->DeleteFile(pvfs->sb, pathname);
    }
    else
    {
        result = pvfs->op->delete(pathname);
    }

    return result;

    //return device_table[index].op->delete(pathname);
    //return vfs_table[index].op->delete(pathname);   //modified by mingxuan 2020-10-18
    // state = f_op_table[index].delete(pathname);
    // if (state == 1) {
    //     debug("          delete file success!");
    // } else {
	// 	DisErrorInfo(state);
    // }
}

// FAT32使用chdir改变工作目录
PUBLIC int do_vopendir(char *path) {
    int state;

    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    // deleted by pg999w 2021
    //index = (int)(pathname[1]-'0');
    // add by pg999w 2021
    index = get_index(pathname);

    // deleted by pg999w 2021
    // state = f_op_table[index].op->opendir(pathname);
    // add by pg999w 2021
    state = vfs_table[index].op->opendir(pathname);
//    if (state == 1) {
//        debug("          open dir success!");
//    } else {
//		DisErrorInfo(state);
//    }
    return state;
}

PUBLIC int do_vcreatedir(char *path) {
    int state;

    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    //index = (int)(pathname[1]-'0');
    int fs_len = get_fs_len(path) + 1;
    index = get_index(pathname);

    struct vfs *pvfs = &vfs_table[index];

    if (pvfs->sb->fs_type == FAT32_TYPE)
    {
        state = pvfs->op->CreateDir(pvfs->sb, pathname);
    }
    else
    {
        state = pvfs->op->createdir(pathname);
    }
    
    //state = f_op_table[index].createdir(pathname);
    //state = vfs_table[index].op->createdir(pathname);
//    if (state == 1) {
//        debug("          create dir success!");
//    } else {
//		DisErrorInfo(state);
//    }
    return state;
}

PUBLIC int do_vdeletedir(char *path) {
    int state;
    int pathlen = strlen(path);
    char pathname[MAX_PATH];

    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    //index = (int)(pathname[1]-'0');
    index = get_index(pathname);

    struct vfs *pvfs = &vfs_table[index];

    if (pvfs->sb->fs_type == FAT32_TYPE)
    {
        state = pvfs->op->DeleteDir(pvfs->sb, pathname);
    }
    else
    {
        state = pvfs->op->deletedir(pathname);
    }

    //state = vfs_table[index].op->deletedir(pathname);
//    if (state == 1) {
//        debug("          delete dir success!");
//    } else {
//		DisErrorInfo(state);
//    }
    return state;
}


PRIVATE int strcmp(const char * s1, const char *s2)
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

//added by ran
PUBLIC int do_vchdir(const char *path) {
    char pathname[MAX_PATH];
    strcpy(pathname, path);
    int index = get_index(pathname);
    struct vfs *pvfs = &vfs_table[index];
    int state = pvfs->op->ChangeDir(pvfs->sb, pathname);
    return state;
}

//added by ran
PUBLIC char* do_vgetcwd(char *buf, int size) {
    if (!buf)
    {
        return 0;
    }
    strncpy(buf, p_proc_current->task.cwd, size);
    return buf;
}

PUBLIC int do_vreaddir(PCHAR dirname, DWORD dir[3], PCHAR filename)
{
    char pathname[MAX_PATH];
    strcpy(pathname, dirname);
    int index = get_index(pathname);
    struct vfs *pvfs = &vfs_table[index];
    pvfs->op->ReadDir(pvfs->sb, pathname, dir, filename);
}