#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SFML/Graphics.h>
#include "midi_parser.h"

#define NOTE_STEP (float)1280 / 74
#define	TICKS 32

typedef struct	Note {
	unsigned	char	pitch;
	unsigned	char	channel;
	unsigned	char	velocity;
	unsigned long	int	timeBeforeAppear;
	unsigned long	int	duration;
} Note;

typedef struct	NoteArray {
	int	length;
	Note	*notes;
} NoteArray;

typedef struct	NoteList {
	Note			*note;
	struct	NoteList	*next;
	struct	NoteList	*prev;
} NoteList;

const sfColor	channelColors[16] = {
	{255, 0  , 0  , 255},
	{0  , 0  , 255, 255},
	{0  , 255, 0  , 255},
	{255, 255, 0  , 255},
	{255, 0  , 255, 255},
	{0  , 255, 255, 255},
	{100, 100, 100, 255},
	{255, 120, 0  , 255},
	{255, 120, 120, 255},
	{120, 0  , 255, 255},
	{255, 255, 255, 255},
	{0  , 0  , 120, 255},
	{120, 120, 255, 255},
	{120, 255, 120, 255},
	{116 , 26 , 26 , 255},
	{200, 200, 200, 255},
};

Note	*createNote(unsigned char pitch, unsigned char channel, unsigned long timeBeforeAppear, unsigned long duration, int velocity)
{
	Note	*note = malloc(sizeof(*note));
	
	if (note) {
		note->pitch = pitch;
		note->channel = channel;
		note->timeBeforeAppear = timeBeforeAppear;
		note->duration = duration;
		note->velocity = velocity;
	}
	return (note);
}

bool	addNote(NoteList *list, Note *data)
{
	if (!data)
		return (false);
	for (; list->next && list->next->note->timeBeforeAppear < data->timeBeforeAppear; list = list->next);
	if (list->note) {
		if (!list->next) {
			list->next = malloc(sizeof(*list->next));
			if (!list->next) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(*list->next));
				return (false);
			}
			list->next->prev = list;
			list->next->next = NULL;
		} else {
			list->next->prev = malloc(sizeof(*list->next->prev));
			if (!list->next) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(*list->next));
				return (false);
			}
			list->next->prev->next = list->next;
			list->next->prev->prev = list;
			list->next = list->next->prev;
		}
		list = list->next;
	}
	list->note = data;
	return (true);
}

void	deleteNoteList(NoteList *list, bool delData)
{
	for (; list->next; list = list->next);
	for (; list; list = list->prev) {
		if (delData)
			free(list->note);
		free(list->next);
	}
}

NoteList	eventsToNote(MidiParser *result)
{
	NoteList	nlist = {NULL, NULL, NULL};
	NoteList	*currlist = &nlist;
	Note		*notes[16][127];
	unsigned long	currentTime = 0;
	MidiNote	*buffer;
	
	for (int i = 0; i < 16; i++)
		memset(notes[i], 0, sizeof(notes[i]));
	for (int i = 0; i < result->nbOfTracks; i++) {
		currentTime = 0;
		currlist = &nlist;
		for (EventList *list = &result->tracks[i].events; list; list = list->next) {
			for (; currlist->next && currlist->next->note->timeBeforeAppear < currentTime; currlist = currlist->next);
			if (list->data->type == MidiNoteReleased) {
				buffer = list->data->infos;
				if (notes[buffer->channel][buffer->pitch])
					notes[buffer->channel][buffer->pitch]->duration = currentTime - notes[buffer->channel][buffer->pitch]->timeBeforeAppear;
				notes[buffer->channel][buffer->pitch] = NULL;
			} else if (list->data->type == MidiNotePressed) {
				buffer = list->data->infos;
				if (notes[buffer->channel][buffer->pitch])
					notes[buffer->channel][buffer->pitch]->duration = currentTime - notes[buffer->channel][buffer->pitch]->timeBeforeAppear;
				if (buffer->velocity) {
					notes[buffer->channel][buffer->pitch] = createNote(buffer->pitch, buffer->channel, currentTime, 0, buffer->velocity);
					if (!addNote(currlist, notes[buffer->channel][buffer->pitch]))
						return ((NoteList){NULL, NULL, NULL});
				} else
					notes[buffer->channel][buffer->pitch] = NULL;
			}
			currentTime += list->data->timeToAppear;
		}
	}
	for (int i = 0; i < 16; i++)
		for (int j = 0; j < 27; j++)
			if (notes[i][j])
				notes[i][j]->duration = currentTime - notes[i][j]->timeBeforeAppear;
	return (nlist);
}

float	getPosForNote(unsigned char pitch)
{
	switch (pitch % 12) {
	case 0:
		return (pitch / 12 * NOTE_STEP * 7);
	case 1:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP - 6);
	case 2:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP);
	case 3:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 2 - 6);
	case 4:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 2);
	case 5:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 3);
	case 6:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 4 - 6);
	case 7:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 4);
	case 8:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 5 - 6);
	case 9:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 5);
	case 10:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 6 - 6);
	case 11:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 6);
	}
	return (pitch / 12 * NOTE_STEP * 7);
}

