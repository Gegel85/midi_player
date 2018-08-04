#include "header.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdbool.h>

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
	for (; list->next; list = list->next);
	if (list->note) {
		list->next = malloc(sizeof(*list->next));
		if (!list->next) {
			printf("Error: Cannot alloc %iB\n", (int)sizeof(*list->next));
			return (false);
		}
		list->next->prev = list;
		list->next->next = NULL;
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

NoteList	eventsToNotes(MidiParser *result)
{
	NoteList	nlist = {NULL, NULL, NULL};
	NoteList	*currlist = &nlist;
	Note		*notes[16][128];
	unsigned long	currentTime = 0;
	MidiNote	*buffer;
	
	for (int i = 0; i < 16; i++)
		memset(notes[i], 0, sizeof(notes[i]));
	for (int i = 0; i < result->nbOfTracks; i++) {
		currentTime = 0;
		for (EventList *list = &result->tracks[i].events; list; list = list->next) {
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
		for (int j = 0; j < 128; j++)
			if (notes[i][j])
				notes[i][j]->duration = currentTime - notes[i][j]->timeBeforeAppear;
	//sortNoteList();
	return (nlist);
}