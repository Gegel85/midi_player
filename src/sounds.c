#include <math.h>
#include <SFML/Audio.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "header.h"

#define SAMPLE_COUNTS	44100 * 4
#define SAMPLE_RATE	44100

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

sfInt16	*createSquareSample(double frequency)
{
	static sfInt16	raw[SAMPLE_COUNTS];
	
	for (int i = 0; i < SAMPLE_COUNTS; i++)
		raw[i] = (110000 / frequency + 3500) * (sin(i * frequency * 2 * M_PI / SAMPLE_RATE) > 0 ? 1 : -1) * exp((float)-i / SAMPLE_RATE);
	return (raw);
}

sfInt16	*createSinSample(double frequency)
{
	static sfInt16	raw[SAMPLE_COUNTS];
	
	for (int i = 0; i < SAMPLE_COUNTS; i++)
		raw[i] = (110000 / frequency + 4000) * sin(i * frequency * 2 * M_PI / SAMPLE_RATE) * exp((float)-i / SAMPLE_RATE);
	return (raw);
}

sfInt16	*createSawtoothSample(double frequency)
{
	static sfInt16	raw[SAMPLE_COUNTS];
	
	for (int i = 0; i < SAMPLE_COUNTS; i++)
		raw[i] = (110000 / frequency + 3500) * ((float)(i * (int)frequency % SAMPLE_RATE * 2) / SAMPLE_RATE - 1) * exp((float)-i / SAMPLE_RATE);
	return (raw);
}

void	loadSounds(char *path, sfSound ***sounds, sfSoundBuffer *soundBuffers[2][128], bool debug, Instrument instrument)
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
		switch (instrument) {
		case PIANO:
			note = getNoteString(j);
			note[0] += 'a' - 'A';
			sprintf(buffer, "%ssounds/Grand Piano4/%smmell.wav", path, note);
			soundBuffers[0][j] = sfSoundBuffer_createFromFile(buffer);
			break;
		case SQUARE:
			soundBuffers[0][j] = sfSoundBuffer_createFromSamples(createSquareSample(getNoteFrequency(j)), SAMPLE_COUNTS, 1, SAMPLE_RATE);
			break;
		case SINUSOIDE:
			soundBuffers[0][j] = sfSoundBuffer_createFromSamples(createSinSample(getNoteFrequency(j)), SAMPLE_COUNTS, 1, SAMPLE_RATE);
			break;
		case SAWTOOTH:
			soundBuffers[0][j] = sfSoundBuffer_createFromSamples(createSawtoothSample(getNoteFrequency(j)), SAMPLE_COUNTS, 1, SAMPLE_RATE);
		}
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
			soundBuffers[0][j] = sfSoundBuffer_copy(soundBuffers[0][i]);
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