#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include <stddef.h>
char path[255];
char filename[14]; //dirsize max is 14
void print_file(char *buf)
{
    uint index=strlen(buf);
        while(index>=0 && buf[index]!='/')
        {
            index--;
        }
        index++;
        char name[255];
        memset(name,'\0',sizeof(name));
        for(int i =0;index<strlen(buf);i++,index++)
        {
            name[i]=buf[index];
        }
        //printf("file name %s\n",name);
        if(strcmp(name,filename)==0)
        {
            printf("%s\n",buf);
        }
}

void find(char *path)
{
    int fd;
    struct stat st;
    
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
    }
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
    }
    if(st.type==T_FILE)
    {
        close(fd);
        print_file(path);
    }
    else if(st.type==T_DIR)
    {
        //遍历dir
        
        char * p = path+strlen(path);//p指向字符串末尾在buf的位置
        *p++ = '/';//附加一个“/”符号
        //printf("path is %s\n",path);
        struct dirent inode;

        while(read(fd,&inode,sizeof(inode)) == sizeof(inode))
        {
            if(inode.inum == 0) continue;
            if(strcmp(".",inode.name)==0 || strcmp("..",inode.name)==0)
            continue;
            memmove(p, inode.name, DIRSIZ);
            find(path);
        }
        close(fd);
    }
}
int main(int argc, char *argv[])
{
  //应该接受两个参数一个是开始查找的路径一个是文件名称
  if(argc!=3)
  {
    printf("find: args num error!\n");
    exit(-1);
  }
  strcpy(filename,argv[2]);
  strcpy(path,argv[1]);
  find(path);
  exit(0);
  return 0;
}
