
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    pid_t pid = fork(); // создаем новый процесс

    if (pid == 0) {
        // Это дочерний процесс
        printf("Running sequential_min_max in child process...\n");

        // Выполняем программу sequential_min_max с аргументами
        if (execv("./sequential_min_max", argv) == -1) {
            perror("execv failed");
            return 1;
        }
    } else if (pid > 0) {
        // Это родительский процесс
        int status;
        wait(&status); // Ожидание завершения дочернего процесса
        printf("Child process finished with status: %d\n", status);
    } else {
        perror("fork failed");
        return 1;
    }

    return 0;
}
