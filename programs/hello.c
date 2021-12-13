
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[]) {

    char *str = (char *) malloc(15);
    strcpy(str, "Hello CVA6!\n");
    printf("%s\n", str);
    free(str);

    return 0;
}
