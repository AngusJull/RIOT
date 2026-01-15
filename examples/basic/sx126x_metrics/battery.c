#include "periph/gpio.h"
#include "periph/adc.h"
#include "adc_arch.h"
#include "ztimer.h"

#define ADC_LINE_VBAT         ADC_LINE(0)     // Battery ADC input
#define ADC_CTRL_PIN          GPIO_PIN(0, 37) // Battery ADC enabled pin

#define R1                    390000 // Resistor divider value in ohms
#define R2                    100000 // Resistor divider value in ohms
#define CONVERSION_SCALING    10     // Scaling to use when applying resistor divider conversion during calculation
#define BATTERY_READ_DELAY_MS 100

int init_battery_adc(void)
{
    int res = adc_init(ADC_LINE_VBAT);
    if (res < 0) {
        return res;
    }
    res = gpio_init(ADC_CTRL_PIN, GPIO_OUT);
    res = adc_set_attenuation(ADC_LINE_VBAT, ADC_ATTEN_DB_12);
    return res;
}

unsigned int read_battery_mv(void)
{
    // Enable the ADC line. Might be better to just leave this high and forget about it
    gpio_set(ADC_CTRL_PIN);
    ztimer_sleep(ZTIMER_MSEC, BATTERY_READ_DELAY_MS);
    int raw = adc_sample(ADC_LINE_VBAT, ADC_RES_12BIT);
    gpio_clear(ADC_CTRL_PIN);

    int adc_mv;
    if (adc_raw_to_voltage(ADC_LINE_VBAT, raw, &adc_mv) < 0) {
        return 0;
    }
    // Negative value doesn't make sense in this case
    if (adc_mv < 0) {
        adc_mv = 0;
    }

    return (adc_mv * ((R1 + R2) * CONVERSION_SCALING / R2)) / CONVERSION_SCALING;
}
