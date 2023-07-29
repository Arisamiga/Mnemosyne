#include <stdio.h>
#include <ctype.h>

#include <exec/types.h>

#include "scan.h"
#include "window.h"

// Mnemosyne Version
char *vers = "\0$VER: Mnemosyne 0.9";

// Declare functions after main
void info(void);

int main(int argc, char **argv)
{
    if (argv[1][0] == '?')
    {
        info();
        return 0;
    }
    if (argc <= 1)
    {
        createWindow();
        return 0;
    }
    // Printf a line
    printf("\n----------------------------------------\n");
    printf("Scanning: %s\n\n", argv[1]);
    printf("| Name: \t\t Size: \n\n");
    scanPath(argv[1], FALSE, 0);
    return 0;
}

void info(void)
{
    printf("Mnemosyne: Starts Application\nMnemosyne (PATH): Scans Selected Path\n");
}