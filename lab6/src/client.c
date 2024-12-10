#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"
#include "utils.h"

struct Server {
  char ip[255];
  int port;
};

void *WaitForResponse(void *args) {
  int sck = *((int *)args);
  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Recieve failed\n");
    exit(1);
  }
  close(sck);
  uint64_t *ans = malloc(sizeof(uint64_t));
  memcpy(ans, response, sizeof(uint64_t));
  return (void *)ans;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'}; // TODO: explain why 255
  FILE *a;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        // TODO: your code here
        if (k <= 0) {
          printf("k must be a positive number!\n");
          return 1;
        }
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        // TODO: your code here
        if (mod <= 0) {
          printf("mod must be a positive number!\n");
          return 1;
        }
        break;
      case 2:
        // TODO: your code here
        a = fopen(optarg, "r");
        if (a == NULL) {
          printf("Failed to open file!\n");
          return 1;
        } else {
          fclose(a);
        }
        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // TODO: for one server here, rewrite with servers from file
  unsigned int servers_num = 0;
  int size = 10;
  struct Server *to = malloc(sizeof(struct Server) * size);
  a = fopen(servers, "r");
  char *cadress;
  char tmp[255] = {'\0'};
  while (!feof(a)) {
    if (servers_num == size) {
      size = size + 10;
      to = realloc(to, sizeof(struct Server) * size);
    }
    fscanf(a, "%s\n", tmp);
    char *marker = strtok(tmp, ":");
    memcpy(to[servers_num].ip, tmp, sizeof(tmp));
    to[servers_num].port = atoi(strtok(NULL, ":"));
    servers_num++;
  }
  if (to[servers_num - 1].port == 0)
    servers_num--;
  fclose(a);
  if (servers_num > k) {
    printf("Number of servers is bigger than k\n");
    servers_num = k;
    printf("Only %llu servers will be used\n", k);
  }
  // TODO: delete this and parallel work between servers
  // to[0].port = 20001;
  // memcpy(to[0].ip, "127.0.0.1", sizeof("127.0.0.1"));

  // TODO: work continiously, rewrite to make parallel
  int ars = k / servers_num;
  int left = k % servers_num;
  int sck[servers_num];
  pthread_t threads[servers_num];
  for (int i = 0; i < servers_num; i++) {
    struct hostent *hostname = gethostbyname(to[i].ip);
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", to[i].ip);
      exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(to[i].port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr_list[0]);

    sck[i] = socket(AF_INET, SOCK_STREAM, 0);
    if (sck[i] < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      exit(1);
    }

    if (connect(sck[i], (struct sockaddr *)&server, sizeof(server)) < 0) {
      fprintf(stderr, "Connection failed\n");
      close(sck[i]);
      exit(1);
    }

    // TODO: for one server
    // parallel between servers
    uint64_t begin = ars * i + (left < i ? left : i) + 1;
    uint64_t end = ars * (i + 1) + (left < i + 1 ? left : i + 1) + 1;

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));

    if (send(sck[i], task, sizeof(task), 0) < 0) {
      fprintf(stderr, "Send failed\n");
      exit(1);
    }

    int *sck_copy = malloc(sizeof(int));
    *sck_copy = sck[i];
    if (pthread_create(&threads[i], NULL, (void *)WaitForResponse,
                       (void *)sck_copy)) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  uint64_t answer = 1;
  uint64_t response = 0;
  for (int i = 0; i < servers_num; i++) {
    uint64_t *response;
    pthread_join(threads[i], (void **)&response);
    answer = MultModulo(*response, answer, mod);
    free(response);
  }

  free(to);

  printf("Factorial %llu by mod %llu = %llu\n", k, mod, answer);
  return 0;
}