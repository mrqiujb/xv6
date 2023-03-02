#include<stdio.h>
int sum_to(int n)
{
    int acc=0;
    for(int i=0;i<=n;i++)
    {
        acc+=i;
    }
    return acc;
}
int main()
{
    int a=sum_to(10);
    return a;
}