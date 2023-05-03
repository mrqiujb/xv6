#include "kernel/types.h"
#include "user/user.h"
#include "stddef.h"

void primes(int readfd,int writefd)
{
    int prenum,nextnum;
    if(read(readfd,&prenum,sizeof(int)))
    {
        if(prenum==-1) return;
        printf("prime %d\n",prenum);
        int pid=fork();
        if(pid==0)
        {
          //傻逼了 光想节省资源使用一个管道
          //这段代码会导致一边读一边写死循环的节奏！！！
              while(read(readfd,&nextnum,sizeof(int)))
            {
              if(nextnum==-1) break;
              if(nextnum%prenum!=0)
              {
                  if(write(writefd,&nextnum,sizeof(int))==-1)
                  {
                    fprintf(2, "%d: write faild!\n",getpid());
                    exit(1);
                  }
              }
            }
            int end_flag=-1;
            if(write(writefd,&end_flag,sizeof(int)))
            {
              fprintf(2, "usage: wirte pipe is error\n");
              exit(1);
            }
            close(readfd);
            close(writefd);
            exit(0);
        }
        else if(pid>0)
        {
            wait(NULL);
            //printf("pid :%d write ok\n",getpid());
            primes(readfd,writefd);
        }
        else
        {
          fprintf(2, "usage: fork is error\n");
          exit(1);
        }
    }
    else
    {
      fprintf(2, "usage: read pipe is error\n");
      exit(1);
    }
    return;
}

// 创建一个进程为主进程 创建一个子进程将2-35写入管道，然后递归判断
//先输出第一个元素，然后将n的倍数的元素写入另外一个管道递归执行直到为空
int main(int argc, char **argv)
{
    int fd[2];
    if(pipe(fd)!=0) //经过pipe函数后，fd存的是两个数，拿到这两个数就能对管道读写
    {
      fprintf(2, "usage: create pipe is error\n");
      exit(1);
    }
    int pid=fork();
    if(pid==0)    //子进程
    {
        for(int i=2;i<36;i++)
        {
          //guide说用数字比acsii码好使，然后xv6 book上是char *buf。所以对i取地址
          if(write(fd[1],&i,sizeof(int))==-1)
          {
            fprintf(2, "usage: wirte pipe is error\n");
            exit(1);
          }
        }
        //标识符，因为只使用一个pipe需要区分一个那一次读写
        int end_flag=-1;
        if(write(fd[1],&end_flag,sizeof(int)))
        {
            fprintf(2, "usage: wirte pipe is error\n");
            exit(1);
        }
        exit(0);
        //执行完成结束占用
    }
    else if(pid>0) //父进程
    {
        wait(NULL);//Wait for a child to exit; exit status in *status; returns child PID.
        //数据已经全部写入管道了
        //guide上的意思应该是对管道复用否则系统资源不够分配
        //当进程没有结束的时候管道应该是不会被释放的
        fprintf(2, "pid %d: write ok!\n",getpid());
        primes(fd[0],fd[1]);
        exit(0);
    }
    else
    {
      fprintf(2, "usage: fork is error\n");
      exit(1);
    }
    return 0;
}

