/*
 * MP3_player.h
 *
 *  Created on: Jan 11, 2026
 *      Author: alexi
 */
#include "MP3_player.h"
#include "usart.h"

#define MP3_stack_header 0
#define MP3_stack_version 1
#define MP3_stack_length 2
#define MP3_stack_command 3
#define MP3_stack_ACK 4
#define MP3_stack_parameter_H 5
#define MP3_stack_parameter_L 6
#define MP3_stack_check_sum 7
#define MP3_stack_end 9
uint8_t MP3_stack[MP3_SEND_LENGTH] = { 0x7E, 0xFF, 06, 00, 00, 00, 00, 00, 00,
		0xEF };
uint8_t MP3_receive_stack[MP3_SEND_LENGTH];

uint16_t MP3_calculate_check_sum(uint8_t *buffer) {
	uint16_t sum = 0;
	for (int i = MP3_stack_version; i < MP3_stack_check_sum; i++) {
		sum += buffer[i];
	}
	return (-sum);
}

void MP3_send_stack() {
	// add the checksum to the message stack
	MP3_stack[MP3_stack_check_sum] = (uint8_t) (MP3_calculate_check_sum(
			MP3_stack) >> 8);
	MP3_stack[MP3_stack_check_sum + 1] = (uint8_t) (MP3_calculate_check_sum(
			MP3_stack));

	HAL_UART_Transmit(&huart6, MP3_stack, MP3_SEND_LENGTH, 10);
}
void MP3_send_command_with_param(uint8_t command, uint16_t parameter,
		uint8_t ack_required) {

	MP3_stack[MP3_stack_command] = command;
	MP3_stack[MP3_stack_ACK] = ack_required;
	MP3_stack[MP3_stack_parameter_H] = (uint8_t) (parameter >> 8);
	MP3_stack[MP3_stack_parameter_L] = (uint8_t) (parameter);

	MP3_send_stack();
}
void MP3_send_command(uint8_t command) {
	MP3_stack[MP3_stack_command] = command;
	MP3_send_stack();
}
void MP3_init() {
	MP3_send_command(MP3_COMMAND_RESET);
	osDelay(2000);
	MP3_send_command_with_param(MP3_COMMAND_SET_VOLUME, 18, 0x00);
	osDelay(100);
	MP3_set_playback_mode(0x00);
	osDelay(100);
}
uint8_t MP3_query_volume() {

	MP3_send_command_with_param(MP3_COMMAND_QUERY_VOLUME, 0x0000, 0x01);
	HAL_StatusTypeDef status = HAL_UART_Receive(&huart6, MP3_receive_stack,
			MP3_SEND_LENGTH, 50);

    if (status == HAL_OK) {
		if (MP3_receive_stack[MP3_stack_command] == MP3_COMMAND_QUERY_VOLUME) {
			return MP3_receive_stack[MP3_stack_parameter_L];
		}
	}
	return 0xFF;
}

uint8_t MP3_get_volume_status_string(char *buffer) {
	uint8_t volume = MP3_query_volume();
	if (volume != 0xFF) {
		snprintf(buffer, 15, "Volume: %u", volume);
	} else {
		snprintf(buffer, 15, "DFPlayer Error");
	}
	return volume;
}
void MP3_set_playback_mode(uint8_t mode) {
	MP3_send_command_with_param(0x11, 0x01, 0x00);

}
void MP3_play_track(uint8_t number) {
	MP3_send_command_with_param(0x03, number, 0x00);
}
void MP3_play_folder(uint8_t number) {
	MP3_send_command_with_param(0x0D, number, 0x00);
}
void MP3_play_sound_effect(uint16_t sound_effect_number) {
	MP3_send_command_with_param(MP3_COMMAND_PLAY_ADVERT, sound_effect_number,
			0x00);
}
