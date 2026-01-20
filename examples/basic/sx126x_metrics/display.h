#ifndef _DISPLAY_H_
#define _DISPLAY_H_

// Provide an ID for this board
#ifndef IDENTIFIER
#  define IDENTIFIER 0
#endif

int init_display_thread(void *ctx);

#endif // _DISPLAY_H_
