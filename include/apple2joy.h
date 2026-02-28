/*
 * Apple2Joy — Apple IIe Joystick Emulation via MCP4251 Digital Potentiometers
 *
 * Maps gamepad analog axes and D-pad directions to the Apple IIe paddle
 * inputs (PDL0/PDL1) via dual digital potentiometers, and gamepad buttons
 * to the Apple IIe pushbutton inputs (PB0/PB1) via GPIO.
 *
 * Hardware connections:
 *   MCP4251 Wiper 0 → Apple IIe PDL(0) / X axis
 *   MCP4251 Wiper 1 → Apple IIe PDL(1) / Y axis
 *   GPIO pin N       → Apple IIe PB0
 *   GPIO pin N+1     → Apple IIe PB1
 */

#ifndef APPLE2JOY_H
#define APPLE2JOY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Configuration                                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t gpio_button_base; /* First GPIO pin for pushbuttons          */
    uint8_t num_buttons;      /* Number of pushbuttons (typically 2)     */
    bool    swap_axes;        /* Swap X/Y axis wiper mapping             */
} apple2joy_config_t;

/** Default configuration: GPIO 6–7 for buttons, no axis swap. */
#define APPLE2JOY_DEFAULT_CONFIG { \
    .gpio_button_base = 6,         \
    .num_buttons      = 2,         \
    .swap_axes        = false,     \
}

/* ------------------------------------------------------------------ */
/*  Axis input (from gamepad)                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t x;        /* Analog X axis (0–255, 128 = center) */
    uint8_t y;        /* Analog Y axis (0–255, 128 = center) */
    uint8_t dpad;     /* D-pad hat value (0–7 = direction, 8 = released) */
    bool    button0;  /* Mapped to PB0 */
    bool    button1;  /* Mapped to PB1 */
} apple2joy_input_t;

/* ------------------------------------------------------------------ */
/*  API                                                               */
/* ------------------------------------------------------------------ */

/**
 * Initialize MCP4251, configure button GPIOs, set axes to center.
 */
void apple2joy_init(const apple2joy_config_t *config);

/**
 * Launch the output driver on core1 for lowest-latency operation.
 *
 * After this call, apple2joy_update() and apple2joy_set_defaults()
 * become non-blocking: they pack the input into a single 32-bit word
 * and push it to the multicore FIFO. Core1 pops the word and performs
 * the actual SPI and GPIO writes.
 *
 * Must be called after apple2joy_init().
 */
void apple2joy_start_async(void);

/**
 * Update Apple IIe outputs from gamepad state.
 * When D-pad is active (dpad < 8), it overrides analog axes with
 * fixed directional values. Otherwise analog X/Y pass through.
 *
 * In async mode (after apple2joy_start_async()), this returns
 * immediately after a single FIFO push.
 */
void apple2joy_update(const apple2joy_input_t *input);

/**
 * Reset outputs to default (centered axes, buttons released).
 *
 * In async mode, this returns immediately after a single FIFO push.
 */
void apple2joy_set_defaults(void);

/**
 * Run a hardware test sequence: sweep pots and toggle buttons.
 * Blocks until complete.
 */
void apple2joy_run_test(void);

#ifdef __cplusplus
}
#endif

#endif /* APPLE2JOY_H */
