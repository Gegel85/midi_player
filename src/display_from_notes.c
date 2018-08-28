#include <stdbool.h>
#include "header.h"

void	displayNotesFromNotesList(Note *notes, int nbOfNotes, int begin, double elapsedTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug, int *nbOfNotesDisplayed)
{
	for (int i = begin; i < nbOfNotes && notes[i].timeBeforeAppear - elapsedTime < frect.height + 100; i++) {
		(*nbOfNotesDisplayed)++;
		displayNote(notes[i].channel, notes[i].pitch, notes[i].timeBeforeAppear - elapsedTime, notes[i].duration + notes[i].timeBeforeAppear - elapsedTime, rec, win, debug);
	}
}
