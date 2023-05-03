#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "stddef.h"

int main(int argc, char *argv[])
{
    if(argc<=1) exit(0);
    char *argvs[MAXARG];
    int countargs=0;
    for(int i =1;i<argc ;i++)
    {
        argvs[countargs++]=argv[i];
    }
    char * tem=(char *)malloc(sizeof(char)*20);
    char tem_char='0';
    int count=0;
    while(read(0,&tem_char,sizeof(char)))
    {
        if(tem_char=='\n')
        {
            tem[count]='\0';
            argvs[countargs]=tem;
            
            int pid = fork();
                if(pid < 0) {
                    printf("xargs : fork failed\n");
                    exit(1);
                }
                if(pid == 0) {
                    if(exec(argv[1], argvs) < 0){
                        printf("xargs : exec echo failed\n");
                        exit(1);
                    }
                    exit(0);
                }
                wait(NULL);
            count=0;
        }
        else
        {
            tem[count]=tem_char;
            count++;
            if(count>=18) exit(1);
        }
    }
    
    exit(0);
    return 0;
}
