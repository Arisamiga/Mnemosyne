#include <proto/intuition.h>
#ifndef SCAN_H
#define SCAN_H

extern char pastPath[256]; // This is the path that was last Scanned

STRPTR returnFormatWithTotal(void);
void scanPath(char *, BOOL, Object *);
void clearScanning(void);

#endif
