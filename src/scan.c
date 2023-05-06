#include <stdio.h>
#include "scan.h"
#include "funcs.h"

int scan(void)
{
    printf("Please enter the name of the file you want to scan: ");
    char name[100];
    scanf("%s", name);
    FILE *file;
    file = fopen(name, "r");
    if (file == NULL)
    {
        printf("File does not exist.\n");
        return 0;
    }
    else
    {
        printf("File exists.\n");

        printf("Scanning...\n");
        char *filename = name;
        printf(
            "Size of file `%s` is %ld bytes.\n",
            filename,
            get_file_size(filename));
    }

    return 0;
}
