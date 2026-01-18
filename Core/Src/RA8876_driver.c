/*
 * RA8876_driver.c
 *
 *  Created on: Dec 26, 2025
 *      Author: alexi
 */
#include "gpio.h";
#include "spi.h";
#include "RA8876_driver.h"
#include "RA8876_reg.h"


enum RA8876_dispMode _text_mode = GRAPHMODE;
uint8_t _symbol = 0;
uint16_t _color = 0;
void RA8876_write(uint8_t write_command, uint8_t data) {
	uint8_t buf[2] = { write_command, data };
	HAL_SPI_Transmit(&hspi5, buf, sizeof(buf), 10);
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
	auto_refresh_frequency = 1000 - 2; // saw in other lib

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

	// 0x01 chip configuration register
	// we need to switch Bit 4 so we select 16Bit
	// 0 0 0 10 0 0 1 = 0x11
	RA8876_write_register(0x01, 0x11);

	// 0x02 memory access control register
	// 01 00 0 00 0 -> 0x40
	RA8876_write_register(0x02, 0x40);

	// 0x03 input control register
	RA8876_write_register(0x03, 0x00);


	// main window control register
	// 0 0 0 0 01 0 0 -> 0x08 or 0x0C
	// 24 bit colors, i think the display can do even more ~ 16mio
	// will do 16 instead
	RA8876_write_register(0x10, 0x04);

	// ignoring 0x11 because thats the pip stuff
	RA8876_write_register(0x11, 0x00);

	// display configuration register
	// 1	 0 0 0 1 00 -> 0x80 use 0x60 to test colorbar
	RA8876_write_register(0x12, 0x80);

	// 0x13 panel scan clock and data settings register
	RA8876_write_register(0x13, 0xC0);

	//horizontal display width in unit of 8 pixels
	uint8_t width = (RA8876_WIDTH / 8) - 1;
	RA8876_write_register(0x14, width);

	//0x15 horizontal width fine tune register leave
	RA8876_write_register(0x15, width % 8);

	// 0x16 horizontal non display period register leave
	uint8_t HND = 144;
	uint8_t hsync_non_display = (HND / 8) - 1;
	RA8876_write_register(0x16, hsync_non_display);
	// 0x17 horizontal non display fine tune leave
	RA8876_write_register(0x17, HND % 8);

	// 0x18 HSYNC Start position
	uint8_t HST = 160;
	uint8_t hsync_start_position = (HST / 8) - 1;
	RA8876_write_register(0x18, hsync_start_position);

	// 0x19 HSYNC pulse width register
	uint8_t HPW = 20;
	uint8_t hsync_pulse_width = (HPW / 8) - 1;
	RA8876_write_register(0x19, hsync_pulse_width);

	// vertical display height register 1 and 2
	// watch out 0x1B are only 2 bits
	uint16_t height = RA8876_HEIGHT - 1;
	uint8_t low_byte = (uint8_t) (height & 0x00FF);
	uint8_t high_byte = (uint8_t) (height >> 8);
	RA8876_write_register(0x1A, low_byte);
	RA8876_write_register(0x1B, high_byte);

	// 0x1C vertical non-display period 0 register leave
	uint8_t VND = 20;
	uint8_t VSYNC_non_display = VND - 1;
	RA8876_write_register(0x1C, VSYNC_non_display);
	// 0x1D vertical non-display period 1 register leave
	RA8876_write_register(0x1D, VSYNC_non_display >> 8);


	// 0x1E vertical VSYNC start position register leave
	uint8_t VST = 12;
	uint8_t VSYNC_start_position = VST - 1;
	RA8876_write_register(0x1E, VSYNC_start_position);

	// 0x1F vertical pulse width register leave
	uint8_t VPW = 3;
	uint8_t VSYNC_pulse_width = VPW - 1;
	RA8876_write_register(0x1F, VSYNC_pulse_width);

	// next up is the canvas configuration
	// 0x50 - 0x53 canvas start address 0 and 1
	RA8876_write_register(0x50, 0x00);
	RA8876_write_register(0x51, 0x00);
	RA8876_write_register(0x52, 0x00);
	RA8876_write_register(0x53, 0x00);
	// i'll leave this at 0 (seems fine)

	// 0x54 canvas image width 0 and 1
	// it is in 4pixel resolutions? in 11 bits
	uint16_t canvas_width = RA8876_WIDTH;

	// this vodoo was the problem
	uint8_t lower_six = (uint8_t) ((canvas_width & 0b0000000000111111) << 2);
	uint8_t higher_five = (uint8_t) ((canvas_width & 0b0000011111000000) >> 6);

	low_byte = (uint8_t) (canvas_width & 0x00FF);
	high_byte = (uint8_t) (canvas_width >> 8);
	RA8876_write_register(0x54, low_byte);
	RA8876_write_register(0x55, high_byte);



	// 0x56 0x57 active window upper left corner x leave as 0
	// 0x58 0x59 active window upper left corner y leave as 0

	// 0x5A 0x5B active window width 0 1
	uint16_t window_width = RA8876_WIDTH;
	low_byte = (uint8_t) (window_width & 0x00FF);
	high_byte = (uint8_t) (window_width >> 8);
	RA8876_write_register(0x5A, low_byte);
	RA8876_write_register(0x5B, high_byte);

	// 0x5C 0x5D active window height 0 1
	uint16_t window_height = RA8876_HEIGHT;
	low_byte = (uint8_t) (window_height & 0x00FF);
	high_byte = (uint8_t) (window_height >> 8);
	RA8876_write_register(0x5C, low_byte);
	RA8876_write_register(0x5D, high_byte);

	// color depth of canvas AND active window
	// 0000 0 0 01 -> 16 Bit
	RA8876_write_register(0x5E, 0x01);
	RA8876_write_register(0x10, 0x04);


	// 0x20 - 0x23 main image start address we leave at 0 for now
	RA8876_write_register(0x20, 0x00);
	RA8876_write_register(0x21, 0x00);
	RA8876_write_register(0x22, 0x00);
	RA8876_write_register(0x23, 0x00);

	//0x24 main image width 0 and 1
	window_width = RA8876_WIDTH;
	low_byte = (uint8_t) (window_width & 0x00FF);
	high_byte = (uint8_t) (window_width >> 8);
	RA8876_write_register(0x24, low_byte);
	RA8876_write_register(0x25, high_byte);
}

