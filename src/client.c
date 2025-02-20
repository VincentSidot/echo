#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <termios.h>
#include <unistd.h>

#include "../includes/array.h"

#define _DEBUG

#ifdef _DEBUG
#define eprintf(...) fprintf(stderr, __VA_ARGS__);
#else
#define eprintf(...)
#endif

#ifdef _UNDERLINE
#define CURSOR_SET_MODE "\033[4m"    // Invert background and foreground
#define CURSOR_RESET_MODE "\033[24m" // Reset all attributes
#else
#define CURSOR_SET_MODE "\033[7m"    // Invert background and foreground
#define CURSOR_RESET_MODE "\033[27m" // Reset all attributes
#endif
#define RESET_ANSI "\033[0m"

#define FETCH_CURSOR_POSITION "\033[6n"

#define set_multichar_buffer(buffer, index, car)                               \
  do {                                                                         \
    for (size_t __multichar_idx = 0; __multichar_idx < (sizeof((car)) - 1);    \
         __multichar_idx++)                                                    \
      (buffer)[(sizeof((car)) - 1) * (index) + __multichar_idx] =              \
          (car)[__multichar_idx];                                              \
  } while (0)

typedef struct s_user_input {
  da_struct(char) size_t cursor_position;
} t_user_input;

typedef struct s_app {
  int width;
  int heigth;
  struct termios original_settings;
  t_user_input user_input;
} t_app;

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
  INPUT_TYPE_CLEAR,     // 0c
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

typedef struct s_cursor_position {
  int row;
  int column;
} t_cursor_position;

struct termios *original_settings = NULL;

inline void set_cursor(bool mode) { puts(mode ? "\033[?25h" : "\033[?25l"); }

inline void clear() { puts("\033[2J"); }
inline void clear_line() { puts("\033[2K"); }
inline void set_cursor_position(int x, int y) { printf("\033[%d;%dH", x, y); }

t_cursor_position get_cursor() {
  t_cursor_position cursor = {0};
  char response[32];

  // Send the escape sequence to get the cursor position
  printf(FETCH_CURSOR_POSITION);
  fflush(stdout);

  // Read the response from the terminal
  fgets(response, sizeof(response), stdin);

  // Parse the response to get the row and column
  if (sscanf(response, "\033[%d;%dR", &cursor.row, &cursor.column) != 2) {
    cursor.row = -1;
    cursor.column = -1;
  }

  return cursor;
}

/**
 * @brief Return a buffer that set the cursor position
 *
 * @param x column cursor position (start at 1)
 * @param y width cursor position (start at 1)
 * @return char* the buffer (static)
 */
char *str_cursor_position(int x, int y) {
  static char buffer[30];
  snprintf(buffer, sizeof(buffer), "\033[%d;%dH", x, y);
  return buffer;
}

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
  puts(RESET_ANSI); // Reset all attributes
  // Clear terminal
  // clear();
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
    restore_terminal_mode(original_settings);
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

/**
 * @brief Display the application layout to the terminal
 *
 * @param app The application object
 */
void display_app(t_app *app) {
  char *line_buffer;
  int width = app->width;
  int heigth = app->heigth;

  // Draw the top border
  line_buffer = malloc(3 * width + 1);

  for (size_t i = 1; i < width - 1; i++) {
    set_multichar_buffer(line_buffer, i, "─");
  }
  set_multichar_buffer(line_buffer, 0, "┌");
  set_multichar_buffer(line_buffer, width - 1, "┐");
  line_buffer[3 * width] = '\0';

  printf("%s%s", str_cursor_position(1, 1), line_buffer);

  for (size_t i = 1; i < heigth - 3; i++) {
    printf("%s│", str_cursor_position(i + 1, width));
    printf("%s│", str_cursor_position(i + 1, 1));
  }

  set_multichar_buffer(line_buffer, 0, "├");
  set_multichar_buffer(line_buffer, width - 1, "┤");
  printf("%s%s", str_cursor_position(heigth - 2, 1), line_buffer);

  printf("%s│", str_cursor_position(heigth - 1, 1));
  printf("%s│", str_cursor_position(heigth - 1, width));

  set_multichar_buffer(line_buffer, 0, "└");
  set_multichar_buffer(line_buffer, width - 1, "┘");
  printf("%s%s", str_cursor_position(heigth, 1), line_buffer);
  free(line_buffer);
}

/**
 * @brief Initialize the application object
 *
 * @param app The application object
 */
void init_app(t_app *app) {
  struct winsize w;

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  app->width = w.ws_col;
  app->heigth = w.ws_row;
  da_init_with_capacity(&app->user_input, sizeof(char) * app->width);
  app->user_input.cursor_position = 0;
  set_terminal_mode(&app->original_settings);
  original_settings = &app->original_settings;
  display_app(app);
}

/**
 * @brief Deinitialize the application object
 *
 * @param app The application object
 */
void deinit_app(t_app *app) {
  da_free(&app->user_input);
  restore_terminal_mode(&app->original_settings);

  original_settings = NULL;
}

/**
 * @brief Display the input message to the terminal
 *
 * @param app The application object
 * @param input The input message to display
 * @param input_len The length of the input message
 */
