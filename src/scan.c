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

#define MAX_BUFFER 256


#include "scan.h"
#include "funcs.h"

ULONG totalSize = 0;

int currentFormat = 0; // 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = TB

struct List contents;

char pastPath[256];

STRPTR returnFormat(void) {
    switch (currentFormat)
    {
    case 0:
        return "B";
        break;
    case 1:
        return "KB";
        break;
    case 2:
        return "MB";
        break;
    case 3:
        return "GB";
        break;
    case 4:
        return "TB";
        break;
    default:
        return "B";
        break;
    }
}

STRPTR returnFormatWithTotal(void){
    STRPTR buffer = AllocVec(64, MEMF_CLEAR);
    SNPrintf(buffer, 64, " (%ld %s)", totalSize, returnFormat());
    return buffer;
}

STRPTR returnGivenFormat(int format) {
    switch (format)
    {
    case 0:
        return "B";
        break;
    case 1:
        return "KB";
        break;
    case 2:
        return "MB";
        break;
    case 3:
        return "GB";
        break;
    case 4:
        return "TB";
        break;
    default:
        return "B";
        break;
    }
}

int correctFormat(ULONG size){
    if (size / 1024 / 1024 / 1024 / 1024 > 0)
        return 4;
    else if (size / 1024 / 1024 / 1024 > 0)
        return 3;
    else if (size / 1024 / 1024 > 0)
        return 2;
    else if (size / 1024 > 0)
        return 1;
    else
        return 0;
}

ULONG devideByFormat(ULONG size){
    switch (currentFormat)
        {
        case 0:
            return size;
            break;
        case 1:
            return size / 1024;
            break;
        case 2:
            return size / 1024 / 1024;
            break;
        case 3:
            return size / 1024 / 1024 / 1024;
            break;
        case 4:
            return size / 1024 / 1024 / 1024 / 1024;
            break;
        default:
            return size;
            break;
        }
}

ULONG devideByGivenFormat(ULONG size, int format){
    switch (format)
        {
        case 0:
            return size;
            break;
        case 1:
            return size / 1024;
            break;
        case 2:
            return size / 1024 / 1024;
            break;
        case 3:
            return size / 1024 / 1024 / 1024;
            break;
        case 4:
            return size / 1024 / 1024 / 1024 / 1024;
            break;
        default:
            return size;
            break;
        }
}

void addToTotalSize(ULONG size)
{
    if (totalSize + size < totalSize)
    {
        // switch to higher format
        currentFormat++;
        totalSize = totalSize / 1024;
        addToTotalSize(size);
        return;
    }

    totalSize += devideByFormat(size);
}

void addToList(char *name, ULONG size, STRPTR format)
{
    if (!format)
        format = returnFormat();
    // printf("Size: %ld\n", size);

    UBYTE *buffer = AllocVec(64, MEMF_CLEAR);
    SNPrintf(buffer, 64, "%s", name);

    STRPTR prebuffer2 = ULongToString(size);

    UBYTE *buffer2 = AllocVec(64, MEMF_CLEAR);
    SNPrintf(buffer2, 64, "%s %s", prebuffer2, format);

    struct Node *node = AllocListBrowserNode(3,
                                             LBNA_Column, 0,
                                             LBNCA_CopyText, TRUE,
                                             LBNCA_Text, buffer,
                                             LBNCA_MaxChars, 40,
                                             LBNA_Column, 1,
                                             LBNCA_CopyText, TRUE,
                                             LBNCA_Text, "",
                                             LBNCA_MaxChars, 40,
                                             LBNA_Column, 2,
                                             LBNCA_CopyText, TRUE,
                                             LBNCA_Text, buffer2,
                                             LBNCA_MaxChars, 40,
                                             LBNCA_Justification, LCJ_RIGHT,
                                             TAG_DONE);

    AddTail(&contents, node);
    FreeVec(buffer);
    FreeVec(prebuffer2);
    FreeVec(buffer2);
}

