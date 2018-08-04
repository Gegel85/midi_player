#include <stdbool.h>
#include "header.h"

void	displayNotesFromNotesList(NoteList *notes, unsigned int elapsedTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug)
{
	for (; notes; notes = notes->next) {
		if (!notes->note)
			continue;
		displayNote(notes->note->channel, notes->note->pitch, notes->note->timeBeforeAppear, notes->note->duration + notes->note->timeBeforeAppear, rec, win, debug);
	}
}