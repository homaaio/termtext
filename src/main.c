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

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("tt");
        return 1;
    }
    char buf[99];
    while (buf, sizeof(buf), file) {
        printf("%s", buf);
    }
    // not read = not open
    fclose(file);

    return 0;
}