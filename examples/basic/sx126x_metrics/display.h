#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "net/netstats.h"

int init_display(void);

void draw_display(unsigned int battery_mv, int display_route_notif, unsigned int identifier, netstats_t *main_stats);

#endif // _DISPLAY_H_
