#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <intuition/classusr.h>
#include <intuition/intuition.h>

#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>

#include <classes/window.h>
#include <classes/requester.h>

#define __CLIB_PRAGMA_LIBCALL

#define MAX_BUFFER 256

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/window.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/button.h>
#include <proto/space.h>
#include <proto/getfile.h>
#include <proto/asl.h>
#include <proto/wb.h>
#include <proto/icon.h>
#include <proto/dos.h>

#include <libraries/gadtools.h>

#include "window.h"
#include "scan.h"
#include "funcs.h"
#include "aboutWin.h"


struct IntuitionBase *IntuitionBase;
struct Library *WindowBase;
struct Library *LayoutBase;
struct Library *ListBrowserBase;
struct Library *ButtonBase;
struct Library *SpaceBase;
struct Library *GetFileBase;
struct Library *TextFieldBase;


enum
{
	OID_BACK_BUTTON,
	OID_MAIN_LIST,
	OID_FILE_REQUESTER,
	OID_MENU_OPEN,
	OID_MENU_ABOUT,
	OID_MENU_QUIT,
	OID_LAST
};

// -------------
// Functions and such
// -------------

static float asValue(STRPTR s)
{
	float v = atof(s);
	return v;
}
static float __SAVE_DS__ __ASM__ myCompare(__REG__(a0, struct Hook *hook), __REG__(a2, Object *obj),
										   __REG__(a1, struct LBSortMsg *msg))
{
	return asValue(msg->lbsm_DataA.Text) - asValue(msg->lbsm_DataB.Text);
}

void checkBackButton(char *pastPath, BOOL doneFirst, Object *backButton) {
	if (pastPath[strlen(pastPath) - 1] != ':' && doneFirst)
		SetAttrs(backButton, GA_Disabled, FALSE, TAG_DONE);
	else
		SetAttrs(backButton, GA_Disabled, TRUE, TAG_DONE);
}

void toggleButtons(Object *windowObject, Object *backButton, Object *listBrowser, Object *fileRequester, char *pastPath, BOOL doneFirst, BOOL option, BOOL Refresh)
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
	char *title = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
	SNPrintf(title, MAX_BUFFER, "%s%s", firstText, secondText);
	SetAttrs(bottomText, GA_Text, title, TAG_DONE);
	if (Refresh)
		DoMethod(windowObject, WM_NEWPREFS);
	FreeVec(title);
}

void updateBottomText(Object *bottomText, Object *windowObject, STRPTR secondText)
{
	// Check that the text is not the same as the current text
	STRPTR bottomTextString = NULL;
	GetAttr(GA_Text, bottomText, (ULONG *)&bottomTextString);
	if (strcmp(bottomTextString, secondText) == 0)
		return;
	
	SetAttrs(bottomText, GA_Text, secondText, TAG_DONE);
	DoMethod(windowObject, WM_NEWPREFS);
}

void updatePathText(Object *fileRequester, STRPTR path) {

	SetAttrs(fileRequester, GETFILE_FullFile, path, TAG_DONE);
	DoMethod(fileRequester, OM_UPDATE);
}

BOOL fileEntered = FALSE;

// -------------
// Main Window Functions
// -------------