void RA8876_PLL_init() {
	// copied for the most part from a library from gfcwfzkm
	// but i needed to use different divisors
	// core pll clock speed -> want
	RA8876_write_register(RA8876_PPLLC1, 0x06);
	RA8876_write_register(RA8876_PPLLC2,
			(RA8876_SCAN_FREQ * 8 / RA8876_OSC_FREQ) - 1);
	// dram frequency
	RA8876_write_register(RA8876_MPLLC1, 0x04);
	RA8876_write_register(RA8876_MPLLC2,
			(RA8876_DRAM_FREQ * 4 / RA8876_OSC_FREQ) - 1);

	// SPLL
	RA8876_write_register(RA8876_SPLLC1, RA8876_SPLLC1_DIVK_1);
	RA8876_write_register(RA8876_SPLLC2,
			(RA8876_CORE_FREQUENCY * 4 / RA8876_OSC_FREQ) - 1);

	HAL_Delay(2);
	RA8876_write_register(RA8876_CCR, 0x80);
	HAL_Delay(5);

}

void RA8876_write_register(uint8_t register_address, uint8_t data) {
	// write data to a register
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_RESET);
	RA8876_write(RA8876_COMMAND_WRITE, register_address);
	RA8876_write(RA8876_DATA_WRITE, data);
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_SET);
}
uint8_t RA8876_read_status_register() {
	uint8_t status = 0;
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi5, RA8876_STATUS_READ, 1, 10);
	HAL_SPI_Receive(&hspi5, &status, 1, 10);
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_SET);
	return (status);
}

