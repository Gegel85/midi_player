#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <SFML/Audio.h>
#include <SFML/Graphics.h>
#include <time.h>
#include "header.h"
#include "midi_parser.h"

float	getPosForNote(unsigned char pitch)
{
	switch (pitch % 12) {
	case 0:
		return (pitch / 12 * NOTE_STEP * 7 + 1);
	case 1:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP - 6);
	case 2:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP + 1);
	case 3:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 2 - 6);
	case 4:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 2 + 1);
	case 5:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 3 + 1);
	case 6:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 4 - 6);
	case 7:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 4 + 1);
	case 8:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 5 - 6);
	case 9:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 5 + 1);
	case 10:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 6 - 6);
	case 11:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 6 + 1);
	}
	return (pitch / 12 * NOTE_STEP * 7);
}

void	displayNote(unsigned char channel, unsigned char pitch, double startTime, double currentTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug)
{
	if (debug)
		printf("Displaying %s from channel %i (startTime: %f, currentTime: %f)\n", getNoteString(pitch), channel, startTime, currentTime);
	switch (pitch % 12) {
	case 0:
	case 2:
	case 4:
	case 5:
	case 7:
	case 9:
	case 11:
		if (debug)
			printf("Size (%f, %f)\nPosition (%f, %f)\n", (NOTE_STEP - 2), (float)(currentTime - startTime), (float)getPosForNote(pitch), (float)(960 - 80 * frect.height / 960 - startTime));
		sfRectangleShape_setFillColor(rec, channelColors[channel]);
		sfRectangleShape_setSize(rec, (sfVector2f){(float)1280 / 74 - 2, currentTime - startTime});
		sfRectangleShape_setOrigin(rec, (sfVector2f){0, currentTime - startTime});
		sfRectangleShape_setPosition(rec, (sfVector2f){getPosForNote(pitch), (float)(960 - (float)80 * frect.height / 960 - startTime)});
		sfRenderWindow_drawRectangleShape(win, rec, NULL);
		break;
	case 1:
	case 3:
	case 6:
	case 8:
	case 10:
		if (debug)
			printf("Size (%f, %f)\nPosition (%f, %f)\n", (NOTE_STEP - 2) / 1.5, (float)(currentTime - startTime), (float)getPosForNote(pitch), (float)(960 - 80 * frect.height / 960 - startTime));
		sfRectangleShape_setFillColor(rec, (sfColor){
			channelColors[channel].r * 0.5,
			channelColors[channel].g * 0.5,
			channelColors[channel].b * 0.5,
			channelColors[channel].a
		});
		sfRectangleShape_setSize(rec, (sfVector2f){(NOTE_STEP - 2) / 1.5, currentTime - startTime});
		sfRectangleShape_setOrigin(rec, (sfVector2f){0, currentTime - startTime});
		sfRectangleShape_setPosition(rec, (sfVector2f){getPosForNote(pitch), 960 - 80 * frect.height / 960 - startTime});
		sfRenderWindow_drawRectangleShape(win, rec, NULL);
		break;
	}
}

