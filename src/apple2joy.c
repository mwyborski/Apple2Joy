/*
 * Apple2Joy — Apple IIe Joystick Emulation via MCP4251 Digital Potentiometers
 *
 * Supports two modes:
 *   Synchronous:  apple2joy_update() performs SPI/GPIO directly (default).
 *   Asynchronous: apple2joy_start_async() launches core1; subsequent calls
 *                 to apple2joy_update() pack the input into a single 32-bit
 *                 FIFO word and return immediately. Core1 pops and applies.
 *
 * FIFO word format (32 bits):
 *   bit 31      : command flag (0 = update, 1 = reset to defaults)
 *   bits 23–16  : X axis
 *   bits 15–8   : Y axis
 *   bits 7–4    : D-pad (0–8)
 *   bit 1       : button1
 *   bit 0       : button0
 */

#include "apple2joy.h"
#include "mcp4251.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"

/* ---------------------------------------------------------------------- */
/*  FIFO encoding                                                         */
/* ---------------------------------------------------------------------- */

#define FIFO_CMD_RESET  (1u << 31)

static inline uint32_t encode_input(const apple2joy_input_t *input)
{
    return ((uint32_t)input->x       << 16)
         | ((uint32_t)input->y       <<  8)
         | ((uint32_t)(input->dpad & 0x0F) << 4)
         | ((uint32_t)input->button1 <<  1)
         | ((uint32_t)input->button0);
}

static inline void decode_input(uint32_t word, apple2joy_input_t *input)
{
    input->x       = (word >> 16) & 0xFF;
    input->y       = (word >>  8) & 0xFF;
    input->dpad    = (word >>  4) & 0x0F;
    input->button1 = (word >>  1) & 1;
    input->button0 =  word        & 1;
}

/* ---------------------------------------------------------------------- */
/*  Wiper position constants                                              */
/* ---------------------------------------------------------------------- */

#define WIPER_MIN 0
#define WIPER_MID 128
#define WIPER_MAX 255

/* ---------------------------------------------------------------------- */
/*  Internal state                                                        */
/* ---------------------------------------------------------------------- */

static apple2joy_config_t s_config;
static volatile bool s_async_active = false;

/* ---------------------------------------------------------------------- */
/*  Axis output helper                                                    */
/* ---------------------------------------------------------------------- */

static void set_axes(uint8_t x, uint8_t y)
{
    if (s_config.swap_axes)
        mcp4251_set_wipers(y, x);
    else
        mcp4251_set_wipers(x, y);
}

/* ---------------------------------------------------------------------- */
/*  D-pad to axis mapping                                                 */
/*                                                                        */
/*  Direction indices: 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW              */
/* ---------------------------------------------------------------------- */

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

/* ---------------------------------------------------------------------- */
/*  Synchronous output (used directly or called by core1)                 */
/* ---------------------------------------------------------------------- */

static void apply_update(const apple2joy_input_t *input)
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

static void apply_defaults(void)
{
    set_axes(WIPER_MID, WIPER_MID);

    for (uint8_t i = 0; i < s_config.num_buttons; i++)
        gpio_put(s_config.gpio_button_base + i, 0);
}

/* ---------------------------------------------------------------------- */
/*  Core1 entry point                                                     */
/* ---------------------------------------------------------------------- */

static void core1_output_loop(void)
{
    for (;;) {
        uint32_t word = multicore_fifo_pop_blocking();

        if (word & FIFO_CMD_RESET) {
            apply_defaults();
        } else {
            apple2joy_input_t input;
            decode_input(word, &input);
            apply_update(&input);
        }
    }
}

/* ---------------------------------------------------------------------- */
/*  Public API                                                            */
/* ---------------------------------------------------------------------- */

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

void apple2joy_start_async(void)
{
    multicore_launch_core1(core1_output_loop);
    s_async_active = true;
}

void apple2joy_update(const apple2joy_input_t *input)
{
    if (s_async_active) {
        multicore_fifo_push_blocking(encode_input(input));
    } else {
        apply_update(input);
    }
}

void apple2joy_set_defaults(void)
{
    if (s_async_active) {
        multicore_fifo_push_blocking(FIFO_CMD_RESET);
    } else {
        apply_defaults();
    }
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
