#include <stdio.h>
#include <string.h>
#define MAX_LINES 1000

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
    char *lines[1000];
    char buf[1024];
    int count = 0;
    while (fgets(buf, sizeof(buf), file)) {
        lines[count] = strdup(buf);
        count++;
        // count = count + 1;
        // printf("%s", buf);
    }
    // not read = not open
    for (int i = 0; i<count; i++) {
        printf("%d %s", i ,lines[i]);
    }
    fclose(file);

    return 0;
}