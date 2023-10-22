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
#include <proto/getfile.h>
#include <proto/asl.h>
#include <proto/wb.h>
#include <proto/icon.h>
#include <proto/dos.h>
#include <proto/gadtools.h>

#include <libraries/gadtools.h>

#include <clib/reaction_lib_protos.h>

//
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <graphics/gfxmacros.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <libraries/asl.h>
#include <utility/tagitem.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <images/glyph.h>
#include <images/label.h>

#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <clib/alib_protos.h>

#include <proto/glyph.h>
#include <proto/label.h>
//

#include "window.h"
#include "scan.h"
#include "funcs.h"


// -------------
// Global Variables / Enums
// -------------

enum
{
	OID_BACK_BUTTON,
	OID_MAIN_LIST,
	OID_FILE_REQUESTER,
	OID_MENU_OPEN_DIR,
	OID_MENU_ABOUT,
	OID_MENU_QUIT,
	OID_SCAN_BUTTON,
	OID_SCAN_OPEN,
	OID_LAST
};

static struct Menu *menu;

static struct NewMenu MenuArray[] = {
	{NM_TITLE, "Project", 0, 0, 0, 0},
	{NM_ITEM, "Open", 0, 0, 0, (APTR)OID_SCAN_OPEN},
	{NM_ITEM, "Open in Workbench...", 0, ITEMENABLED, 0, (APTR)OID_MENU_OPEN_DIR},
	{NM_ITEM, NM_BARLABEL,0,0,0,0 },
	{NM_ITEM, "About...", 0, 0, 0, (APTR)OID_MENU_ABOUT},
	{NM_ITEM, "Quit...", 0, 0, 0, (APTR)OID_MENU_QUIT},
	{NM_END, NULL, 0, 0, 0, NULL}
};

// Make array to store values
struct  {
    int Column;
    BOOL Sorting;
} ColumnSorting[] = {
    { 0, LBMSORT_FORWARD },
    { 1, LBMSORT_FORWARD }
};

BOOL fileEntered = FALSE;

// -------------
// Functions and such
// -------------

static float asValue(STRPTR s)
{
	float v = atof(s);
	// Check if first char is a < sign
	if (s[0] == '<')
		v = 0.001f;
	return v;
}
static float __SAVE_DS__ __ASM__ myCompare(__REG__(a0, struct Hook *hook), __REG__(a2, Object *obj),
										   __REG__(a1, struct LBSortMsg *msg))
{
	return asValue(msg->lbsm_DataA.Text) - asValue(msg->lbsm_DataB.Text);
}

static int __SAVE_DS__ __ASM__ myCompare2(__REG__(a0, struct Hook *hook), __REG__(a2, Object *obj),
										   __REG__(a1, struct LBSortMsg *msg))
{
	return strcmp(string_to_lower(msg->lbsm_DataA.Text, strlen(msg->lbsm_DataA.Text)), string_to_lower(msg->lbsm_DataB.Text, strlen(msg->lbsm_DataB.Text)));
}



void UpdateMenu(struct Window *intuiwin, BOOL enabled){
	APTR *visualInfo;
	ULONG error = (ULONG)NULL;
	struct Screen *screen = intuiwin->WScreen;

	FreeMenus(menu);
	if (visualInfo = GetVisualInfo(screen, NULL)) {
		if (menu = CreateMenus(MenuArray, GTMN_SecondaryError, &error, TAG_DONE)) {
			if (LayoutMenus(menu, visualInfo, GTMN_NewLookMenus, TRUE, TAG_DONE)) {
				if (SetMenuStrip(intuiwin, menu)) {
					if (enabled)
						RefreshWindowFrame(intuiwin);
				} else {
					// printf("Error setting menu strip\n");
				}
			} else {
				// printf("Error laying out menu\n");
			}
		} else {
			printf("Error creating menu\n");
		}
		FreeVisualInfo(visualInfo);
	}
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
	char *title = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
	snprintf(title, MAX_BUFFER, "%s%s", firstText, secondText);
	SetAttrs(bottomText, GA_Text, title, TAG_DONE);
	if (Refresh)
		DoMethod(windowObject, WM_NEWPREFS);
	FreeVec(title);
}

