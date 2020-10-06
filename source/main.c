#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
char ***cmd_cutter(char **cmd, int *number) {
    int cnt = 1, w_cnt = 0, newsize = 0;
    char ***cmd_arr = malloc(sizeof(char**));
    cmd_arr[0] = NULL;
    if (!cmd_arr) {
        perror("Malloc err");
        return NULL;
    }
    for (int i = 0; cmd[i] != NULL; i++) {
        if (cmd[i][0] != '|') {
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
int iodetector(char **cmd, int *ioflag, int *ind) {
//  ioflag = 0 <=> in from file, = 1 <=> out in file, = 2 <=> no redirect
    int fd = 0, flag = 2, i = 0;
    for (i = 0; cmd[i] != NULL; i++) {
        if (cmd[i][0] == '>') {
            if (cmd[i + 1] != NULL) {
                printf("%s ", cmd[i + 1]);
                fd = open(cmd[i + 1], O_WRONLY | O_CREAT | O_TRUNC,
                                  S_IRUSR | S_IWUSR);
                flag = 1;
                break;
            }
        }
        if (cmd[i][0] == '<') {
            if (cmd[i + 1] != NULL) {
                fd = open(cmd[i + 1], O_RDONLY | O_CREAT | O_TRUNC,
                                  S_IRUSR | S_IWUSR);
                flag = 0;
                break;
            }
        }
    }
    *ind = i;
    *ioflag = flag;
    return fd;
}
int execute(char **cmd) {
    int fd = 0, ioflag = 2, ind = 0;
    fd = iodetector(cmd, &ioflag, &ind);
    if (ioflag != 2) {
        cmd[ind] = NULL;
        dup2(fd, ioflag);
        close(fd);
    }
    if (execvp(cmd[0], cmd) < 0) {
        perror("exec failed\n");
        return 1;
    }
    close(fd);
    return 0;
}
int main_cmd_exec(char **cmd) {
    int cmd_num = 0, next_fd[2] = {0, 1}, prev_fd[2] = {0, 1};
    char ***cmd_arr = cmd_cutter(cmd, &cmd_num);
    pid_t pid;
    for (int i = 0; i < cmd_num; i++) {
        if (i != (cmd_num - 1)) {
            pipe(next_fd);
        }
        if ((pid = fork()) == 0) {
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
            execute(cmd_arr[i]);
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
            wait(NULL);
        }
    }
    if (cmd_num > 1) {
        close(prev_fd[0]);
        close(prev_fd[1]);
    }
    for (int i = 0; i < cmd_num; i++) {
        free(cmd_arr[i]);
    }
    free(cmd_arr);
    return 0;
}
int main() {
    char **list = NULL;
    char STP_WRD[] = "quit";
    int n = 0;
    list = get_list();
    while (strcmp(list[0], STP_WRD)) {
        n = main_cmd_exec(list);
        free_list(list);
        list = get_list();
        if (n == 1) {
            return 0;
        }
    }
    free_list(list);
    return 0;
}
