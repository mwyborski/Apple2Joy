/*
 * MCP4251 — Dual 8-bit Digital Potentiometer (SPI)
 *
 * Datasheet: Microchip MCP4251
 * References:
 *   https://github.com/kulbhushanchand/MCP4251
 *   https://gist.github.com/matt448/9612191
 */

#ifndef MCP4251_H
#define MCP4251_H

#include <stdint.h>
#include "hardware/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Register addresses                                                */
/* ------------------------------------------------------------------ */

#define MCP4251_REG_WIPER_0   0x00
#define MCP4251_REG_WIPER_1   0x10
#define MCP4251_REG_TCON      0x40
#define MCP4251_REG_STATUS    0x50

/* ------------------------------------------------------------------ */
/*  Command bits                                                      */
/* ------------------------------------------------------------------ */

#define MCP4251_CMD_WRITE     0x00
#define MCP4251_CMD_READ      0x0C
#define MCP4251_CMD_INCREMENT 0x04
#define MCP4251_CMD_DECREMENT 0x08

/* ------------------------------------------------------------------ */
/*  Terminal connection (TCON) presets                                 */
/* ------------------------------------------------------------------ */

#define MCP4251_TCON_BOTH_OFF  0x00
#define MCP4251_TCON_POT0_ON   0x0F
#define MCP4251_TCON_POT1_ON   0xF0
#define MCP4251_TCON_BOTH_ON   0xFF

/* ------------------------------------------------------------------ */
/*  API                                                               */
/* ------------------------------------------------------------------ */

/**
 * Initialize SPI and configure the MCP4251 with both pots enabled.
 * Uses spi0 and the default Pico SPI pins.
 */
void mcp4251_init(void);

/**
 * Release SPI resources (currently a no-op placeholder).
 */
void mcp4251_deinit(void);

/**
 * Set wiper positions for both potentiometers.
 *
 * @param wiper0  Position for potentiometer 0 (0–255)
 * @param wiper1  Position for potentiometer 1 (0–255)
 */
void mcp4251_set_wipers(uint8_t wiper0, uint8_t wiper1);

/**
 * Set a single wiper position.
 *
 * @param reg    Register: MCP4251_REG_WIPER_0 or MCP4251_REG_WIPER_1
 * @param value  Wiper position (0–255)
 */
void mcp4251_write(uint8_t reg, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* MCP4251_H */
