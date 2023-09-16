// This is where function that can be used by multiple files are defined.
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/listbrowser.h>

#include <exec/types.h>

#include "funcs.h"

int returnFormatValue(STRPTR format){
    if(strcmp(format, "B") == 0)
    {
        return 0;
    }
    else if(strcmp(format, "KB") == 0)
    {
        return 1;
    }
    else if(strcmp(format, "MB") == 0)
    {
        return 2;
    }
    else if(strcmp(format, "GB") == 0)
    {
        return 3;
    }
    else if(strcmp(format, "TB") == 0)
    {
        return 4;
    }
    else
    {
        return 0;
    }
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
			if (FIblock->fib_FileName[0] == '\0' || resultSize == 0){
				return;
			}

            strncpy(result, FIblock->fib_FileName, resultSize - 1);
			result[resultSize - 1] = '\0';

            FreeVec(FIblock);
        }
        UnLock(pathLock);
    }
}

STRPTR floatToString(float num)
{
    STRPTR buffer = AllocVec(64, MEMF_ANY);
    sprintf(buffer, "%.2f", num);
    return buffer;
}

STRPTR ULongToString(ULONG num)
{
    STRPTR buffer = AllocVec(64, MEMF_ANY);
    SNPrintf(buffer, 64, "%lu", num);
    return buffer;
}

float presentageFromULongs(ULONG num1, ULONG num2, STRPTR num1Format, STRPTR num2Format)
{
    // printf("%ld\t%ld\n", num1, num2);
    if (num1Format != num2Format){
        int num1FormatValue = returnFormatValue(num1Format);
        int num2FormatValue = returnFormatValue(num2Format);
        // printf("Values: %d\t%d\n", num1FormatValue, num2FormatValue);
        if (num1FormatValue > num2FormatValue)
        {
            // printf("1\n");
            while (num1FormatValue != num2FormatValue)
            {
                num1 *= 1024;
                num1FormatValue--;
            }
        }
        if (num1FormatValue < num2FormatValue)
        {
            // printf("2\n");
            while (num1FormatValue != num2FormatValue)
            {
                num1 /= 1024;
                num1FormatValue++;
            }
        }
    }
	if (num1 == 0 || num2 == 0)
	{
		return 0;
	}
    float percentage = (((float)num1 / (float)num2) * 100.0);
    // printf("%ld\t%ld\t%f\n", num1, num2, percentage);
    // printf("Both Formats: %s\t%s\n", num1Format, num2Format);
    // printf("Float: %f\n", percentage);
    return percentage;
}

ULONG stringToULONG(char *string)
{
    ULONG result = 0;
    int i = 0;
    while (string[i] != '\0')
    {
        if (isdigit(string[i]))
        {
            result *= 10;
            result += string[i] - '0';
        }
        i++;
    }
    return result;
}

BOOL clearList(struct List list){
	struct Node *node = list.lh_Head;
	if (node == NULL)
	{
		return FALSE;
	}
	while (node->ln_Succ)
	{
		struct Node *nextNode = node->ln_Succ;
		Remove(node);
		FreeListBrowserNode(node);
		node = nextNode;
	}
	return TRUE;
}

float stringToFloat(STRPTR value)
{
	float result = 0;
	int i = 0;
	while (value[i] != '\0')
	{
		if (isdigit(value[i]))
		{
			result *= 10;
			result += value[i] - '0';
		}
		i++;
	}
	return result;
}
