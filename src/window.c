#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
#include <proto/button.h>
#include <proto/space.h>

#include "window.h"
#include "scan.h"
#include "funcs.h"

char *vers = "\0$VER: Mnemosyne 0.1";
struct IntuitionBase *IntuitionBase;
struct Library *WindowBase;
struct Library *LayoutBase;
struct Library *ListBrowserBase;
struct Library *ButtonBase;
struct Library *SpaceBase;

enum
{
	OID_BACK_BUTTON,
	OID_MAIN_LIST,
	OID_LAST
};

static ULONG asValue(STRPTR s)
{
	ULONG v = atoi(s);
	return v;
}
static ULONG __SAVE_DS__ __ASM__ myCompare(__REG__(a0, struct Hook *hook), __REG__(a2, Object *obj),
										   __REG__(a1, struct LBSortMsg *msg))
{
	return asValue(msg->lbsm_DataA.Text) - asValue(msg->lbsm_DataB.Text);
}

void toggleButtons(Object *windowObject, Object *backButton, Object *listBrowser, BOOL option)
{
	SetAttrs(backButton, GA_Disabled, option, TAG_DONE);
	SetAttrs(listBrowser, LISTBROWSER_TitleClickable, !option, TAG_DONE);
	SetAttrs(listBrowser, GA_Disabled, option, TAG_DONE);
	DoMethod(windowObject, WM_NEWPREFS);
}

