/*
 * USB Gamepad → Apple IIe Joystick Adapter Firmware
 *
 * Reads USB game controllers via TJUH and outputs to an Apple IIe
 * joystick port via Apple2Joy (MCP4251 digital potentiometers + GPIO).
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"

#include "tjuh.h"
#include "apple2joy.h"

/* ------------------------------------------------------------------ */
/*  Gamepad → Apple IIe mapping                                       */
/* ------------------------------------------------------------------ */

static void on_gamepad_report(uint8_t dev_addr, const tjuh_gamepad_report_t *rpt)
{
    (void)dev_addr;

    apple2joy_input_t input = {
        .x       = rpt->x,
        .y       = rpt->y,
        .dpad    = rpt->dpad,
        .button0 = rpt->cross    || rpt->triangle,
        .button1 = rpt->circle   || rpt->square,
    };

    apple2joy_update(&input);
}

static void on_gamepad_connect(uint8_t dev_addr, uint16_t vid, uint16_t pid)
{
    printf("[Firmware] Gamepad connected: dev=%u VID=%04x PID=%04x\r\n", dev_addr, vid, pid);
}

static void on_gamepad_disconnect(uint8_t dev_addr)
{
    printf("[Firmware] Gamepad disconnected: dev=%u\r\n", dev_addr);
    apple2joy_set_defaults();
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int main(void)
{
    board_init();
    printf("USB Gamepad to Apple IIe Joystick Adapter\r\n");

    /* Initialize Apple IIe output (MCP4251 + buttons) */
    apple2joy_config_t joy_config = APPLE2JOY_DEFAULT_CONFIG;
    apple2joy_init(&joy_config);

    /* Uncomment to run hardware test on startup */
    /* apple2joy_run_test(); */

    /* Initialize USB host with callbacks */
    tjuh_config_t usb_config = {
        .on_report     = on_gamepad_report,
        .on_connect    = on_gamepad_connect,
        .on_disconnect = on_gamepad_disconnect,
    };
    tjuh_init(&usb_config);

    while (1) {
        tuh_task();
    }

    return 0;
}