void cleanexit(Object *windowObject);
void processEvents(Object *windowObject, struct Window *intuiwin, Object *listBrowser, Object *backButton, BOOL doneFirst, struct Hook CompareHook, Object *bottomText, Object *fileRequester);
void createWindow(void)
{
	struct Window *intuiwin = NULL;
	Object *windowObject = NULL;

	struct ColumnInfo *ci;
	struct Hook CompareHook;

	struct MsgPort *appPort;

	static struct NewMenu MenuArray[] = {
		{NM_TITLE, "Project", 0, 0, 0, 0},
		{NM_ITEM, "Open Current Dir...", 0, 0, 0, (APTR)OID_MENU_OPEN},
		{NM_ITEM, NM_BARLABEL,0,0,0,0 },
		{NM_ITEM, "About...", 0, 0, 0, (APTR)OID_MENU_ABOUT},
		{NM_ITEM, "Quit...", 0, 0, 0, (APTR)OID_MENU_QUIT},

		{NM_END, NULL, 0, 0, 0, NULL}
	};

	Object *mainLayout = NULL;
	Object *upperLayout = NULL;

	Object *listBrowser = NULL;
	Object *backButton = NULL;
	Object *spaceGadget = NULL;
	Object *bottomText = NULL;
	Object *fileRequester = NULL;

	struct List contents;
	WORD i;

	BOOL doneFirst = FALSE;

	if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 47)))
	{
		printf("Failed to open intuition.library!\n");
		cleanexit(NULL);
	}
	if (!(UtilityBase = OpenLibrary("utility.library", 47)))
	{
		printf("Failed to open utility.library!\n");
		cleanexit(NULL);
	}
	if (!(WindowBase = OpenLibrary("window.class", 47)))
	{
		printf("Failed to open window.class!\n");
		cleanexit(NULL);
	}
	if (!(LayoutBase = OpenLibrary("gadgets/layout.gadget", 47)))
	{
		printf("Failed to open gadgets/layout.gadget!\n");
		cleanexit(NULL);
	}
	if (!(ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 47)))
	{
		printf("Failed to open gadgets/listbrowser.gadget!\n");
		cleanexit(NULL);
	}
	if (!(ButtonBase = OpenLibrary("gadgets/button.gadget", 47)))
	{
		printf("Failed to open gadgets/button.gadget!\n");
		cleanexit(NULL);
	}
	if (!(SpaceBase = OpenLibrary("gadgets/space.gadget", 47)))
	{
		printf("Failed to open gadgets/space.gadget!\n");
		cleanexit(NULL);
	}
	if (!(GetFileBase = OpenLibrary("gadgets/getfile.gadget", 47)))
	{
		printf("Failed to open gadgets/getfile.gadget!\n");
		cleanexit(NULL);
	}
	if (!(TextFieldBase = OpenLibrary("gadgets/texteditor.gadget", 47)))
	{
		printf("Failed to open gadgets/texteditor.gadget!\n");
		cleanexit(NULL);
	}
	

	NewList(&contents);

	UBYTE *buffer = AllocVec(64, MEMF_ANY);
	UBYTE *buffer1 = AllocVec(64, MEMF_ANY);
	UBYTE *buffer2 = AllocVec(64, MEMF_ANY);

	struct Node *node;
	SNPrintf(buffer, 64, "Select a");
	SNPrintf(buffer1, 64, "folder");
	SNPrintf(buffer2, 64, "to scan!");
	node = AllocListBrowserNode(3,
								LBNA_Column, 0,
								LBNCA_CopyText, TRUE,
								LBNCA_Text, buffer,
								LBNCA_MaxChars, 40,
								LBNA_Column, 1,
								LBNCA_CopyText, TRUE,
								LBNCA_Text, buffer1,
								LBNCA_MaxChars, 40,
								LBNA_Column, 2,
								LBNCA_CopyText, TRUE,
								LBNCA_Text, buffer2,
								LBNCA_MaxChars, 40,
								TAG_DONE);
	if (node)
		AddTail(&contents, node);
	
	FreeVec(buffer);
	FreeVec(buffer1);
	FreeVec(buffer2);

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
	CompareHook.h_Entry = (float(*)())myCompare;
	CompareHook.h_SubEntry = NULL;
	CompareHook.h_Data = NULL;

	ci = AllocLBColumnInfo(3,
						   LBCIA_Column, 0,
						   LBCIA_Title, "Name",
						   LBCIA_Weight, 80,
						   LBCIA_AutoSort, TRUE,
						   LBCIA_Sortable, TRUE,
						   LBCIA_Column,1,
						   LBCIA_Title, "%",
						   LBCIA_Weight, 45,
						   LBCIA_AutoSort, TRUE,
						   LBCIA_Sortable, TRUE,
							LBCIA_CompareHook, &CompareHook,	
						   LBCIA_Column, 2,
						   LBCIA_Title, "Size",
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
							LISTBROWSER_TitleClickable, FALSE, 
							LISTBROWSER_Spacing, 1,
							TAG_END);

	bottomText = NewObject(BUTTON_GetClass(), NULL,
						   GA_Text, "Welcome to Mnemosyne!",
						   GA_ReadOnly, TRUE,
						   BUTTON_BevelStyle, BVS_GROUP,
						   TAG_END);

	fileRequester = NewObject(GETFILE_GetClass(), NULL, 
								GA_ID, OID_FILE_REQUESTER,
								GETFILE_DrawersOnly, TRUE, 
								GETFILE_ReadOnly, TRUE,
								GETFILE_TitleText, "Select a Folder to Scan:",
							    GA_RelVerify, TRUE,
								TAG_END);

	upperLayout = NewObject(LAYOUT_GetClass(), NULL,
							LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
							LAYOUT_DeferLayout, TRUE,
							LAYOUT_SpaceInner, TRUE,
							LAYOUT_SpaceOuter, TRUE,
							LAYOUT_VertAlignment, LALIGN_CENTER,
							LAYOUT_AddChild, backButton,
								CHILD_MaxWidth, 32,
							LAYOUT_AddChild, fileRequester,
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

	appPort = CreateMsgPort();
	windowObject = NewObject(WINDOW_GetClass(), NULL,
							 WINDOW_Position, WPOS_CENTERSCREEN,
							 WINDOW_NewMenu, MenuArray,
							 WINDOW_IconifyGadget, TRUE,
							 WINDOW_IconTitle, "Mnemosyne",
							 WINDOW_Icon, GetDiskObject("PROGDIR:Mnemosyne"),
							 WINDOW_AppPort, appPort,
							 WA_Activate, TRUE,
							 WA_Title, "Mnemosyne 0.1",
							 WA_DragBar, TRUE,
							 WA_CloseGadget, TRUE,
							 WA_DepthGadget, TRUE,
							 WA_SizeGadget, TRUE,
							 WA_NewLookMenus, TRUE,
							 WA_InnerWidth, 355,
							 WA_InnerHeight, 150,
							 WA_IDCMP, IDCMP_CLOSEWINDOW,
							 WINDOW_Layout, mainLayout,
							 TAG_DONE);
	if (!windowObject)
		cleanexit(NULL);
	if (!(intuiwin = (struct Window *)DoMethod(windowObject, WM_OPEN, NULL)))
		cleanexit(windowObject);
	processEvents(windowObject, intuiwin, listBrowser, backButton, doneFirst, CompareHook, bottomText, fileRequester);
	DoMethod(windowObject, WM_CLOSE);
	clearList(contents);
	cleanexit(windowObject);
}
void processEvents(Object *windowObject, struct Window *intuiwin, Object *listBrowser, Object *backButton, BOOL doneFirst, struct Hook CompareHook, Object *bottomText, Object *fileRequester)
{
	ULONG windowsignal;
	ULONG receivedsignal;
	ULONG result;
	WORD code;
	BOOL end = FALSE;
	BOOL scanning = FALSE;

	GetAttr(WINDOW_SigMask, windowObject, &windowsignal);
	while (!end)
	{
		receivedsignal = Wait(windowsignal);
		if (receivedsignal & windowsignal) // if we received a signal
		{
			while ((result = DoMethod(windowObject, WM_HANDLEINPUT, &code)) != WMHI_LASTMSG)
			{
				switch (result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
						end = TRUE;
						break;
					case WMHI_ICONIFY:
						DoMethod(windowObject, WM_ICONIFY, NULL);
						break;
					case WMHI_UNICONIFY:
						DoMethod(windowObject, WM_OPEN, NULL);
						break;
					case WMHI_MENUPICK: {
						struct Menu *menuStrip;
						GetAttr(WINDOW_MenuStrip, windowObject, (ULONG *)&menuStrip);
						struct MenuItem *menuItem = ItemAddress(menuStrip, code);
						APTR item = GTMENUITEM_USERDATA(menuItem);
						ULONG itemIndex = (ULONG)item;
						switch (itemIndex)
						{
							case OID_MENU_OPEN:
								// printf("Clicked Open");
								if(pastPath && doneFirst)
									OpenWorkbenchObjectA(pastPath, TAG_DONE);
								break;
							case OID_MENU_ABOUT:
								// printf("Clicked About\n");
								toggleBusyPointer(windowObject, TRUE);

								aboutWin(TRUE);
								
								// printf("About window closed\n");
								toggleBusyPointer(windowObject, FALSE);
								break;
							case OID_MENU_QUIT:
								end = TRUE;
								break;

							// default:
							//  // This is for testing only
							// 	printf("Unhandled event: %d\n", result & WMHI_MENUMASK);
							// 	break;
						}
						break;
					}
					case WMHI_GADGETUP:
						switch (result & WMHI_GADGETMASK)
						{
							case OID_FILE_REQUESTER:
							{
								int fileSelect = gfRequestDir(fileRequester, intuiwin);
								if (fileSelect){

									struct List contents;
									NewList(&contents);

									TEXT *path = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
									ULONG pathPtr;
									GetAttr(GETFILE_FullFile, fileRequester, &pathPtr);
									SNPrintf(path, MAX_BUFFER, "%s", pathPtr);
									BPTR lock = Lock(path, ACCESS_READ);
									if (!lock)
									{
										updateBottomText(bottomText, windowObject, "Invalid Path, Select a valid path");
										FreeVec(path);
										break;
									}
									UnLock(lock);

									UBYTE *buffer = AllocVec(64, MEMF_ANY);
									UBYTE *buffer1 = AllocVec(64, MEMF_ANY);
									UBYTE *buffer2 = AllocVec(64, MEMF_ANY);
									struct Node *node;
									SNPrintf(buffer, 64, "Click here to");
									SNPrintf(buffer1, 64, "start");
									SNPrintf(buffer2, 64, "Scanning...");
									node = AllocListBrowserNode(3,
																LBNA_Column, 0,
																LBNCA_CopyText, TRUE,
																LBNCA_Text, buffer,
																LBNCA_MaxChars, 40,
																LBNA_Column, 1,
																LBNCA_CopyText, TRUE,
																LBNCA_Text, buffer1,
																LBNCA_MaxChars, 40,
																LBNA_Column, 2,
																LBNCA_CopyText, TRUE,
																LBNCA_Text, buffer2,
																LBNCA_MaxChars, 40,
																TAG_DONE);
									if (node)
										AddTail(&contents, node);
									SetAttrs(listBrowser, LISTBROWSER_Labels, (ULONG)&contents, TAG_DONE);

									updateBottomText(bottomText, windowObject, "Ready to Scan!");
									fileEntered = TRUE;
									doneFirst = FALSE;
									SetAttrs(listBrowser, LISTBROWSER_TitleClickable, FALSE, TAG_DONE);

									// free the buffer variables
									FreeVec(buffer);
									FreeVec(buffer1);
									FreeVec(buffer2);
								}
								break;
							}
							case OID_BACK_BUTTON:
							{
								if (scanning || !doneFirst || pastPath[strlen(pastPath) - 1] == ':')
									break;
								
								char *parentPath = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
								getParentPath(pastPath, parentPath, MAX_BUFFER);
								// printf("Parent Path: %s\n", parentPath);

								char *parentName = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
								getNameFromPath(parentPath, parentName, MAX_BUFFER);

								if (parentPath[strlen(parentPath) - 1] != ':')
								{
									strcat(parentPath, "/");
								}

								updateBottomTextW2Text(bottomText, windowObject, "Scanning: ", parentName, FALSE);
								
								toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, TRUE, TRUE);

								scanning = TRUE;

								scanPath(parentPath, FALSE, listBrowser);

								scanning = FALSE;

								updatePathText(fileRequester, parentPath);

								updateBottomTextW2Text(bottomText, windowObject, "Current: ", parentName, FALSE);
								
								toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, FALSE, TRUE);
								
								FreeVec(parentPath);
								FreeVec(parentName);
								break;
							}
							case OID_MAIN_LIST:
							{
								if (scanning)
								{
									// printf("Scanning already in progress\n");
									break;
								}

								if (!fileEntered)
								{
									// printf("No file entered\n");
									updateBottomText(bottomText, windowObject, "Please Select a folder");
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
									// printf("No event\n");
									break;
								}

								if (node)
								{
									STRPTR text = NULL;
									GetListBrowserNodeAttrs(node, LBNA_Column, 0, LBNCA_Text, &text, TAG_DONE);
									if (text)
									{
										const int len = strlen(text);

										char *parentPath = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);

										// printf("Selected %s\n", text);
										getParentPath(text, parentPath, MAX_BUFFER);

										// Check if the selected item is a directory
										if (len > 0 && text[len - 1] != '/' && doneFirst)
										{
											break;
										}

										if (doneFirst && text)
										{
											char *newPath = (char *)AllocVec(MAX_BUFFER, MEMF_CLEAR);
											if (pastPath[strlen(pastPath) - 1] != '/' && pastPath[strlen(pastPath) - 1] != ':'){
												strcat(pastPath, "/");
											}

											SNPrintf(newPath, MAX_BUFFER, "%s%s", &pastPath, text);
											updatePathText(fileRequester, newPath);
											updateBottomTextW2Text(bottomText, windowObject, "Scanning: ", text, FALSE);
											toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, TRUE, TRUE);

											scanning = TRUE;

											scanPath(newPath, FALSE, listBrowser);

											toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, FALSE, FALSE);

											scanning = FALSE;
										}
										else
										{
											scanning = TRUE;
											TEXT *buffer = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
											ULONG pathPtr;
											GetAttr(GETFILE_FullFile, fileRequester, &pathPtr);
											SNPrintf(buffer, MAX_BUFFER, "%s", pathPtr);

											updateBottomTextW2Text(bottomText, windowObject, "Scanning: ", (STRPTR)pathPtr, FALSE);

											toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, TRUE, TRUE);


											scanPath((char *)pathPtr, FALSE, listBrowser);

											FreeVec(buffer);
											scanning = FALSE;
											toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, FALSE, FALSE);
											doneFirst = TRUE;
											DoGadgetMethod((struct Gadget*)listBrowser, intuiwin, NULL, LBM_SORT, NULL, 1, LBMSORT_REVERSE, &CompareHook);
										}

										// printf("Donefirst: %d\n", doneFirst);
										char *parentName = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
										getNameFromPath(pastPath, parentName, MAX_BUFFER);
										updateBottomTextW2Text(bottomText, windowObject, "Current: ", parentName, TRUE);

										FreeVec(parentName);
										FreeVec(parentPath);
									}
								}
								break;
							}
							// default:
							// 	// This is for testing only
							// 	printf("Unhandled event of category %ld\n", result & WMHI_GADGETMASK);
							// 	break;
						}
						// default:
						// 	// This is for testing only
						// 	printf("Unhandled event of class %ld\n", result & WMHI_CLASSMASK);
						// 	break;
						break;
				}
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
	CloseLibrary(GetFileBase);
	CloseLibrary(TextFieldBase);
	clearScanning();
	exit(0);
}
