#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if(argc!=1)
    {
        fprintf(2, "Usage: pingpong argument error...\n");
        exit(1);
    }
    int p1[2];
    int p2[2];
    pipe(p1);
    pipe(p2);
    char message[1]="a";
    if(fork()==0) //father
    {
        write(p1[1],&message,sizeof(message)); //write
        char temp='0';
        while (read(p2[0],&temp,1)==0){}
        printf("%d: received pong\n",getpid());
    }
    else //child
    {
        char temp='0';
        while (read(p1[0],&temp,1)==0){}
        printf("%d: received ping\n",getpid());
        write(p2[1],&temp,sizeof(temp));
    }
    exit(0);
}
