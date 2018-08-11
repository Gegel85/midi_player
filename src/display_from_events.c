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

void	displayNotes(EventList **allevents, double *allticks, char playingNotes[16][128], int nbOfTracks, sfRenderWindow *win, sfRectangleShape *rec, int *nbOfNoteDisplayed, bool debug)
{
	double		time2 = 0;
	double		rectSize[16][128][16];
	double		ticks;
	EventList	*events;
	double		lowest[16][128];

	*nbOfNoteDisplayed = 0;
	for (int i = 0; i < 16; i++)
		for (int j = 0; j < 128; j++) {
			for (int k = 0; k < 16; k++)
				rectSize[i][j][k] = -1;
			lowest[i][j] = frect.height + 100;
		}
	for (int i = 0; i < nbOfTracks; i++) {
		events = allevents[i];
		ticks = allticks[i];
		if (debug) {
			printf("Ticks: %f\n", ticks);
			printf("[\n");
			for (double time = 0; events && time + events->data->timeToAppear - ticks < frect.height + 100; events = events->next) {
				time += events->data->timeToAppear;
				printf("\t{\n");
				printf("\t\ttype = %s,\n", getMidiEventTypeString(events->data->type));
				printf("\t\ttime = %i,\n", events->data->timeToAppear);
				printf("\t\tcurrentTime = %.0f\n", time);
				if (events->data->type == MidiNotePressed || events->data->type == MidiNoteReleased) {
					printf("\t\tchannel = %u,\n", ((MidiNote *)events->data->infos)->channel);
					printf("\t\tpitch = %u (%s),\n", ((MidiNote *)events->data->infos)->pitch, getNoteString(((MidiNote *)events->data->infos)->pitch));
					printf("\t\tvelocity = %u,\n", ((MidiNote *)events->data->infos)->velocity);
				} else if (events->data->type == MidiTempoChanged)
					printf("\t\ttempo = %i,\n", *(int *)events->data->infos);
				printf("\t},\n");
			}
			printf("]\n");
		}
		events = allevents[i];
		for (double time = 0; events && time + events->data->timeToAppear - ticks < frect.height + 100; events = events->next) {
			time += events->data->timeToAppear;
			if (!events->data)
				continue;
			if (events->data->type == MidiNotePressed) {
				if(debug)
					printf("Note %s is pressed on channel %i ! (%f - %f)\n", 
						getNoteString(((MidiNote *)events->data->infos)->pitch),
						((MidiNote *)events->data->infos)->channel,
						time,
						ticks
					);
				for (int k = 0; k < 16; k++)
					if (rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k] < 0) {
						rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k] = time - ticks;
						break;
					}
			} else if (events->data->type == MidiNoteReleased) {
				int k = 15;
				for (; k > 0; k--)
					if (rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k] >= 0)
						break;
				if (rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k] < 0 && !playingNotes[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch]) {
					if (debug)
						printf("Note %s is released on channel %i but it is not being played !\n", 
							getNoteString(((MidiNote *)events->data->infos)->pitch),
							((MidiNote *)events->data->infos)->channel
						);
					rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k] = -1;
					continue;
				}
				if (debug)
					printf(
						"displayNote(%i, %i, %f, %f - %f, %p, %p, %s);\n",
						((MidiNote *)events->data->infos)->channel,
						((MidiNote *)events->data->infos)->pitch,
						rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k],
						time,
						ticks,
						rec,
						win,
						debug ? "true" : "false"
					);
				*nbOfNoteDisplayed = *nbOfNoteDisplayed + 1;
				displayNote(
					((MidiNote *)events->data->infos)->channel,
					((MidiNote *)events->data->infos)->pitch,
					rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k],
					time - ticks,
					rec,
					win,
					debug
				);
				if (lowest[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] > rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k])
					lowest[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] = rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k];
				for (int i = k; i < 15; i++)
					rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k] = rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][k + 1];
				rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch][15] = -1;
			}
			time2 = time;
		}
		for (unsigned char i = 0; i < 16; i++)
			for (unsigned char j = 0; j < 128; j++)
				for (int k = 0; k < 16; k++)
					if (rectSize[i][j][k] >= 0) {
						*nbOfNoteDisplayed = *nbOfNoteDisplayed + 1;
						if (lowest[i][j] > rectSize[i][j][k])
							lowest[i][j] = rectSize[i][j][k];
						displayNote(i, j, rectSize[i][j][k], frect.height + 100, rec, win, debug);
					}
		if(debug)
			printf(!events ? "End of events list !\n\n" : "Stopped because %.2f + %i - %.3f >= %.3f (Next event: %s)\n\n",
				time2,
				events ? events->data->timeToAppear : 0,
				ticks,
				frect.height + 100,
				getEventString(events ? events->data : NULL)
			);
	}
	for (unsigned char i = 0; i < 16; i++)
		for (unsigned char j = 0; j < 128; j++)
			if (playingNotes[i][j] && lowest[i][j] > 0)
				displayNote(i, j, 0, lowest[i][j], rec, win, debug);
	if(debug)printf("\n\n");
}


void	updateEvents(EventList **events, double *tmp, int nbOfTracks, char playingNotes[16][128], MidiInfos *infos, sfSound *sounds[2][128], bool debug, unsigned int *notes, double time, unsigned char volume)
{
	for (int i = 0; i < nbOfTracks; i++)
		tmp[i] += time;
	for (int i = 0; i < nbOfTracks; i++)
		while (events[i] && events[i]->data->timeToAppear < tmp[i]) {
			tmp[i] -= events[i]->data->timeToAppear;
			if (events[i]->data->type == MidiNotePressed) {
				playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch]++;
				if (sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch]) {
					if (debug)
						printf("Playing note %s on channel %i\n", getNoteString(((MidiNote *)events[i]->data->infos)->pitch), ((MidiNote *)events[i]->data->infos)->channel);
					sfSound_setVolume(sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch], (float)((MidiNote *)events[i]->data->infos)->velocity * volume / 127);
					sfSound_play(sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch]);
				}
				*notes = *notes + 1;
			} else if (events[i]->data->type == MidiNoteReleased) {
				if (playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch] > 0)
					playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch]--;
				if (!playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch] && sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch]) {
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
