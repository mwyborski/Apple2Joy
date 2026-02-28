/*
 * MCP4251 — Dual 8-bit Digital Potentiometer (SPI)
 */

#include "mcp4251.h"

#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"

/* ------------------------------------------------------------------ */
/*  SPI chip-select helpers                                           */
/* ------------------------------------------------------------------ */

static inline void cs_select(void)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(void)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

void mcp4251_init(void)
{
    spi_init(spi0, 10 * 1000 * 1000);

    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN,  GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN,  GPIO_FUNC_SPI);

    bi_decl(bi_3pins_with_func(PICO_DEFAULT_SPI_RX_PIN,
                               PICO_DEFAULT_SPI_TX_PIN,
                               PICO_DEFAULT_SPI_SCK_PIN,
                               GPIO_FUNC_SPI));

    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);

    /* Enable both potentiometers via TCON register */
    uint8_t buf[2] = { MCP4251_REG_TCON, MCP4251_TCON_BOTH_ON };
    cs_select();
    spi_write_blocking(spi0, buf, 2);
    cs_deselect();
}

void mcp4251_deinit(void)
{
    /* Placeholder for future resource cleanup */
}

void mcp4251_write(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    cs_select();
    spi_write_blocking(spi0, buf, 2);
    cs_deselect();
}

void mcp4251_set_wipers(uint8_t wiper0, uint8_t wiper1)
{
    uint8_t buf[2];

    cs_select();

    buf[0] = MCP4251_REG_WIPER_0;
    buf[1] = wiper0;
    spi_write_blocking(spi0, buf, 2);

    buf[0] = MCP4251_REG_WIPER_1;
    buf[1] = wiper1;
    spi_write_blocking(spi0, buf, 2);

    cs_deselect();
}