void	updateEvents(exec_state_t *state, sfSound ***sounds, bool debug, double time, unsigned char volume, MidiParser *result)
{
	for (unsigned int i = 0; i < state->nbOfTracks; i++)
		state->bufferedTicks[i] += time;
	for (unsigned int i = 0; i < state->nbOfTracks; i++) {
		for (; state->begin[i] < result->tracks[i].nbOfNotes && state->elapsedTicks > result->tracks[i].notes[state->begin[i]].timeBeforeAppear + result->tracks[i].notes[state->begin[i]].duration; state->begin[i]++);
		if (debug)
			printf(
				"%i: Ticks: %.3f, Next event: %p {type = %i, timeToAppear = %i, infos = %p}\n",
				i,
				state->bufferedTicks[i],
				state->events[i],
				state->events[i]->type,
				state->events[i]->timeToAppear,
				state->events[i]->infos
			);
		while ((state->events[i]->infos || state->events[i]->type) && state->events[i]->timeToAppear < state->bufferedTicks[i]) {
			state->bufferedTicks[i] -= state->events[i]->timeToAppear;
			if (state->events[i]->type == MidiNotePressed) {
				state->playingNotes[((MidiNote *)state->events[i]->infos)->channel][((MidiNote *)state->events[i]->infos)->pitch]++;
				if (sounds[((MidiNote *)state->events[i]->infos)->channel % 2][((MidiNote *)state->events[i]->infos)->pitch]) {
					if (debug)
						printf("Playing note %s on channel %i\n", getNoteString(((MidiNote *)state->events[i]->infos)->pitch), ((MidiNote *)state->events[i]->infos)->channel);
					sfSound_setVolume(sounds[((MidiNote *)state->events[i]->infos)->channel % 2][((MidiNote *)state->events[i]->infos)->pitch], (float)((MidiNote *)state->events[i]->infos)->velocity * volume / 127);
					sfSound_play(sounds[((MidiNote *)state->events[i]->infos)->channel % 2][((MidiNote *)state->events[i]->infos)->pitch]);
					state->fadeSpeed[((MidiNote *)state->events[i]->infos)->channel % 2][((MidiNote *)state->events[i]->infos)->pitch] = 0;
					state->notesVolume[((MidiNote *)state->events[i]->infos)->channel % 2][((MidiNote *)state->events[i]->infos)->pitch] = ((MidiNote *)state->events[i]->infos)->velocity * 65535 / 127;
				}
				state->notesPlayed++;
			} else if (state->events[i]->type == MidiNoteReleased) {
				if (state->playingNotes[((MidiNote *)state->events[i]->infos)->channel][((MidiNote *)state->events[i]->infos)->pitch] > 0)
					state->playingNotes[((MidiNote *)state->events[i]->infos)->channel][((MidiNote *)state->events[i]->infos)->pitch]--;
				if (!state->playingNotes[((MidiNote *)state->events[i]->infos)->channel][((MidiNote *)state->events[i]->infos)->pitch] &&
				    sounds[((MidiNote *)state->events[i]->infos)->channel % 2][((MidiNote *)state->events[i]->infos)->pitch]) {
					for (int k = ((MidiNote *)state->events[i]->infos)->channel % 2; k < 18; k += 2)
						if (k >= 16)
							state->fadeSpeed[((MidiNote *)state->events[i]->infos)->channel % 2][((MidiNote *)state->events[i]->infos)->pitch] = -1;
						else if (state->playingNotes[k][((MidiNote *)state->events[i]->infos)->pitch])
							break;
				}
			} else if (state->events[i]->type == MidiTempoChanged)
				state->tempoInfos.tempo = *(int *)state->events[i]->infos;
			else if (state->events[i]->type == MidiNewTimeSignature)
				state->tempoInfos.signature = *(MidiTimeSignature *)state->events[i]->infos;
			state->events[i]++;
		}
	}
	if (debug)
		printf("\n");
}

void	updateSounds(sfSound ***sounds, exec_state_t *state, unsigned char volume, double time)
{
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 128; j++) {
			if (!sounds[i][j])
				break;
			if (state->notesVolume[i][j] >= state->fadeSpeed[i][j] * time * 516)
				state->notesVolume[i][j] -= state->fadeSpeed[i][j] * time * 516;
			else
				state->notesVolume[i][j] = 0;
			if (state->notesVolume[i][j])
				sfSound_setVolume(sounds[i][j], (float)state->notesVolume[i][j] * volume / 65535);
			else
				sfSound_stop(sounds[i][j]);
		}
}

void	ThreadFunc(void *args)
{
	struct	data_s	*data = args;
	float		seconds;
	double		time;

	while (!data->leave && (data->debug || !noEventsLeft(data->execState->events, data->parserResult->nbOfTracks))) {
		seconds = sfTime_asSeconds(sfClock_getElapsedTime(data->clock));
		sfClock_restart(data->clock);
		time = data->settings->speed * seconds * data->execState->tempoInfos.signature.ticksPerQuarterNote * 128000000 / (data->execState->tempoInfos.tempo ?: 10000000);
		if (data->settings->go) {
			data->execState->elapsedTicks += time;
			data->execState->midiClockTicks += 128 * data->execState->tempoInfos.signature.ticksPerQuarterNote * seconds;
			updateEvents(data->execState, data->sounds, data->debug, time, data->settings->volume, data->parserResult);
		}
		updateSounds(data->sounds, data->execState, data->settings->volume, seconds);
		while (data->loading) {
			sfClock_restart(data->clock);
			nanosleep((struct timespec[1]){{0, 6666667}}, NULL);
		}
	}
}
