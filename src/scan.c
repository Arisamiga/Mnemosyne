#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/listbrowser.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include "scan.h"
#include "funcs.h"

long totalSize = 0;
struct List contents;

char pastPath[256];

void addToList(char *name, long size)
{
    UBYTE buffer[64];
    SNPrintf(buffer, 64, "%s", name);
    STRPTR buffer2 = longToString(size);
    printf("Adding to list: %s, %s\n", buffer, buffer2);
    struct Node *node = AllocListBrowserNode(3,
                                             LBNA_Column, 0,
                                             LBNCA_CopyText, TRUE,
                                             LBNCA_Text, buffer,
                                             LBNCA_MaxChars, 40,
                                             LBNA_Column, 1,
                                             LBNCA_CopyText, TRUE,
                                             LBNCA_Text, buffer2,
                                             LBNCA_MaxChars, 40,
                                             TAG_DONE);

    AddTail(&contents, node);
}

void scanPath(char *path, BOOL subFoldering, Object *listGadget)
{
    if (!subFoldering){
        printf("Path: %s\n", path);
        strncpy(pastPath, path, 256);
        NewList(&contents);
    }

    BPTR lockPath = Lock(path, ACCESS_READ);
    if (!lockPath)
    {
        printf("Path Doesn't Exist: %s\n", path);
        return;
    }

    // Check if path is file or folder with Examine()
    struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocVec(sizeof(struct FileInfoBlock), MEMF_CLEAR);

    if (!Examine(lockPath, fib))
    {
        printf("Examine Failed on path: %s\n", path);
        goto exit;
    }

    // If file return size
    if (fib->fib_DirEntryType < 0)
    {
        if (listGadget)
        {
            addToList(fib->fib_FileName, fib->fib_Size);
        }
        if (!subFoldering && !listGadget)
            printf("%s: %ld bytes\n", fib->fib_FileName, fib->fib_Size);

        totalSize += fib->fib_Size;
        goto exit;
    }
    // If folder scan recursivly and return size for each child
    if (fib->fib_DirEntryType > 0)
    {

        while (ExNext(lockPath, fib))
        {
            // TODO: Check that IoErr will return ERROR_NO_MORE_ENTRIES if it returns anything else throw error or notify user
            if (fib->fib_DirEntryType > 0)
            {
                // Scan SubFolders
                char newPath[256];
                strcpy(newPath, path);
                if (newPath[strlen(newPath) - 1] != ':' && newPath[strlen(newPath) - 1] != '/')
                {
                    strcat(newPath, "/");
                }
                strcat(newPath, fib->fib_FileName);
                // if(!subFoldering && !listGadget)
                //     printf("---- Scanning SubFolder: %s\n", newPath);
                long oldTotalSize = totalSize;
                scanPath(newPath, TRUE, 0);
                if (!subFoldering && !listGadget)
                {
                    strcat(fib->fib_FileName, "/");
                    // TODO: Manage names above 20 chars long so it doesn't break the table
                    printf("| %-20s: %12ld bytes\n", fib->fib_FileName, totalSize - oldTotalSize);
                }
                if (listGadget)
                {
                    strcat(fib->fib_FileName, "/");
                    addToList(fib->fib_FileName, totalSize - oldTotalSize);
                }
                continue;
            }
            if (listGadget)
            {
                addToList(fib->fib_FileName, fib->fib_Size);
            }
            if (!subFoldering && !listGadget)
                // TODO: Manage names above 20 chars long so it doesn't break the table
                printf("| %-20s: %12ld bytes\n", fib->fib_FileName, fib->fib_Size);
            totalSize += fib->fib_Size;
        }
    }
exit:
    if (listGadget)
    {
        SetAttrs(listGadget, LISTBROWSER_Labels, (ULONG)&contents, TAG_DONE);
    }
    if (!subFoldering && !listGadget)
    {
        printf("\n--> Total Size Of Path Given: %ld bytes\n\n", totalSize);
    }
    FreeVec(fib);
    UnLock(lockPath);
    return;
}