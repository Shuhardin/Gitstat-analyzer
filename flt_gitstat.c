#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define G2F "/tmp/gitstat_g2f.fifo"
#define F2R "/tmp/gitstat_f2r.fifo"

int main() {
    if (mkfifo(G2F, 0600) == -1 && errno != EEXIST) { perror("mkfifo g2f"); return 1; }
    if (mkfifo(F2R, 0600) == -1 && errno != EEXIST) { perror("mkfifo f2r"); return 1; }

    int fd_in = open(G2F, O_RDONLY);
    if (fd_in == -1) { perror("open g2f"); return 1; }

    int fd_out = open(F2R, O_WRONLY);
    if (fd_out == -1) { perror("open f2r"); return 1; }

    FILE *fin = fdopen(fd_in, "r");
    char line[256];
    int dropped = 0;

    while (fgets(line, sizeof(line), fin)) {
        char key[32];
        int val, aux;
        if (sscanf(line, "%31[^;];%d;%d", key, &val, &aux) == 3) {
            if (aux < 3) {
                dropped++;
            } else {
                char out[256];
                snprintf(out, sizeof(out), "%s;%d\n", key, val);
                write(fd_out, out, strlen(out));
            }
        }
    }

    char drop_msg[64];
    snprintf(drop_msg, sizeof(drop_msg), "DROPPED %d\n", dropped);
    write(fd_out, drop_msg, strlen(drop_msg));

    fclose(fin);
    close(fd_out);

    unlink(G2F);
    return 0;
}
