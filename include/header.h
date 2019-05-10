#define NOTE_STEP (float)1280 / 75
#define MAX_SOUNDS 256
#define MAX_BUFFERS 128
#define MAX_CHANNELS 16
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
P: Change instrument to Sawtooth wave"

#include <SFML/Graphics.h>
#include <SFML/Audio.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "midi_parser.h"

extern const sfColor	channelColors[MAX_CHANNELS];

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
} Settings;

typedef struct {
	sfSound	*sound;
	double	volume;
	int	pitch;
	int	channel;
	int	fadeSpeed;
	bool	released;
} PlayingSound;

typedef struct {
	char		playingNotes[MAX_CHANNELS][MAX_BUFFERS];
	double		elapsedTicks;
	unsigned int	nbOfTracks;
	double		*bufferedTicks;
	Event		**events;
	MidiInfos	tempoInfos;
	double		midiClockTicks;
	unsigned int	notesPlayed;
	int		*begin;
	PlayingSound	*sounds;
	int		currentSound;
} State;

typedef struct {
	sfSprite	*sprite;
	sfTexture	*texture;
	char		*path;
	char		**extensions;
	int		nbOfExtensions;
} Sprite;

typedef struct {
	char		*name;
	struct stat	stats;
	bool		isEnd;
} FileInfos;

typedef struct {
	char	*path;
	char	*extension;
} Sprite_config;

typedef struct {
	sfSoundBuffer	*buffer;
	double		pitch;
} SoundBuffer;

struct data_s {
	sfRenderWindow	*window;
	Settings	*settings;
	State		*execState;
	MidiParser	*parserResult;
	sfRectangleShape*rect;
	sfText		*text;
	sfView		*view;
	bool		debug;
	bool		loading;
	bool		leave;
	SoundBuffer	*buffers;
	PlayingSound	*sounds;
	sfClock		*clock;
};

extern		sfFloatRect	frect;
extern	const	Sprite_config	configs[];

Sprite	*loadConfig(char *path);
void	ThreadFunc(void *args);
char	*getEventString(Event *event);
int	dispMsg(char *title, char *content, int variate);
char	*getMidiEventTypeString(EventType type);
NoteList	eventsToNotes(MidiParser *result);
bool    noEventsLeft(Event **events, int nbOfTracks);
void	displayPianoKeys(char playingNotes[MAX_CHANNELS][128], sfRectangleShape *rec, sfRenderWindow *win);
char	*exploreFile(char *path, sfFont *font, Sprite *sprites);
size_t	strlen_unicode(sfUint32 *str);
sfUint32*convertStringToUnicode(unsigned char *str, sfUint32 *buffer);
size_t	calcStringLen(sfUint32 *str);
char	*convertUnicodeToString(sfUint32 *str, char *buffer);
void	updateSounds(State *state, unsigned char volume, double time);
void	loadSounds(char *path, PlayingSound *sounds, SoundBuffer *soundBuffers, bool debug, Instrument instrument);
void	updateEvents(State *state, bool debug, SoundBuffer *soundBuffers, double time, MidiParser *result);
void	displayNote(unsigned char channel, unsigned char pitch, double startTime, double currentTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug);
void	displayNotesFromNotesList(Track *track, int begin, State *state, sfRectangleShape *rec, sfRenderWindow *win, bool debug, int *nbOfNotesDisplayed);
