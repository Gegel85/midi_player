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
	default:
		return (pitch / 12 * NOTE_STEP * 7);
	}
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

void	playSound(State *state, SoundBuffer *buffer, MidiNote *note)
{
	int start = state->currentSound;

	if (!buffer->buffer)
		return (void)printf("No buffer !\n");

	while (!state->sounds[state->currentSound].sound) {
		state->currentSound++;
		state->currentSound %= MAX_SOUNDS;
		if (state->currentSound == start)
			return (void)printf("None found\n");
	}

	state->sounds[state->currentSound].released = false;
	state->sounds[state->currentSound].pitch = note->pitch;
	state->sounds[state->currentSound].channel = note->channel;
	state->sounds[state->currentSound].volume = note->velocity * 65535 / 127;
	state->sounds[state->currentSound].fadeSpeed = 0;
	sfSound_stop(state->sounds[state->currentSound].sound);
	sfSound_setBuffer(state->sounds[state->currentSound].sound, buffer->buffer);
	sfSound_setPitch(state->sounds[state->currentSound].sound, buffer->pitch);
	sfSound_setVolume(state->sounds[state->currentSound].sound, note->velocity * 65535 / 127);
	sfSound_play(state->sounds[state->currentSound].sound);
	state->currentSound++;
	state->currentSound %= MAX_SOUNDS;
}

void	updateEvents(State *state, bool debug, SoundBuffer *soundBuffers, double time, MidiParser *result)
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
				MidiNote *note = state->events[i]->infos;

				if (debug)
					printf("Playing note %s on channel %i\n", getNoteString(note->pitch), note->channel);
				state->playingNotes[note->channel][note->pitch]++;
				playSound(state, &soundBuffers[note->pitch], note);
				state->notesPlayed++;
			} else if (state->events[i]->type == MidiNoteReleased) {
				MidiNote *note = state->events[i]->infos;

				if (state->playingNotes[note->channel][note->pitch] > 0) {
					state->playingNotes[note->channel][note->pitch]--;
					for (int j = state->currentSound; j != (state->currentSound - 1 + MAX_SOUNDS) % MAX_SOUNDS; j = (j + 1) % MAX_SOUNDS)
						if (state->sounds[j].sound && !state->sounds[j].released && state->sounds[j].pitch == note->pitch && state->sounds[j].channel == note->channel) {
							state->sounds[i].fadeSpeed = -1;
							state->sounds[i].released = true;
							break;
						}
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

void	updateSounds(State *state, unsigned char volume, double time)
{
	for (int j = 0; j < MAX_SOUNDS; j++) {
		if (!state->sounds[j].sound)
			continue;
		if (state->sounds[j].volume >= state->sounds[j].fadeSpeed * time * 516)
			state->sounds[j].volume -= state->sounds[j].fadeSpeed * time * 516;
		else
			state->sounds[j].volume = 0;
		if (state->sounds[j].volume)
			sfSound_setVolume(state->sounds[j].sound, (float)state->sounds[j].volume * volume / 65535);
		else
			sfSound_stop(state->sounds[j].sound);
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
			updateEvents(data->execState, data->debug, data->buffers, time, data->parserResult);
		}
		updateSounds(data->execState, data->settings->volume, seconds);
		while (data->loading) {
			sfClock_restart(data->clock);
			nanosleep((struct timespec[1]){{0, 6666667}}, NULL);
		}
	}
}
