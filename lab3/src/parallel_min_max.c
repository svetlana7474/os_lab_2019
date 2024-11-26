#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include "find_min_max.h"
#include "utils.h"
int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    bool with_files = false;

    while (true) {
        int current_optind = optind ? optind : 1;
        static struct option options[] = {{"seed", required_argument, 0, 0},
                                          {"array_size", required_argument, 0, 0},
                                          {"pnum", required_argument, 0, 0},
                                          {"by_files", no_argument, 0, 'f'},
                                          {0, 0, 0, 0}};
        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);
        if (c == -1) break;

        switch (c) {
        case 0:
            switch (option_index) {
            case 0:
                seed = atoi(optarg);
                if (seed <= 0) {
                    printf("Seed must be a positive number\n");
                    return 1;
                }
                break;
            case 1:
                array_size = atoi(optarg);
                if (array_size <= 0) {
                    printf("Array size must be a positive number\n");
                    return 1;
                }
                break;
            case 2:
                pnum = atoi(optarg);
                if (pnum <= 0) {
                    printf("Pnum must be a positive number\n");
                    return 1;
                }
                break;
            case 3:
                with_files = true;
                break;
            default:
                printf("Unknown option index: %d\n", option_index);
            }
            break;
        case 'f':
            with_files = true;
            break;
        case '?':
            break;
        default:
            printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed num --array_size num --pnum num [--by_files]\n", argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);
    int active_child_processes = 0;

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    int pipefd[2 * pnum];
    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            if (pipe(pipefd + 2 * i) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            active_child_processes += 1;
            if (child_pid == 0) {
                int start = i * (array_size / pnum);
                int end = (i + 1) * (array_size / pnum);
                if (i == pnum - 1) end = array_size;

                struct MinMax local_min_max = GetMinMax(array, start, end);

                if (with_files) {
                    char filename[256];
                    sprintf(filename, "min_max_%d.txt", i);
                    FILE *file = fopen(filename, "w");
                    if (file == NULL) {
                        perror("fopen");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(file, "%d %d\n", local_min_max.min, local_min_max.max);
                    fclose(file);
                } else {
                    close(pipefd[2 * i]); // Закрываем чтение
                    write(pipefd[2 * i + 1], &local_min_max, sizeof(struct MinMax));
                    close(pipefd[2 * i + 1]); // Закрываем запись
                }
                exit(0);
            }
        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    while (active_child_processes > 0) {
        wait(NULL);
        active_child_processes -= 1;
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX;
        int max = INT_MIN;

        if (with_files) {
            char filename[256];
            sprintf(filename, "min_max_%d.txt", i);
            FILE *file = fopen(filename, "r");
            if (file == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            fscanf(file, "%d %d", &min, &max);
            fclose(file);
        } else {
            close(pipefd[2 * i + 1]);  // Закрываем запись
            struct MinMax local_min_max;
            read(pipefd[2 * i], &local_min_max, sizeof(struct MinMax));
            close(pipefd[2 * i]);  // Закрываем чтение
            min = local_min_max.min;
            max = local_min_max.max;
        }

        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    fflush(NULL);
    return 0;
}
