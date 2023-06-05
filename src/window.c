#include <clib/macros.h>
#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/button.h>
#include <proto/layout.h>
#include <proto/window.h>

#include <libraries/gadtools.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <classes/window.h>

#include "window.h"


// TODO: Make a standard Intuition Window
// TODO: Find out why Reference to _LabelBase and _ListBrowserBase which are used for the Label and ListBrowser classes are undefined.
struct Library *ButtonBase = NULL,
               *LayoutBase = NULL,
               *WindowBase = NULL;

void createWindow( void )
{
	struct Window *window;
	Object *Win_Object;
	ULONG signal, result;
	ULONG done = FALSE;

	/* Open the classes we will use. Note, reaction.lib SAS/C or DICE autoinit
	 * can do this for you automatically.
	 */
	if( WindowBase = (struct Library*) OpenLibrary("window.class",0L) )
	{
		if( LayoutBase = (struct Library*) OpenLibrary("gadgets/layout.gadget",0L) )
		{
			if( ButtonBase = (struct Library*) OpenLibrary("gadgets/button.gadget",0L) )
			{
				/* Create the window object.
				 */
				Win_Object = WindowObject,
					WA_ScreenTitle, "ReAction",
					WA_Title, "Window",
					WA_SizeGadget, TRUE,
					WA_Left, 40,
					WA_Top, 30,
					WA_DepthGadget, TRUE,
					WA_DragBar, TRUE,
					WA_CloseGadget, TRUE,
					WA_Activate, TRUE,
					WINDOW_ParentGroup, HGroupObject,
						TAligned, 
						LAYOUT_SpaceOuter, TRUE,
						StartVGroup,
							StartMember, ButtonObject,
								GA_Text, "Button",
							EndMember,
							StartMember, ButtonObject,
								GA_Text, "Button",
							EndMember,
							StartMember, ButtonObject,
								GA_Text, "Button",
							EndMember,
							StartMember, ButtonObject,
								GA_Text, "Button",
							EndMember,
							StartMember, ButtonObject,
								GA_Text, "Button",
							EndMember,
							StartMember, ButtonObject,
								GA_Text, "Button",
							EndMember,
							StartMember, ButtonObject,
								GA_Text, "Button",
							EndMember,
							StartMember, ButtonObject,
								GA_Text, "Button",
							EndMember,
							StartMember, ButtonObject,
								GA_Text, "Button",
							EndMember,
							StartMember, ButtonObject,
								GA_Text, "Button",
							EndMember,
						End,
					EndMember,
				EndWindow;

				/*  Object creation sucessful?
				 */
				if( Win_Object )
				{
					/*  Open the window.
					 */
					if( window = (struct Window *) RA_OpenWindow(Win_Object) )
					{
						ULONG wait;
						
						/* Obtain the window wait signal mask.
						 */
						GetAttr( WINDOW_SigMask, Win_Object, &signal );

						/* Input Event Loop
						 */
						while( !done )
						{
							wait = Wait(signal|SIGBREAKF_CTRL_C);
							
							if (wait & SIGBREAKF_CTRL_C) done = TRUE;
							else

							while ((result = RA_HandleInput(Win_Object,NULL)) != WMHI_LASTMSG)
							{
								switch(result)
								{
									case WMHI_CLOSEWINDOW:
										done = TRUE;
										break;
								}
							}
						}
					}

					/* Disposing of the window object will
					 * also close the window if it is
					 * already opened and it will dispose of
					 * all objects attached to it.
					 */
					DisposeObject( Win_Object );
				}
			}
		}
	}

	/* Close the classes.
	 */
	if (LayoutBase) CloseLibrary( (struct Library *)LayoutBase );
	if (ButtonBase) CloseLibrary( (struct Library *)ButtonBase );
	if (WindowBase) CloseLibrary( (struct Library *)WindowBase );
}
