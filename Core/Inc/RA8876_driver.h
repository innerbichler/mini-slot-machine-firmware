/*
 * RA8876_driver.h
 *
 *  Created on: Dec 26, 2025
 *      Author: alexi
 */

#ifndef INC_RA8876_DRIVER_H_
#define INC_RA8876_DRIVER_H_



void RA8876_PLL_init();
void RA8876_SDRAM_init();
void RA8876_display_init();

void RA8876_write_register(uint8_t, uint8_t);
uint8_t RA8876_read_register(uint8_t);
uint8_t RA8876_read_status();

void RA8876_display_on();
void RA8876_display_off();
void RA8876_color_bar_test_on();
void RA8876_color_bar_test_off();
void RA8876_clear_screen();
void RA8876_set_mode(uint8_t text_mode);

void RA8876_set_foreground_color(uint16_t color);
void RA8876_set_background_color(uint32_t color);
void RA8876_set_text_coordinates(uint16_t x0, uint16_t y0);

void RA8876_print(char *text);
void RA8876_draw_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
		uint16_t color);
void RA8876_draw_image_BTE(int16_t x, int16_t y, uint16_t width,
		uint16_t height,
		const uint16_t *imageData);
void RA8876_draw_mario(int x, int y);

// the four options of interacting with the chip
// A0 is bit 7 -> 0 for command/status, 1 for data
// WR is bit 6 -> 0 for write, 1 for read
#define RA8876_COMMAND_WRITE   0b00000000
#define RA8876_DATA_WRITE  0x80
#define RA8876_STATUS_READ 0b01000000
#define RA8876_DATA_READ   0b11000000

#define RA8876_CORE_FREQUENCY 100 // core Mhz
#define RA8876_OSC_FREQ     10	  // crystal clock
#define RA8876_DRAM_FREQ    100  // SDRAM clock frequency MHz
#define RA8876_SCAN_FREQ     50 // Panel Scan clock frequency MHz

#define RA8876_WIDTH 1024
#define RA8876_HEIGHT 600
enum RA8876_status {
	RA8876_INT_PIN_STATE = 0x01,
	RA8876_OPERATION_MODE = 0x02,
	RA8876_SDRAM_READY = 0x04,
	RA8876_CORE_BUSY = 0x08,
	RA8876_MEMR_FIFO_EMPTY = 0x10,
	RA8876_MEMR_FIFO_FULL = 0x20,
	RA8876_MEMW_FIFO_EMPTY = 0x40,
	RA8876_MEMW_FIFO_FULL = 0x80
};
enum RA8876_dispMode {
	GRAPHMODE = 0, TEXTMODE = 1
};

#endif /* INC_RA8876_DRIVER_H_ */
