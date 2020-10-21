#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

enum CONV_TYPE {
    PIPE_CONV,
    CONJ_CONV
};

void handler(int signo) {
    kill(0, SIGINT);
    printf("SIGINT received");
    exit(1);
}

int strcmp(char *str1, char *str2) {  //  0 - eq, 1 - s1 < s2, 2 - s1 > s2
    int i = -1;
    do {
        i++;
        if (str1[i] < str2[i])
            return 1;
        if (str1[i] > str2[i])
            return 2;
    } while (str1[i] && str2[i]);
    return 0;
}

pid_t *add_pid(pid_t *pids, pid_t pid, int *bg_size) {
    pids = realloc(pids, (*bg_size + 1) * sizeof(pid_t));
    pids[*bg_size] = pid;
    (*bg_size)++;
    return pids;
}

char *get_word(char *last_ch) {
    char ch, *word = NULL;
    ch = getchar();
    int len = 0;
    word = malloc(sizeof(char));
    while (ch != ' ' && ch != '\n') {
        word = realloc(word, (len + 1) * sizeof(char));
        word[len] = ch;
        len++;
        ch = getchar();
    }
    *last_ch = ch;
    word = realloc(word, (len + 1) * sizeof(char));
    word[len] = '\0';
    return word;
}

char **get_list() {
    char **list = NULL, last_ch;
    int count = 1;
    list = malloc(sizeof(char*));
    if (!list)
        return NULL;
    list[0] = get_word(&last_ch);
    while (last_ch != '\n') {
        list = realloc(list, (count + 1) * sizeof(char*));
        list[count] = get_word(&last_ch);
        count++;
    }
    list = realloc(list, (count + 1) * sizeof(char*));
    list[count] = NULL;
    return list;
}

void free_list(char **list) {
    int i;
    if (list == NULL)
        return;
    for (i = 0; list[i] != NULL; i++) {
        free(list[i]);
    }
    free(list[i]);
    free(list);
    return;
}

char ***cmd_cutter(char **cmd, int *number, char *sep_word) {
    int cnt = 1, w_cnt = 0, newsize = 0;
    char ***cmd_arr = malloc(sizeof(char**));
    cmd_arr[0] = NULL;
    if (!cmd_arr) {
        perror("Malloc err");
        return NULL;
    }
    for (int i = 0; cmd[i] != NULL; i++) {
        if (strcmp(cmd[i], sep_word) != 0) {
            newsize = (w_cnt + 1) * sizeof(char*);
            cmd_arr[cnt - 1] = realloc(cmd_arr[cnt - 1], newsize);
            cmd_arr[cnt - 1][w_cnt] = cmd[i];
            w_cnt++;
        } else {
            newsize = (w_cnt + 1) * sizeof(char*);
            cmd_arr[cnt - 1] = realloc(cmd_arr[cnt - 1], newsize);
            cmd_arr[cnt - 1][w_cnt] = NULL;
            cmd_arr = realloc(cmd_arr, (cnt + 1) * sizeof(char**));
            cmd_arr[cnt] = NULL;
            cnt++;
            w_cnt = 0;
        }
    }
    newsize = (w_cnt + 1) * sizeof(char*);
    cmd_arr[cnt - 1] = realloc(cmd_arr[cnt - 1], newsize);
    cmd_arr[cnt - 1][w_cnt] = NULL;
    *number = cnt;
    return cmd_arr;
}

int word_search(char **cmd, char* word) {
    int pos = 0;
    for (pos = 0; cmd[pos] != NULL; pos++) {
        if (!strcmp(word, cmd[pos])) {
            return pos;
        }
    }
    return -1;
}

void io_detector(char **cmd) {
    int in_pos = -1, out_pos = -1, in_fd = 0, out_fd = 1;
    in_pos = word_search(cmd, "<");
    out_pos = word_search(cmd, ">");
    if (out_pos != -1) {
        out_fd = open(cmd[out_pos + 1], O_WRONLY | O_CREAT | O_TRUNC,
                                S_IRUSR | S_IWUSR);
        cmd[out_pos] = NULL;
        dup2(out_fd, 1);
        close(out_fd);
    }
    if (in_pos != -1) {
        in_fd = open(cmd[in_pos + 1], O_RDONLY,
                                S_IRUSR | S_IWUSR);
        cmd[in_pos] = NULL;
        dup2(in_fd, 0);
        close(in_fd);
    }
    return;
}

int execute(char **cmd) {
    io_detector(cmd);
    if (execvp(cmd[0], cmd) < 0) {
        perror("exec failed");
        return -1;
    }
    return 0;
}

