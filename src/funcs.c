// This is where function that can be used by multiple files are defined.
#include <stdio.h>
#include <ctype.h>

#include <proto/alib.h>
#include <proto/utility.h>

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