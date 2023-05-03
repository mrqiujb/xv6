#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


char *
fmtname(char *path)
{
    static char buf[DIRSIZ + 1] = {'\0'};
    char *p;
    // ./a/v/v/d/ssassssss.s
    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    // string char a[10] "sssssssss\0"
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), '\0', DIRSIZ - strlen(p));
    return buf;
}

void find(char *name, char *path)
{
    char buf[512] = {'\0'};
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type)
    {
        /*
            ./a/b ./b
            find . b
        */
    case T_FILE:
        if (strcmp(fmtname(path), name) == 0)
            printf("%s\n", path);
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        buf[strlen(path)] = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {

            if (de.inum == 0)
                continue;
            if (strcmp(".", de.name) == 0 || strcmp("..", de.name) == 0)
                continue;
            memmove(&buf[strlen(path) + 1], de.name, DIRSIZ);
            find(name, buf);
        }
        break;
    }
    close(fd);
}
// find
int main(int argc, char *argv[])
{
    // find xx xx
    if (argc != 3)
    {
        fprintf(2, "find: arg number error!\n");
        exit(1);
    }
    char *path = argv[1];
    char *name = argv[2];
    find(name, path);
    exit(0);
}
