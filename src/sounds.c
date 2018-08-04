#include <math.h>
#include <SFML/Audio.h>
#include <stdio.h>
#include <string.h>
#include "header.h"

double	getNoteFrequency(char note)
{
	return (pow(2, (double)(note - 69) / 12.0) * 440);
}

int	findClosestBuffer(sfSoundBuffer *buffers[128], int startIndex)
{
	for (int i = 0; startIndex + i < 128 || startIndex - i >= 0; i++) {
		if (startIndex + i < 128 && buffers[startIndex + i])
			return (startIndex + i);
		if (startIndex - i >= 0 && buffers[startIndex - i])
			return (startIndex - i);
	}
	return (-1);
}

void	loadSounds(char *path, sfSound *sounds[2][128], sfSoundBuffer *soundBuffers[2][128], bool debug)
{
	char		buffer[1024];
	char		*note;
	double		pitch[128];	
	sfSoundBuffer	*buffers[128];

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 128; j++) {
			sounds[i][j] = NULL;
			soundBuffers[i][j] = NULL;
		}
	memset(pitch, 0, sizeof(pitch));
	for (int j = 0; j < 128; j++) {
		note = getNoteString(j);
		note[0] += 'a' - 'A';
		sprintf(buffer, "%ssounds/Grand Piano4/%smmell.wav", path, note);
		soundBuffers[0][j] = sfSoundBuffer_createFromFile(buffer);
		if (soundBuffers[0][j]) {
			sounds[0][j] = sfSound_create();
			if (sounds[0][j])
				sfSound_setBuffer(sounds[0][j], soundBuffers[0][j]);
		} else
			sounds[0][j] = NULL;
	}
	for (int i = 0; i < 128; i++)
		buffers[i] = soundBuffers[0][i];
	for (int j = 0, i = 0; j < 128; j++) {
		if (!soundBuffers[0][j]) {
			i = findClosestBuffer(buffers, j);
			if (i < 0) {
				printf("No sound could be loaded\n");
				return;
			}
			soundBuffers[0][j] = soundBuffers[0][i];
			if (soundBuffers[0][j]) {
				sounds[0][j] = sfSound_create();
				if (sounds[0][j]) {
					sfSound_setBuffer(sounds[0][j], soundBuffers[0][j]);
					sfSound_setPitch(sounds[0][j], getNoteFrequency(j) / getNoteFrequency(i));
					pitch[j] = getNoteFrequency(j) / getNoteFrequency(i);
					if (debug)printf("Creating %i from %i (Ratio: %f)\n", j, i, pitch[j]);
				}
			}
		}
	}
	for (int j = 0; j < 128; j++) {
		soundBuffers[1][j] = soundBuffers[0][j] ? sfSoundBuffer_copy(soundBuffers[0][j]) : NULL;
		if (soundBuffers[0][j]) {
			sounds[1][j] = sfSound_create();
			if (sounds[1][j])
				sfSound_setBuffer(sounds[1][j], soundBuffers[0][j]);
			if (pitch[j])
				sfSound_setPitch(sounds[1][j], pitch[j]);
		}
	}
	if (debug) {
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 128; j++) {
				if (!sounds[i][j]) {
					debug = false;
					printf("Sound[%i][%i] is not set !\n", i, j);
				}
			}
		}
		if (debug)
			printf("No sound problems found\n");
	}
}