void scanPath(char *path, BOOL subFoldering, Object *listGadget)
{
    if (!subFoldering)
    {
		if (listGadget)
		{
			strncpy(pastPath, path, 256);
        	NewList(&contents);
		}
        totalSize = 0;
        currentFormat = 0;
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
            int format = correctFormat(fib->fib_Size);
            addToList(fib->fib_FileName, devideByGivenFormat(fib->fib_Size, format), returnGivenFormat(format));
        }
        if (!subFoldering && !listGadget)
        {
            int format = correctFormat(fib->fib_Size);
            printf("%s: %lu %s\n", fib->fib_FileName, devideByGivenFormat(fib->fib_Size, format), returnGivenFormat(format));
        }

        addToTotalSize(fib->fib_Size);
        goto exit;
    }

    // If folder scan recursivly and return size for each child
    if (fib->fib_DirEntryType > 0)
    {
        while (ExNext(lockPath, fib))
        {
			// Check if The next entity is a folder
            if (fib->fib_DirEntryType > 0)
            {
                // Scan SubFolders
                char *newPath = (char *)AllocVec(256, MEMF_CLEAR);
                strcpy(newPath, path);
                if (newPath[strlen(newPath) - 1] != ':' && newPath[strlen(newPath) - 1] != '/')
                {
                    strcat(newPath, "/");
                }
                strcat(newPath, fib->fib_FileName);
                // if(!subFoldering && !listGadget)
                //     printf("---- Scanning SubFolder: %s\n", newPath);

                ULONG oldTotalSize = totalSize;

                scanPath(newPath, TRUE, 0);

                if (!subFoldering && !listGadget)
                {
                    strcat(fib->fib_FileName, "/");
                    printf("| %-20s: %12lu %s\n", fib->fib_FileName, totalSize - oldTotalSize, returnFormat());
                }
                if (listGadget)
                {
                    strcat(fib->fib_FileName, "/");

                    if (totalSize < oldTotalSize){
                        // Devide oldTotalSize according to currentFormat
                        oldTotalSize = devideByFormat(oldTotalSize);
                    }
                    // printf("Total: %ld - %ld = %ld\n", totalSize, oldTotalSize, totalSize - oldTotalSize);
                    if((long)(totalSize - oldTotalSize) < 0 || currentFormat == 0){
                        int format = correctFormat(totalSize - oldTotalSize);
                        addToList(fib->fib_FileName, devideByGivenFormat(totalSize - oldTotalSize, format), returnGivenFormat(format));
                    } else {
                        addToList(fib->fib_FileName, totalSize - oldTotalSize, returnFormat());
                    }
                }
				FreeVec(newPath);
                continue;
            }

			// The Bellow will run only if its a File.
            if (listGadget)
            {
                int format = correctFormat(fib->fib_Size);
                // printf("Adding to list: %s \t Size: %ld\n", fib->fib_FileName, fib->fib_Size);
                addToList(fib->fib_FileName, devideByGivenFormat(fib->fib_Size, format), returnGivenFormat(format));
            }
            if (!subFoldering && !listGadget)
            {
                int format = correctFormat(fib->fib_Size);
                printf("| %-20.20s: %12lu %s\n", fib->fib_FileName, devideByGivenFormat(fib->fib_Size, format), returnGivenFormat(format));
            }
            addToTotalSize(fib->fib_Size);
        }
    }
exit:
    if (listGadget)
    {
        // Presentage of total size Calculations
        // struct List *list = (struct List *)&contents;
        // struct Node *node = list->lh_Head;
		struct Node *node = contents.lh_Head;
        while (node->ln_Succ)
        {
            struct Node *nextNode = node->ln_Succ;
            ULONG *initBuffer = AllocVec(sizeof(ULONG), MEMF_CLEAR);
            struct TagItem *tagList = (struct TagItem *)AllocVec(sizeof(struct TagItem) * 2, MEMF_CLEAR);
            tagList[0].ti_Tag = LBNA_Column;
            tagList[0].ti_Data = 2;
            tagList[1].ti_Tag = LBNCA_Text;
            tagList[1].ti_Data = (ULONG)initBuffer;
            tagList[2].ti_Tag = TAG_DONE;

            GetListBrowserNodeAttrsA(node, tagList);
            // Get last 2 characters from word
            char *format = (char *)AllocVec(3, MEMF_CLEAR);
            strncpy(format, (char *)initBuffer[0] + strlen((char *)initBuffer[0]) - 2, 2);
            ULONG firstNumber = stringToULONG((char *)initBuffer[0]);
            STRPTR buffer = floatToString(presentageFromULongs(firstNumber, totalSize, format, returnFormat()));
			if(stringToFloat(buffer) < 0.01 && firstNumber != 0){
				strcpy(buffer, "<0.01");
			}
            strcat(buffer, "%");
            tagList[0].ti_Tag = LBNA_Column;
            tagList[0].ti_Data = 1;
            tagList[1].ti_Tag = LBNCA_Text;
            tagList[1].ti_Data = (ULONG)buffer;
            tagList[2].ti_Tag = TAG_DONE;

            SetListBrowserNodeAttrsA(node, tagList);
            node = nextNode;
            FreeVec(format);
            // FreeVec(tagList);
            FreeVec(initBuffer);
            FreeVec(buffer);
        }
        SetAttrs(listGadget, LISTBROWSER_Labels, (ULONG)&contents, TAG_DONE);
    }
    if (!subFoldering && !listGadget)
    {
        printf("\n--> Total Size Of Path Given: %lu %s\n\n", totalSize, returnFormat());
    }
    FreeVec(fib);
    UnLock(lockPath);
    return;
}

void clearScanning (void) {
    clearList(contents);
    pastPath[0] = '\0';
    totalSize = 0;
    currentFormat = 0;
}