void	displayPianoKeys(char playingNotes[16][127], sfRectangleShape *rec, sfRenderWindow *win)
{
	bool	drawOneBefore = false;
	float	last = 0;
	sfColor	color;

	sfRectangleShape_setOrigin(rec, (sfVector2f){0, 0});
	if (!rec)
		return;
	for (int i = 0; i < 127; i++) {
		switch (i % 12) {
		case 0:
		case 2:
		case 4:
		case 5:
		case 7:
		case 9:
		case 11:
			sfRectangleShape_setPosition(rec, (sfVector2f){last + 1, 880});
			color = (sfColor){255, 255, 255, 255};
			for (int j = 0; j < 16; j++)
				if (playingNotes[j][i]) {
					color = channelColors[j];
					break;
				}
			sfRectangleShape_setFillColor(rec, color);
			sfRectangleShape_setSize(rec, (sfVector2f){NOTE_STEP - 2, 80});
			sfRenderWindow_drawRectangleShape(win, rec, NULL);
			if (drawOneBefore) {
				sfRectangleShape_setPosition(rec, (sfVector2f){last - 6, 880});
				color = (sfColor){0, 0, 0, 255};
				for (int j = 0; j < 16; j++)
					if (playingNotes[j][i - 1]) {
						color = (sfColor){
							channelColors[j].r * 0.5,
							channelColors[j].g * 0.5,
							channelColors[j].b * 0.5,
							channelColors[j].a
						};
						break;
					}
				sfRectangleShape_setFillColor(rec, color);
				sfRectangleShape_setSize(rec, (sfVector2f){(NOTE_STEP - 2) / 1.5, 40});
				sfRenderWindow_drawRectangleShape(win, rec, NULL);
			}
			drawOneBefore = false;
			last += (float)1280 / 74;
			break;
		case 1:
		case 3:
		case 6:
		case 8:
		case 10:
			drawOneBefore = true;
			break;
		}
	}
	if (drawOneBefore) {
		sfRectangleShape_setPosition(rec, (sfVector2f){last - 6, 880});
		sfRectangleShape_setFillColor(rec, (sfColor){0, 0, 0, 255});
		sfRectangleShape_setSize(rec, (sfVector2f){(NOTE_STEP - 2) / 1.5, 40});
		sfRenderWindow_drawRectangleShape(win, rec, NULL);
	}
}

void	displayNote(unsigned char channel, unsigned char pitch, int startTime, int currentTime, int ticks, sfRectangleShape *rec, sfRenderWindow *win)
{
	switch (pitch % 12) {
	case 0:
	case 2:
	case 4:
	case 5:
	case 7:
	case 9:
	case 11:
		sfRectangleShape_setFillColor(rec, channelColors[channel]);
		sfRectangleShape_setSize(rec, (sfVector2f){(float)1280 / 74 - 2, currentTime - startTime});
		sfRectangleShape_setOrigin(rec, (sfVector2f){0, currentTime - startTime});
		sfRectangleShape_setPosition(rec, (sfVector2f){getPosForNote(pitch), 880 + ticks - startTime});
		sfRenderWindow_drawRectangleShape(win, rec, NULL);
		break;
	case 1:
	case 3:
	case 6:
	case 8:
	case 10:
		sfRectangleShape_setFillColor(rec, (sfColor){
			channelColors[channel].r * 0.5,
			channelColors[channel].g * 0.5,
			channelColors[channel].b * 0.5,
			channelColors[channel].a
		});
		sfRectangleShape_setSize(rec, (sfVector2f){((float)1280 / 74 - 2) / 1.5, currentTime - startTime});
		sfRectangleShape_setOrigin(rec, (sfVector2f){0, currentTime - startTime});
		sfRectangleShape_setPosition(rec, (sfVector2f){getPosForNote(pitch), 880 + ticks - startTime});
		sfRenderWindow_drawRectangleShape(win, rec, NULL);
		break;
	}
}

void	displayNotes(EventList **allevents, int *allticks, char playingNotes[16][127], int nbOfTracks, sfRenderWindow *win, sfRectangleShape *rec)
{
	int		time = 0;
	int		rectSize[16][127];
	int		ticks;
	EventList	*events;

	for (int i = 0; i < 16; i++)
		memset(rectSize[i], -1, sizeof(rectSize[i]));
	for (int i = 0; i < nbOfTracks; i++) {
		events = allevents[i];
		ticks = allticks[i];
		time = 0;
		for (; events && time + events->data->timeToAppear - ticks < 1900; events = events->next) {
			time += events->data->timeToAppear;
			if (!events->data)
				continue;
			if (events->data->type == MidiNotePressed) {
				if (rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] >= 0)
					displayNote(
						((MidiNote *)events->data->infos)->channel,
						((MidiNote *)events->data->infos)->pitch,
						rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch],
						time,
						ticks,
						rec,
						win
					);
				rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] = time;
			} else if (events->data->type == MidiNoteReleased) {
				displayNote(
					((MidiNote *)events->data->infos)->channel,
					((MidiNote *)events->data->infos)->pitch,
					rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch],
					time,
					ticks,
					rec,
					win
				);
				rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] = -2;
			}
		}
	}
	for (unsigned char i = 0; i < 16; i++)
		for (unsigned char j = 0; j < 127; j++)
			if (rectSize[i][j] >= 0)
				displayNote(i, j, rectSize[i][j], 880, 0, rec, win);
			else if (rectSize[i][j] != -2 && playingNotes[i][j])
				displayNote(i, j, 0, 880, 0, rec, win);
}

