/*
 * RA8876_driver.c
 *
 *  Created on: Dec 26, 2025
 *      Author: alexi
 */
#include "gpio.h";
#include "spi.h";
#include "RA8876_reg.h"

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


void RA8876_write(uint8_t write_command, uint8_t data) {
	uint8_t buf[2] = { write_command, data };
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, buf, sizeof(buf), 10);
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_SET);
}

uint8_t RA8876_read(uint8_t read_command) {
	uint8_t msg = 123;
	uint8_t tx_buf[2] = { read_command, 0x00 };
	uint8_t rx_buf[2] = { 123, 123 };
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, tx_buf, 2, 10);
	HAL_SPI_Receive(&hspi2, rx_buf, 2, 10);
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_SET);
	return (rx_buf[1]);
}
void RA8876_SDRAM_init() {

	// configure the sdram, after this function you should
	// wait for the E4 Bit 0x01 to change because i dont
	// want a possible infinite loop in lib code

	// i have W9812G6KH-6 RAM
	// 16Bit 4 Banks 2M Words so

	// attribute reg 0 0 1 01 001 -> 0x29
	// row is A0-A11 and col A0-A8
	RA8876_write_register(0xE0, 0x29);

	//mode and extended mode reg 000 00 011
	// only CAS latency is 3 rest is for mobile sdram (not what i have)
	RA8876_write_register(0xE1, 0x03);

	// auto refresh interval reg 0 and 1 E2 is low-byte send first
	// dram frequency will be same as core clock
	uint16_t tref = 64; // ms
	uint16_t row_size = 4096; // 12 Bit
	uint16_t auto_refresh_frequency = (tref * RA8876_CORE_FREQUENCY * 1000)
			/ (row_size); // the Mega and the milli give us 1000
	auto_refresh_frequency = auto_refresh_frequency - 2; // saw in other lib

	uint8_t low_byte = (uint8_t) (auto_refresh_frequency & 0x00FF);
	uint8_t high_byte = (uint8_t) (auto_refresh_frequency >> 8);
	RA8876_write_register(0xE2, low_byte);
	RA8876_write_register(0xE3, high_byte);
	
	// control register
	// 16bit  and 0x01 just starts initialisation
	RA8876_write_register(0xE4, 0x01);

}

void RA8876_display_init() {
	// setup the display, tell it width and height

	// main window control register
	// 0 0 0 0 1x 0 0 -> 0x08
	// 24 bit colors, i think the display can do even more ~ 16mio
	RA8876_write_register(0x10, 0x0C);

	// ignoring 0x11 because thats the pip stuff
	RA8876_write_register(0x11, 0x60);

	// display configuration register
	// 1 0 0 0 0 00 -> 0x80 use 0xA0 to test colorbar
	RA8876_write_register(0x12, 0x00);

	// 0x13 panel scan clock and data settings register leave as is

	//horizontal display width in unit of 8 pixels
	uint8_t width = (1024 / 8) - 1;
	RA8876_write_register(0x14, width);

	// 0x15 horizontal width fine tune register leave

	// 0x16 horizontal non display period register leave
	uint8_t HND = 160;
	uint8_t hsync_non_display = (HND / 8) - 1;
	RA8876_write_register(0x16, hsync_non_display);
	// 0x17 horizontal non display fine tune leave

	// 0x18 HSYNC Start position
	uint8_t HST = 16;
	uint8_t hsync_start_position = (HST / 8) - 1;
	RA8876_write_register(0x18, hsync_start_position);

	// 0x19 HSYNC pulse width register
	uint8_t HPW = 48;
	uint8_t hsync_pulse_width = (HPW / 8) - 1;
	RA8876_write_register(0x19, hsync_pulse_width);

	// vertical display height register 1 and 2
	// watch out 0x1B are only 2 bits
	uint16_t height = 600 - 1;
	uint8_t low_byte = (uint8_t) (height & 0x00FF);
	uint8_t high_byte = (uint8_t) (height >> 8);
	RA8876_write_register(0x1A, low_byte);
	RA8876_write_register(0x1B, high_byte);

	// 0x1C vertical non-display period 0 register leave
	uint8_t VND = 30;
	uint8_t VSYNC_non_display = VND - 1;
	RA8876_write_register(0x1C, VSYNC_non_display);
	// 0x1D vertical non-display period 1 register leave

	// 0x1E vertical VSYNC start position register leave
	uint8_t VST = 20;
	uint8_t VSYNC_start_position = VST - 1;
	RA8876_write_register(0x1E, VSYNC_start_position);

	// 0x1F vertical pulse width register leave
	uint8_t VPW = 10;
	uint8_t VSYNC_pulse_width = VPW - 1;
	RA8876_write_register(0x1F, VSYNC_pulse_width);

	// color depth of canvas AND active window
	// 0000 0 0 11
	RA8876_write_register(0x5E, 3);

	RA8876_write_register(0x5C, 0x00);
	RA8876_write_register(0x5D, 0x04);

}

void RA8876_PLL_init() {
	// copied for the most part from a library from gfcwfzkm

	// core pll clock speed -> want
	RA8876_write_register(0x05, RA8876_PPLLC1_EXT_DIV8_gc);
	RA8876_write_register(0x06, (RA8876_SCAN_FREQ * 8 / RA8876_OSC_FREQ) - 1);
	// dram frequency
	RA8876_write_register(0x07, RA8876_MPLLC1_DIVK1_gc);
	RA8876_write_register(0x08, (RA8876_DRAM_FREQ * 4 / RA8876_OSC_FREQ) - 1);

	// SPLL
	RA8876_write_register(0x09, RA8876_SPLLC1_DIVK_1);
	RA8876_write_register(0x0A,
			(RA8876_CORE_FREQUENCY * 4 / RA8876_OSC_FREQ) - 1);

	// 5. Apply the clocks
	HAL_Delay(2);
	RA8876_write_register(0x01, 0x80);
	HAL_Delay(5);


}

void RA8876_write_register(uint8_t register_address, uint8_t data) {
	// write data to a register
	RA8876_write(RA8876_COMMAND_WRITE, register_address);
	RA8876_write(RA8876_DATA_WRITE, data);

}
uint8_t RA8876_read_status_register() {
	uint8_t status = 0;
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, RA8876_STATUS_READ, 1, 10);
	HAL_SPI_Receive(&hspi2, &status, 1, 10);
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_SET);
	return (status);
}

uint8_t RA8876_read_register(uint8_t register_address) {
	uint8_t tx_buf[2];
	uint8_t rx_buf[2] = { 0, 0 };

	RA8876_write(RA8876_COMMAND_WRITE, register_address);
	HAL_Delay(1);
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_RESET);
	tx_buf[0] = 0xC0;
	tx_buf[1] = 0x00;

	HAL_SPI_TransmitReceive(&hspi2, tx_buf, rx_buf, 2, 10);
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_SET);

	return rx_buf[1];
}		


//void RA8876_drawPixel(uint16_t x0, uint16_t y0, uint16_t color) {
//
//	RA8876_setMode(GRAPHMODE);
//	RA8876_setPixelCoords(x0, y0);
//	RA8876_writeCMD(RA8876_MRWDP);
//	RA8876_writeData(color);
//	RA8876_writeData(color >> 8);
//}

void RA8876_display_on() {
	RA8876_write_register(0x12, 0x40);
}
void RA8876_display_off() {
	RA8876_write_register(0x12, 0x00);
}
void RA8876_color_bar_test_on() {
	RA8876_write_register(0x12, 0x60);
}
void RA8876_color_bar_test_off() {
	RA8876_write_register(0x12, 0x40);
}




