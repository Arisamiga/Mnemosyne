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

#include <proto/string.h>

#include <libraries/gadtools.h>

#include <clib/reaction_lib_protos.h>

//
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/asl.h>
#include <utility/tagitem.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <images/bitmap.h>
#include <proto/bitmap.h>
#include <graphics/gfx.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>

#include <clib/alib_protos.h>

#include <workbench/workbench.h>
#include <workbench/startup.h>

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
	OID_MENU_NO_ROUND,
	OID_MENU_ENABLE_GRAPH,
	OID_SCAN_BUTTON,
	OID_SCAN_OPEN,
	OID_GIVEN_PATH,
	OID_LAST
};

static struct Menu *menu;

static struct NewMenu MenuArray[] = {
	{NM_TITLE, "Project", 0, 0, 0, 0},
	{NM_ITEM, "Open", "O", 0, 0, (APTR)OID_SCAN_OPEN},
	{NM_ITEM, "Open in Workbench...", 0, ITEMENABLED, 0, (APTR)OID_MENU_OPEN_DIR},
	{NM_ITEM, NM_BARLABEL,0,0,0,0 },
	{NM_ITEM, "About...", "A", 0, 0, (APTR)OID_MENU_ABOUT},
	{NM_ITEM, "Quit...", "Q", 0, 0, (APTR)OID_MENU_QUIT},
	{NM_TITLE, "Settings", 0, 0, 0, 0},
	{NM_ITEM, "Round Numbers", 0, CHECKIT, 0, (APTR)OID_MENU_NO_ROUND},
	{NM_ITEM, "Enable Graph", 0, CHECKIT, 0, (APTR)OID_MENU_ENABLE_GRAPH},

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
static BOOL scanning = FALSE;
static Object *completionButton = NULL;
static Object *mainWindowObject = NULL;

// Progress callback used by scanPath to report the current path being scanned.
static void scanProgressCallback(const char *path, void *userData)
{
	if (!path || !userData)
		return;
	Object **objs = (Object **)userData;
	Object *bottomText = objs[0];
	Object *windowObject = objs[1];
	if (!bottomText || !windowObject)
		return;

	char *displayName = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
	if (!displayName)
		return;
	getNameFromPath((char *)path, displayName, MAX_BUFFER);


	if ((rand() % 1000) == 0) {
		char *eggName = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
		if (eggName) {
			snprintf(eggName, MAX_BUFFER, "%s :D", displayName);
			updateBottomTextW2Text(bottomText, windowObject, "Scanning: ", eggName, TRUE);
			FreeVec(eggName);
		}
	} else {
		updateBottomTextW2Text(bottomText, windowObject, "Scanning: ", displayName, TRUE);
	}
	FreeVec(displayName);
}

static void clearCompletionBitmap(Object *bottomText)
{
	if (!completionButton || !EnableGraphOption)
		return;

	SetAttrs(completionButton,
			 GA_Image, NULL,
			 GA_Width, 0,
			 GA_Height, 0,
			 GA_Disabled, TRUE,
			 TAG_DONE);
	if (mainWindowObject)
		DoMethod(mainWindowObject, WM_NEWPREFS);
}


static void showCompletionBitmap(Object *bottomText, struct Gadget *listbrowser)
{
    if (!completionButton || !EnableGraphOption)
        return;

    if (!listbrowser)
        return;

    struct NodeData nodeArray[128];
    ULONG nodeCount = collectAllNodePercentages(listbrowser, nodeArray, 128);

    if (nodeCount == 0)
        return;

    struct Image *image1 = NULL;
    struct BitMap *bm = AllocVec(sizeof(struct BitMap), MEMF_CLEAR);
    if (!bm)
        return;

	/* Use current gadget size so the graph fills the entire completion area. */
	ULONG bitmapWidth = 0;
	ULONG bitmapHeight = 0;
	GetAttr(GA_Width, completionButton, &bitmapWidth);
	GetAttr(GA_Height, completionButton, &bitmapHeight);

	if (bitmapWidth < 16)
		bitmapWidth = 200;
	if (bitmapHeight < 16)
		bitmapHeight = 100;

    InitBitMap(bm, 4, bitmapWidth, bitmapHeight);

    for (int plane = 0; plane < 4; plane++) {
        bm->Planes[plane] = AllocRaster(bitmapWidth, bitmapHeight);
        if (!bm->Planes[plane]) {
            FreeVec(bm);
            return;
        }
        memset(bm->Planes[plane], 0, RASSIZE(bitmapWidth, bitmapHeight));
    }

    struct RastPort rp;
    InitRastPort(&rp);
    rp.BitMap = bm;

    drawTreemapRectangles(&rp, nodeArray, nodeCount, bitmapWidth, bitmapHeight);

    image1 = BitMapObject,
        BITMAP_BitMap, bm,
        BITMAP_Width, bitmapWidth,
		BITMAP_Height, bitmapHeight,
    EndImage;

    SetAttrs(completionButton,
             GA_Image, image1,
             GA_Width, bitmapWidth,
			 GA_Height, bitmapHeight,
             GA_Disabled, FALSE,
             TAG_DONE);

    if (mainWindowObject)
        DoMethod(mainWindowObject, WM_NEWPREFS);
}


// -------------
// Functions and such
// -------------




void UpdateMenuToolTypes() {
	if (NoRoundOption) {
		MenuArray[7] = (struct NewMenu){NM_ITEM, "Round Numbers", 0, CHECKIT, 0, (APTR)OID_MENU_NO_ROUND};
	}
	else {
		MenuArray[7] = (struct NewMenu){NM_ITEM, "Round Numbers", 0, CHECKIT|CHECKED, 0, (APTR)OID_MENU_NO_ROUND};
	}
	if (EnableGraphOption) {
		MenuArray[8] = (struct NewMenu){NM_ITEM, "Enable Graph", 0, CHECKIT|CHECKED, 0, (APTR)OID_MENU_ENABLE_GRAPH};
	}
	else {
		MenuArray[8] = (struct NewMenu){NM_ITEM, "Enable Graph", 0, CHECKIT, 0, (APTR)OID_MENU_ENABLE_GRAPH};
	}
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



void updateMenuItems(struct Window *intuiwin, BOOL enabled){
	if (enabled == TRUE && WorkbenchBase->lib_Version >= 44){
		MenuArray[2] = (struct NewMenu){NM_ITEM, "Open in Workbench...", "W", 0, 0, (APTR)OID_MENU_OPEN_DIR};
	}
	else {
		MenuArray[2] = (struct NewMenu){NM_ITEM, "Open in Workbench...", 0, ITEMENABLED, 0, (APTR)OID_MENU_OPEN_DIR};
	}
	UpdateMenuToolTypes();
	// SetAttrs(windowObject, WINDOW_NewMenu, MenuArray, TAG_DONE);
	UpdateMenu(intuiwin, FALSE);
}

void setFileSequence(
	struct Window *intuiwin,
	Object *bottomText,
	Object *windowObject,
	Object *scanButton,
	struct Gadget *listBrowser,
	Object *backButton,
	BOOL doneFirst,
	TEXT *path
)
{
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
		setFileSequence(intuiwin, bottomText, windowObject, scanButton, listBrowser, backButton, doneFirst, path);
	}
}
void scanningSequence(int type,
		struct Window *intuiwin,
		Object *windowObject,
		Object *bottomText,
		Object *scanButton,
		Object *backButton,
		struct Gadget *listBrowser,
		Object *fileRequester,
		BOOL doneFirst,
		struct Hook CompareHook,
		char *givenPath,
		struct MsgPort *appPort)
{
	if (scanning)
	{
		// printf("Scanning already in progress\n");
		return;
	}

