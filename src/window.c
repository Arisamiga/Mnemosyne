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

#include "window.h"
#include "scan.h"

char *vers = "\0$VER: BOOPSIdemo 1 (09.12.2019)";
struct IntuitionBase *IntuitionBase;
struct Library *WindowBase;
struct Library *LayoutBase;
struct Library *ListBrowserBase;

struct ColumnInfo ci[] =
	{
		{80, "Name", 0},
		{60, "Size", 0},
		{-1, (STRPTR)~0, -1}};

void cleanexit(Object *windowObject);
void processEvents(Object *windowObject, Object *listBrowser, BOOL doneFirst);
void createWindow(void)
{
	struct Window *intuiwin = NULL;
	Object *windowObject = NULL;
	Object *mainLayout = NULL;
	Object *listBrowser = NULL;
	struct List contents;
	WORD i;

	BOOL doneFirst = FALSE;


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

	UBYTE buffer[64];
	UBYTE buffer2[64];
	struct Node *node;
	SNPrintf(buffer, 64, "Click here to start");
	SNPrintf(buffer2, 64, "Scanning...");
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

	listBrowser = NewObject(LISTBROWSER_GetClass(), NULL,
							GA_ID, 1,
							GA_RelVerify, TRUE,
							LISTBROWSER_Labels, (ULONG)&contents,
							LISTBROWSER_ColumnInfo, (ULONG)&ci,
							LISTBROWSER_ColumnTitles, TRUE,
							LISTBROWSER_MultiSelect, FALSE,
							LISTBROWSER_Separators, TRUE,
							LISTBROWSER_ShowSelected, FALSE,
							LISTBROWSER_Spacing, 1,
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
	processEvents(windowObject, listBrowser, doneFirst);
	DoMethod(windowObject, WM_CLOSE);
	cleanexit(windowObject);
}
void processEvents(Object *windowObject, Object *listBrowser, BOOL doneFirst)
{
	ULONG windowsignal;
	ULONG receivedsignal;
	ULONG result;
	ULONG code;
	BOOL end = FALSE;
	BOOL scanning = FALSE;
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
			case WMHI_GADGETUP:
				switch (result & WMHI_GADGETMASK)
				{
				case 1:
				{
					if (scanning)
					{
						printf("Scanning already in progress\n");
						break;
					}
					printf("CLicked on gadget 1\n");
					// Get the current selection
					struct Node *node = NULL;
					WORD selected = 0;
					GetAttr(LISTBROWSER_SelectedNode, listBrowser, &node);
					GetAttr(LISTBROWSER_Selected, listBrowser, &selected);
					if (node)
					{
						// Get the text of the first column
						STRPTR text = NULL;
						GetListBrowserNodeAttrs(node, LBNA_Column, 0, LBNCA_Text, &text, TAG_DONE);
						if (text)
						{
							const int len = strlen(text);
							if ( len > 0 && text[len-1] != '/' && doneFirst )
							{ 
								printf("Not a folder\n");
								break;
							}
							printf("Selected %s\n", text);
							SetAttrs(windowObject, WA_Title, "Scanning...", TAG_DONE);
							if(doneFirst && text){
								char newPath[256];
								SNPrintf(newPath, 256, "Amiga:%s%s", text, "/");
								scanning = TRUE;
								scanPath(newPath, FALSE, listBrowser);
								scanning = FALSE;
							}
							else{
								scanning = TRUE;
								scanPath("Amiga:", FALSE, listBrowser);
								scanning = FALSE;
								doneFirst = TRUE;
							}
							SetAttrs(windowObject, WA_Title, "Mnemosyne 0.1", TAG_DONE);
							DoMethod(windowObject, WM_NEWPREFS);
						}
					}
					// SetAttrs(windowObject, WA_Title, "Scanning...", TAG_DONE);
					// scanPath("Amiga:", FALSE, listBrowser);
					// SetAttrs(windowObject, WA_Title, "Mnemosyne 0.1", TAG_DONE);
					// DoMethod(windowObject, WM_NEWPREFS);
					break;
				}
				default:
					printf("Unhandled event of category %ld\n", result & WMHI_GADGETMASK);

				break;
				}
				break;
				// default:
				// 	printf("Unhandled event of category %ld\n", result & WMHI_CLASSMASK);
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
