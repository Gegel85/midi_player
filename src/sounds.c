#include <math.h>
#include <SFML/Audio.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "header.h"

#define SAMPLE_COUNTS	44100 * 4
#define SAMPLE_RATE	44100

double	getNoteFrequency(char note)
{
	return (pow(2, (double)(note - 69) / 12.0) * 440);
}

int	findClosestBuffer(SoundBuffer buffers[MAX_BUFFERS], int startIndex)
{
	for (int i = 0; startIndex + i < MAX_BUFFERS || startIndex - i >= 0; i++) {
		if (startIndex + i < MAX_BUFFERS && buffers[startIndex + i].buffer)
			return (startIndex + i);
		if (startIndex - i >= 0 && buffers[startIndex - i].buffer)
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

void	loadSounds(char *path, PlayingSound *sounds, SoundBuffer *soundBuffers, bool debug, Instrument instrument)
{
	char		buffer[1024];
	char		*note;
	SoundBuffer	buffers[MAX_BUFFERS];

	for (int j = 0; j < MAX_BUFFERS; j++) {
		switch (instrument) {
		case PIANO:
			note = getNoteString(j);
			note[0] += 'a' - 'A';
			sprintf(buffer, "%ssounds/Grand Piano4/%smmell.wav", path, note);
			soundBuffers[j].buffer = sfSoundBuffer_createFromFile(buffer);
			break;
		case SQUARE:
			soundBuffers[j].buffer = sfSoundBuffer_createFromSamples(createSquareSample(getNoteFrequency(j)), SAMPLE_COUNTS, 1, SAMPLE_RATE);
			break;
		case SINUSOIDE:
			soundBuffers[j].buffer = sfSoundBuffer_createFromSamples(createSinSample(getNoteFrequency(j)), SAMPLE_COUNTS, 1, SAMPLE_RATE);
			break;
		case SAWTOOTH:
			soundBuffers[j].buffer = sfSoundBuffer_createFromSamples(createSawtoothSample(getNoteFrequency(j)), SAMPLE_COUNTS, 1, SAMPLE_RATE);
		}
	}

	for (int i = 0; i < MAX_BUFFERS; i++)
		buffers[i] = soundBuffers[i];

	for (int j = 0, i = 0; j < MAX_BUFFERS; j++) {
		if (!soundBuffers[j].buffer) {
			i = findClosestBuffer(buffers, j);
			if (i < 0) {
				printf("No sound could be loaded\n");
				return;
			}
			soundBuffers[j] = buffers[i];
			soundBuffers[j].pitch = getNoteFrequency(j) / getNoteFrequency(i);
		} else
			soundBuffers[j].pitch = 1;
	}

	for (int i = 0; i < MAX_SOUNDS; i++) {
		memset(&sounds[i], 0, sizeof(sounds[i]));
		sounds[i].sound = sfSound_create();
		sounds[i].released = true;
	}

	if (debug) {
		for (int i = 0; i < MAX_SOUNDS; i++) {
			if (!sounds[i].sound) {
				debug = false;
				printf("Sound[%i]] is not set !\n", i);
			}
		}
		if (debug)
			printf("No sound problems found\n");
	}
}