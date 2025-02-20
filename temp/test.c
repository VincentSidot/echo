//usr/bin/gcc "$0" -o "${0%.c}" && ${0%.c} $@ ; rm -f "${0%.c}"; exit
#include <assert.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

void set_cursor(bool mode) { puts(mode ? "\033[?25h" : "\033[?25l"); }
void clear() { puts("\033[2J"); }

struct termios original_settings;
/**
 * @brief Set the terminal mode object
 *
 * @param app The application object
 */
void set_terminal_mode(struct termios *original_settings) {
  struct termios new_settings;
  // Get the current terminal attributes
  tcgetattr(STDIN_FILENO, original_settings);
  // Copy the current settings to modify them
  new_settings = *original_settings;
  // Disable canonical mode (line buffering) and echoing
  new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
  // Apply the new settings
  tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
  // Hide cursor
  set_cursor(false);
  // Clear terminal
  clear();
}
/**
 * @brief Restore the terminal mode object
 *
 * @param app The application object
 */
void restore_terminal_mode(struct termios *original_settings) {
  if (original_settings == NULL)
    return;
  // Restore the original settings
  tcsetattr(STDIN_FILENO, TCSANOW, original_settings);
  // Show cursor
  set_cursor(true);
  // Clear terminal
  clear();
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
    restore_terminal_mode(&original_settings);
#ifdef _DEBUG
    printf("Signal %d received, terminal restored\n", sig);
#endif
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
int main() {
  set_terminal_mode(&original_settings);

  struct pollfd fds = {STDIN_FILENO, POLLIN, 0};
  char c;
  while (1) {
    c = getchar();
    printf("You entered: %d\n", c);
    poll(&fds, 1, 100);
    while (fds.revents & POLLIN) {
      c = getchar();
      printf("You entered: %d\n", c);
      poll(&fds, 1, 100);
    }

    printf("================================\n");

    if (c == 3) { // Ctrl+C
      break;
    }
  }

  restore_terminal_mode(&original_settings);
  return 0;
}