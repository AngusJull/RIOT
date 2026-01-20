#include "display.h"

#include "u8g2.h"
#include "u8x8_riotos.h"
#include <stdio.h>

#define DISPLAY_DEACTIVATE_PIN GPIO_PIN(0, 36) // OLED Power Control
#define DISPLAY_RST_PIN        GPIO_PIN(0, 21) // OLED Reset

#define DISPLAY_I2C            0    // I2C Line, could be made into a
#define DISPLAY_I2C_ADDR       0x3c // I2C Address

#define MAX_STRLEN             30
#define MAX_BATTERY            9999 // Maximum value for battery, to prevent using more than four chars
#define MAX_IDENTIFIER         99
#define DEFAULT_PAD_X          3
#define DEFAULT_PAD_Y          3

static u8g2_t u8g2;
static u8x8_riotos_t user_data = {
    .device_index = DISPLAY_I2C,
    .pin_cs = GPIO_UNDEF,
    .pin_dc = GPIO_UNDEF,
    .pin_reset = DISPLAY_RST_PIN,
};

static void next_line(unsigned int *cursor_x, unsigned int *cursor_y, unsigned int font_height)
{
    *cursor_x = 0;
    *cursor_y += font_height;
}

// Prepare the display for drawing
int init_display(void)
{
    gpio_init(DISPLAY_DEACTIVATE_PIN, GPIO_OUT);
    gpio_clear(DISPLAY_DEACTIVATE_PIN);

    u8g2_Setup_ssd1306_i2c_128x64_noname_1(&u8g2, U8G2_R0, u8x8_byte_hw_i2c_riotos, u8x8_gpio_and_delay_riotos);

    u8g2_SetUserPtr(&u8g2, &user_data);
    u8g2_SetI2CAddress(&u8g2, DISPLAY_I2C_ADDR);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    return 0;
}

// Draw to the display, providng all parameters that will be shown on the display
void draw_display(unsigned int battery_mv, int display_route_notif, unsigned int identifier, netstats_t *main_stats)
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

        if (battery_mv > MAX_BATTERY) {
            battery_mv = MAX_BATTERY;
        }
        snprintf(text_buffer, sizeof(text_buffer), "%.4dmV", battery_mv);
        cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, text_buffer) + DEFAULT_PAD_X;

        if (identifier > MAX_IDENTIFIER) {
            identifier = MAX_IDENTIFIER;
        }
        snprintf(text_buffer, sizeof(text_buffer), "ID:%.2d", identifier);
        cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, text_buffer) + DEFAULT_PAD_X;

        // If there was recently something routed, display something on the display
        if (display_route_notif) {
            cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, "PKT ROUTED");
        }
        next_line(&cursor_x, &cursor_y, DEFAULT_PAD_Y);

        cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, "STATS");
        next_line(&cursor_x, &cursor_y, DEFAULT_PAD_Y);

        snprintf(text_buffer, sizeof(text_buffer), "TX SUCC:%u FAIL:%u BYTE:%u", main_stats->tx_success, main_stats->tx_failed, main_stats->tx_bytes);
        cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, text_buffer);
        next_line(&cursor_x, &cursor_y, DEFAULT_PAD_Y);

        snprintf(text_buffer, sizeof(text_buffer), "RX TOT:%u BYTE: %u", main_stats->rx_count, main_stats->tx_bytes);
        cursor_x += u8g2_DrawStr(&u8g2, cursor_x, cursor_y, text_buffer);
        next_line(&cursor_x, &cursor_y, DEFAULT_PAD_Y);

        // Look into special fonts with icons for better UI elements
    } while (u8g2_NextPage(&u8g2));
}