void display_input(t_app *app) {
  int width = app->width;
  int heigth = app->heigth;
  int input_len =
      app->user_input.count > width - 3 ? width - 3 : app->user_input.count;
  int cursor_position = app->user_input.cursor_position;

  // Draw the message
  set_cursor_position(heigth - 1, 2);
  clear_line();

  printf("%s│ > ", str_cursor_position(heigth - 1, 1));
  if (cursor_position >= input_len) {
    printf("%.*s" CURSOR_SET_MODE "%c" CURSOR_RESET_MODE, input_len,
           app->user_input.items, ' ');

    printf("%s│", str_cursor_position(heigth - 1, width));
  } else {
    printf("%.*s" CURSOR_SET_MODE "%c" CURSOR_RESET_MODE "%.*s",
           cursor_position, app->user_input.items,
           app->user_input.items[cursor_position],
           input_len - cursor_position - 1,
           app->user_input.items + cursor_position + 1);

    printf("%s│", str_cursor_position(heigth - 1, width));
  }

  // Flush the output
  fflush(stdout);
}

#ifdef _DEBUG
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
  }
}

void print_app(t_app *app) {
  eprintf("Width: %d\n", app->width);
  eprintf("Heigth: %d\n", app->heigth);
  eprintf("User input: %.*s\n", (int)app->user_input.count,
          app->user_input.items);
  eprintf("User input count: %zu\n", app->user_input.count);
  eprintf("Cursor position: %zu\n", app->user_input.cursor_position);
}

#endif

t_input get_input() {
#define PARSE_BUFFER(buffer, expected)                                         \
  memcmp(buffer, expected, sizeof(expected) - 1) == 0

#define INPUT_BUFFER_SIZE 10
  size_t readed = 0;
  t_input input = {0};
  char buffer[INPUT_BUFFER_SIZE];

  readed = read(STDIN_FILENO, &buffer, INPUT_BUFFER_SIZE);

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
    else if (buffer[0] == '\x0c') // CTRL+L
      input.type = INPUT_TYPE_CLEAR;
    else {
      input.type = INPUT_TYPE_KEY;
      input.key = buffer[0];
    }

  } else {
    input.type = INPUT_TYPE_UNKNOWN;
  }

  return input;
}

void handle_arrow(t_app *app, t_arrow_key arrow) {
  switch (arrow) {
  case ARROW_UP:
    break;
  case ARROW_DOWN:
    break;
  case ARROW_RIGHT:
    if (app->user_input.cursor_position < app->user_input.count) {
      app->user_input.cursor_position++;
    }
    break;
  case ARROW_LEFT:
    if (app->user_input.cursor_position > 0) {
      app->user_input.cursor_position--;
    }
    break;
  }
}

void handle_backspace(t_app *app) {
  if (app->user_input.cursor_position > 0) {
    if (app->user_input.cursor_position == app->user_input.count) {
      eprintf("Remove at end\n");
      app->user_input.count--; // Remove the last character (fast remove)
    } else {
      eprintf("Remove at %zu\n", app->user_input.cursor_position - 1);
      da_remove(&app->user_input, app->user_input.cursor_position - 1);
    }
    app->user_input.cursor_position--;
  }
}

void handle_delete(t_app *app) {
  if (app->user_input.cursor_position < app->user_input.count) {
    da_remove(&app->user_input, app->user_input.cursor_position);
  }
}

void handle_key(t_app *app, char key) {
  if (app->user_input.count >= app->user_input.cursor_position) {
    da_insert(&app->user_input, app->user_input.cursor_position, key);
  } else {
    da_append(&app->user_input, key);
  }
  app->user_input.cursor_position++;
}

bool handle_input(t_app *app) {
  t_input c;

  // Read the input
  c = get_input();

  switch (c.type) {
  case INPUT_TYPE_CTRL_C:
    kill(getpid(), SIGINT);
    break;
  case INPUT_TYPE_ESC:
    return false;
  case INPUT_TYPE_ARROW:
    handle_arrow(app, c.arrow);
    break;
  case INPUT_TYPE_ENTER:
    // Do something with the input
    break;
  case INPUT_TYPE_BACKSPACE:
    handle_backspace(app);
    break;
  case INPUT_TYPE_KEY:
    handle_key(app, c.key);
    break;
  case INPUT_TYPE_DELETE:
    handle_delete(app);
    break;
  case INPUT_TYPE_BEGIN:
    app->user_input.cursor_position = 0;
    break;
  case INPUT_TYPE_END:
    app->user_input.cursor_position = app->user_input.count;
    break;
  case INPUT_TYPE_CLEAR:
    da_clear(&app->user_input);
    app->user_input.cursor_position = 0;
    break;
  case INPUT_TYPE_UNKNOWN:
    break;
  }

  return true;
}

int main() {
  t_app app = {0};

  init_app(&app);
  register_signal();

  int i = 0;
  do {
    print_app(&app);
    eprintf("===========================\n");
    display_input(&app);
  } while (handle_input(&app));

  deinit_app(&app);

  return 0;
}