	if (!fileEntered)
	{
		// printf("No file entered\n");
		updateBottomText(bottomText, windowObject, "Please Select a folder");
		return;
	}

	BPTR lockPath = Lock(givenPath, ACCESS_READ);
	if (!lockPath)
	{
		updateBottomText(bottomText, windowObject, "Invalid Path, Select a valid path");
		return;
	}
	UnLock(lockPath);

	scanning = TRUE;
	clearCompletionBitmap(bottomText);

	char *parentName = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);

	getNameFromPath(givenPath, parentName, MAX_BUFFER);

	if (type == OID_MAIN_LIST || type == OID_GIVEN_PATH)
		updatePathText(fileRequester, givenPath);

	updateBottomTextW2Text(bottomText, windowObject, "Scanning: ", parentName, FALSE);

	SetAttrs(scanButton, GA_Disabled, TRUE, TAG_DONE);

	toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, TRUE, TRUE);

	/* Pass progress callback so the UI receives periodic updates while
	 * scanning. We pass a small stack array as userData; it remains valid
	 * for the duration of the synchronous scan.
	 */
	Object *cbData[2];
	cbData[0] = bottomText;
	cbData[1] = windowObject;
	scanPath(givenPath, FALSE, listBrowser, scanProgressCallback, cbData);

	// Drop any Workbench icon-drop messages that arrived during scanning.
	flushAppPort(appPort);

	scanning = FALSE;

	toggleButtons(windowObject, backButton, listBrowser, fileRequester, pastPath, doneFirst, FALSE, FALSE);

	if (EnableGraphOption){
		ColumnSorting[1].Sorting = LBMSORT_REVERSE;
		DoGadgetMethod(listBrowser, intuiwin, NULL, LBM_SORT, NULL, 2, ColumnSorting[1].Sorting, &CompareHook);
	}
	else
	{
		ColumnSorting[1].Sorting = LBMSORT_REVERSE;
		DoGadgetMethod(listBrowser, intuiwin, NULL, LBM_SORT, NULL, 1, ColumnSorting[1].Sorting, &CompareHook);
	}

	if(type == OID_BACK_BUTTON)
		updatePathText(fileRequester, givenPath);

	updateMenuItems(intuiwin, TRUE);

	STRPTR TotalText = returnFormatWithTotal();
	updateBottomTextW2AndTotal(bottomText, windowObject, "Current: ", parentName, TotalText, TRUE);

	showCompletionBitmap(bottomText, listBrowser);

	// Remove focus from the listbrowser
	SetAttrs(listBrowser, LISTBROWSER_Selected, -1, TAG_DONE);

	FreeVec(TotalText);
	FreeVec(parentName);
}