uint8_t RA8876_read_register(uint8_t register_address) {
	uint8_t tx_buf[2];
	uint8_t rx_buf[2] = { 0, 0 };

	uint8_t buf[2] = { RA8876_COMMAND_WRITE, register_address };
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi5, buf, sizeof(buf), 10);
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_RESET);
	tx_buf[0] = 0xC0;
	tx_buf[1] = 0x00;

	HAL_SPI_TransmitReceive(&hspi5, tx_buf, rx_buf, 2, 10);
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_SET);

	return rx_buf[1];
}		

void RA8876_set_mode(enum RA8876_dispMode text_mode) {
	// Activate Text Mode
	uint8_t reg_temp;
	reg_temp = RA8876_read_register(RA8876_ICR);
	if (text_mode) {
		reg_temp |= RA8876_ICR_TEXT_MODE_EN;
	} else {
		reg_temp &= ~RA8876_ICR_TEXT_MODE_EN;
	}

	RA8876_write_register(RA8876_ICR, reg_temp);
	_text_mode = text_mode;
}


void RA8876_display_on() {
	uint8_t content = RA8876_read_register(0x12);
	content |= 0x40;
	RA8876_write_register(0x12, content);
}
void RA8876_display_off() {
	RA8876_write_register(0x12, 0x00);
}
void RA8876_color_bar_test_on() {
	uint8_t content = RA8876_read_register(0x12);
	content |= 0x60;
	RA8876_write_register(0x12, content);
}
void RA8876_color_bar_test_off() {
	RA8876_display_on();
}

void RA8876_set_point_1_and_2(uint16_t x_start, uint16_t y_start,
		uint16_t x_end, uint16_t y_end) {
	// set the points of Line Square Triangle
	// for triangle you also need to set point 3
	// 12 Bit in total
	uint8_t low_byte = (uint8_t) (x_start & 0x00FF);
	uint8_t high_byte = (uint8_t) (x_start >> 8);
	RA8876_write_register(RA8876_DLHSR0, low_byte);
	RA8876_write_register(RA8876_DLHSR1, high_byte);
	low_byte = (uint8_t) (y_start & 0x00FF);
	high_byte = (uint8_t) (y_start >> 8);
	RA8876_write_register(RA8876_DLVSR0, low_byte);
	RA8876_write_register(RA8876_DLVSR1, high_byte);
	// end point
	low_byte = (uint8_t) (x_end & 0x00FF);
	high_byte = (uint8_t) (x_end >> 8);
	RA8876_write_register(RA8876_DLHER0, low_byte);
	RA8876_write_register(RA8876_DLHER1, high_byte);
	low_byte = (uint8_t) (y_end & 0x00FF);
	high_byte = (uint8_t) (y_end >> 8);
	RA8876_write_register(RA8876_DLVER0, low_byte);
	RA8876_write_register(RA8876_DLVER1, high_byte);
}
void RA8876_clear_screen() {
	RA8876_draw_rectangle(0, 0, RA8876_WIDTH, RA8876_HEIGHT,
			RA8876_BACKGROUND_PRIMARY_COLOR, 1);
}
void RA8876_draw_circle(uint16_t x_start, uint16_t y_start,
		uint16_t major_radius, uint16_t color, uint8_t filled) {
	if (_text_mode)
		RA8876_set_mode(GRAPHMODE);

	// 0x7B 0x7C 0x7D 0x7E set the center of the circle
	uint8_t low_byte = (uint8_t) (x_start & 0x00FF);
	uint8_t high_byte = (uint8_t) (x_start >> 8);
	RA8876_write_register(0x7B, low_byte);
	RA8876_write_register(0x7C, high_byte);
	low_byte = (uint8_t) (y_start & 0x00FF);
	high_byte = (uint8_t) (y_start >> 8);
	RA8876_write_register(0x7D, low_byte);
	RA8876_write_register(0x7E, high_byte);

	// 0x77 0x78 0x79 0x7A set major and minor radius
	RA8876_write_register(0x77, major_radius);
	RA8876_write_register(0x78, major_radius >> 8);
	RA8876_write_register(0x79, major_radius);
	RA8876_write_register(0x7A, major_radius >> 8);

	RA8876_set_foreground_color(color);
	// draw 0x76
	// bit5= 0 bit4= 0 bit6 is fill bit7 is start!
	// 11 00 0000 = 0xB0 or 0x80 for not fill
	if (filled) {
		RA8876_write_register(0x76, 0xC0);
	} else {
		RA8876_write_register(0x76, 0x80);

	}
	// Wait until drawing is done
	while (RA8876_read_register(0x76) & 0x80) {
	}
}
void RA8876_draw_diamond(uint16_t x_center, uint16_t y_center, uint16_t height,
		uint16_t width, uint16_t color, uint16_t filled) {
	// build a diamond made up of two triangles
	// height is the complete height, same with width
	uint16_t y_1 = y_center - (height / 2);
	uint16_t y_2 = y_center + (height / 2);
	uint16_t x_2 = x_center - (width / 2);
	uint16_t x_3 = x_center + (width / 2);
	// upper triangle
	RA8876_draw_triangle(x_center, y_1, x_2, y_center, x_3, y_center, color,
			filled);
	RA8876_draw_triangle(x_2, y_center, x_center, y_2, x_3, y_center, color,
			filled);
}

