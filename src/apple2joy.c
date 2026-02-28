/*
 * Apple2Joy — Apple IIe Joystick Emulation via MCP4251 Digital Potentiometers
 */

#include "apple2joy.h"
#include "mcp4251.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

/* ------------------------------------------------------------------ */
/*  Wiper position constants                                          */
/* ------------------------------------------------------------------ */

#define WIPER_MIN 0
#define WIPER_MID 128
#define WIPER_MAX 255

/* ------------------------------------------------------------------ */
/*  Internal state                                                    */
/* ------------------------------------------------------------------ */

static apple2joy_config_t s_config;

/* ------------------------------------------------------------------ */
/*  Axis output helper                                                */
/* ------------------------------------------------------------------ */

static void set_axes(uint8_t x, uint8_t y)
{
    if (s_config.swap_axes)
        mcp4251_set_wipers(y, x);
    else
        mcp4251_set_wipers(x, y);
}

/* ------------------------------------------------------------------ */
/*  D-pad to axis mapping                                             */
/*                                                                    */
/*  Direction indices: 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW          */
/* ------------------------------------------------------------------ */

typedef struct { uint8_t x; uint8_t y; } axis_pair_t;

static const axis_pair_t s_dpad_map[8] = {
    [0] = { WIPER_MID, WIPER_MIN }, /* N  */
    [1] = { WIPER_MAX, WIPER_MIN }, /* NE */
    [2] = { WIPER_MAX, WIPER_MID }, /* E  */
    [3] = { WIPER_MAX, WIPER_MAX }, /* SE */
    [4] = { WIPER_MID, WIPER_MAX }, /* S  */
    [5] = { WIPER_MIN, WIPER_MAX }, /* SW */
    [6] = { WIPER_MIN, WIPER_MID }, /* W  */
    [7] = { WIPER_MIN, WIPER_MIN }, /* NW */
};

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

void apple2joy_init(const apple2joy_config_t *config)
{
    if (config)
        s_config = *config;
    else {
        apple2joy_config_t defaults = APPLE2JOY_DEFAULT_CONFIG;
        s_config = defaults;
    }

    mcp4251_init();

    for (uint8_t i = 0; i < s_config.num_buttons; i++) {
        uint pin = s_config.gpio_button_base + i;
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, 0);
    }

    set_axes(WIPER_MID, WIPER_MID);
}

void apple2joy_update(const apple2joy_input_t *input)
{
    if (input->dpad < 8)
        set_axes(s_dpad_map[input->dpad].x, s_dpad_map[input->dpad].y);
    else
        set_axes(input->x, input->y);

    if (s_config.num_buttons >= 1)
        gpio_put(s_config.gpio_button_base, input->button0);
    if (s_config.num_buttons >= 2)
        gpio_put(s_config.gpio_button_base + 1, input->button1);
}

void apple2joy_set_defaults(void)
{
    set_axes(WIPER_MID, WIPER_MID);

    for (uint8_t i = 0; i < s_config.num_buttons; i++)
        gpio_put(s_config.gpio_button_base + i, 0);
}

void apple2joy_run_test(void)
{
    printf("[Apple2Joy] Button test\n");
    for (uint8_t btn = 0; btn < s_config.num_buttons; btn++) {
        uint pin = s_config.gpio_button_base + btn;
        for (int j = 0; j < 8; j++) {
            gpio_put(pin, 1);
            sleep_ms(250);
            gpio_put(pin, 0);
            sleep_ms(250);
        }
    }

    printf("[Apple2Joy] Pot sweep: 0 -> 255\n");
    mcp4251_set_wipers(0, 0);
    sleep_ms(500);
    for (uint16_t i = 0; i < 256; i += 10) {
        mcp4251_set_wipers((uint8_t)i, (uint8_t)i);
        sleep_ms(500);
    }

    printf("[Apple2Joy] Pot sweep: 255 -> 0\n");
    mcp4251_set_wipers(255, 255);
    sleep_ms(500);
    for (uint16_t i = 0; i < 256; i += 10) {
        mcp4251_set_wipers((uint8_t)(255 - i), (uint8_t)(255 - i));
        sleep_ms(500);
    }

    printf("[Apple2Joy] Pot cross-sweep\n");
    mcp4251_set_wipers(0, 0);
    for (uint16_t i = 0; i < 256; i += 10) {
        mcp4251_set_wipers((uint8_t)i, (uint8_t)(255 - i));
        sleep_ms(500);
    }
    mcp4251_set_wipers(255, 0);
    sleep_ms(500);
    for (uint16_t i = 0; i < 256; i += 10) {
        mcp4251_set_wipers((uint8_t)(255 - i), (uint8_t)i);
        sleep_ms(500);
    }

    set_axes(WIPER_MID, WIPER_MID);
    printf("[Apple2Joy] Test complete\n");
}
