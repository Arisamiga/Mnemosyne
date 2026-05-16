// This is where function that can be used by multiple files are defined.
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/listbrowser.h>
#include <proto/icon.h>
#include <proto/gadtools.h>
#include <proto/bitmap.h>
#include <proto/button.h>
#include <proto/intuition.h>
#include <proto/getfile.h>
#include <proto/graphics.h>

#include <workbench/icon.h>

#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <images/bitmap.h>
#include <libraries/gadtools.h>
#include <classes/window.h>
#include <gadgets/getfile.h>
#include <graphics/rastport.h>

#include "funcs.h"

BOOL NoRoundOption = FALSE;
BOOL EnableGraphOption = FALSE;

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

size_t strlcpy(char *dest, const char *source, size_t size)
{
   size_t src_size = 0;
   size_t n = size;

   if (n)
      while (--n && (*dest++ = *source++)) src_size++;

   if (!n)
   {
      if (size) *dest = '\0';
      while (*source++) src_size++;
   }

   return src_size;
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
            strlcpy(result, FIblock->fib_FileName, resultSize);
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
    snprintf(buffer, 64, "%lu", num);
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

char getLastCharSafely(const char* str) {
    if (str == NULL || *str == '\0') {
        // Handle invalid input or empty string
        return '\0';
    }

    const char* lastCharPtr = str;
    while (*(lastCharPtr + 1) != '\0') {
        lastCharPtr++;
    }

    return *lastCharPtr;
}

char* getLastTwoChars(const char* str) {
    if (str == NULL || *str == '\0') {
        // Handle invalid input or empty string
        return NULL;
    }

    size_t length = 0;
    while (str[length] != '\0') {
        length++;
    }

    if (length <= 1) {
        // Handle string with less than two characters
        return NULL;
    }

    return (char*)(str + length - 2);
}

char *string_to_lower(const char *text, size_t len)
{
    char *result = AllocVec(len + 1, MEMF_ANY);
    for (size_t i = 0; i < len; i++)
        result[i] = tolower(text[i]);
    result[len] = '\0';
    return result;
}

size_t safeStrlen(const char *str)
{
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// Get path from where the program is running

char* getProgramPath() {
    char* path = AllocVec(sizeof(char) * 256, MEMF_CLEAR);
    if (path == NULL) {
        // Handle error
        return NULL;
    }

    BPTR lock = Lock("", ACCESS_READ);
    if (lock)
    {
        NameFromLock(lock, path, 256);
        // Append Mnemosyne to the path
        strcat(path, "Mnemosyne");
        UnLock(lock);
    }
    return path;
}

void initializeIconTooltypes(void)
{

	// Get path from where the program is running
	char* path = getProgramPath();

	if (IconBase)
	{
		struct DiskObject *diskObj = GetDiskObjectNew(path);
		if(diskObj)
		{
			// Check if the tooltypes are empty
			if (diskObj->do_ToolTypes == NULL)
			{
				// printf("Empty\n");
				NoRoundOption = FALSE;
                EnableGraphOption = FALSE;
				FreeDiskObject(diskObj);
				return;
			}

			char *buf = AllocVec(sizeof(char) * 256, MEMF_CLEAR);

			for (STRPTR *tool_types = diskObj->do_ToolTypes; (buf = *tool_types); ++tool_types)
			{
				// printf("%s\n", buf);
                if (strncmp(buf, "NOROUND", 7) == 0)
                {
					NoRoundOption = TRUE;
                }
                if (strncmp(buf, "ENABLEGRAPH", 11) == 0)
                {
                    EnableGraphOption = TRUE;
                }
			}
			// printf("%s\n", result);
			FreeVec(buf);
			FreeDiskObject(diskObj);
		}
	}
}

void updateIconTooltypes (void)
{
	// Get path from where the program is running
	char* path = getProgramPath();

	if (IconBase)
	{
		struct DiskObject *diskObj = GetIconTags(path, TAG_DONE);
		if(diskObj)
		{
			// Create array with the new tooltypes
            char **newToolTypes = AllocVec(sizeof(char *) * 3, MEMF_CLEAR);
			if (newToolTypes)
			{
				if (NoRoundOption)
				{
					newToolTypes[0] = "NOROUND";
				}
				else
				{
					newToolTypes[0] = "(NOROUND)";
				}
                if (EnableGraphOption)
                {
                    newToolTypes[1] = "ENABLEGRAPH";
                }
                else
                {
                    newToolTypes[1] = "(ENABLEGRAPH)";
                }
                newToolTypes[2] = NULL;

				diskObj->do_ToolTypes = (STRPTR *)newToolTypes;

				LONG errorCode;
				BOOL success;
				success = PutIconTags(path, diskObj,
					ICONPUTA_DropNewIconToolTypes, TRUE,
					ICONA_ErrorCode, &errorCode,
				TAG_DONE);

				if(success == FALSE)
				{
					printf("Error: %ld\n", errorCode);
				}

				FreeVec(newToolTypes);
			}
		}
	}
}

// ------------ Utility Functions for Window (moved from window.c) -----------

// Extract the color pen from a bitmap (4-bit)
ULONG getBitmapColorPen(struct BitMap *bm)
{
    if (!bm || bm->BytesPerRow == 0)
        return 1;  // Fallback to pen 1

    UBYTE *planes[4];
    for (int i = 0; i < 4; i++) {
        planes[i] = bm->Planes[i];
        if (!planes[i])
            return 1;
    }

    UBYTE byte0 = planes[0][0];
    UBYTE byte1 = planes[1][0];
    UBYTE byte2 = planes[2][0];
    UBYTE byte3 = planes[3][0];

    ULONG pen = ((byte3 & 0x80) >> 7) * 8 |
                ((byte2 & 0x80) >> 7) * 4 |
                ((byte1 & 0x80) >> 7) * 2 |
                ((byte0 & 0x80) >> 7);

    return pen > 0 ? pen : 1;
}

// Collect all node percentages from the list browser WITH COLORS from column 0 images
ULONG collectAllNodePercentages(struct Gadget *listbrowser, struct NodeData *nodeArray, ULONG maxNodes)
{
    struct List *nodeList = NULL;
    GetAttr(LISTBROWSER_Labels, listbrowser, (ULONG *)&nodeList);

    if (!nodeList)
        return 0;

    ULONG nodeCount = 0;
    struct Node *node;
    for (node = nodeList->lh_Head; node->ln_Succ && nodeCount < maxNodes; node = node->ln_Succ) {
        STRPTR percentText = NULL;
        ULONG percentColumn = 2;

        GetListBrowserNodeAttrs(node,
                                LBNA_Column, percentColumn,
                                LBNCA_Text, &percentText,
                                TAG_DONE);

        if (percentText) {
            float percent = atof(percentText);
            if (percent > 100.0f) percent = 100.0f;
            if (percent < 0.0f) percent = 0.0f;

            nodeArray[nodeCount].percent = percent;

            struct Image *image = NULL;
            GetListBrowserNodeAttrs(node,
                                    LBNA_Column, 0,
                                    LBNCA_Image, &image,
                                    TAG_DONE);

            if (image) {
                struct BitMap *imgBitMap = NULL;
                GetAttr(BITMAP_BitMap, image, (ULONG *)&imgBitMap);
                if (imgBitMap) {
                    nodeArray[nodeCount].colorPen = getBitmapColorPen(imgBitMap);
                } else {
                    nodeArray[nodeCount].colorPen = (nodeCount % 14) + 1;
                }
            } else {
                nodeArray[nodeCount].colorPen = (nodeCount % 14) + 1;
            }

            nodeCount++;
        }
    }

    return nodeCount;
}

// Simple row-based treemap layout
void drawTreemapRectangles(struct RastPort *rp, struct NodeData *nodeArray, ULONG nodeCount, ULONG bitmapWidth, ULONG bitmapHeight)
{
    if (nodeCount == 0)
        return;

    float totalPercent = 0.0f;
    for (ULONG i = 0; i < nodeCount; i++) {
        totalPercent += nodeArray[i].percent;
    }

    if (totalPercent < 0.01f)
        totalPercent = 100.0f;

    ULONG x = 0, y = 0;
    ULONG rowHeight = bitmapHeight / (nodeCount > 10 ? 10 : (nodeCount > 5 ? 5 : 3));
    if (rowHeight < 2) rowHeight = 2;

    ULONG currentRow = 0;
    ULONG rowY = 0;
    ULONG nextRowY = rowHeight;

    for (ULONG i = 0; i < nodeCount; i++) {
        float fraction = nodeArray[i].percent / totalPercent;
        ULONG itemWidth = (ULONG)(fraction * bitmapWidth);
        if (itemWidth < 1) itemWidth = 1;

        if (x + itemWidth > bitmapWidth) {
            x = 0;
            rowY = nextRowY;
            nextRowY += rowHeight;
            if (nextRowY > bitmapHeight) nextRowY = bitmapHeight;
            currentRow++;
        }

        if (rowY < bitmapHeight) {
            ULONG rectHeight = nextRowY - rowY;
            if (rowY + rectHeight > bitmapHeight)
                rectHeight = bitmapHeight - rowY;

            SetAPen(rp, nodeArray[i].colorPen);
            RectFill(rp, x, rowY, x + itemWidth - 1, rowY + rectHeight - 1);

            if (x + itemWidth < bitmapWidth) {
                SetAPen(rp, 0);
                RectFill(rp, x + itemWidth, rowY, x + itemWidth, rowY + rectHeight - 1);
            }

            x += itemWidth + 1;
        }
    }
}

void flushAppPort(struct MsgPort *appPort)
{
	if (!appPort)
		return;

	struct AppMessage *appMsg;
	while ((appMsg = (struct AppMessage *)GetMsg(appPort)))
	{
		ReplyMsg((struct Message *)appMsg);
	}
}

float asValue(STRPTR s)
{
	float v = atof(s);
	// Check if first char is a < sign
	if (s[0] == '<')
		v = 0.001f;
	return v;
}

float __SAVE_DS__ __ASM__ myCompare(__REG__(a0, struct Hook *hook), __REG__(a2, Object *obj),
									   __REG__(a1, struct LBSortMsg *msg))
{
	return asValue(msg->lbsm_DataA.Text) - asValue(msg->lbsm_DataB.Text);
}

int __SAVE_DS__ __ASM__ myCompare2(__REG__(a0, struct Hook *hook), __REG__(a2, Object *obj),
									   __REG__(a1, struct LBSortMsg *msg))
{
	return strcmp(string_to_lower(msg->lbsm_DataA.Text, safeStrlen(msg->lbsm_DataA.Text)), string_to_lower(msg->lbsm_DataB.Text, safeStrlen(msg->lbsm_DataB.Text)));
}

void checkBackButton(char *pastPath, BOOL doneFirst, Object *backButton) {
	if (getLastCharSafely(pastPath) != ':' && doneFirst && pastPath[0] != '\0')
		SetAttrs(backButton, GA_Disabled, FALSE, TAG_DONE);
	else
		SetAttrs(backButton, GA_Disabled, TRUE, TAG_DONE);
}

void toggleButtons(Object *windowObject, Object *backButton, struct Gadget *listBrowser, Object *fileRequester, char *pastPath, BOOL doneFirst, BOOL option, BOOL Refresh)
{
	SetAttrs(windowObject, WA_BusyPointer, option, TAG_DONE);
	SetAttrs(backButton, GA_Disabled, option, TAG_DONE);
	SetAttrs(listBrowser, LISTBROWSER_TitleClickable, !option, TAG_DONE);
	SetAttrs(fileRequester, GA_Disabled, option, TAG_DONE);
	SetAttrs(listBrowser, GA_Disabled, option, TAG_DONE);

	if(!option){
		checkBackButton(pastPath, doneFirst, backButton);
	}
	if (Refresh)
		DoMethod(windowObject, WM_NEWPREFS);
}

void toggleBusyPointer(Object *windowObject, BOOL option)
{
	SetAttrs(windowObject, WA_BusyPointer, option, TAG_DONE);
}

void updateBottomTextW2Text(Object *bottomText, Object *windowObject, char *firstText, STRPTR secondText, BOOL Refresh)
{
	char *title = AllocVec(sizeof(char) * 256, MEMF_CLEAR);
	snprintf(title, 256, "%s%s", firstText, secondText);
	SetAttrs(bottomText, GA_Image, NULL, GA_Text, title, TAG_DONE);
	if (Refresh)
		DoMethod(windowObject, WM_NEWPREFS);
	// FreeVec(title);
}

void updateBottomTextW2AndTotal(Object *bottomText, Object *windowObject, char *firstText, STRPTR secondText, STRPTR totalText, BOOL Refresh)
{
	char *title = AllocVec(sizeof(char) * 256, MEMF_CLEAR);
	snprintf(title, 256, "%s%s%s", firstText, secondText, totalText);
	SetAttrs(bottomText, GA_Image, NULL, GA_Text, title, TAG_DONE);
	if (Refresh)
		DoMethod(windowObject, WM_NEWPREFS);
	FreeVec(title);
}

void updateBottomText(Object *bottomText, Object *windowObject, STRPTR secondText)
{
	// Check that the text is not the same as the current text
	STRPTR bottomTextString = NULL;
	GetAttr(GA_Text, bottomText, (ULONG *)&bottomTextString);
	// if (strcmp(bottomTextString, secondText) == 0)
	// 	return;

	SetAttrs(bottomText, GA_Image, NULL, GA_Text, secondText, TAG_DONE);
	DoMethod(windowObject, WM_NEWPREFS);
}

void updatePathText(Object *fileRequester, STRPTR path) {
	SetAttrs(fileRequester, GETFILE_FullFile, path, TAG_DONE);
}
