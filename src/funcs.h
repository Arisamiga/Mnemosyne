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
char getLastCharSafely(const char*);
char* getLastTwoChars(const char*);
size_t strlcpy(char *dest, const char *source, size_t size);
char *string_to_lower(const char *, size_t);
size_t safeStrlen(const char *, size_t);
#endif
