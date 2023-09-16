#ifndef FUNCS_H
#define FUNCS_H
void getParentPath(char *, char *, int);
void getNameFromPath(char *, char *, unsigned int);
STRPTR ULongToString(ULONG);
STRPTR floatToString(float);
BOOL clearList(struct List);
ULONG stringToULONG(char *);
float presentageFromULongs(ULONG, ULONG, STRPTR, STRPTR);
float stringToFloat(STRPTR);
#endif
