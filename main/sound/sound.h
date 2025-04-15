#ifndef SOUND_H
#define SOUND_H

#include <stdlib.h>

extern uint8_t* urna_sound_long;  // Global variable for storing the WAV data
extern size_t urna_sound_long_size;  // Global variable for storing the size of the WAV data

extern uint8_t* urna_sound_short;
extern size_t urna_sound_short_size;

void init_sound_semaphore();
void play_urna_sound_long();
void play_urna_sound_short();
uint8_t* load_sound_to_memory(const char* file_path, size_t* file_size);

#endif  // SOUND_H