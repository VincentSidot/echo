#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define _DA_INIT_CAPACITY 16
#include "../includes/array.h"

#define BUFF_SIZE 1024
#define SERVER_PORT 5000

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

typedef struct {
  da_struct(struct pollfd)
} s_da_fd;

typedef struct {
  da_struct(struct sockaddr_in)
} s_da_addr;

typedef struct {
  s_da_addr *addrs;
  s_da_fd *fds;
} s_context;

// Global application context (useful for signal handler)
s_context *ctx = {0};

/**
 * @brief Cleanup the server (close all file descriptors)
 *
 */
void cleanup(s_context *ctx) {
  if (ctx == NULL) {
    return;
  }
  // Cleanup
  da_foreach_unsafe(ctx->fds, item) { close(item->fd); }
  da_free(ctx->fds);
  da_free(ctx->addrs);
  free(ctx);
}

/**
 * @brief Register a client file descriptor
 *
 * @param fds file descriptor dynamic array
 * @param fd file descriptor
 */
void register_client(s_da_fd *fds, int fd) {
  struct pollfd s_fd;
  s_fd.fd = fd;
  s_fd.events = POLLIN;
  // Server is always the first item.
  da_append(fds, s_fd);
}

/**
 * @brief Register a server file descriptor
 *
 * @param fds file descriptor dynamic array
 * @param fd file descriptor
 */
void register_server(s_da_fd *fds, int fd) {
  // Server is always the first item.
  da_clear(fds);
  register_client(fds, fd);
}

/**
 * @brief Handle signal to close server file descriptor
 *
 */
void handle_signal(int sig) {
  struct sigaction act;
  switch (sig) {
  case SIGSEGV:
  case SIGINT:
  case SIGTERM:
    cleanup(ctx);
    break;
  }

  // Restore old signal
  act.sa_handler = SIG_DFL;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(sig, &act, NULL);

  // Resend the signal
  kill(getpid(), sig);
}

/**
 * @brief Register signals
 *
 */
void register_signal() {
  struct sigaction act;

  sigemptyset(&act.sa_mask);

  act.sa_handler = handle_signal;
  act.sa_flags = SA_SIGINFO;

  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGSEGV, &act, NULL);

  return;
}

/**
 * @brief Simply echo the data back to the client
 *
 * @param fd file descriptor
 * @return int 0 if success, 1 if connection closed
 */
int echo_server(int fd) {
  int readed = 0;
  int writed = 0;
  char buffer[BUFF_SIZE];
  readed = read(fd, buffer, BUFF_SIZE);
  if (readed == 0) {
    return 1; // Connection closed
  } else if (readed == -1) {
    eprintf("Error read failed: %s\n", strerror(errno));
    return 1;
  } else {
    writed = write(fd, buffer, readed);
    if (writed == -1) {
      eprintf("Error write failed: %s\n", strerror(errno));
      return 1;
    }
  }
  return 0;
}

/**
 * @brief Initialize the server
 *
 * @return int file descriptor of the server
 */
int init_server(s_context *ctx) {
  struct sockaddr_in addr;
  int sockopt = 1;
  int fd = 0;

  // Create socket
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    eprintf("Error socket failed: %s\n", strerror(errno));
    return -1;
  }

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt))) {
    eprintf("Error setsockopt failed: %s\n", strerror(errno));
    cleanup(ctx);
    return -1;
  }

  // Setup the server
  memset(&addr, '0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(SERVER_PORT);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    eprintf("Error bind failed: %s\n", strerror(errno));
    cleanup(ctx);
    return -1;
  };

  listen(fd, 10);
  printf("Server started on port %d\n", (int)SERVER_PORT);

  return fd;
}

/**
 * @brief run the server
 *
 * @return int
 */
int run_server(s_context *ctx) {
  while (true) {
    s_da_fd *fds = ctx->fds;
    s_da_addr *addrs = ctx->addrs;
    char *ip = NULL;
    int connfd = 0;
    int poll_status = 0;
    struct sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);

    poll_status = poll(fds->items, fds->count, -1); // Wait indefinitely

    if (poll_status == -1) {
      eprintf("Error poll failed: %s\n", strerror(errno));
      return -1;
    }

    // Echo server
    for (size_t i = 1; i < fds->count; i++) {
      if (fds->items[i].revents & POLLIN) {
        if (echo_server(fds->items[i].fd)) {
          ip = inet_ntoa(addrs->items[i - 1].sin_addr);
          printf("Connection from %s, port %d closed\n", ip,
                 ntohs(addrs->items[i - 1].sin_port));

          close(fds->items[i].fd);

          // Remove doesn't preserve order but we remove at same
          // time so it should be fine.
          da_fast_remove(fds, i--);
          da_fast_remove(addrs, i);
        }
      }
    }

    // Check if we have an incoming connection
    if (fds->items[0].revents & POLLIN) {
      connfd = accept(fds->items[0].fd, (struct sockaddr *)&client_addr,
                      &client_size);
      ip = inet_ntoa(client_addr.sin_addr);
      // Display addr of connected
      printf("Connection from %s, port %d\n", ip, ntohs(client_addr.sin_port));

      register_client(fds, connfd);
      da_append(addrs, client_addr);
    }
  }
  return 0;
}

int main() {
  s_da_fd fds = {0};
  s_da_addr addrs = {0};
  int fd;

  ctx = malloc(sizeof(s_context));

  ctx->fds = &fds;
  ctx->addrs = &addrs;

  // Initialize the server
  fd = init_server(ctx);

  // Register the server
  register_server(&fds, fd);

  // Create a signal handler
  register_signal();

  // Run the server
  run_server(ctx);

  // Cleanup
  cleanup(ctx);
  return 0;
}