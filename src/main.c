#include <stdio.h>
#include <ctype.h>

#include <exec/types.h>

#include "scan.h"
#include "window.h"

// Declare functions after main
void cancel(void);
void credits(void);
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

void cancel(void)
{
    // TODO: Enable ability to CTRL+C to cancel ongoing scan.
    printf("Cancelling... Thanks so much for using Mnemosyne!\n");
}

void credits(void)
{
    printf("Credits: (SoonTM)\n");
}