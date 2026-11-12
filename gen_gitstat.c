#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define G2F "/tmp/gitstat_g2f.fifo"

int main() {
    if (mkfifo(G2F, 0600) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return 1;
    }

    int fd = open(G2F, O_WRONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    FILE *f = fopen("gitstat.csv", "r");
    if (!f) {
        perror("fopen");
        close(fd);
        return 1;
    }

    char line[256];
    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f)) {
        char id[32], key[32];
        int val, aux;
        if (sscanf(line, "%31[^;];%31[^;];%d;%d", id, key, &val, &aux) == 4) {
            char out[256];
            snprintf(out, sizeof(out), "%s;%d;%d\n", key, val, aux);
            write(fd, out, strlen(out));
        }
    }

    fclose(f);
    close(fd);
    return 0;
}
