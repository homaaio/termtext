#include <stdio.h>

// "tt <file_name>" command:

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage `tt <file>` \n");
        return 1;
    } else if (argc >= 3) {
        fprintf(stderr, "can't open 2 and more files \n");
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        perror("tt");
        return 1;
    }

    // not read = not open
    fclose(f);

    return 0;
}