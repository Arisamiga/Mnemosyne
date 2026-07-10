#include <proto/intuition.h>
#ifndef SCAN_H
#define SCAN_H

extern char pastPath[256]; // This is the path that was last Scanned

STRPTR returnFormatWithTotal(void);

void scanPath(char *,
    BOOL,
    struct Gadget *,
    void (*progress_cb)(const char *path, void *userData),
    void *userData);
void clearScanning(void);

// Stop scanning functions
void openStopWindow(void);
BOOL checkStopMessage(void);
void closeStopWindow(void);
extern BOOL shouldStopScanning;

#endif
