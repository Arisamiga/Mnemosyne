// This is where function that can be used by multiple files are defined.
#include <stdio.h>
#include <ctype.h>

#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <exec/types.h>

#include "funcs.h"

long get_file_size(char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
        return -1;

    if (fseek(fp, 0, SEEK_END) < 0)
    {
        fclose(fp);
        return -1;
    }

    long size = ftell(fp);
    // release the resources when not required
    fclose(fp);
    return size;
}

int GetListLength(struct List *list)
{
    struct Node *node;
    int count = 0;
    for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
        count++;
    return count;
}

void getParentPath(char *filename, char *result, int resultSize)
{
    BPTR fileLock = Lock(filename, SHARED_LOCK);
    if (fileLock)
    {
        BPTR folderLock = ParentDir(fileLock);
        NameFromLock(folderLock, result, resultSize);

        UnLock(folderLock);
        UnLock(fileLock);
    }
}
void getNameFromPath(char *path, char *result, unsigned int resultSize)
{
    BPTR pathLock = Lock(path, SHARED_LOCK);
    if (pathLock)
    {
        struct FileInfoBlock *FIblock = (struct FileInfoBlock *)AllocVec(sizeof(struct FileInfoBlock), MEMF_CLEAR);

        if (Examine(pathLock, FIblock))
        {
            strncpy(result, FIblock->fib_FileName, resultSize);
            FreeVec(FIblock);
        }
        UnLock(pathLock);
    }
}