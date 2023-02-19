#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
//argc : 参数的数量
//argv： 具体的参数 char **
// char * 字符串
//char ** 字符串数组

int main(int argc, char *argv[])
{
    //sleep x 
    if(argc!=2)
    {
        fprintf(2, "Usage: sleep argument error...\n");
        exit(1);
    }
    int nums=atoi(argv[1]);
    sleep(nums);
    exit(0);
}