void RA8876_draw_triangle(uint16_t x_1, uint16_t y_1, uint16_t x_2,
		uint16_t y_2, uint16_t x_3, uint16_t y_3, uint16_t color,
		uint8_t filled) {
	if (_text_mode)
		RA8876_set_mode(GRAPHMODE);

	RA8876_set_point_1_and_2(x_1, y_1, x_2, y_2);
	// 0x70 0x71 0x72 0x73 point 3 of triangle
	uint8_t low_byte = (uint8_t) (x_3 & 0x00FF);
	uint8_t high_byte = (uint8_t) (x_3 >> 8);
	RA8876_write_register(0x70, low_byte);
	RA8876_write_register(0x71, high_byte);
	low_byte = (uint8_t) (y_3 & 0x00FF);
	high_byte = (uint8_t) (y_3 >> 8);
	RA8876_write_register(0x72, low_byte);
	RA8876_write_register(0x73, high_byte);
	RA8876_set_foreground_color(color);

	// on triangle fill is Bit 5
	// 1 0 1 000 10
	if (filled) {
		RA8876_write_register(0x67, 0xA2);
	} else {
		RA8876_write_register(0x67, 0x82);

	}
	while (RA8876_read_register(0x67) & 0x80) {
	}
}
void RA8876_draw_rectangle(uint16_t x_start, uint16_t y_start, uint16_t x_end,
		uint16_t y_end,
		uint16_t color, uint8_t filled) {

	if (_text_mode)
		RA8876_set_mode(GRAPHMODE);

	RA8876_set_point_1_and_2(x_start, y_start, x_end, y_end);
	RA8876_set_foreground_color(color);

	// draw 0x76
	// bit5= 1 bit4= 0 bit6 is fill bit7 is start!
	// 11 10 0000 = 0xE0 or 0xA0 for not fill
	if (filled) {
	RA8876_write_register(0x76, 0xE0);
	} else {
		RA8876_write_register(0x76, 0xA0);

	}
	// Wait until drawing is done
	while (RA8876_read_register(0x76) & 0x80) {
	}
}
uint32_t RA8876_color_from_RGB(uint8_t red, uint8_t green, uint8_t blue,
		uint8_t alpha) {
	// helper function to mush together the values because i want all
	// functions to use uint32_t for color
	uint32_t color = (uint32_t) alpha << 24 | (uint32_t) red << 16
			| (uint32_t) green << 8 | (uint32_t) blue;
	return (color);
}

