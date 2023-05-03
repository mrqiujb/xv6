#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char **argv)
{
    //no use args
    if(argc!=1)
    {
        fprintf(2, "usage: args is error\n");
        exit(1);
    }
    int fd[2]; //fd[0] --read fd[1] --write
    if(pipe(fd)!=0)
    {
        fprintf(2, "usage: create pipe faild!\n");
        exit(1);
    }
    int pid=fork();
    if(pid<0) //创建失败
    {
        fprintf(2, "usage: create process faild!\n");
        exit(1);
    } 
    else if(pid==0) //子进程
    {
        sleep(5);
        char buf[255];
        char *msg="pong";

        if(read(fd[0], buf, sizeof(buf))==-1)
        {
            fprintf(2, "%d: read faild!\n",getpid());
            exit(1);
        }
        close(fd[0]);

        printf("%d: received ping\n",getpid());

        if(write(fd[1], msg, strlen(msg))==-1)
        {
            fprintf(2, "%d: write faild!\n",getpid());
            exit(1);
        }
        close(fd[1]);
        exit(0);
    }
    else //父进程
    {
        char buf[255];
        char *msg="ping";
        if(write(fd[1], msg, strlen(msg))==-1)
        {
            fprintf(2, "%d: write faild!\n",getpid());
            exit(1);
        }
        close(fd[1]);
        
        sleep(30);
        if(read(fd[0], buf, sizeof(buf))==-1)
        {
            fprintf(2, "%d: read faild!\n",getpid());
            exit(1);
        }
        close(fd[0]);
        printf("%d: received pong\n",getpid());
        exit(0);
    }
    exit(0);
}