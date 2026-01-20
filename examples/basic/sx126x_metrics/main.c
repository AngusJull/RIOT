#include "shell.h"

#include <stdio.h>

#include "display.h"
#include "stats.h"
#include "pkt_capture.h"

static void *_shell_loop(void *ctx)
{
    (void)ctx;
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
    return NULL;
}

int main(void)
{
    init_display_thread(NULL);
    init_pkt_capture_thread(NULL);
    init_stats_thread(NULL);
    (void)puts("Threads started, running shell\n");
    _shell_loop(NULL);

    return 0;
}
