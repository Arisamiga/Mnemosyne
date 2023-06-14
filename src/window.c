#include <stdlib.h>
#include <intuition/classusr.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <classes/window.h>
#define __CLIB_PRAGMA_LIBCALL
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/window.h>
#include <proto/intuition.h>
#include <proto/utility.h>
char *vers = "\0$VER: BOOPSIdemo 1 (09.12.2019)";
struct IntuitionBase *IntuitionBase;
struct Library *WindowBase;
struct Library *LayoutBase;
struct Library *ListBrowserBase;

struct ColumnInfo ci[] =
{
	{ 80, "Name", 0 },
	{ 60, "Size", 0 },
	{ -1, (STRPTR)~0, -1 }
};

struct {STRPTR text; LONG size;} file[] =
{
	{ "File 1", 100 },
	{ "File 2", 200 },
	{ "File 3", 300 },
	{ NULL, 0 }
};

void cleanexit(Object *windowObject);
void processEvents(Object *windowObject);
void createWindow(void)
{
	struct Window *intuiwin = NULL;
	Object *windowObject = NULL;
	Object *mainLayout = NULL;
	Object *listBrowser = NULL;
	struct List contents;
	WORD i;
	if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 47)))
		cleanexit(NULL);
	if (!(UtilityBase = OpenLibrary("utility.library", 47)))
		cleanexit(NULL);
	if (!(WindowBase = OpenLibrary("window.class", 47)))
		cleanexit(NULL);
	if (!(LayoutBase = OpenLibrary("gadgets/layout.gadget", 47)))
		cleanexit(NULL);
	if (!(ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 47)))
		cleanexit(NULL);
	NewList(&contents);
	for (i = 0; i < 3; i++)
	{
		UBYTE buffer[64];
		UBYTE buffer2[64];
		struct Node *node;
		SNPrintf(buffer, 64, "Number %ld", i);
		SNPrintf(buffer2, 64, "Size %ld", i * 100);
		node = AllocListBrowserNode(3,
						LBNA_Column, 0,
							LBNCA_CopyText, TRUE,
							LBNCA_Text, buffer,
							LBNCA_MaxChars, 40,
						LBNA_Column, 1,
							LBNCA_CopyText, TRUE,
							LBNCA_Text, buffer2,
							LBNCA_MaxChars, 40,
						TAG_DONE);
		if (node)
			AddTail(&contents, node);
	}

	listBrowser = NewObject(LISTBROWSER_GetClass(), NULL,
							GA_ID, 1,
							GA_RelVerify, TRUE,
							LISTBROWSER_Labels, (ULONG)&contents,
							LISTBROWSER_ColumnInfo, (ULONG)&ci,
							LISTBROWSER_ColumnTitles, TRUE,
							LISTBROWSER_MultiSelect, FALSE,
							LISTBROWSER_Separators, TRUE,
							LISTBROWSER_ShowSelected, FALSE,
							LISTBROWSER_Editable, TRUE,
							TAG_END);
	mainLayout = NewObject(LAYOUT_GetClass(), NULL,
						   LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						   LAYOUT_DeferLayout, TRUE,
						   LAYOUT_SpaceInner, TRUE,
						   LAYOUT_SpaceOuter, TRUE,
						   LAYOUT_AddChild, listBrowser,
						   TAG_DONE);
	windowObject = NewObject(WINDOW_GetClass(), NULL,
							 WINDOW_Position, WPOS_CENTERSCREEN,
							 WA_Activate, TRUE,
							 WA_Title, "Mnemosyne 0.1",
							 WA_DragBar, TRUE,
							 WA_CloseGadget, TRUE,
							 WA_DepthGadget, TRUE,
							 WA_SizeGadget, TRUE,
							 WA_InnerWidth, 300,
							 WA_InnerHeight, 150,
							 WA_IDCMP, IDCMP_CLOSEWINDOW,
							 WINDOW_Layout, mainLayout,
							 TAG_DONE);
	if (!windowObject)
		cleanexit(NULL);
	if (!(intuiwin = (struct Window *)DoMethod(windowObject, WM_OPEN, NULL)))
		cleanexit(windowObject);
	processEvents(windowObject);
	DoMethod(windowObject, WM_CLOSE);
	cleanexit(windowObject);
}
void processEvents(Object *windowObject)
{
	ULONG windowsignal;
	ULONG receivedsignal;
	ULONG result;
	ULONG code;
	BOOL end = FALSE;
	GetAttr(WINDOW_SigMask, windowObject, &windowsignal);
	while (!end)
	{
		receivedsignal = Wait(windowsignal);
		while ((result = DoMethod(windowObject, WM_HANDLEINPUT, &code)) != WMHI_LASTMSG)
		{
			switch (result & WMHI_CLASSMASK)
			{
			case WMHI_CLOSEWINDOW:
				end = TRUE;
				break;
			}
		}
	}
}
void cleanexit(Object *windowObject)
{
	if (windowObject)
		DisposeObject(windowObject);
	CloseLibrary((struct Library *)IntuitionBase);
	CloseLibrary(UtilityBase);
	CloseLibrary(WindowBase);
	CloseLibrary(LayoutBase);
	CloseLibrary(ListBrowserBase);
	exit(0);
}
