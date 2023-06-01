#include <stdio.h>
#include <ctype.h>

#include <exec/types.h>

#include "scan.h"
#include "window.h"

// Declare functions after main
void initialChoice(int);
void cancel(void);
void credits(void);
int mainNoArgs(void);
void info(void);



int main(int argc, char **argv)
{
    if (argc <= 1)
    {
        mainNoArgs();
        return 0;
    }

    if (argc > 2)
    {
        // TODO: Not supported return information.
        return 0;
    }

    if (argv[1][0] == '?')
    {
        info();
        return 0;
    }
    if (argv[1][0] == '!')
    {
        createWindow();
        printf("Window created!\n");
        return 0;
    }
    // Printf a line
    printf("\n----------------------------------------\n");
    printf("Scanning: %s\n\n", argv[1]);
    printf("| Name: \t\t Size: \n\n");
    scanPath(argv[1], FALSE);

    return 0;
}

int mainNoArgs(void)
{
    printf("\nWelcome to Mnemosyne!\n\n To get started:\n  Type '1' to start the scan,\n  Type '2' to cancel,\n  Type '3' if you want to see credits.\n");
    int choices = 0;
    scanf("%d", &choices);
    if (choices != 1 && choices != 2 && choices != 3)
    {
        printf("Invalid choice, please try again.\n");
    }
    else
    {
        initialChoice(choices);
    }
    return 0;
}

void initialChoice(int choices)
{
    if (choices == 1)
    {
        scan();
    }
    else if (choices == 2)
    {
        cancel();
    }
    else if (choices == 3)
    {
        credits();
    }
}

void info(void)
{
    printf("Mnemosyne: Starts CLI\nMnemosyne (PATH): Scans Selected Path\n");
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