void RA8876_set_foreground_color(uint16_t color) {
	// set foreground color, we are 16 Bit colores
	// Red:   5 bits (bits 15-11)
	// Green: 6 bits (bits 10-5)
	// Blue:  5 bits (bits 4-0)
	uint8_t red = (uint8_t) ((color >> 11) & 0x1F) << 3;
	uint8_t green = (uint8_t) ((color >> 5) & 0x3F) << 2;
	uint8_t blue = (uint8_t) (color & 0x1F) << 3;

	RA8876_write_register(RA8876_FGCR, red);
	RA8876_write_register(RA8876_FGCG, green);
	RA8876_write_register(RA8876_FGCB, blue);
}
void RA8876_set_background_color(uint32_t color) {
	uint8_t red = (uint8_t) ((color & 0x00FF0000) >> 16);
	uint8_t green = (uint8_t) ((color & 0x0000FF00) >> 8);
	uint8_t blue = (uint8_t) (color & 0x000000FF);
	RA8876_write_register(RA8876_BGCR, red);
	RA8876_write_register(RA8876_BGCG, green);
	RA8876_write_register(RA8876_BGCB, blue);
}
void RA8876_write_data_16bit(uint16_t data) {
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_RESET);
	RA8876_write(RA8876_DATA_WRITE, (uint8_t) (data >> 8));
	RA8876_write(RA8876_DATA_WRITE, (uint8_t) (data & 0xFF));
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_SET);

}
void RA8876_draw_mario(int x, int y) {
	RA8876_draw_image_BTE(x, y, 16, 16, mario_16x16);
}
void RA8876_draw_image_BTE(int16_t x, int16_t y, uint16_t width,
		uint16_t height,
		const uint16_t *imageData) {

	// 9D 9E 9F 9A Source1 memory start address 0-3
//	RA8876_write_register(0x9D, 0x00);
//	RA8876_write_register(0x9E, 0x00);
//	RA8876_write_register(0x9F, 0x00);
//	RA8876_write_register(0xA0, 0x00);

	// A1 A2 set image Width
	RA8876_write_register(0xA1, (uint8_t) (RA8876_WIDTH & 0xFF));
	RA8876_write_register(0xA2, (uint8_t) (RA8876_WIDTH >> 8));

	// A3 A4 is X pos A5 A6 is Y Source1
	RA8876_write_register(0xA3, (uint8_t) (x & 0xFF));
	RA8876_write_register(0xA4, (uint8_t) (x >> 8));
	RA8876_write_register(0xA5, (uint8_t) (y & 0xFF));
	RA8876_write_register(0xA6, (uint8_t) (y >> 8));

	// A7 A8 A9 AA is destination memory address
	RA8876_write_register(0xA7, 0x00);
	RA8876_write_register(0xA8, 0x00);
	RA8876_write_register(0xA9, 0x00);
	RA8876_write_register(0xAA, 0x00);

	// AB AC destination image width
	RA8876_write_register(0xAB, RA8876_WIDTH & 0xFF);
	RA8876_write_register(0xAC, RA8876_WIDTH >> 8);

	// AD AE is destination X pos and AF B0 is destination Y
	RA8876_write_register(0xAD, (uint8_t) (x & 0xFF));
	RA8876_write_register(0xAE, (uint8_t) (x >> 8));
	RA8876_write_register(0xAF, (uint8_t) (y & 0xFF));
	RA8876_write_register(0xB0, (uint8_t) (y >> 8));

	// B1 B2 is BTE width register
	RA8876_write_register(0xB1, (width) & 0xFF);
	RA8876_write_register(0xB2, (width) >> 8);
	// B3 B4 is BTE height register
	RA8876_write_register(0xB3, (height) & 0xFF);
	RA8876_write_register(0xB4, (height) >> 8);


	// 92 source1/destination color depth
	// 0 01 001 01 -> 0x25 for 16 bit everywhere
	RA8876_write_register(0x92, 0x25);

	// 0x91 BTE function control register 1
	// 1100 0000 -> 0xF0 means MPU WRITE with source0 from mpu
	RA8876_write_register(0x91, 0xC0);

	// 0x90 BTE function control register 0
	// 000 1 00 0 = 0x10
	RA8876_write_register(0x90, 0x10);

	RA8876_write_register(0x04, 0x00); 

	// we have to do sending manually because it would interpret sent
	// commands as data, we just have to stream it
	uint8_t buf[2] = { 0, 0 };
	uint8_t test[1] = { RA8876_DATA_WRITE };
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi5, test, sizeof(1), 10);
	for (uint32_t i = 0; i < (width * height); i++) {

		buf[0] = (uint8_t) (imageData[i] & 0x00ff);
		buf[1] = (uint8_t) (imageData[i] >> 8);

		HAL_SPI_Transmit(&hspi5, buf, sizeof(buf), 10);
		// we need to add some delay so the display doesnt lose info
		if ((i % 512) == 0) {
			osDelay(1);
		}

	}
	HAL_GPIO_WritePin(display_CS_GPIO_Port, display_CS_Pin, GPIO_PIN_SET);


	while (RA8876_read_register(0x90) & 0x10) {
		osDelay(5);
	}
}
void RA8876_SLOT_clear() {
	// clear the center to be ready for the next run
	// just a rectangle in the middle

	uint16_t width = 1024;
	RA8876_draw_rectangle(0, 128, width, 600 - 128,
			RA8876_BACKGROUND_PRIMARY_COLOR, 1);
}