void updateBottomTextW2AndTotal(Object *bottomText, Object *windowObject, char *firstText, STRPTR secondText, STRPTR totalText, BOOL Refresh)
{
	char *title = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
	snprintf(title, MAX_BUFFER, "%s%s%s", firstText, secondText, totalText);
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
	// if (strcmp(bottomTextString, secondText) == 0)
	// 	return;

	SetAttrs(bottomText, GA_Text, secondText, TAG_DONE);
	DoMethod(windowObject, WM_NEWPREFS);
}

void updatePathText(Object *fileRequester, STRPTR path) {
	SetAttrs(fileRequester, GETFILE_FullFile, path, TAG_DONE);
}

void updateMenuItems(struct Window *intuiwin, BOOL enabled){
	if (enabled == TRUE && WorkbenchBase->lib_Version >= 44){
		MenuArray[2] = (struct NewMenu){NM_ITEM, "Open in Workbench...", 0, 0, 0, (APTR)OID_MENU_OPEN_DIR};
	}
	else {
		MenuArray[2] = (struct NewMenu){NM_ITEM, "Open in Workbench...", 0, ITEMENABLED, 0, (APTR)OID_MENU_OPEN_DIR};
	}
	// SetAttrs(windowObject, WINDOW_NewMenu, MenuArray, TAG_DONE);
	UpdateMenu(intuiwin, FALSE);
}

void fileRequesterSequence(Object *fileRequester,
	struct Window *intuiwin,
	Object *bottomText,
	Object *windowObject,
	Object *scanButton,
	struct Gadget *listBrowser,
	Object *backButton,
	BOOL doneFirst)
{
	int fileSelect = gfRequestDir(fileRequester, intuiwin);
	if (fileSelect){

		struct List contents;
		NewList(&contents);

		TEXT *path = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
		ULONG pathPtr;
		GetAttr(GETFILE_FullFile, fileRequester, &pathPtr);
		strlcpy(path, (const char*)pathPtr, MAX_BUFFER - 1);
		path[MAX_BUFFER - 1] = '\0';
		BPTR lock = Lock(path, ACCESS_READ);
		SetAttrs(listBrowser, GA_DISABLED, TRUE, TAG_DONE);
		SetAttrs(backButton, GA_Disabled, TRUE, TAG_DONE);
		if (!lock)
		{
			SetAttrs(scanButton, GA_Disabled, TRUE, TAG_DONE);

			updateMenuItems(intuiwin, FALSE);

			updateBottomText(bottomText, windowObject, "Invalid Path, Select a valid path");
			FreeVec(path);
			return;
		}
		UnLock(lock);
		SetAttrs(scanButton, GA_Disabled, FALSE, TAG_DONE);

		updateMenuItems(intuiwin, FALSE);

		updateBottomText(bottomText, windowObject, "Ready to Scan!");
		fileEntered = TRUE;
		doneFirst = FALSE;
		SetAttrs(listBrowser, LISTBROWSER_TitleClickable, FALSE, TAG_DONE);
	}
}


// -------------
// Main Window Functions
// -------------

void cleanexit(Object *windowObject, struct MsgPort *appPort);
void processEvents(Object *windowObject,
				   struct Window *intuiwin,
				   struct Gadget *listBrowser,
				   Object *backButton,
				   BOOL doneFirst,
				   struct Hook CompareHook,
				   struct Hook NameHook,
				   Object *bottomText,
				   Object *fileRequester,
				   Object *scanButton);

