#ifndef FUNCS_H
#define FUNCS_H
long get_file_size(char *);
int GetListLength(struct List *);
void getParentPath(char *, char *, int);
void getNameFromPath(char *, char *, unsigned int);
STRPTR longToString(long);
#endif