void	updateEvents(EventList **events, int *tmp, int nbOfTracks, char playingNotes[16][127])
{
	for (int i = 0; i < nbOfTracks; i++)
		tmp[i] += TICKS;
	for (int i = 0; i < nbOfTracks; i++)
		while (events[i] && events[i]->data->timeToAppear < tmp[i]) {
			tmp[i] -= events[i]->data->timeToAppear;
			if (events[i]->data->type == MidiNotePressed)
				playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch] = ((MidiNote *)events[i]->data->infos)->velocity;
			else if (events[i]->data->type == MidiNoteReleased)
				playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch] = 0;
			events[i] = events[i]->next;
		}
}

bool	noEventsLeft(EventList **events, int nbOfTracks)
{
	for (int i = 0; i < nbOfTracks; i++) {
		if (events[i]) {
			return (false);
		}
	}
	return (true);
}

void	displayMidi(MidiParser *result, char *path)
{
	EventList	*events[result->nbOfTracks];
	sfVideoMode	mode = {1280, 960, 32};
	sfEvent		event;
	sfRectangleShape*rect = sfRectangleShape_create();
	sfRenderWindow	*window = sfRenderWindow_create(mode, path, sfClose, NULL);
	char		playingNotes[16][127];
	unsigned int	elapsedTicks = 0;
	char		buffer[100];
	sfText		*text = sfText_create();
	sfFont		*font = sfFont_createFromFile("arial.ttf");
	int		tmp[result->nbOfTracks];
	
	if (!window || !text)
		return;
	for (int i = 0; i < 16; i++)
		memset(playingNotes[i], 0, sizeof(playingNotes[i]));
	memset(tmp, 0, sizeof(tmp));
	for (int i = 0; i < result->nbOfTracks; i++)
		events[i] = &result->tracks[i].events;
	sfText_setCharacterSize(text, 10);
	sfRectangleShape_setOutlineColor(rect, (sfColor){0, 0, 0, 255});
	sfRectangleShape_setOutlineThickness(rect, 2);
	sfText_setFont(text, font);
	sfText_setColor(text, (sfColor){255, 255, 255, 255});
	sfRenderWindow_setFramerateLimit(window, 60);
	sfRenderWindow_clear(window, (sfColor){120, 120, 200, 255});
	sfText_setPosition(text, (sfVector2f){600, 450});
	sfText_setString(text, "Loading");
	sfRenderWindow_drawText(window, text, NULL);
	sfRenderWindow_display(window);
	sfText_setCharacterSize(text, 10);
	sfText_setPosition(text, (sfVector2f){0, 0});
	while (sfRenderWindow_isOpen(window)) {
		while (sfRenderWindow_pollEvent(window, &event)) {
			if (event.type == sfEvtClosed)
				sfRenderWindow_close(window);
		}
		sfRenderWindow_clear(window, (sfColor){0, 0, 0, 255});
		displayNotes(events, tmp, playingNotes, result->nbOfTracks, window, rect);
		displayPianoKeys(playingNotes, rect, window);
		sprintf(buffer, "Ticks %u", elapsedTicks);
		sfText_setString(text, buffer);
		sfRenderWindow_drawText(window, text, NULL);
		sfRenderWindow_display(window);
		elapsedTicks += TICKS;
		updateEvents(events, tmp, result->nbOfTracks, playingNotes);
		if (noEventsLeft(events, result->nbOfTracks))
			sfRenderWindow_close(window);
        }
	sfRenderWindow_destroy(window);
	sfFont_destroy(font);
	sfText_destroy(text);
}

int	main(int argc, char **args)
{
	MidiParser	*result;

	if (argc != 2 && argc != 3) {
		printf("Usage: %s <file.mid> [debug]\n", args[0]);
		return (EXIT_FAILURE);
	}
	result = parseMidi(args[1], argc == 3 && strcmp(args[2], "debug") == 0);
	if (!result)
		printf("An error occured when reading %s\n", args[1]);
	else {
		printf("Finished to read %s: format %hi, %hi tracks, %i notes, ", args[1], result->format, result->nbOfTracks, result->nbOfNotes);
		if (result->fps) {
			printf("division: %i FPS and %i ticks/frame\n", result->fps, result->ticks);
		} else
			printf("division: %i ticks / 1/4 note\n", result->ticks);
		displayMidi(result, args[1]);
		deleteMidiParserStruct(result);
	}
	return (EXIT_SUCCESS);
}