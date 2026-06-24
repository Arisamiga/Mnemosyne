#ifndef FUNCS_H
#define FUNCS_H

#include <exec/ports.h>
#include <exec/types.h>
#include <graphics/rastport.h>
#include <intuition/gadgetclass.h>
#include <proto/intuition.h>
// Forward declarations
struct Image;
struct BitMap;
struct RastPort;
struct List;
struct Hook;
struct LBSortMsg;
struct Object;
struct Gadget;
struct MsgPort;

// Helper structure for treemap visualization
struct NodeData {
    float percent;
    ULONG colorPen; // 0-15 for 4-bit bitmap
};

void getParentPath(char *, char *, int);
void getNameFromPath(char *, char *, unsigned int);
STRPTR ULongToString(ULONG);
STRPTR floatToString(float);
BOOL clearList(struct List);
ULONG stringToULONG(char *);
float presentageFromULongs(ULONG, ULONG, STRPTR, STRPTR);
float stringToFloat(STRPTR);
char getLastCharSafely(const char *);
char *getLastTwoChars(const char *);
size_t strlcpy(char *dest, const char *source, size_t size);
char *string_to_lower(const char *, size_t);
size_t safeStrlen(const char *str);
void initializeIconTooltypes(void);
void updateIconTooltypes(void);

// Utility functions for window rendering and UI
ULONG getBitmapColorPen(struct BitMap *);
ULONG collectAllNodePercentages(struct Gadget *, struct NodeData *, ULONG);
void drawTreemapRectangles(
    struct RastPort *, struct NodeData *, ULONG, ULONG, ULONG);
void flushAppPort(struct MsgPort *);
float asValue(STRPTR);
float __SAVE_DS__ __ASM__ myCompare(__REG__(a0, struct Hook *),
    __REG__(a2, Object *),
    __REG__(a1, struct LBSortMsg *));
int __SAVE_DS__ __ASM__ myCompare2(__REG__(a0, struct Hook *),
    __REG__(a2, Object *),
    __REG__(a1, struct LBSortMsg *));
void checkBackButton(char *, BOOL, Object *);
void toggleButtons(
    Object *, Object *, struct Gadget *, Object *, char *, BOOL, BOOL, BOOL);
void toggleBusyPointer(Object *, BOOL);
void updateBottomTextW2Text(Object *, Object *, char *, STRPTR, BOOL);
void updateBottomTextW2AndTotal(
    Object *, Object *, char *, STRPTR, STRPTR, BOOL);
void updateBottomText(Object *, Object *, STRPTR);
void updatePathText(Object *, STRPTR);

// ------------ Variables -----------

extern BOOL NoRoundOption;
extern BOOL EnableGraphOption;

#endif
