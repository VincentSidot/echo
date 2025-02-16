#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct termios original_settings;

void set_terminal_mode() {
  struct termios new_settings;
  // Get the current terminal attributes
  tcgetattr(STDIN_FILENO, &original_settings);

  // Copy the current settings to modify them
  new_settings = original_settings;

  // Disable canonical mode (line buffering) and echoing
  new_settings.c_lflag &= ~(ICANON | ECHO);

  // Apply the new settings
  tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
}

void restore_terminal_mode() {
  // Restore the original settings
  tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);
}

int main() {
  set_terminal_mode();

  // Fetch window size
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  printf("Window size: %d rows, %d columns\n", w.ws_row, w.ws_col);

  // Example operation: Read a single character without echoing
  printf("Press any key (will not be echoed): ");
  getchar();
  printf("\nKey pressed.\n");

  restore_terminal_mode();
  return 0;
}
