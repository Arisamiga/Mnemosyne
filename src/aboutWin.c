#include eeee
#include <stdio.h>
#include <stdlib.h>
#include <intuition/classusr.h>
#include <gadgets/layout.h>
#include <classes/window.h>

#define __CLIB_PRAGMA_LIBCALL

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/layout.h>
#include <proto/window.h>
#include <proto/intuition.h>
#include <proto/texteditor.h>

void cleanAboutexit(Object *windowObject);
void processAboutEvents(Object *windowObject);
void aboutWin(void)
{
    struct Window *Aintuiwin = NULL;
    Object *AwindowObject = NULL;
    Object *AmainLayout = NULL;

    STRPTR aboutText = "\n" \
    "Copyright (c) 2023 Aris (Arisamiga) Sokianos\n\n"
    "Mnemosyne is an open source disk utility application for AmigaOS 3.2,\n" \
    "which can be used to see how much disk space your files and folders are taking up.\n\n" \
    "\"Mnemosyne\", in Greek mythology is the goddess of memory.\n\n" \
    "Thank you so much for using Mnemosyne :D\n\n" \
    "Report bugs or request features at:\nhttps://github.com/Arisamiga/Mnemosyne\n\n" \
    "Distributed without warranty under the terms of the GNU General Public License.";

    AmainLayout = NewObject(LAYOUT_GetClass(), NULL,
                            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                            LAYOUT_DeferLayout, TRUE,
                            LAYOUT_SpaceInner, TRUE,
                            LAYOUT_SpaceOuter, TRUE,
                            LAYOUT_AddChild, NewObject(TEXTEDITOR_GetClass(), NULL,
                                                        GA_ID, 0,
                                                        GA_RelVerify, TRUE,
                                                        GA_TEXTEDITOR_Contents, aboutText,
                                                        GA_TEXTEDITOR_Flow, GV_TEXTEDITOR_Flow_Left,
                                                        GA_TEXTEDITOR_ReadOnly, TRUE,
                                                        TAG_DONE),
                                CHILD_MinWidth,   320,
                                CHILD_MinHeight,  200,
                            TAG_DONE);
    AwindowObject = NewObject(WINDOW_GetClass(), NULL,
                              WINDOW_Position, WPOS_CENTERSCREEN,
                              WA_Activate, TRUE,
                              WA_Title, "About",
                              WA_DragBar, TRUE,
                              WA_CloseGadget, TRUE,
                              WA_DepthGadget, TRUE,
                              WA_SizeGadget, TRUE,
                              WA_InnerWidth, 320,
                              WA_InnerHeight, 200,
                              WINDOW_Layout, AmainLayout,
                              TAG_DONE);
    if (!(Aintuiwin = (struct Window *)DoMethod(AwindowObject, WM_OPEN, NULL)))
        cleanAboutexit(AwindowObject);
    processAboutEvents(AwindowObject);
    DoMethod(AwindowObject, WM_CLOSE);
    cleanAboutexit(AwindowObject);
}

void processAboutEvents(Object *windowObject)
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

void cleanAboutexit(Object *AwindowObject)
{
    if (AwindowObject)
        DisposeObject(AwindowObject);
}
