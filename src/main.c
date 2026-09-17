#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define MAX_LINES 1000

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: tt <file>\n");
        return 1;
    } else if (argc >= 3) {
        fprintf(stderr, "can't open 2 and more files\n");
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("tt");
        return 1;
    }
    char *lines[MAX_LINES];
    char buf[1024];
    int count = 0;
    int offset = 0;
    int screen_height = 20;

    while (fgets(buf, sizeof(buf), file)) {
        if (count >= MAX_LINES) break;
        buf[strcspn(buf, "\n")] = 0;
        lines[count] = strdup(buf);
        if (lines[count] == NULL) {
            perror("strdup");
            break;
        }
        count++;
    }
    fclose(file);

    struct termios orig;
    tcgetattr(STDIN_FILENO, &orig);
    struct termios raw = orig;
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    while (offset < count) {
        printf("\033[2J\033[H");
        for (int i = offset; i < offset + screen_height && i < count; i++) {
            printf("%d %s\r\n", i, lines[i]);
        }

        int c = getchar();
        if (c == 'q') break;

        offset += screen_height;
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);

    for (int i = 0; i < count; i++) {
        free(lines[i]);
    }

    return 0;
}