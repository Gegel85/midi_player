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

typedef struct {
	bool		dontDisplay;
	bool		displayHUD;
	char		volume;
	Instrument	instrument;
	bool		go;
	double		speed;
} settings_t;

typedef struct {
	char		playingNotes[16][128];
	unsigned short	notesVolume[2][128];
	unsigned char	fadeSpeed[2][128];
	double		elapsedTicks;
	unsigned int	nbOfTracks;
	double		*bufferedTicks;
	Event		**events;
	MidiInfos	tempoInfos;
	double		midiClockTicks;
	unsigned int	notesPlayed;
	int		*begin;
} exec_state_t;

struct data_s {
	sfRenderWindow	*window;
	settings_t	*settings;
	exec_state_t	*execState;
	MidiParser	*parserResult;
	sfRectangleShape*rect;
	sfText		*text;
	bool		debug;
	sfClock		*clock;
};

extern sfFloatRect	frect;

#if defined _WIN32 || defined __WIN32 || defined __WIN32__
#include <windows.h>

DWORD WINAPI ThreadFunc(void *args);
#else

#endif
char	*getEventString(Event *event);
char	*getMidiEventTypeString(EventType type);
NoteList	eventsToNotes(MidiParser *result);
void	displayPianoKeys(char playingNotes[16][128], sfRectangleShape *rec, sfRenderWindow *win);
void	updateSounds(sfSound *sounds[2][128], exec_state_t *state, unsigned char volume, double time);
void	loadSounds(char *path, sfSound *sounds[2][128], sfSoundBuffer *soundBuffers[2][128], bool debug, Instrument instrument);
void	updateEvents(exec_state_t *state, sfSound *sounds[2][128], bool debug, double time, unsigned char volume, MidiParser *result);
void	displayNote(unsigned char channel, unsigned char pitch, double startTime, double currentTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug);
void	displayNotesFromNotesList(Track *track, int begin, exec_state_t *state, sfRectangleShape *rec, sfRenderWindow *win, bool debug, int *nbOfNotesDisplayed);
