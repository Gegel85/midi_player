#define NOTE_STEP (float)1280 / 75
#define	TICKS 32

#include <SFML/Graphics.h>
#include <SFML/Audio.h>
#include <stdbool.h>
#include "midi_parser.h"

extern const sfColor	channelColors[16];

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

typedef struct {
	int	tempo;
	
} MidiInfos;

extern sfFloatRect	frect;

char	*getEventString(Event *event);
NoteList	eventsToNotes(MidiParser *result);
void	displayPianoKeys(char playingNotes[16][128], sfRectangleShape *rec, sfRenderWindow *win);
void	loadSounds(char *path, sfSound *sounds[2][128], sfSoundBuffer *soundBuffers[2][128], bool debug);
void	displayNotesFromNotesList(NoteList *notes, unsigned int elapsedTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug);
void	displayNote(unsigned char channel, unsigned char pitch, int startTime, int currentTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug);
void	displayNotes(EventList **allevents, double *allticks, char playingNotes[16][128], int nbOfTracks, sfRenderWindow *win, sfRectangleShape *rec, bool debug);
void	updateEvents(EventList **events, double *tmp, int nbOfTracks, char playingNotes[16][128], MidiInfos *infos, sfSound *sounds[2][128], bool debug, unsigned int *speed, unsigned int *notes, double time, unsigned char volume);