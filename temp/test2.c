//usr/bin/gcc "$0" -o "${0%.c}" && ${0%.c} $@ ; rm -f "${0%.c}"; exit
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 30

#define eprintf(...) fprintf(stderr, __VA_ARGS__);

typedef enum __attribute__((__packed__)) e_input_type {
  INPUT_TYPE_KEY,
  INPUT_TYPE_ESC,       // 1b
  INPUT_TYPE_ARROW,     //
                        // UP 1b 5b 41
                        // DOWN 1b 5b 42
                        // RIGHT 1b 5b 43
                        // LEFT 1b 5b 44
  INPUT_TYPE_ENTER,     // a
  INPUT_TYPE_BACKSPACE, // 7f
  INPUT_TYPE_DELETE,    // 1b 5b 33 7e
  INPUT_TYPE_CTRL_C,    // 3
  INPUT_TYPE_BEGIN,     // 1b 5b 48 | 1
  INPUT_TYPE_END,       // 1b 5b 46 | 5
  INPUT_TYPE_UNKNOWN
} t_input_type;

typedef enum __attribute__((__packed__)) e_arrow_key {
  ARROW_UP,
  ARROW_DOWN,
  ARROW_RIGHT,
  ARROW_LEFT
} t_arrow_key;

typedef struct s_input {
  t_input_type type;
  union {
    char key;
    t_arrow_key arrow;
  };
} t_input;

// Function to set terminal to raw mode
void set_raw_mode() {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ICANON | ECHO | ISIG); // Disable canonical mode and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// Function to restore terminal to original mode
void reset_terminal_mode() {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag |= (ICANON | ECHO | ISIG); // Enable canonical mode and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
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
    reset_terminal_mode();
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

#define PARSE_BUFFER(buffer, expected)                                         \
  memcmp(buffer, expected, sizeof(expected)-1) == 0 

t_input get_input() {
#define INPUT_BUFFER_SIZE 10
  size_t readed = 0;
  t_input input = {0};
  char buffer[INPUT_BUFFER_SIZE];

  readed = read(STDIN_FILENO, &buffer, INPUT_BUFFER_SIZE);

  // Print buffer as hex values
  eprintf("Buffer[%zu]: ", readed);
  for (size_t i = 0; i < readed; i++) {
    eprintf("%02x ", buffer[i]);
  }
  eprintf("\n");

  if (readed == 4) {
    if (PARSE_BUFFER(buffer, "\x1b\x5b\x33\x7e")) // DELETE
      input.type = INPUT_TYPE_DELETE;
    else
      input.type = INPUT_TYPE_UNKNOWN;
  } else if (readed == 3) {
    if (PARSE_BUFFER(buffer, "\x1b\x5b\x48")) // BEGIN
      input.type = INPUT_TYPE_BEGIN;
    else if (PARSE_BUFFER(buffer, "\x1b\x5b\x46")) // END
      input.type = INPUT_TYPE_END;
    else if (PARSE_BUFFER(buffer, "\x1b\x5b")) { // ARROW
      input.type = INPUT_TYPE_ARROW;
      if (buffer[2] == '\x41')
        input.key = ARROW_UP;
      else if (buffer[2] == '\x42')
        input.key = ARROW_DOWN;
      else if (buffer[2] == '\x43')
        input.key = ARROW_RIGHT;
      else if (buffer[2] == '\x44')
        input.key = ARROW_LEFT;
      else
        input.type = INPUT_TYPE_UNKNOWN;
    } else
      input.type = INPUT_TYPE_UNKNOWN;
  } else if (readed == 1) {
    if (buffer[0] == '\x1b') // ESC
      input.type = INPUT_TYPE_ESC;
    else if (buffer[0] == '\x0a') // ENTER
      input.type = INPUT_TYPE_ENTER;
    else if (buffer[0] == '\x7f') // BACKSPACE
      input.type = INPUT_TYPE_BACKSPACE;
    else if (buffer[0] == '\x03') // CTRL+C
      input.type = INPUT_TYPE_CTRL_C;
    else if (buffer[0] == '\x01') // CTRL+A
      input.type = INPUT_TYPE_BEGIN;
    else if (buffer[0] == '\x05') // CTRL+E
      input.type = INPUT_TYPE_END;
    else {
      input.type = INPUT_TYPE_KEY;
      input.key = buffer[0];
    }
  } else {
    input.type = INPUT_TYPE_UNKNOWN;
  }

  return input;
}

void print_key(t_input *key) {
  char *arrow = NULL;

  switch (key->type) {
  case INPUT_TYPE_ARROW:
    switch (key->arrow) {
    case ARROW_UP:
      arrow = "UP";
      break;
    case ARROW_DOWN:
      arrow = "DOWN";
      break;
    case ARROW_RIGHT:
      arrow = "RIGHT";
      break;
    case ARROW_LEFT:
      arrow = "LEFT";
      break;
    }
    eprintf("Arrow key: %s\n", arrow);
    break;
  case INPUT_TYPE_KEY:
    eprintf("Key: %c\n", key->key);
    break;
  case INPUT_TYPE_ENTER:
    eprintf("Enter\n");
    break;
  case INPUT_TYPE_UNKNOWN:
    eprintf("Unknown\n");
    break;
  case INPUT_TYPE_CTRL_C:
    eprintf("Ctrl+C\n");
    break;
  case INPUT_TYPE_ESC:
    eprintf("ESC\n");
    break;
  case INPUT_TYPE_BACKSPACE:
    eprintf("Backspace\n");
    break;
  case INPUT_TYPE_DELETE:
    eprintf("Delete\n");
    break;
  case INPUT_TYPE_BEGIN:
    eprintf("Begin\n");
    break;
  case INPUT_TYPE_END:
    eprintf("End\n");
    break;
  }
}

int main() {
  char buffer[BUFFER_SIZE];
  size_t readed = 0;
  t_input key = {0};

  printf("Press arrow keys or ESC (use 'q' to quit):\n");

  set_raw_mode();
  register_signal();

  while (1) {
    key = get_input();
    print_key(&key);

    if (key.type == INPUT_TYPE_ESC ||
        (key.type == INPUT_TYPE_KEY && key.key == 'q')) {
      break;
    }
  }
end_loop:

  reset_terminal_mode();
  printf("Exiting...\n");

  return 0;
}