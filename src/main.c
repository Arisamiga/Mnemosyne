#include <stdio.h>
#include <proto/exec.h>

#include "scan.h"
#include "window.h"

// Mnemosyne Version
char *vers = "\0$VER: Mnemosyne 1.0.2";

struct IntuitionBase *IntuitionBase;
struct Library *UtilityBase;
struct Library *WindowBase;
struct Library *LayoutBase;
struct Library *ListBrowserBase;
struct Library *ButtonBase;
struct Library *GetFileBase;
struct Library *GadToolsBase;
struct Library *WorkbenchBase;

// Declare functions after main
void info(void);

BOOL openLibraries(void)
{
	if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 1)))
	{
		printf("Failed to open intuition.library! Make sure the version is above v47.\n");
		return FALSE;
	}
	if (!(UtilityBase = OpenLibrary("utility.library", 1)))
	{
		printf("Failed to open utility.library! Make sure the version is above v47.\n");
		return FALSE;
	}
	if (!(WindowBase = OpenLibrary("window.class", 1)))
	{
		printf("Failed to open window.class! Make sure the version is above v47.\n");
		return FALSE;
	}
	if (!(LayoutBase = OpenLibrary("gadgets/layout.gadget", 1)))
	{
		printf("Failed to open gadgets/layout.gadget! Make sure the version is above v47.\n");
		return FALSE;
	}
	if (!(ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 1)))
	{
		printf("Failed to open gadgets/listbrowser.gadget! Make sure the version is above v47.\n");
		return FALSE;
	}
	if (!(ButtonBase = OpenLibrary("gadgets/button.gadget", 1)))
	{
		printf("Failed to open gadgets/button.gadget! Make sure the version is above v47.\n");
		return FALSE;
	}
	if (!(GetFileBase = OpenLibrary("gadgets/getfile.gadget", 1)))
	{
		printf("Failed to open gadgets/getfile.gadget! Make sure the version is above v47.\n");
		return FALSE;
	}
	if ((GadToolsBase = OpenLibrary("gadtools.library", 1)) == NULL) {
		printf( "Could not open gadtools.library\n");
		return FALSE;
	}
	if ((WorkbenchBase = OpenLibrary("workbench.library", 1)) == NULL) {
		printf( "Could not open workbench.library\n");
		return FALSE;
	}

	return TRUE;
}

void closeLibraries(void)
{
	if (IntuitionBase)
		CloseLibrary((struct Library *)IntuitionBase);
	if (UtilityBase)
		CloseLibrary(UtilityBase);
	if (WindowBase)
		CloseLibrary(WindowBase);
	if (LayoutBase)
		CloseLibrary(LayoutBase);
	if (ListBrowserBase)
		CloseLibrary(ListBrowserBase);
	if (ButtonBase)
		CloseLibrary(ButtonBase);
	if (GetFileBase)
		CloseLibrary(GetFileBase);
	if (GadToolsBase)
		CloseLibrary(GadToolsBase);
	if (WorkbenchBase)
		CloseLibrary(WorkbenchBase);
}


int main(int argc, char **argv)
{
	if (argc <= 1)
	{
		if (openLibraries())
		{
			createWindow();
		}
		closeLibraries();
		return 0;
	}
	if (argv[1][0] == '?')
	{
		info();
		return 0;
	}
	// Printf a line
	printf("\n----------------------------------------\n");
	printf("Scanning: %s\n\n", argv[1]);
	printf("| Name: \t\t Size: \n\n");
	scanPath(argv[1], FALSE, FALSE);
	return 0;
}

void info(void)
{
	printf("Mnemosyne: Starts Application\nMnemosyne (PATH): Scans Selected Path\n");
}