void createWindow(void)
{
	struct Window *intuiwin = NULL;
	Object *windowObject = NULL;

	struct Hook CompareHook;
	struct Hook NameHook;

	struct MsgPort *appPort;

	Object *mainLayout = NULL;
	Object *upperLayout = NULL;
	Object *upperRightLayout = NULL;

	struct Gadget *listBrowser = NULL;
	Object *backButton = NULL;
	Object *bottomText = NULL;
	Object *fileRequester = NULL;
	Object *scanButton = NULL;

	struct List contents;

	BOOL doneFirst = FALSE;

	NewList(&contents);

	struct Node *node;
	node = AllocListBrowserNode(3,
								LBNA_Column, 0,
								LBNCA_CopyText, TRUE,
								LBNCA_Text, "",
								LBNCA_MaxChars, 40,
								LBNA_Column, 1,
								LBNCA_CopyText, TRUE,
								LBNCA_Text, "",
								LBNCA_MaxChars, 40,
								LBNA_Column, 2,
								LBNCA_CopyText, TRUE,
								LBNCA_Text, "",
								LBNCA_MaxChars, 40,
								TAG_DONE);
	if (node)
		AddTail(&contents, node);

	backButton = NewObject(BUTTON_GetClass(), NULL,
						   GA_ID, OID_BACK_BUTTON,
						   GA_RelVerify, TRUE,
						//    GA_Width, 10,
						//    GA_Height, 10,
						   GA_Text, "Back",
						   GA_Disabled, TRUE, // Disabled so it doesn't go back to SYS:
						   TAG_END);

	/* initialize CompareHook for sorting the column */
	CompareHook.h_Entry = (ULONG(*)())myCompare;
	CompareHook.h_SubEntry = NULL;
	CompareHook.h_Data = NULL;

	NameHook.h_Entry = (ULONG(*)())myCompare2;
	NameHook.h_SubEntry = NULL;
	NameHook.h_Data = NULL;

	struct ColumnInfo ci[] =
	{
		{80, "Name", CIF_SORTABLE},
		{45, "Approx %", CIF_SORTABLE},
		{60, "Size", 0},
		{ -1, (STRPTR)~0, -1 }
	};
	listBrowser = (struct Gadget *)ListBrowserObject,
					GA_ID, OID_MAIN_LIST,
					GA_RelVerify, TRUE,
					GA_Disabled, TRUE,
					LISTBROWSER_Labels, &contents,
					LISTBROWSER_ColumnInfo, &ci,
					LISTBROWSER_ColumnTitles, TRUE,
					LISTBROWSER_MultiSelect, FALSE,
					LISTBROWSER_Separators, TRUE,
					LISTBROWSER_ShowSelected, FALSE,
					LISTBROWSER_TitleClickable, TRUE,
					ListBrowserEnd;
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

	scanButton = NewObject(BUTTON_GetClass(), NULL,
						   GA_Text, "Scan",
						   GA_RelVerify, TRUE,
						   GA_ID, OID_SCAN_BUTTON,
						   GA_Disabled, TRUE,
						   TAG_DONE);

	upperRightLayout = NewObject(LAYOUT_GetClass(), NULL,
							LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
							LAYOUT_DeferLayout, TRUE,
							LAYOUT_SpaceInner, TRUE,
							LAYOUT_SpaceOuter, TRUE,
							LAYOUT_AddChild, fileRequester,
								CHILD_WeightedHeight, 10,
							LAYOUT_AddChild, scanButton,
							TAG_DONE);

	upperLayout = NewObject(LAYOUT_GetClass(), NULL,
							LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
							LAYOUT_DeferLayout, TRUE,
							LAYOUT_SpaceInner, TRUE,
							LAYOUT_SpaceOuter, FALSE,
							LAYOUT_VertAlignment, LALIGN_CENTER,
							LAYOUT_AddChild, backButton,
								CHILD_WeightedWidth, 10,
							LAYOUT_AddChild, upperRightLayout,
							TAG_DONE);

	mainLayout = NewObject(LAYOUT_GetClass(), NULL,
						   LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						   LAYOUT_DeferLayout, TRUE,
						   LAYOUT_SpaceInner, TRUE,
						   LAYOUT_SpaceOuter, TRUE,
						   LAYOUT_EvenSize, TRUE,
						   LAYOUT_AddChild, upperLayout,
						   		CHILD_WeightedHeight, 10,
						   LAYOUT_AddChild, listBrowser,
						   LAYOUT_AddChild, bottomText,
						   		CHILD_WeightedHeight, 10,
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
							 WA_Title, "Mnemosyne 1.0.1",
							 WA_DragBar, TRUE,
							 WA_CloseGadget, TRUE,
							 WA_DepthGadget, TRUE,
							 WA_SizeGadget, TRUE,
							 WA_NewLookMenus, TRUE,
							 WA_InnerWidth, 355,
							 WA_InnerHeight, 150,
							 WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_MENUPICK,
							 WINDOW_Layout, mainLayout,
							 TAG_DONE);
	if (!windowObject)
		cleanexit(NULL, NULL);
	if (!(intuiwin = (struct Window *)DoMethod(windowObject, WM_OPEN, NULL)))
		cleanexit(windowObject, appPort);
	UpdateMenu(intuiwin, TRUE);
	processEvents(windowObject, intuiwin, listBrowser, backButton, doneFirst, CompareHook, NameHook, bottomText, fileRequester, scanButton);
	DoMethod(windowObject, WM_CLOSE);
	clearList(contents);
	cleanexit(windowObject, appPort);
}
void processEvents(Object *windowObject,
				   struct Window *intuiwin,
				   struct Gadget *listBrowser,
				   Object *backButton,
				   BOOL doneFirst,
				   struct Hook CompareHook,
				   struct Hook NameHook,
				   Object *bottomText,
				   Object *fileRequester,
				   Object *scanButton)
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
					{
						if (( intuiwin = (struct Window *)DoMethod(windowObject, WM_OPEN, NULL)))
							GetAttr(WINDOW_SigMask, windowObject, &windowsignal);
					}
						break;
					case WMHI_MENUPICK: {
						struct Menu *menuStrip;
						GetAttr(WINDOW_MenuStrip, windowObject, (ULONG *)&menuStrip);
						struct MenuItem *menuItem = ItemAddress(menu, code);
						if (!menuItem)
							break;
						APTR item = GTMENUITEM_USERDATA(menuItem);
						ULONG itemIndex = (ULONG)item;
						switch (itemIndex)
						{
							case OID_SCAN_OPEN:
							{
								fileRequesterSequence(fileRequester,
													  intuiwin,
													  bottomText,
													  windowObject,
													  scanButton,
													  listBrowser,
													  backButton,
													  doneFirst);
								break;
							}
							case OID_MENU_OPEN_DIR:
								// printf("Clicked Open");
								if(pastPath[0] != '\0' && doneFirst)
									OpenWorkbenchObjectA(pastPath, TAG_DONE);
								break;
							case OID_MENU_ABOUT:
								// printf("Clicked About\n");
								toggleBusyPointer(windowObject, TRUE);
								struct EasyStruct requesterAbout = {
									sizeof(struct EasyStruct),
									0,
									"About",
									"Copyright (c) 2023 Aris (Arisamiga) \nSokianos\n\n"
									"Mnemosyne is an open source disk \nutility application for AmigaOS 3.2,\n" \
									"which can be used to see how much \ndisk space your files and folders \nare taking up.\n\n" \
									"\"Mnemosyne\", in Greek mythology is \nthe goddess of memory.\n\n" \
									"Thank you so much for using \nMnemosyne :D\n\n" \
									"Report bugs or request features at:\nhttps://github.com/Arisamiga/Mnemosyne\n\n" \
									"Distributed without warranty under \nthe terms of the \nGNU General Public License.",
									"OK"
								};
								EasyRequest(intuiwin, &requesterAbout, NULL, NULL);

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
								fileRequesterSequence(fileRequester,
													  intuiwin,
													  bottomText,
													  windowObject,
													  scanButton,
													  listBrowser,
													  backButton,
													  doneFirst);
								break;
							}
							case OID_BACK_BUTTON:
							{

								if (scanning || !doneFirst || getLastCharSafely(pastPath) == ':')
									break;

								char *parentPath = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
								getParentPath(pastPath, parentPath, MAX_BUFFER);
								// printf("Parent Path: %s\n", parentPath);

								char *parentName = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
								getNameFromPath(parentPath, parentName, MAX_BUFFER);

								if (getLastCharSafely(parentPath) != ':')
								{
									__asm_strncat(parentPath, "/",2);
								}

								updateBottomTextW2Text(bottomText, windowObject, "Scanning: ", parentName, FALSE);

								toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, TRUE, TRUE);

								scanning = TRUE;

								scanPath(parentPath, FALSE, listBrowser);

								scanning = FALSE;

								updatePathText(fileRequester, parentPath);

								DoGadgetMethod((struct Gadget*)listBrowser, intuiwin, NULL, LBM_SORT, NULL, 1, LBMSORT_REVERSE, &CompareHook);
								ColumnSorting[1].Sorting = LBMSORT_REVERSE;

								updateMenuItems(intuiwin, TRUE);

								STRPTR TotalText = returnFormatWithTotal();
								updateBottomTextW2AndTotal(bottomText, windowObject, "Current: ", parentName, TotalText, FALSE);

								toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, FALSE, TRUE);

								FreeVec(TotalText);
								FreeVec(parentPath);
								FreeVec(parentName);
								break;
							}
							case OID_SCAN_BUTTON:
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

								scanning = TRUE;
								TEXT *buffer = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
								ULONG pathPtr;

								GetAttr(GETFILE_FullFile, fileRequester, &pathPtr);

								snprintf(buffer, MAX_BUFFER, "%s", (char *)pathPtr);

								char *parentName = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);

								getNameFromPath(buffer, parentName, MAX_BUFFER);

								updateBottomTextW2Text(bottomText, windowObject, "Scanning: ", (STRPTR)parentName, FALSE);

								SetAttrs(scanButton, GA_Disabled, TRUE, TAG_DONE);

								toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, TRUE, TRUE);


								scanPath((char *)pathPtr, FALSE, listBrowser);

								FreeVec(buffer);

								scanning = FALSE;

								toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, FALSE, FALSE);

								doneFirst = TRUE;

								DoGadgetMethod((struct Gadget*)listBrowser, intuiwin, NULL, LBM_SORT, NULL, 1, LBMSORT_REVERSE, &CompareHook);
								ColumnSorting[1].Sorting = LBMSORT_REVERSE;

								updateMenuItems(intuiwin, TRUE);

								// printf("Donefirst: %d\n", doneFirst);
								STRPTR TotalText = returnFormatWithTotal();
								updateBottomTextW2AndTotal(bottomText, windowObject, "Current: ", parentName, TotalText, TRUE);

								FreeVec(parentName);
								FreeVec(TotalText);
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

								if (!node){
									ULONG column = 0;
									GetAttr(LISTBROWSER_RelColumn, listBrowser, &column);
									// printf("Colums: %ld\n", column);
									// printf("Current sorting: %d\n", ColumnSorting[column].Sorting);
									ColumnSorting[column].Sorting = !ColumnSorting[column].Sorting;
									if (column == 1)
										DoGadgetMethod((struct Gadget*)listBrowser, intuiwin, NULL, LBM_SORT, NULL, column, ColumnSorting[column].Sorting, &CompareHook);
									else if (column == 0)
										DoGadgetMethod((struct Gadget*)listBrowser, intuiwin, NULL, LBM_SORT, NULL, column, ColumnSorting[column].Sorting, &NameHook);
									break;
								}
								STRPTR text = NULL;
								GetListBrowserNodeAttrs(node, LBNA_Column, 0, LBNCA_Text, &text, TAG_DONE);
								if (text)
								{
									const int len = strlen(text);

									char *parentPath = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);

									// printf("Selected %s\n", text);
									getParentPath(text, parentPath, MAX_BUFFER);

									// Check if the selected item is not a directory
									if (len > 0 && text[len - 1] != '/' && doneFirst)
									{
										// Remove focus from the listbrowser
										SetAttrs(listBrowser, LISTBROWSER_Selected, -1, TAG_DONE);
										break;
									}

									if (doneFirst && text != NULL)
									{
										char *newPath = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
										if (getLastCharSafely(pastPath) != '/' && getLastCharSafely(pastPath) != ':'){
											strcat(pastPath, "/");
										}

										snprintf(newPath, MAX_BUFFER, "%s%s", pastPath, text);
										updatePathText(fileRequester, newPath);
										updateBottomTextW2Text(bottomText, windowObject, "Scanning: ", text, FALSE);
										toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, TRUE, TRUE);

										scanning = TRUE;

										scanPath(newPath, FALSE, listBrowser);

										toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, FALSE, FALSE);

										scanning = FALSE;
									}

									DoGadgetMethod((struct Gadget*)listBrowser, intuiwin, NULL, LBM_SORT, NULL, 1, LBMSORT_REVERSE, &CompareHook);
									ColumnSorting[1].Sorting = LBMSORT_REVERSE;

									updateMenuItems(intuiwin, TRUE);
									// printf("Donefirst: %d\n", doneFirst);
									char *parentName = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
									getNameFromPath(pastPath, parentName, MAX_BUFFER);
									STRPTR TotalText = returnFormatWithTotal();
									updateBottomTextW2AndTotal(bottomText, windowObject, "Current: ", parentName, TotalText, TRUE);

									// Remove focus from the listbrowser
									SetAttrs(listBrowser, LISTBROWSER_Selected, -1, TAG_DONE);

									FreeVec(TotalText);
									FreeVec(parentName);
									FreeVec(parentPath);
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
						// break;
				}
			}
		}
	}
}
void cleanexit(Object *windowObject, struct MsgPort *appPort)
{
	if (windowObject)
		DisposeObject(windowObject);

	if (appPort)
		DeleteMsgPort(appPort);
	clearScanning();
}
