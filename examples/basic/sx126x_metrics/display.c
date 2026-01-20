#include "thread.h"
#include "u8g2.h"
#include "u8x8_riotos.h"
#include "ztimer.h"
#include <stdio.h>

#include "stats.h"
#include "display.h"

#define DISPLAY_DEACTIVATE_PIN GPIO_PIN(0, 36) // OLED Power Control
#define DISPLAY_RST_PIN        GPIO_PIN(0, 21) // OLED Reset

#define DISPLAY_I2C            0    // I2C Line, could be made into a
#define DISPLAY_I2C_ADDR       0x3c // I2C Address

// Maximum character widths for integers
#define MAX_STRLEN             30
#define MAX_BAT_WIDTH          4 // Maximum value for battery, to prevent using more than four chars
#define MAX_ID_WIDTH           2
#define MAX_BYTE_WIDTH         5
#define MAX_COUNT_WIDTH        3

// Allow integer widths to be used in string formatting
#define _STRINGIFY(x)          #x
// Force expansion of the enclosed macro
#define STR(macro)             _STRINGIFY(macro)

#define DEFAULT_PAD_X          3
#define DEFAULT_PAD_Y          3

#define THREAD_PRIORITY_MED    (THREAD_PRIORITY_MAIN - 2)
static char _stack[THREAD_STACKSIZE_MEDIUM];

static u8g2_t u8g2;
static u8x8_riotos_t user_data = {
    .device_index = DISPLAY_I2C,
    .pin_cs = GPIO_UNDEF,
    .pin_dc = GPIO_UNDEF,
    .pin_reset = DISPLAY_RST_PIN,
};

static void next_line(unsigned int *cursor_x, unsigned int *cursor_y, unsigned int font_height, unsigned int pad_y)
{
    *cursor_x = 0;
    *cursor_y += font_height + pad_y;
}

static inline unsigned int clamp_width(unsigned int value, unsigned int width)
{
    unsigned int max = 0;
    for (unsigned int i = 0; i < width; i++) {
        max *= 10; //NOLINT
        max += 9;  //NOLINT
    }

    if (value > max) {
        return max;
    }
    return value;
}

// Draw to the display, providng all parameters that will be shown on the display
void draw_display(unsigned int battery_mv, int display_route_notif, unsigned int identifier, struct netstat_record *main_stats)
{
    static char text_buffer[MAX_STRLEN];

    // Use a monospaced (for readability and predictability in size of output) font with restricted character set (less overhead from unused chars)
    u8g2_SetFont(&u8g2, u8g2_font_6x10_mr);
    const unsigned int font_height = (unsigned char)u8g2_GetMaxCharHeight(&u8g2);

    // Loop for transferring data to the display incrementally, completes when all data transferred
    u8g2_FirstPage(&u8g2);
    do {
        unsigned int cursor_x = 0;
        // u8g2 draws downwards, so set cursor to after the first line on the display
        unsigned int cursor_y = font_height;

        snprintf(text_buffer, sizeof(text_buffer), "%." STR(MAX_BAT_WIDTH) "dmV", clamp_width(battery_mv, MAX_BAT_WIDTH));
        cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, text_buffer) + DEFAULT_PAD_X;

        snprintf(text_buffer, sizeof(text_buffer), "ID:%." STR(MAX_ID_WIDTH) "d", clamp_width(identifier, MAX_ID_WIDTH));
        cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, text_buffer) + DEFAULT_PAD_X;

        // If there was recently something routed, display something on the display
        if (display_route_notif) {
            cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, "|ROUTED|");
        }
        next_line(&cursor_x, &cursor_y, font_height, DEFAULT_PAD_Y);

        cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, "STATS");
        next_line(&cursor_x, &cursor_y, font_height, DEFAULT_PAD_Y);

        snprintf(text_buffer, sizeof(text_buffer), "TX PKT SUCC:%u FAIL:%u",
                 clamp_width(main_stats->tx_success, MAX_COUNT_WIDTH),
                 clamp_width(main_stats->tx_failed, MAX_COUNT_WIDTH));
        cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, text_buffer);
        next_line(&cursor_x, &cursor_y, font_height, DEFAULT_PAD_Y);

        snprintf(text_buffer, sizeof(text_buffer), "RX PKT TOT:%u",
                 clamp_width(main_stats->rx_count, MAX_COUNT_WIDTH));
        cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, text_buffer);
        next_line(&cursor_x, &cursor_y, font_height, DEFAULT_PAD_Y);

        // Look into special fonts with icons for better UI elements
    } while (u8g2_NextPage(&u8g2));
}

// Get a certain number of bytes from a buffer
static void get_record(tsrb_t *tsrb, uint8_t *buf, size_t size)
{
    // Means there's at least one record in the buffer
    if (tsrb_avail(tsrb) >= size) {
        // If this fails, we get no bytes and buf isn't overwritten, or there's a new record and we get the new one
        tsrb_get(tsrb, buf, size);
    }
}

static void *_display_loop(void *ctx)
{
    struct display_thread_args *args = ctx;
    while (1) {
        struct power_record power = { 0 };
        struct netstat_record netstat = { 0 };
        struct capture_record capture = { 0 };

        get_record(args->power_ringbuffer, (uint8_t *)&power, sizeof(power));
        get_record(args->netstat_ringbuffer, (uint8_t *)&netstat, sizeof(netstat));
        get_record(args->power_ringbuffer, (uint8_t *)&capture, sizeof(capture));

        // TODO: Do something with the time associated with the capture thing
        draw_display(power.millivolts, 1, IDENTIFIER, &netstat);
        ztimer_sleep(ZTIMER_SEC, 1);
    }
    return NULL;
}

// Prepare the display for drawing
int init_display_thread(struct display_thread_args *args)
{
    gpio_init(DISPLAY_DEACTIVATE_PIN, GPIO_OUT);
    gpio_clear(DISPLAY_DEACTIVATE_PIN);

    u8g2_Setup_ssd1306_i2c_128x64_noname_1(&u8g2, U8G2_R0, u8x8_byte_hw_i2c_riotos, u8x8_gpio_and_delay_riotos);

    u8g2_SetUserPtr(&u8g2, &user_data);
    u8g2_SetI2CAddress(&u8g2, DISPLAY_I2C_ADDR);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    thread_create(_stack, sizeof(_stack), THREAD_PRIORITY_MED, 0, _display_loop, args, "display");
    return 0;
}
