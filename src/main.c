#include <stdio.h>
#include "scan.h"

// Declare functions after main
int initialChoice(int);
int scan(void);
int cancel(void);
int credits(void);

int main(void)
{
    printf("\nWelcome to Mnemosyne!\n\n To get started:\n  Type '1' to start the scan,\n  Type '2' to cancel,\n  Type '3' if you want to see credits.\n");
    int choices = 0;
    scanf("%d", &choices);
    initialChoice(choices);
    return 0;
}

int initialChoice(int choices)
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
        printf("Invalid input, please try again.\n");
        scanf("%d", &choices);
        initialChoice(choices);
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