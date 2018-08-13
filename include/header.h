#define NOTE_STEP (float)1280 / 75
#define CONTROLS "\n\
Space: Pause/Unpause\n\
PageUp: Volume Up\n\
PageDown: Volume Down\n\
Num Pad +: Zoom in\n\
Num Pad -: Zoom out\n\
X: Hide/Show notes\n\
A: Close Window but still play\n\
W: Hide/Show HUD\n\
Up arrow: Faster, faster !\n\
Down arrow: Slower !\n\
Left arrow: Go back in time\n\
Right arrow: Go further\n\
Home Key : Go back to the beginning"

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
	int			tempo;
	int			clockTicksPerSecond;
	MidiTimeSignature	signature;
} MidiInfos;

extern sfFloatRect	frect;

char	*getEventString(Event *event);
char	*getMidiEventTypeString(EventType type);
NoteList	eventsToNotes(MidiParser *result);
void	displayPianoKeys(char playingNotes[16][128], sfRectangleShape *rec, sfRenderWindow *win);
void	loadSounds(char *path, sfSound *sounds[2][128], sfSoundBuffer *soundBuffers[2][128], bool debug);
void	displayNotesFromNotesList(NoteList *notes, unsigned int elapsedTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug);
void	updateSounds(sfSound *sounds[2][128], unsigned short notesVolume[2][128], unsigned char fadeSpeed[2][128], unsigned char volume, double time);
void	displayNote(unsigned char channel, unsigned char pitch, double startTime, double currentTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug);
void	displayNotes(EventList **allevents, double *allticks, char playingNotes[16][128], int nbOfTracks, sfRenderWindow *win, sfRectangleShape *rec, int *nbOfNoteDisplayed, bool debug);
void	updateEvents(EventList **events, double *tmp, int nbOfTracks, char playingNotes[16][128], MidiInfos *infos, sfSound *sounds[2][128], unsigned short notesVolume[2][128], char fadeSpeed[2][128], bool debug, unsigned int *notes, double time, unsigned char volume);