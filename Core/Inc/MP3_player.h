/*
 * MP3_player.h
 *
 *  Created on: Jan 11, 2026
 *      Author: alexi
 */

#ifndef INC_MP3_PLAYER_H_
#define INC_MP3_PLAYER_H_
#include <stdint.h>

void MP3_play_track(uint8_t number);
void MP3_play_sound_effect(uint16_t sound_effect_number);
void MP3_send_command_with_param(uint8_t command, uint16_t parameter,
		uint8_t ack_required);
uint8_t MP3_query_volume();
uint8_t MP3_get_volume_status_string(char *buffer);
void MP3_init(void);

#define MP3_COMMAND_PLAY_TRACK  0x03
#define MP3_COMMAND_RESET       0x0C
#define MP3_COMMAND_NEXT_TRACK  0x01
#define MP3_COMMAND_SET_VOLUME  0x06
#define MP3_COMMAND_QUERY_VOLUME 0x44
#define MP3_COMMAND_PLAY_ADVERT 0x13
#define MP3_COMMAND_SET_PLAYBACK_MODE 0x19


#define MP3_SEND_LENGTH 10
#endif /* INC_MP3_PLAYER_H_ */
