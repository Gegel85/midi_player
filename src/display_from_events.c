#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <SFML/Audio.h>
#include <SFML/Graphics.h>
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

void	displayNote(unsigned char channel, unsigned char pitch, int startTime, int currentTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug)
{
	if (debug)
		printf("Displaying %s from channel %i (startTime: %i, currentTime: %i)\n", getNoteString(pitch), channel, startTime, currentTime);
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
		sfRectangleShape_setPosition(rec, (sfVector2f){getPosForNote(pitch), 960 - 80 * frect.height / 960 - startTime});
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

void	displayNotes(EventList **allevents, double *allticks, char playingNotes[16][128], int nbOfTracks, sfRenderWindow *win, sfRectangleShape *rec, bool debug)
{
	int		time = 0;
	int		rectSize[16][128];
	double		ticks;
	EventList	*events;

	for (int i = 0; i < 16; i++)
		memset(rectSize[i], -1, sizeof(rectSize[i]));
	for (int i = 0; i < nbOfTracks; i++) {
		events = allevents[i];
		ticks = allticks[i];
		time = 0;
		for (; events && (int)(time += events->data->timeToAppear) - (int)ticks < frect.height; events = events->next) {
			if (!events->data)
				continue;
			if (events->data->type == MidiNotePressed) {
				if(debug)
					printf("Note %s is pressed on channel %i ! (%i)\n", 
						getNoteString(((MidiNote *)events->data->infos)->pitch),
						((MidiNote *)events->data->infos)->channel,
						time
					);
				rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] = time - ticks;
			} else if (events->data->type == MidiNoteReleased) {
				if (rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] == -2)
					continue;
				if (rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] < 0 && !playingNotes[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch]) {
					rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] = -2;
					continue;
				}
				displayNote(
					((MidiNote *)events->data->infos)->channel,
					((MidiNote *)events->data->infos)->pitch,
					rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch],
					time - ticks,
					rec,
					win,
					debug
				);
				rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] = -2;
			}
		}
		if(debug)
			printf(!events ? "End of events list !\n\n" : "Stopped because %i + %i - %3.f () >= 1900 (Next event: %s)\n\n",
				time,
				events->data->timeToAppear,
				ticks,
				getEventString(events ? events->data : NULL)
			);
		for (unsigned char i = 0; i < 16; i++)
			for (unsigned char j = 0; j < 128; j++)
				if (rectSize[i][j] >= 0) {
					displayNote(i, j, rectSize[i][j], time, rec, win, debug);
					rectSize[i][j] = -2;
				}
	}
	for (unsigned char i = 0; i < 16; i++)
		for (unsigned char j = 0; j < 128; j++)
			if (rectSize[i][j] != -2 && playingNotes[i][j])
				displayNote(i, j, 0, frect.height, rec, win, debug);
	if(debug)printf("\n\n");
}


void	updateEvents(EventList **events, double *tmp, int nbOfTracks, char playingNotes[16][128], MidiInfos *infos, sfSound *sounds[2][128], bool debug, double *speed, unsigned int *notes, double time, unsigned char volume)
{
	for (int i = 0; i < nbOfTracks; i++)
		tmp[i] += time;
	for (int i = 0; i < nbOfTracks; i++)
		while (events[i] && events[i]->data->timeToAppear < tmp[i]) {
			tmp[i] -= events[i]->data->timeToAppear;
			if (events[i]->data->type == MidiNotePressed) {
				playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch] = ((MidiNote *)events[i]->data->infos)->velocity;
				if (sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch]) {
					if (debug)
						printf("Playing note %s on channel %i\n", getNoteString(((MidiNote *)events[i]->data->infos)->pitch), ((MidiNote *)events[i]->data->infos)->channel);
					sfSound_setVolume(sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch], (float)((MidiNote *)events[i]->data->infos)->velocity * volume / 127);
					sfSound_play(sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch]);
				}
				*notes = *notes + 1;
			} else if (events[i]->data->type == MidiNoteReleased) {
				playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch] = 0;
				if (sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch]) {
					for (int k = ((MidiNote *)events[i]->data->infos)->channel % 2; k < 18; k += 2)
						if (k >= 16)
							sfSound_stop(sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch]);
						else if (playingNotes[k][((MidiNote *)events[i]->data->infos)->pitch])
							break;
				}
			} else if (events[i]->data->type == MidiTempoChanged)
				infos->tempo = *(int *)events[i]->data->infos;
			else if (events[i]->data->type == MidiNewTimeSignature)
				infos->signature = *(MidiTimeSignature *)events[i]->data->infos;
			events[i] = events[i]->next;
		}
}
