#include <stdbool.h>
#include "header.h"

void	displayNotesFromNotesList(Track *track, int begin, exec_state_t *state, sfRectangleShape *rec, sfRenderWindow *win, bool debug, int *nbOfNotesDisplayed)
{
	for (int i = begin; i < track->nbOfNotes && track->notes[i].timeBeforeAppear - state->elapsedTicks < frect.height + 100; i++) {
		(*nbOfNotesDisplayed)++;
		displayNote(
			track->notes[i].channel,
			track->notes[i].pitch,
			track->notes[i].timeBeforeAppear - state->elapsedTicks,
			track->notes[i].duration + track->notes[i].timeBeforeAppear - state->elapsedTicks,
			rec,
			win,
			debug
		);
	}
}