void cleanexit(Object *windowObject);
void processEvents(Object *windowObject, struct Window *intuiwin, Object *listBrowser, Object *backButton, BOOL doneFirst, struct Hook CompareHook, Object *bottomText);
void createWindow(void)
{
	struct Window *intuiwin = NULL;
	Object *windowObject = NULL;

	struct ColumnInfo *ci;
	struct Hook CompareHook;

	Object *mainLayout = NULL;
	Object *upperLayout = NULL;

	Object *listBrowser = NULL;
	Object *backButton = NULL;
	Object *spaceGadget = NULL;
	Object *bottomText = NULL;

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
	if (!(ButtonBase = OpenLibrary("gadgets/button.gadget", 47)))
		cleanexit(NULL);
	if (!(SpaceBase = OpenLibrary("gadgets/space.gadget", 47)))
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

	backButton = NewObject(BUTTON_GetClass(), NULL,
						   GA_ID, OID_BACK_BUTTON,
						   GA_RelVerify, TRUE,
						   GA_Width, 10,
						   GA_Height, 10,
						   GA_Text, "Back",
						   GA_Disabled, TRUE, // Disabled so it doesn't go back to SYS:
						   TAG_END);

	spaceGadget = NewObject(SPACE_GetClass(), NULL,
							GA_ReadOnly, TRUE);

	/* initialize CompareHook for sorting the column */
	CompareHook.h_Entry = (ULONG(*)())myCompare;
	CompareHook.h_SubEntry = NULL;
	CompareHook.h_Data = NULL;

	ci = AllocLBColumnInfo(2,
						   LBCIA_Column, 0,
						   LBCIA_Title, "Name",
						   LBCIA_Weight, 80,
						   LBCIA_AutoSort, TRUE,
						   LBCIA_Sortable, FALSE,
						   LBCIA_Column, 1,
						   LBCIA_Title, "Size (bytes)",
						   LBCIA_SortArrow, TRUE,
						   LBCIA_AutoSort, TRUE,
						   LBCIA_Sortable, TRUE,
						   LBCIA_CompareHook, &CompareHook,
						   LBCIA_Weight, 60,
						   TAG_DONE);

	listBrowser = NewObject(LISTBROWSER_GetClass(), NULL,
							GA_ID, OID_MAIN_LIST,
							GA_RelVerify, TRUE,
							LISTBROWSER_Labels, (ULONG)&contents,
							LISTBROWSER_ColumnInfo, ci,
							LISTBROWSER_ColumnTitles, TRUE,
							LISTBROWSER_MultiSelect, FALSE,
							LISTBROWSER_Separators, TRUE,
							LISTBROWSER_ShowSelected, FALSE,
							LISTBROWSER_TitleClickable, TRUE,
							LISTBROWSER_Spacing, 1,
							TAG_END);

	bottomText = NewObject(BUTTON_GetClass(), NULL,
						   GA_Text, "",
						   GA_ReadOnly, TRUE,
						   BUTTON_BevelStyle, BVS_GROUP,
						   TAG_END);

	upperLayout = NewObject(LAYOUT_GetClass(), NULL,
							LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
							LAYOUT_DeferLayout, TRUE,
							LAYOUT_SpaceInner, TRUE,
							LAYOUT_SpaceOuter, TRUE,
							LAYOUT_AddChild, backButton,
								CHILD_MaxWidth, 32,
							LAYOUT_AddChild, spaceGadget,
							TAG_DONE);

	mainLayout = NewObject(LAYOUT_GetClass(), NULL,
						   LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						   LAYOUT_DeferLayout, TRUE,
						   LAYOUT_SpaceInner, TRUE,
						   LAYOUT_SpaceOuter, TRUE,
						   LAYOUT_AddChild, upperLayout,
						   		CHILD_MaxHeight, 32,
						   LAYOUT_AddChild, listBrowser,
						   LAYOUT_AddChild, bottomText,
						   		CHILD_MaxHeight, 10,
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
	processEvents(windowObject, intuiwin, listBrowser, backButton, doneFirst, CompareHook, bottomText);
	DoMethod(windowObject, WM_CLOSE);
	cleanexit(windowObject);
}
void processEvents(Object *windowObject, struct Window *intuiwin, Object *listBrowser, Object *backButton, BOOL doneFirst, struct Hook CompareHook, Object *bottomText)
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
				case OID_BACK_BUTTON:
				{
					if (scanning)
					{
						printf("Scanning already in progress\n");
						break;
					}
					if (!doneFirst)
					{
						printf("No previous path\n");
						break;
					}
					if (pastPath[strlen(pastPath) - 1] == ':')
					{
						printf("Cannot go back\n");
						break;
					}
					int resultSize = 256;
					char *parentPath = AllocVec(sizeof(char) * resultSize, MEMF_CLEAR);
					getParentPath(pastPath, parentPath, resultSize);
					printf("Parent Path: %s\n", parentPath);

					char *parentName = AllocVec(sizeof(char) * resultSize, MEMF_CLEAR);
					getNameFromPath(parentPath, parentName, resultSize);
					char *title = AllocVec(sizeof(char) * resultSize, MEMF_CLEAR);

					if (parentPath[strlen(parentPath) - 1] != ':')
					{
						strcat(parentPath, "/");
					}

					SNPrintf(title, resultSize, "Scanning: %s", parentName);
					SetAttrs(bottomText, GA_Text, title, TAG_DONE);
					scanning = TRUE;
					toggleButtons(windowObject, backButton, listBrowser, TRUE);

					scanPath(parentPath, FALSE, listBrowser);

					scanning = FALSE;
					toggleButtons(windowObject, backButton, listBrowser, FALSE);
					SNPrintf(title, resultSize, "Current: %s", parentName);
					SetAttrs(bottomText, GA_Text, title, TAG_DONE);

					if (pastPath[strlen(pastPath) - 1] == ':' || !doneFirst)
						SetAttrs(backButton, GA_Disabled, TRUE, TAG_DONE);
					else
						SetAttrs(backButton, GA_Disabled, FALSE, TAG_DONE);

					DoMethod(windowObject, WM_NEWPREFS);
					break;
				}
				case OID_MAIN_LIST:
				{
					if (scanning)
					{
						printf("Scanning already in progress\n");
						break;
					}

					// Get the current selection
					struct Node *node = NULL;
					ULONG selected = 0;
					ULONG event = 0;
					GetAttr(LISTBROWSER_RelEvent, listBrowser, &event);
					GetAttr(LISTBROWSER_SelectedNode, listBrowser, (ULONG *)&node);
					GetAttr(LISTBROWSER_Selected, listBrowser, &selected);

					if (event == 0)
					{
						printf("No event\n");
						break;
					}

					if (node)
					{
						// Get the text of the first column
						STRPTR text = NULL;
						GetListBrowserNodeAttrs(node, LBNA_Column, 0, LBNCA_Text, &text, TAG_DONE);
						if (text)
						{
							const int len = strlen(text);
							int resultSize = 256;
							char *parentPath = AllocVec(sizeof(char) * resultSize, MEMF_CLEAR);

							printf("Selected %s\n", text);
							getParentPath(text, parentPath, resultSize);


							printf("Parent Path: %s\n", parentPath);
							if (len > 0 && text[len - 1] != '/' && doneFirst)
							{
								printf("Not a folder\n");
								printf("Path: %s\n", pastPath);
								break;
							}
							char *title = AllocVec(sizeof(char) * 256, MEMF_CLEAR);
							SNPrintf(title, resultSize, "Scanning: %s", text);
							SetAttrs(bottomText, GA_Text, title, TAG_DONE);
							DoMethod(windowObject, WM_NEWPREFS);
							if (doneFirst && text)
							{
								char newPath[256];
								SNPrintf(newPath, 256, "%s%s", &pastPath, text);
								toggleButtons(windowObject, backButton, listBrowser, TRUE);
								scanning = TRUE;
								scanPath(newPath, FALSE, listBrowser);
								toggleButtons(windowObject, backButton, listBrowser, FALSE);
								scanning = FALSE;
							}
							else
							{
								toggleButtons(windowObject, backButton, listBrowser, TRUE);
								scanning = TRUE;
								scanPath("Amiga:", FALSE, listBrowser);
								scanning = FALSE;
								toggleButtons(windowObject, backButton, listBrowser, FALSE);
								doneFirst = TRUE;
								DoGadgetMethod(listBrowser, intuiwin, NULL, LBM_SORT, NULL, 1, LBMSORT_REVERSE, &CompareHook);
							}

							if (pastPath[strlen(pastPath) - 1] == ':' || !doneFirst)
								SetAttrs(backButton, GA_Disabled, TRUE, TAG_DONE);
							else
								SetAttrs(backButton, GA_Disabled, FALSE, TAG_DONE);
							char *parentName = AllocVec(sizeof(char) * resultSize, MEMF_CLEAR);
							getNameFromPath(pastPath, parentName, resultSize);
							SNPrintf(title, resultSize, "Current: %s", parentName);
							FreeVec(parentName);
							SetAttrs(bottomText, GA_Text, title, TAG_DONE);
							DoMethod(windowObject, WM_NEWPREFS);
						}
					}
					break;
				}
				default:
					printf("Unhandled event of category %ld\n", result & WMHI_GADGETMASK);
					break;
				}
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
	CloseLibrary(ButtonBase);
	CloseLibrary(SpaceBase);
	exit(0);
}
