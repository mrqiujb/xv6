#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char **argv)
{
    int ticks;
    if(argc!=2 || argv[1]==0)
    {
        fprintf(2, "usage: args is error\n");
        exit(1);
    }
    int b=2;
    fprintf(2, "usage: args is error %d\n",b);
    ticks=atoi(argv[1]);
    sleep(ticks);
    exit(0);
}