// -------------
// Main Window Functions
// -------------

void cleanexit(Object *windowObject, struct MsgPort *appPort, struct AppWindow *appWin);
void processEvents(Object *windowObject,
				   struct Window *intuiwin,
				   struct Gadget *listBrowser,
				   struct MsgPort *appPort,
				   Object *backButton,
				   BOOL doneFirst,
				   struct Hook CompareHook,
				   struct Hook NameHook,
				   Object *bottomText,
				   Object *fileRequester,
				   Object *scanButton,
				   char *givenPath);
void createWindow(char *Path)
{
	struct Window *intuiwin = NULL;
	Object *windowObject = NULL;

	struct Hook CompareHook;
	struct Hook NameHook;

	struct MsgPort *appPort;

	struct AppWindow *appWin;

	Object *mainLayout = NULL;
	Object *upperLayout = NULL;
	Object *upperRightLayout = NULL;
	Object *lowerLayout = NULL;

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

	struct ColumnInfo ciWithColours[] =
	{
		{10, "", 0},
		{80, "Name", CIF_SORTABLE},
		{45, "Approx %", CIF_SORTABLE},
		{60, "Size", 0},
		{ -1, (STRPTR)~0, -1 }
	};

	struct ColumnInfo ciWithoutColours[] =
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
			LISTBROWSER_ColumnInfo, &ciWithoutColours,
			LISTBROWSER_ColumnTitles, TRUE,
			LISTBROWSER_MultiSelect, FALSE,
			LISTBROWSER_Separators, TRUE,
			LISTBROWSER_ShowSelected, FALSE,
			LISTBROWSER_TitleClickable, TRUE,
			ListBrowserEnd;

	if (EnableGraphOption){
		node = AllocListBrowserNode(4,
						LBNA_Column, 0,
						LBNCA_Image, NULL,
						LBNA_Column, 1,
						LBNCA_CopyText, TRUE,
						LBNCA_Text, "",
						LBNCA_MaxChars, 40,
						LBNA_Column, 2,
						LBNCA_CopyText, TRUE,
						LBNCA_Text, "",
						LBNCA_MaxChars, 40,
						LBNA_Column, 3,
						LBNCA_CopyText, TRUE,
						LBNCA_Text, "",
						LBNCA_MaxChars, 40,
						TAG_DONE);

		if (node)
			NewList(&contents);
			AddTail(&contents, node);


		listBrowser = (struct Gadget *)ListBrowserObject,
					GA_ID, OID_MAIN_LIST,
					GA_RelVerify, TRUE,
					GA_Disabled, TRUE,
					LISTBROWSER_Labels, &contents,
					LISTBROWSER_ColumnInfo, &ciWithColours,
					LISTBROWSER_ColumnTitles, TRUE,
					LISTBROWSER_MultiSelect, FALSE,
					LISTBROWSER_Separators, TRUE,
					LISTBROWSER_ShowSelected, FALSE,
					LISTBROWSER_TitleClickable, TRUE,
					ListBrowserEnd;

	}

	UpdateMenuToolTypes();



	bottomText = NewObject(STRING_GetClass(), NULL,
						GA_ReadOnly, TRUE,
						STRINGA_TextVal, "Welcome to Mnemosyne!",
						STRINGA_Justification, GACT_STRINGCENTER,
						TAG_END);

	completionButton = NewObject(BUTTON_GetClass(), NULL,
						GA_ReadOnly, TRUE,
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

	lowerLayout = NewObject(LAYOUT_GetClass(), NULL,
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					LAYOUT_DeferLayout, TRUE,
					LAYOUT_SpaceInner, TRUE,
					LAYOUT_SpaceOuter, FALSE,
					LAYOUT_AddChild, completionButton,
						CHILD_WeightedHeight, 40,
					LAYOUT_AddChild, bottomText,
						CHILD_WeightedHeight, 10,
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


	if (EnableGraphOption)
	{
		mainLayout = NewObject(LAYOUT_GetClass(), NULL,
							LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
							LAYOUT_DeferLayout, TRUE,
							LAYOUT_SpaceInner, TRUE,
							LAYOUT_SpaceOuter, TRUE,
							LAYOUT_EvenSize, FALSE,
							LAYOUT_AddChild, upperLayout,
									CHILD_WeightedHeight, 10,
							LAYOUT_AddChild, listBrowser,
									CHILD_WeightedHeight, 55,
							LAYOUT_AddChild, lowerLayout,
									CHILD_WeightedHeight, 35,
							TAG_DONE);
	}

	appPort = CreateMsgPort();
	windowObject = NewObject(WINDOW_GetClass(), NULL,
							 WINDOW_Position, WPOS_CENTERSCREEN,
							 WINDOW_NewMenu, MenuArray,
							 WINDOW_IconifyGadget, TRUE,
							 WINDOW_IconTitle, "Mnemosyne",
							 WINDOW_Icon, GetDiskObject("PROGDIR:Mnemosyne"),
							 WINDOW_AppPort, appPort,
							 WA_Activate, TRUE,
							 WA_Title, "Mnemosyne 1.2.0",
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
			mainWindowObject = windowObject;
	if (!windowObject)
		cleanexit(NULL, NULL, NULL);
	if (!(intuiwin = (struct Window *)DoMethod(windowObject, WM_OPEN, NULL)))
		cleanexit(windowObject, appPort, appWin);
	appWin = AddAppWindow(1, 0, intuiwin, appPort, NULL);
	UpdateMenu(intuiwin, TRUE);
	processEvents(windowObject, intuiwin, listBrowser, appPort, backButton, doneFirst, CompareHook, NameHook, bottomText, fileRequester, scanButton, Path);
	DoMethod(windowObject, WM_CLOSE);
	clearList(contents);
	cleanexit(windowObject, appPort, appWin);
}
void processEvents(Object *windowObject,
				   struct Window *intuiwin,
				   struct Gadget *listBrowser,
				   struct MsgPort *appPort,
				   Object *backButton,
				   BOOL doneFirst,
				   struct Hook CompareHook,
				   struct Hook NameHook,
				   Object *bottomText,
				   Object *fileRequester,
				   Object *scanButton,
				   char *givenPath)
{
	ULONG windowsignal;
	ULONG receivedsignal;
	ULONG result;
	WORD code;
	BOOL end = FALSE;

	if (givenPath != NULL){
		fileEntered = TRUE;
		if (getLastCharSafely(givenPath) != ':')
		{
			__asm_strncat(givenPath, "/",2);
		}
		updatePathText(fileRequester, givenPath);
		scanningSequence(OID_GIVEN_PATH, intuiwin, windowObject, bottomText, scanButton, backButton, listBrowser, fileRequester, doneFirst, CompareHook, givenPath, appPort);
		doneFirst = TRUE;
	}

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
					case WMHI_MENUPICK:
					{
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
								updatePathText(fileRequester, "");
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
									"Mnemosyne is an open source disk \nutility application for AmigaOS 3.x,\n" \
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
							case OID_MENU_NO_ROUND:
							{
								toggleBusyPointer(windowObject, TRUE);

								NoRoundOption = !NoRoundOption;
								UpdateMenuToolTypes();
								updateIconTooltypes();
								UpdateMenu(intuiwin, TRUE);

								toggleBusyPointer(windowObject, FALSE);
								break;
							}
							case OID_MENU_ENABLE_GRAPH:
							{
								toggleBusyPointer(windowObject, TRUE);

								EnableGraphOption = !EnableGraphOption;
								UpdateMenuToolTypes();
								updateIconTooltypes();
								UpdateMenu(intuiwin, TRUE);

								struct EasyStruct requesterAbout = {
									sizeof(struct EasyStruct),
									0,
									"Notice",
									"Mnemosyne will now exit for the graph option to take effect.",
									"OK"
								};
								EasyRequest(intuiwin, &requesterAbout, NULL, NULL);
								end = TRUE;
								break;
							}
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
								updatePathText(fileRequester, "");
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

								if (getLastCharSafely(parentPath) != ':')
								{
									__asm_strncat(parentPath, "/",2);
								}

								scanningSequence(OID_BACK_BUTTON, intuiwin, windowObject, bottomText, scanButton, backButton, listBrowser, fileRequester, doneFirst, CompareHook, parentPath, appPort);
								doneFirst = TRUE;

								FreeVec(parentPath);
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

								TEXT *buffer = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
								ULONG pathPtr;

								GetAttr(GETFILE_FullFile, fileRequester, &pathPtr);

								snprintf(buffer, MAX_BUFFER, "%s", (char *)pathPtr);

								scanningSequence(OID_SCAN_BUTTON, intuiwin, windowObject, bottomText, scanButton, backButton, listBrowser, fileRequester, doneFirst, CompareHook, buffer, appPort);
								doneFirst = TRUE;
								FreeVec(buffer);

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
									ULONG stateIndex = 0;   // index into ColumnSorting[0..1]
									GetAttr(LISTBROWSER_RelColumn, listBrowser, &column);

									if (EnableGraphOption) {
										if (column == 0) {
											break; // image column, not sortable
										}
										stateIndex = column - 1; // map Name(1)->0, Approx(2)->1
									} else {
										stateIndex = column;     // Name(0)->0, Approx(1)->1
									}

									if (stateIndex > 1) {
										break; // safety
									}

									ColumnSorting[stateIndex].Sorting = !ColumnSorting[stateIndex].Sorting;

									if (stateIndex == 1) {
										DoGadgetMethod((struct Gadget *)listBrowser, intuiwin, NULL,
													LBM_SORT, NULL, column, ColumnSorting[stateIndex].Sorting, &CompareHook);
									} else {
										DoGadgetMethod((struct Gadget *)listBrowser, intuiwin, NULL,
													LBM_SORT, NULL, column, ColumnSorting[stateIndex].Sorting, &NameHook);
									}
									break;
								}
								STRPTR text = NULL;
								if (EnableGraphOption)
									GetListBrowserNodeAttrs(node, LBNA_Column, 1, LBNCA_Text, &text, TAG_DONE);
								else
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
										scanningSequence(OID_MAIN_LIST, intuiwin, windowObject, bottomText, scanButton, backButton, listBrowser, fileRequester, doneFirst, CompareHook, newPath, appPort);
										doneFirst = TRUE;
									}
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
		struct AppMessage *appMsg;
		while ((appMsg = (struct AppMessage *)GetMsg(appPort))) {
			if (scanning) {
				// Drop/ignore icon drop events while scanning
				ReplyMsg((struct Message *)appMsg);
				continue;
			}
			for (int i = 0; i < appMsg->am_NumArgs; i++) {
				struct WBArg *arg = &appMsg->am_ArgList[i];
				char *fullPath = AllocVec(sizeof(char) * MAX_BUFFER, MEMF_CLEAR);
				NameFromLock(arg->wa_Lock, fullPath, MAX_BUFFER);
				// printf("Received dropped file: %s\n", fullPath);
				if (i == 0) {
					updatePathText(fileRequester, fullPath);
					fileEntered = TRUE;
					// Instead of scanning immediately, just update the requester and UI
					setFileSequence(intuiwin, bottomText, windowObject, scanButton, listBrowser, backButton, doneFirst, fullPath);
					doneFirst = TRUE;
				}
				FreeVec(fullPath);
			}
			ReplyMsg((struct Message *)appMsg);
		}
	}
}
void cleanexit(Object *windowObject, struct MsgPort *appPort, struct AppWindow *appWin)
{
	if (windowObject)
		DisposeObject(windowObject);

	if (appWin)
    	RemoveAppWindow(appWin);

	if (appPort)
		DeleteMsgPort(appPort);
	clearScanning();
}
