#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
void prime(int read_fd,int write_fd)
{
    int read_buf[32]={0};
    read(read_fd,read_buf,sizeof(int)*32);

    int first=read_buf[0];
    
    printf("prime %d\n",first);
    if(first==31) exit(0);
    int i=0,j=0;
    int wirte_buf[32]={0};
    while (read_buf[j]!=0)
    {
        if(read_buf[j]%first==0)
        {
            j++;
            continue;
        }
        wirte_buf[i++]=read_buf[j++];
    }
    
    if(fork()>0)
    {
        write(write_fd,wirte_buf,sizeof(int)*32);
        wait(0);
    }
    else
    {
        prime(read_fd,write_fd);
        wait(0);
    }
}
int main(int argc, char *argv[])
{
    if(argc!=1)
    {
        fprintf(2, "Usage: primes argument error...\n");
        exit(1);
    }
    int p[2];
    pipe(p);
    if(fork()>0) //father
    {
        for (int i = 2; i < 33; i++)
        {
            write(p[1],&i,sizeof(int));
        }
        wait(0);
    }
    else //child
    {
        prime(p[0],p[1]);
        wait(0);
    }
    exit(0);
}
