#include <stdio.h>
#include <ctype.h>
#include "scan.h"

// Declare functions after main
void initialChoice(int);
int cancel(void);
int credits(void);
int mainNoArgs(void);

int main(int argc, char **argv) {
    printf("DBG: %d\n", argc);
    if(argc <= 1){
        mainNoArgs();
        return 0;
    }

    if(argc > 2) {
        //TODO: Not supported return information.
        return 0;
    }
    scanPath(argv[1]);

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
    else {
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
    else
    {
        printf("Invalid choice, please try again.\n");
    }
}

int cancel(void)
{
    printf("Cancelling... Thanks so much for using Mnemosyne!\n");
    return 0;
}

int credits(void)
{
    printf("Credits: (SoonTM)\n");
    return 0;
}