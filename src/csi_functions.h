#ifndef CSI_FUNCTIONS
#define CSI_FUNCTIONS


#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <assert.h>


void clear_till_end();
void clear_line();
void clear_screen();
void clear_till_EOL();
void move_forward(size_t);
void move_backward(size_t);
void move_up(size_t);
void move_down(size_t);
void set_cursor(size_t, size_t);
void use_alternate_screen_buffer();
void use_standard_screen_buffer();

bool prompt_message(const char *, const char *);
bool prompt_message_bottom_center(const char * message, const char * csi_mod, const struct winsize * window_size);
bool prompt_bottom_center(char, const struct winsize *, const char *, const char *, ...);


#endif /* CSI_FUNCTIONS */
