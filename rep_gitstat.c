// Шухардин Илья 8И42
// Именованные каналы (FIFO)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define F2R "/tmp/gitstat_f2r.fifo"

struct stat_data {
    char key[32];
    int count;
    int sum;
};

int cmp(const void *a, const void *b) {
    return strcmp(((struct stat_data*)a)->key, ((struct stat_data*)b)->key);
}

int main() {
    if (mkfifo(F2R, 0600) == -1 && errno != EEXIST) { perror("mkfifo"); return 1; }

    int fd = open(F2R, O_RDONLY);
    if (fd == -1) { perror("open"); return 1; }

    FILE *fin = fdopen(fd, "r");
    struct stat_data data[100];
    int n = 0;
    int dropped = 0;
    char line[256];

    while (fgets(line, sizeof(line), fin)) {
        if (strncmp(line, "DROPPED", 7) == 0) {
            sscanf(line, "DROPPED %d", &dropped);
            continue;
        }

        char key[32];
        int val;
        if (sscanf(line, "%31[^;];%d", key, &val) == 2) {
            int found = 0;
            for (int i = 0; i < n; i++) {
                if (strcmp(data[i].key, key) == 0) {
                    data[i].count++;
                    data[i].sum += val;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                strcpy(data[n].key, key);
                data[n].count = 1;
                data[n].sum = val;
                n++;
            }
        }
    }

    qsort(data, n, sizeof(struct stat_data), cmp);

    int total = 0;
    int global_sum = 0;
    for (int i = 0; i < n; i++) {
        printf("%s\t%d\t%d\n", data[i].key, data[i].count, data[i].sum);
        total += data[i].count;
	global_sum += data[i].sum;
    }
    printf("total=%d, dropped=%d; всего строк: %d; сумма по всем ключам: %d\n", total, dropped, total + dropped, global_sum);

    fclose(fin);

    unlink(F2R);
    return 0;
}