uint8_t RA8876_SLOT_draw_roll(uint8_t number,
		uint8_t filled) {
	// call this to draw a symbol (shape) into 0-2 abschnitte
	// this draws purely the shapes.
	uint16_t width = (1024 / 3);
	uint16_t shape_size = 200;
	uint16_t x = ((width / 2) - (shape_size / 2) + (width * number) + 5);
	uint16_t y = ((RA8876_HEIGHT / 2) - (shape_size / 2));
	uint8_t symbols[5] = { TREE, PRESENT, DIAMOND, PACMAN, SNOWMAN };
	uint16_t colors[3] = { RA8876_FOREGROUND_PRIMARY_COLOR,
	RA8876_FOREGROUND_SECONDARY_COLOR, RA8876_FOREGROUND_TERITARY_COLOR };
	RA8876_SLOT_draw_symbol(x, y, shape_size, symbols[_symbol], colors[_color],
			filled,
			1);
	uint8_t drawn_symbol = symbols[_symbol];
	_symbol++;
	if (_symbol > 4) {
		_symbol = 0;
	}
	_color++;
	if (_color > 2) {
		_color = 0;
	}
	return (drawn_symbol);
}

void RA8876_draw_pacman(uint16_t x, uint16_t y, uint16_t shape_size) {
	RA8876_draw_circle(x, y, shape_size, 0xffe0, 1);
	uint8_t eye_x = shape_size / 2;
	uint8_t eye_y = shape_size / 2;

	RA8876_draw_circle(x, y - eye_y, shape_size / 6,
			RA8876_BACKGROUND_PRIMARY_COLOR,
			1);
	uint8_t mouth_x = shape_size + 2;
	uint8_t mouth_y = shape_size + 2;
	RA8876_draw_triangle(x, y, x + mouth_x, y + mouth_y, x + mouth_x,
			y - mouth_y,
			RA8876_BACKGROUND_PRIMARY_COLOR, 1);
}
void RA8876_draw_tree(uint16_t x, uint16_t y, uint16_t shape_size) {
	RA8876_draw_triangle(x, y - (shape_size / 2), x - (shape_size / 2),
			y + (shape_size / 4), x + (shape_size / 2), y + (shape_size / 4),
			0x01e0, 1);
	RA8876_draw_rectangle(x - (shape_size / 5), y + (shape_size / 4),
			x + (shape_size / 5), y + (shape_size / 2), 0x38e0, 1);
}
void RA8876_draw_present(uint16_t x, uint16_t y, uint16_t shape_size) {
	// just a square with a horizonzal and vertical rect
	// x and y are the center
	RA8876_draw_rectangle(x - (shape_size / 2), y - (shape_size / 2),
			x + (shape_size / 2), y + (shape_size / 2), 0x0780, 1);

	uint16_t wrapping_width = shape_size / 4;
	RA8876_draw_rectangle(x - (wrapping_width / 2),
			y - (shape_size / 2),
			x + (wrapping_width / 2), y + (shape_size / 2), 0xf800,
			1);
	RA8876_draw_rectangle(x - (shape_size / 2), y - (wrapping_width / 2),
			x + (shape_size / 2), y + (wrapping_width / 2), 0xf800, 1);
}
void RA8876_draw_snowman(uint16_t x, uint16_t y, uint16_t shape_size) {
	// shapesize is the distance to the side of a rectangle equivalent
	// bottom circle is half of size
	uint16_t half_size = shape_size / 2;
	RA8876_draw_circle(x, y + half_size, shape_size / 2, 0xffff, 1);

	RA8876_draw_circle(x, y - (half_size / 2), shape_size / 4, 0xffff, 1);
	// the hat is the upper 1/4 we do shape_size / 5 to make the brim standout
	RA8876_draw_rectangle(x - (shape_size / 6), y - shape_size,
			x + (shape_size / 6),
			y - half_size,
			0x28c0, 1);
	// the brim
	RA8876_draw_rectangle(x - (shape_size / 4), y - half_size,
			x + (shape_size / 4), y - (shape_size / 3), 0x28c0, 1);

	// last the nose, he doesnt need eyes
	// starting point is the center of the head
	uint16_t head_y = y - (shape_size / 4);
	RA8876_draw_triangle(x, head_y, x + half_size, head_y + (shape_size / 10),
			x - (shape_size / 10), head_y + (shape_size / 8), 0xfca0, 1);
}
void RA8876_draw_death_start(uint16_t x, uint16_t y, uint16_t shape_size) {
	RA8876_draw_circle(x, y, shape_size, 0x630c, 1);

	uint16_t outer_ring = shape_size / 3;
	uint16_t inner_ring = shape_size / 2;
	RA8876_draw_circle(x + outer_ring, y - outer_ring, shape_size / 4, 0x18c3,
			1);
	RA8876_draw_circle(x + outer_ring, y - outer_ring, shape_size / 7, 0x1062,
			1);

	// remove some chunks from the left side
	uint16_t left_x = x - shape_size;
	RA8876_draw_rectangle(left_x, y - shape_size,
			left_x + outer_ring,
			y - outer_ring,
			RA8876_BACKGROUND_PRIMARY_COLOR, 1);


}

