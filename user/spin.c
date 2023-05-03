/*
 * @Author: qiujingbao qiujingbao@qq.com
 * @Date: 2023-04-09 15:22:38
 * @LastEditors: qiujingbao qiujingbao@qq.com
 * @LastEditTime: 2023-04-09 15:25:11
 * @FilePath: /xv6-labs-2020/user/spin.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "kernel/types.h"
#include "user/user.h"

int main(int argc ,char ** argv)
{
    int pid;
    char c;
    pid=fork();
    if(pid==0)
    {
        c='/';
    }
    else
    {
        printf("parrent pid is %d , child is %d\n",getpid(),pid);
        c='\\';
    }
    for (int i = 0;; i++)
    {
        if(i%1000000==0)
        {
            write(2,&c,1);
        }
    }
    
}