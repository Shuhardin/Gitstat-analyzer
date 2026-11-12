#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_KEYS 32
#define MAX_LINE 256

struct child_ctx {
    char key[32];
    int p_in[2];
    int p_out[2];
    pid_t pid;
};

int main() {
    struct child_ctx ctx[MAX_KEYS];
    int num_keys = 0;
    char line[MAX_LINE];
    FILE *f = fopen("gitstat.csv", "r");
    if (!f) { perror("fopen"); return 1; }

    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f)) {
        char id[32], key[32];
        int val, aux;
        if (sscanf(line, "%31[^;];%31[^;];%d;%d", id, key, &val, &aux) == 4) {
            int found = 0;
            for (int i = 0; i < num_keys; i++) {
                if (strcmp(ctx[i].key, key) == 0) { found = 1; break; }
            }
            if (!found) {
                strcpy(ctx[num_keys].key, key);
                if (pipe(ctx[num_keys].p_in) == -1 || pipe(ctx[num_keys].p_out) == -1) {
                    perror("pipe"); return 1;
                }
                num_keys++;
            }
        }
    }

    for (int i = 0; i < num_keys; i++) {
        ctx[i].pid = fork();
        if (ctx[i].pid == -1) { perror("fork"); return 1; }

        if (ctx[i].pid == 0) {
            close(ctx[i].p_in[1]);
            close(ctx[i].p_out[0]);

            for (int j = 0; j < i; j++) {
                close(ctx[j].p_in[0]); close(ctx[j].p_in[1]);
                close(ctx[j].p_out[0]); close(ctx[j].p_out[1]);
            }

            FILE *in = fdopen(ctx[i].p_in[0], "r");
            char cline[MAX_LINE];
            int count = 0;
            int sum = 0;
            int max_val = -1;

            while (fgets(cline, sizeof(cline), in)) {
                char id[32], ckey[32];
                int val, aux;
                if (sscanf(cline, "%31[^;];%31[^;];%d;%d", id, ckey, &val, &aux) == 4) {
                    if (aux >= 3) {
                        count++;
                        sum += val;
                        if (val > max_val) max_val = val;
                    }
                }
            }
            fclose(in);

	    srand(getpid());
	    sleep(rand() % 4);

            char out[MAX_LINE];
            int n = snprintf(out, sizeof(out), "%s;%d;%d;%d\n", ctx[i].key, count, sum, max_val);
            write(ctx[i].p_out[1], out, n);
            close(ctx[i].p_out[1]);

            _exit(count);
        }

        close(ctx[i].p_in[0]);
        close(ctx[i].p_out[1]);
    }

    fseek(f, 0, SEEK_SET);
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        char id[32], key[32];
        int val, aux;
        if (sscanf(line, "%31[^;];%31[^;];%d;%d", id, key, &val, &aux) == 4) {
            for (int i = 0; i < num_keys; i++) {
                if (strcmp(ctx[i].key, key) == 0) {
                    write(ctx[i].p_in[1], line, strlen(line));
                    break;
                }
            }
        }
    }
    fclose(f);

    for (int i = 0; i < num_keys; i++) {
        close(ctx[i].p_in[1]);
    }

    int leader_idx = 0;
    int max_sum = -1;

    for (int i = 0; i < num_keys; i++) {
        char res[MAX_LINE];
        int count_from_string = 0;
        int n = read(ctx[i].p_out[0], res, sizeof(res) - 1);

        if (n > 0) {
            res[n] = '\0';
            char rkey[32];
            int sum, max_val;

            if (sscanf(res, "%31[^;];%d;%d;%d", rkey, &count_from_string, &sum, &max_val) == 4) {
                printf("%s\t%d\t%d\t%d\n", rkey, count_from_string, sum, max_val);
                if (sum > max_sum) {
                    max_sum = sum;
                    leader_idx = i;
                }
            }
        } else {
            fprintf(stderr, "Ошибка чтения из канала для ключа %s\n", ctx[i].key);
            return 1;
        }
        close(ctx[i].p_out[0]);

        int status;
        if (waitpid(ctx[i].pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (count_from_string != exit_code) {
                fprintf(stderr, "MISMATCH: ключ %s, в канале count=%d, код выхода=%d\n",
                        ctx[i].key, count_from_string, exit_code);
                return 1;
            }
        } else {
            fprintf(stderr, "Потомок для ключа %s завершился аварийно\n", ctx[i].key);
            return 1;
        }
    }

    printf("leader: %s (sum=%d)\n", ctx[leader_idx].key, max_sum);
    return 0;
}
