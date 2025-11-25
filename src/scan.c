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

struct Values {
	ULONG value;
	int format;
};

struct Values correctFormat(ULONG size, int format){
	struct Values values = {size, format};
    while(values.value >= 1024 && values.format < 4) {
        values.value >>= 10;  /* Divide by 1024 using bit shift */
        values.format++;
    }
	return values;
}

ULONG devideByGivenFormat(ULONG size, int format){
    switch (format)
        {
        case 0:
            return size;
        case 1:
            return size >> 10;  /* Divide by 1024 */
        case 2:
            return size >> 20;  /* Divide by 1024^2 */
        case 3:
            return size >> 30;  /* Divide by 1024^3 */
        case 4:
            /* For TB, need to handle carefully to avoid overflow */
            return (size >> 20) >> 20;  /* Divide by 1024^4 */
        default:
            return size;
        }
}



STRPTR returnFormatWithTotal(void){
    static char buffer[64];
    if (NoRoundOption == TRUE){
		snprintf(buffer, 64, " (%lu %s)", totalSize, returnGivenFormat(currentFormat));
	} else {
		struct Values format = correctFormat(totalSize, currentFormat);
		snprintf(buffer, 64, " (%lu %s)", format.value, returnGivenFormat(format.format));
	}

    return buffer;
}

void addToTotalSize(ULONG size)
{
    if (totalSize + size < totalSize)
    {
        // switch to higher format
        currentFormat++;
        totalSize = totalSize / 1024;
		size = size / 1024;
        addToTotalSize(size);
        return;
    }

    totalSize += devideByGivenFormat(size, currentFormat);
}

void addToList(char *name, ULONG size, STRPTR format)
{
    if (!format)
        format = returnGivenFormat(currentFormat);

    char buffer[64];
    char buffer2[64];
    
    snprintf(buffer, 64, "%s", name);
    snprintf(buffer2, 64, "%lu %s", size, format);

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
}

void addFileSequence(struct Gadget *listGadget, struct FileInfoBlock *fib, BOOL subFoldering){
	if (listGadget)
	{
		struct Values format = correctFormat(fib->fib_Size, 0);
		addToList(fib->fib_FileName, format.value, returnGivenFormat(format.format));
	}
	if (!subFoldering && !listGadget)
	{
		struct Values format = correctFormat(fib->fib_Size, 0);
		printf("| %-20.20s: %12lu %s\n", fib->fib_FileName, format.value, returnGivenFormat(format.format));
	}

	addToTotalSize(fib->fib_Size);
}

void scanPath(char *path, BOOL subFoldering, struct Gadget *listGadget)
{
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

	if (!subFoldering)
    {
		if (listGadget)
		{
			if (path != pastPath && path[0] != '\0')
			{
				strlcpy(pastPath, path, MAX_BUFFER);
			}

        	NewList(&contents);
		}
        totalSize = 0;
        currentFormat = 0;
    }

    // If file return size
    if (fib->fib_DirEntryType < 0)
    {
		addFileSequence(listGadget, fib, subFoldering);
        goto exit;
    }

    // If folder scan recursively and return size for each child
    if (fib->fib_DirEntryType > 0)
    {
        while (ExNext(lockPath, fib))
        {
			// Check if The next entity is a folder
            if (fib->fib_DirEntryType > 0)
            {
                // Scan SubFolders
                char newPath[256];
                char lastChar = getLastCharSafely(path);
                
                if (lastChar != ':' && lastChar != '/')
                {
                    snprintf(newPath, 256, "%s/%s", path, fib->fib_FileName);
                }
                else
                {
                    snprintf(newPath, 256, "%s%s", path, fib->fib_FileName);
                }

                ULONG oldTotalSize = totalSize;

                scanPath(newPath, TRUE, 0);

                if (!subFoldering && !listGadget)
                {
                    strcat(fib->fib_FileName, "/");
					if (NoRoundOption == FALSE) {
						struct Values format = correctFormat(totalSize - oldTotalSize, currentFormat);
                    	printf("| %-20s: %12lu %s\n", fib->fib_FileName, format.value, returnGivenFormat(format.format));
					} else {
                    	printf("| %-20s: %12lu %s\n", fib->fib_FileName, totalSize - oldTotalSize, returnGivenFormat(currentFormat));
					}
                }
                if (listGadget)
                {
                    strcat(fib->fib_FileName, "/");

                    if (totalSize < oldTotalSize){
                        // Divide oldTotalSize according to currentFormat
                        oldTotalSize = devideByGivenFormat(oldTotalSize, currentFormat);
                    }
                    // printf("Total: %ld - %ld = %ld\n", totalSize, oldTotalSize, totalSize - oldTotalSize);
                    if((long)(totalSize - oldTotalSize) < 0 || currentFormat == 0 || NoRoundOption == FALSE){
                        struct Values format = correctFormat(totalSize - oldTotalSize, currentFormat);
                        addToList(fib->fib_FileName, format.value, returnGivenFormat(format.format));
                    } else {
                        addToList(fib->fib_FileName, totalSize - oldTotalSize, returnGivenFormat(currentFormat));
                    }
                }
                continue;
            }

			// The Below will run only if it's a File.
			addFileSequence(listGadget, fib, subFoldering);
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
			if (initBuffer[0] == '\0'){
				node = nextNode;
				FreeVec(tagList);
				FreeVec(initBuffer);
				continue;
			}

            // Get last 2 characters from word
            char *format = getLastTwoChars((char *)initBuffer[0]);
            ULONG firstNumber = stringToULONG((char *)initBuffer[0]);
            float percentage = presentageFromULongs(firstNumber, totalSize, format, returnGivenFormat(currentFormat));
            
            char buffer[64];
			if(percentage < 0.01 && firstNumber != 0){
				snprintf(buffer, 64, "<0.01%%");
			} else {
                snprintf(buffer, 64, "%.2f%%", percentage);
            }
            
            tagList[0].ti_Tag = LBNA_Column;
            tagList[0].ti_Data = 1;
            tagList[1].ti_Tag = LBNCA_Text;
            tagList[1].ti_Data = (ULONG)buffer;
            tagList[2].ti_Tag = TAG_DONE;

            SetListBrowserNodeAttrsA(node, tagList);
            node = nextNode;
            // FreeVec(tagList);
            FreeVec(initBuffer);
        }
        SetAttrs(listGadget, LISTBROWSER_Labels, (ULONG)&contents, TAG_DONE);
    }
    if (!subFoldering && !listGadget)
    {
		if (NoRoundOption == FALSE) {
			struct Values format = correctFormat(totalSize, currentFormat);
        	printf("\n--> Total Size Of Path Given: %lu %s\n\n", format.value, returnGivenFormat(format.format));
		} else {
        	printf("\n--> Total Size Of Path Given: %lu %s\n\n", totalSize, returnGivenFormat(currentFormat));
		}
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