int change_dir(char **cmd) {
    char *home = getenv("HOME");
    if (strcmp(cmd[0], "cd") == 0) {
        if (cmd[1] == NULL || (strcmp(cmd[1], "~") == 0)) {
            chdir(home);
        } else {
            chdir(cmd[1]);
        }
        return 1;
    }
    return 0;
}

int pipe_conv(char ***cmd_arr, int cmd_num, pid_t **pids_in_bg, int *bg_size) {
    int next_fd[2] = {0, 1}, prev_fd[2] = {0, 1};
    int pos = -1, bg_flag = 0;
    pid_t pid;
    for (int i = 0; i < cmd_num; i++) {
        if (change_dir(cmd_arr[i]) == 1) {
            continue;
        }
        if (i != (cmd_num - 1)) {
            pipe(next_fd);
        }
        pid = fork();
        bg_flag = 0;
        pos = word_search(cmd_arr[i], "&");
        if (pos != -1) {
            *(pids_in_bg) = add_pid(*(pids_in_bg), pid, bg_size);
            bg_flag = 1;
            cmd_arr[i][pos] = NULL;
        }
        if (pid == 0) {
            if (i != 0) {
                dup2(prev_fd[0], 0);
                close(prev_fd[0]);
                close(prev_fd[1]);
            }
            if (i != (cmd_num - 1)) {
                dup2(next_fd[1], 1);
                close(next_fd[0]);
                close(next_fd[1]);
            }
            if (execute(cmd_arr[i]) == -1)
                return -1;
            return 1;
        } else if (pid > 0) {
            if (i != 0) {
                close(prev_fd[0]);
                close(prev_fd[1]);
            }
            if (i != (cmd_num - 1)) {
                prev_fd[0] = next_fd[0];
                prev_fd[1] = next_fd[1];
            }
            if (bg_flag == 0)
                wait(NULL);
        }
    }
    if (cmd_num > 1) {
        close(prev_fd[0]);
        close(prev_fd[1]);
    }
    return 0;
}

int conj_conv(char ***cmd_arr, int cmd_num, pid_t **pids_in_bg, int *bg_size) {
    int pos = -1, bg_flag;
    int wstatus;
    pid_t pid;
    for (int i = 0; i < cmd_num; i++) {
        if (change_dir(cmd_arr[i]) == 1) {
                continue;
        }
        pid = fork();
        bg_flag = 0;
        pos = word_search(cmd_arr[i], "&");
        if (pos != -1) {
            *(pids_in_bg) = add_pid(*(pids_in_bg), pid, bg_size);
            bg_flag = 1;
            cmd_arr[i][pos] = NULL;
        }
        if (pid == 0) {
            if (execute(cmd_arr[i]) == -1)
                return -1;
            return 1;
        }
        if (bg_flag == 0) {
            wait(&wstatus);
        }
        if (WEXITSTATUS(wstatus) != 0)
            break;
    }
    return 0;
}

int main_cmd_exec(char **cmd, pid_t **pids_in_bg, int *bg_size) {
    int cmd_num = 0, conv_type = CONJ_CONV;
    int res = 0;
    char ***cmd_arr = cmd_cutter(cmd, &cmd_num, "&&");
    if (cmd_num == 1) {
        for (int i = 0; i < cmd_num; i++) {
            free(cmd_arr[i]);
        }
        free(cmd_arr);
        cmd_arr = cmd_cutter(cmd, &cmd_num, "|");
        conv_type = PIPE_CONV;
    }
    if (conv_type == PIPE_CONV) {
        res = pipe_conv(cmd_arr, cmd_num, pids_in_bg, bg_size);
    } else {
        res = conj_conv(cmd_arr, cmd_num, pids_in_bg, bg_size);
    }
    for (int i = 0; i < cmd_num; i++) {
        free(cmd_arr[i]);
    }
    free(cmd_arr);
    return res;
}

int is_exit(char *cmd) {
    return (strcmp(cmd, "quit") && strcmp(cmd, "exit"));
}

int inf_loop() {
    char **list = NULL;
    int n = 0, bg_size = 0;
    pid_t *pids_in_bg = NULL;
    list = get_list();
    while (is_exit(list[0])) {
        if (strcmp(list[0], "\0") != 0) {
            n = main_cmd_exec(list, &(pids_in_bg), &bg_size);
        }
        free_list(list);
        if (n == -1) {
            return 1;
        }
        list = get_list();
        if (n == 1) {
            return 0;
        }
    }
    free_list(list);
    for (int i = 0; i < bg_size; i++) {
        waitpid(pids_in_bg[i], NULL, 0);
    }
    return 0;
}

int main() {
    signal(SIGINT, handler);
    int result = inf_loop();
    return result;
}
