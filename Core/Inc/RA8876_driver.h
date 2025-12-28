/*
 * RA8876_driver.h
 *
 *  Created on: Dec 26, 2025
 *      Author: alexi
 */

#ifndef INC_RA8876_DRIVER_H_
#define INC_RA8876_DRIVER_H_

uint8_t RA8876_read_register(uint8_t);
uint8_t RA8876_read_status_register();

void RA8876_PLL_init();
void RA8876_SDRAM_init();
void RA8876_display_init();
void RA8876_fill_red();
uint8_t RA8876_read();
void RA8876_write_register(uint8_t, uint8_t);

void RA8876_display_on();
void RA8876_display_off();
void RA8876_color_bar_test_on();
void RA8876_color_bar_test_off();



#endif /* INC_RA8876_DRIVER_H_ */