void RA8876_SLOT_draw_symbol(uint16_t x, uint16_t y, uint16_t shape_size,
		uint8_t symbol, uint16_t color, uint8_t filled, uint8_t clear) {
	// symbol helper, draw any of the symbols at starting x and y

	// just to clear
	if (clear) {
	RA8876_draw_rectangle(x, y, x + shape_size, y + shape_size,
			RA8876_BACKGROUND_PRIMARY_COLOR, 1);
	}
	switch (symbol) {
	case CIRCLE:

		y = y + shape_size / 2;
		x = x + shape_size / 2;
		RA8876_draw_circle(x, y, shape_size / 2, color, filled);
		break;
	case TRIANGLE:
		x = x + shape_size / 2;

		uint16_t x_2 = x - shape_size / 2;
		uint16_t y_2 = y + shape_size;
		uint16_t x_3 = x + shape_size / 2;
		uint16_t y_3 = y + shape_size;

		RA8876_draw_triangle(x, y, x_2, y_2, x_3, y_3, color, filled);
		break;
	case DIAMOND:
		x = x + shape_size / 2;
		y = y + shape_size / 2;

		RA8876_draw_diamond(x, y, shape_size, shape_size, color, filled);
		break;
	case PACMAN:
		y = y + shape_size / 2;
		x = x + shape_size / 2;
		RA8876_draw_pacman(x, y, shape_size / 2);
		break;
	case SNOWMAN:
		y = y + shape_size / 2;
		x = x + shape_size / 2;
		RA8876_draw_snowman(x, y, shape_size / 2);
		break;
	case PRESENT:
		y = y + shape_size / 2;
		x = x + shape_size / 2;
		RA8876_draw_present(x, y, shape_size);
		break;
	case TREE:
		y = y + shape_size / 2;
		x = x + shape_size / 2;
		RA8876_draw_tree(x, y, shape_size);
		break;
	default:
		RA8876_draw_rectangle(x, y, x + shape_size, y + shape_size, color,
				filled);

	}
}
void RA8876_fill_gradient_128x128(uint16_t *arr, uint16_t c1, uint16_t c2) {
	static const int8_t map[4] = { -2, 1, 2, -1 };

	int r1 = (c1 & 0xF800) >> 8;
	int g1 = (c1 & 0x07E0) >> 3;
	int b1 = (c1 & 0x001F) << 3;

	int r2 = (c2 & 0xF800) >> 8;
	int g2 = (c2 & 0x07E0) >> 3;
	int b2 = (c2 & 0x001F) << 3;

	for (int y = 0; y < 128; y++) {
		int ry = r1 + ((r2 - r1) * y) / 127;
		int gy = g1 + ((g2 - g1) * y) / 127;
		int by = b1 + ((b2 - b1) * y) / 127;

		for (int x = 0; x < 128; x++) {
			int d = map[((y & 1) << 1) | (x & 1)];

			int r = ry + d;
			int g = gy + d;
			int b = by + d;

			if (r < 0)
				r = 0;
			else if (r > 255)
				r = 255;
			if (g < 0)
				g = 0;
			else if (g > 255)
				g = 255;
			if (b < 0)
				b = 0;
			else if (b > 255)
				b = 255;

			arr[y * 128 + x] = (uint16_t) (((r & 0xF8) << 8) | ((g & 0xFC) << 3)
					| (b >> 3));
		}
	}
}
void RA8876_fill_bottom_gradient() {
	static uint16_t animBuffer[128 * 128];

	// first gradient is the top and bottom
	RA8876_fill_gradient_128x128(animBuffer, RA8876_BACKGROUND_PRIMARY_COLOR,
			RA8876_BACKGROUND_SECONDARY_COLOR);
	for (int i = 0; i < 8; i++) {
		RA8876_draw_image_BTE((i * 128), 600 - 128, 128, 128, animBuffer);


	}

	RA8876_fill_gradient_128x128(animBuffer, RA8876_BACKGROUND_SECONDARY_COLOR,
			RA8876_BACKGROUND_PRIMARY_COLOR);
	for (int i = 0; i < 8; i++) {
		RA8876_draw_image_BTE(i * 128, 0, 128, 128, animBuffer
);

	}
	int size = 32;
	uint8_t symbols[4] = { PRESENT, SNOWMAN, DIAMOND, TREE, };
	uint16_t symbol = 0;
	uint16_t colors[3] = { RA8876_FOREGROUND_PRIMARY_COLOR,
	RA8876_FOREGROUND_SECONDARY_COLOR, RA8876_FOREGROUND_TERITARY_COLOR };
	uint16_t color = 0;
	for (int i = 0; i < 16; i++) {
		RA8876_SLOT_draw_symbol((i * 64) + (size / 2), 600 - 64 - (size / 2),
				size, symbols[symbol], colors[color], 1, 0);
		RA8876_SLOT_draw_symbol((i * 64) + (size / 2), 64 - (size / 2),
				size,
				symbols[symbol], colors[color], 1, 0);
		symbol++;
		if (symbol > 3) {
			symbol = 0;
		}
		color++;
		if (color > 2) {
			color = 0;
		}
	}
}
