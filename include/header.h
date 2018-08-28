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
Home Key : Go back to the beginning\n\
U: Change instrument to Piano\n\
I: Change instrument to Square wave\n\
O: Change instrument to Sin wave\n\
P: Change instrument to Sawtooth wave\n\
L: Switch back to the old display system"

#include <SFML/Graphics.h>
#include <SFML/Audio.h>
#include <stdbool.h>
#include "midi_parser.h"

extern const sfColor	channelColors[16];

typedef struct {
	int			tempo;
	int			clockTicksPerSecond;
	MidiTimeSignature	signature;
} MidiInfos;

typedef enum {
	PIANO,
	SQUARE,
	SINUSOIDE,
	SAWTOOTH,
} Instrument;

extern sfFloatRect	frect;

char	*getEventString(Event *event);
char	*getMidiEventTypeString(EventType type);
NoteList	eventsToNotes(MidiParser *result);
void	displayPianoKeys(char playingNotes[16][128], sfRectangleShape *rec, sfRenderWindow *win);
void	loadSounds(char *path, sfSound *sounds[2][128], sfSoundBuffer *soundBuffers[2][128], bool debug, Instrument instrument);
void	updateSounds(sfSound *sounds[2][128], unsigned short notesVolume[2][128], unsigned char fadeSpeed[2][128], unsigned char volume, double time);
void	displayNote(unsigned char channel, unsigned char pitch, double startTime, double currentTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug);
void	displayNotesFromNotesList(Note *notes, int nbOfNotes, int begin, double elapsedTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug, int *nbOfNotesDisplayed);
void	displayNotes(Event **allevents, double *allticks, char playingNotes[16][128], int nbOfTracks, sfRenderWindow *win, sfRectangleShape *rec, int *nbOfNoteDisplayed, bool debug);
void	updateEvents(Event **events, double *tmp, int nbOfTracks, char playingNotes[16][128], MidiInfos *infos, sfSound *sounds[2][128], unsigned short notesVolume[2][128], unsigned char fadeSpeed[2][128], bool debug, unsigned int *notes, double time, unsigned char volume, int *begin, MidiParser *result, double elapsedTime);
