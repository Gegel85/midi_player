#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include "concatf.h"
#include "midi_parser.h"
#include "header.h"

#if defined _WIN32 || defined __WIN32 || defined __WIN32__
#include <windows.h>
#include <signal.h>

char	*strsignal(int signum)
{
	switch (signum) {
	case 2:
		return ("Interrupted");
	case 3:
		return ("Quit");
	case 4:
		return ("Illegal hardware instruction");
	case 6:
		return ("Aborted");
	case 7:
		return ("Bus error");
	case 8:
		return ("Floating point exception");
	case 10:
		return ("User defined signal 1");
	case 11:
		return ("Segmentation fault");
	case 12:
		return ("User defined signal 2");
	case 13:
		return ("Broken pipe");
	case 14:
		return ("Timer expired");
	case 15:
		return ("Terminated");
	default:
		return ("Unknown signal");
	}
}

void	sighandler(int signum)
{
	MessageBox(NULL, "Caught fatal signal.\n\nClick OK to close the application", strsignal(signum), 0);
	exit(EXIT_FAILURE);
	signal(11, NULL);
	*(char *)NULL = *(char *)NULL; //Let's do this kernel. Come on, I wait you !
}

char	*realpath(char *path, char *buffer)
{
	if (!GetFullPathNameA(path, MAX_PATH + 1, buffer, NULL))
		return (NULL);
	return (buffer);
}
#endif

int	my_strcmp(char *str1, char *str2)
{
	for (int i = 0; str1[i]; i++)
		if (tolower(str1[i]) != tolower(str2[i]))
			return (str1[i] - str2[i]);
	return (str1[strlen(str1)] - str2[strlen(str1)]);
}

char	*getMidiEventTypeString(EventType type)
{
	switch (type) {
	case MidiSequenceNumber:
		return ("MidiSequenceNumber");
	case MidiTextEvent:
		return ("MidiTextEvent");
	case MidiNewLyric:
		return ("MidiNewLyric");
	case MidiNewMarker:
		return ("MidiNewMarker");
	case MidiNewCuePoint:
		return ("MidiNewCuePoint");
	case MidiNewChannelPrefix:
		return ("MidiNewChannelPrefix");
	case MidiPortChange:
		return ("MidiPortChange");
	case MidiTempoChanged:
		return ("MidiTempoChanged");
	case MidiSMTPEOffset:
		return ("MidiSMTPEOffset");
	case MidiNewTimeSignature:
		return ("MidiNewTimeSignature");
	case MidiNewKeySignature:
		return ("MidiNewKeySignature");
	case MidiSequencerSpecificEvent:
		return ("MidiSequencerSpecificEvent");
	case MidiNoteReleased:
		return ("MidiNoteReleased");
	case MidiNotePressed:
		return ("MidiNotePressed");
	case MidiPolyphonicPressure:
		return ("MidiPolyphonicPressure");
	case MidiControllerValueChanged:
		return ("MidiControllerValueChanged");
	case MidiProgramChanged:
		return ("MidiProgramChanged");
	case MidiPressureOfChannelChanged:
		return ("MidiPressureOfChannelChanged");
	case MidiPitchBendChanged:
		return ("MidiPitchBendChanged");
	}
	return ("Unknown event");
}

char	*getEventString(Event *event)
{
	static char	buffer[1024];
	char		*type;

	if (!event)
		return (NULL);
	type = getMidiEventTypeString(event->type);
	if (event->type == MidiNoteReleased || event->type == MidiNotePressed)
		sprintf(buffer,
			"%s (%i) in %i ticks [channel: %i, pitch: %i (%s), velocity: %i]",
			type,
			event->type,
			event->timeToAppear,
			((MidiNote *)event->infos)->channel,
			((MidiNote *)event->infos)->pitch,
			getNoteString(((MidiNote *)event->infos)->pitch),
			((MidiNote *)event->infos)->velocity
		);
	else
		sprintf(buffer, "%s (%i) in %i ticks", type, event->type, event->timeToAppear);
	return (buffer);
}

bool	noEventsLeft(Event **events, int nbOfTracks)
{
	for (int i = 0; i < nbOfTracks; i++) {
		if (events[i]->infos || events[i]->type) {
			return (false);
		}
	}
	return (true);
}

bool	displayMidi(char *progPath, MidiParser *result, bool debug, sfRenderWindow *window, sfSound ***sounds, sfSoundBuffer *soundBuffers[2][128], sfText *text)
{
	sfEvent		event;
	sfView		*view = sfView_createFromRect(frect);
	sfClock		*clock;
	float		seconds;
	bool		returnValue = true;
	bool		isEnd = false;
	struct	data_s	data;
	char		buffer[1000];
	int		nbOfNotesDisplayed;
	exec_state_t	state;
static	settings_t	settings = {false, true, 50, PIANO, false, 0};
	sfThread	*thread;

	sfText_setColor(text, (sfColor){255, 255, 255, 255});
	sfRenderWindow_setView(window, view);
	memset(&data, 0, sizeof(data));
	memset(&state, 0, sizeof(state));
	state.bufferedTicks = malloc(result->nbOfTracks * sizeof(*state.bufferedTicks));
	state.begin = malloc(result->nbOfTracks * sizeof(*state.begin));
	state.events = malloc(result->nbOfTracks * sizeof(*state.events));
	if (!state.begin || !state.bufferedTicks || !state.events)
		exit(EXIT_FAILURE);
	memset(state.bufferedTicks, 0, result->nbOfTracks * sizeof(*state.bufferedTicks));
	memset(state.begin, 0, result->nbOfTracks * sizeof(*state.begin));
	settings.go = !debug;
	settings.speed = (float)result->ticks / 1000;
	state.nbOfTracks = result->nbOfTracks;
	data.settings = &settings;
	data.execState = &state;
	data.parserResult = result;
	data.rect = sfRectangleShape_create();
	data.text = text;
	data.clock = sfClock_create();
	data.sounds = sounds;
	sfRectangleShape_setOutlineColor(data.rect, (sfColor){0, 0, 0, 255});
	sfRectangleShape_setOutlineThickness(data.rect, 2);
	state.tempoInfos.signature.ticksPerQuarterNote = 24;
	for (int i = 0; i < result->nbOfTracks; i++)
		state.events[i] = result->tracks[i].events;
	sfText_setCharacterSize(data.text, 10);
	sfText_setPosition(data.text, (sfVector2f){0, frect.top});
	sfText_setScale(data.text, (sfVector2f){1, frect.height / 960});
	sfText_setPosition(data.text, (sfVector2f){0, frect.top});
	sfText_setScale(data.text, (sfVector2f){1, frect.height / 960});
	clock = sfClock_create();
	thread = sfThread_create(ThreadFunc, &data);
	sfThread_launch(thread);
	while (!isEnd) {
		seconds = sfTime_asSeconds(sfClock_getElapsedTime(clock));
		sfClock_restart(clock);
		while (sfRenderWindow_pollEvent(window, &event)) {
			if (event.type == sfEvtClosed) {
				sfRenderWindow_close(window);
				isEnd = true;
				returnValue = false;
			} else if (event.type == sfEvtKeyPressed) {
				if (event.key.code == sfKeySpace)
					settings.go = !settings.go;
				else if (event.key.code == sfKeyPageUp && settings.volume < 100)
					settings.volume++;
				else if (event.key.code == sfKeyPageDown && settings.volume > 0)
					settings.volume--;
				else if (event.key.code == sfKeyX)
					settings.dontDisplay = !settings.dontDisplay;
				else if (event.key.code == sfKeyM)
					settings.volume = 0;
				else if (event.key.code == sfKeyS)
					isEnd = true;
				else if (event.key.code == sfKeyU && settings.instrument != PIANO) {
					settings.instrument = PIANO;
					for (int i = 0; i < 2; i++)
						for (int j = 0; j < 128; j++) {
							sfSound_destroy(sounds[i][j]);
							sounds[i][j] = NULL;
							sfSoundBuffer_destroy(soundBuffers[i][j]);
							soundBuffers[i][j] = NULL;
						}
					data.loading = true;
					loadSounds(progPath, sounds, soundBuffers, debug, PIANO);
					data.loading = false;
					sfClock_restart(data.clock);
				} else if (event.key.code == sfKeyI && settings.instrument != SQUARE) {
					settings.instrument = SQUARE;
					for (int i = 0; i < 2; i++)
						for (int j = 0; j < 128; j++) {
							sfSound_destroy(sounds[i][j]);
							sounds[i][j] = NULL;
							sfSoundBuffer_destroy(soundBuffers[i][j]);
							soundBuffers[i][j] = NULL;
						}
					data.loading = true;
					loadSounds(progPath, sounds, soundBuffers, debug, SQUARE);
					data.loading = false;
					sfClock_restart(clock);
				} else if (event.key.code == sfKeyO && settings.instrument != SINUSOIDE) {
					settings.instrument = SINUSOIDE;
					for (int i = 0; i < 2; i++)
						for (int j = 0; j < 128; j++) {
							sfSound_destroy(sounds[i][j]);
							sounds[i][j] = NULL;
							sfSoundBuffer_destroy(soundBuffers[i][j]);
							soundBuffers[i][j] = NULL;
						}
					data.loading = true;
					loadSounds(progPath, sounds, soundBuffers, debug, SINUSOIDE);
					data.loading = false;
					sfClock_restart(clock);
				} else if (event.key.code == sfKeyP && settings.instrument != SAWTOOTH) {
					settings.instrument = SAWTOOTH;
					for (int i = 0; i < 2; i++)
						for (int j = 0; j < 128; j++) {
							sfSound_destroy(sounds[i][j]);
							sounds[i][j] = NULL;
							sfSoundBuffer_destroy(soundBuffers[i][j]);
							soundBuffers[i][j] = NULL;
						}
					data.loading = true;
					loadSounds(progPath, sounds, soundBuffers, debug, SAWTOOTH);
					data.loading = false;
					sfClock_restart(clock);
				} else if (!settings.go && event.key.code == sfKeyRight) {
					state.elapsedTicks += 100 * settings.speed;
					updateEvents(&state, sounds, debug, 100, settings.volume, result);
				} else if (event.key.code == sfKeyW)
					settings.displayHUD = !settings.displayHUD;
				else if (event.key.code == sfKeyLeft) {
					state.elapsedTicks -= 100 * settings.speed;
					for (int i = 0; i < 16; i++)
						for (int j = 0; j < 128; j++)
							state.playingNotes[i][j] = 0;
					for (int i = 0; i < result->nbOfTracks; i++) {
						state.bufferedTicks[i] = state.elapsedTicks - 100;
						state.events[i] = result->tracks[i].events;
					}
					state.notesPlayed = 0;
					for (int i = 0; i < result->nbOfTracks; i++)
						while ((state.events[i]->infos || state.events[i]->type) && state.events[i]->timeToAppear < state.bufferedTicks[i]) {
							state.bufferedTicks[i] -= state.events[i]->timeToAppear;
							if (state.events[i]->type == MidiNotePressed) {
								state.notesPlayed++;
								state.playingNotes[((MidiNote *)state.events[i]->infos)->channel][((MidiNote *)state.events[i]->infos)->pitch] = ((MidiNote *)state.events[i]->infos)->velocity;
							} else if (state.events[i]->type == MidiNoteReleased)
								state.playingNotes[((MidiNote *)state.events[i]->infos)->channel][((MidiNote *)state.events[i]->infos)->pitch] = 0;
							state.events[i]++;
						}
				} else if (event.key.code == sfKeyUp)
					settings.speed += 0.02;
				else if (event.key.code == sfKeyDown)
					settings.speed -= 0.02;
				else if (event.key.code == sfKeyAdd) {
					frect.height /= 1.1;
					frect.top = 960 - frect.height;
					sfView_reset(view, frect);
					sfRectangleShape_setOutlineThickness(data.rect, frect.height / 960 > 1 ? 2 : frect.height / 960 * 2);
					sfRenderWindow_setView(window, view);
					sfText_setPosition(data.text, (sfVector2f){0, frect.top});
					sfText_setScale(data.text, (sfVector2f){1, frect.height / 960});
				} else if (event.key.code == sfKeySubtract) {
					frect.height *= 1.1;
					frect.top = 960 - frect.height;
					sfView_reset(view, frect);
					sfRectangleShape_setOutlineThickness(data.rect, frect.height / 960 > 1 ? 2 : frect.height / 960 * 2);
					sfRenderWindow_setView(window, view);
					sfText_setPosition(data.text, (sfVector2f){0, frect.top});
					sfText_setScale(data.text, (sfVector2f){1, frect.height / 960});
				} else if (event.key.code == sfKeyHome) {
					for (int i = 0; i < 16; i++)
						memset(state.playingNotes[i], 0, sizeof(state.playingNotes[i]));
					state.elapsedTicks = 0;
					state.notesPlayed = 0;
					state.midiClockTicks = 0;
					for (int i = 0; i < result->nbOfTracks; i++) {
						state.bufferedTicks[i] = 0;
						state.begin[i] = 0;
						state.events[i] = result->tracks[i].events;
					}
				}
			}
		}
		//if (sfRenderWindow_hasFocus(window)) {
		        nbOfNotesDisplayed = 0;
			sfRenderWindow_clear(window, (sfColor){50, 155, 155, 255});
			if (!settings.dontDisplay) {
				for (int i = 0; i < result->nbOfTracks; i++)
					displayNotesFromNotesList(&result->tracks[i], state.begin[i], &state, data.rect, window, debug, &nbOfNotesDisplayed);
			}
			displayPianoKeys(state.playingNotes, data.rect, window);
			if (settings.displayHUD) {
				sprintf(buffer,
					"%.3f FPS\nTicks %.3f\nMidiclock ticks: %.3f\nSpeed: %.3f\nMicroseconds / clock tick: %i\nClock ticks / second: %i\nNotes on screen: %i\nNotes played: %u/%u\nZoom level: %.3f%%\nVolume: %u%%\nCurrent instrument: %s\n\n\nControls:%s\n",
					1 / seconds,
					state.elapsedTicks,
					state.midiClockTicks,
					settings.speed,
					state.tempoInfos.tempo,
					state.tempoInfos.signature.ticksPerQuarterNote * 128,
					nbOfNotesDisplayed,
					state.notesPlayed,
					result->nbOfNotes,
					960 * 100 / frect.height,
					settings.volume,
					settings.instrument == PIANO ? "Piano" :
					settings.instrument == SQUARE ? "Square wave" :
					settings.instrument == SINUSOIDE ? "Sin wave" :
					settings.instrument == SAWTOOTH ? "Sawtooth wave" : "Error",
					CONTROLS
				);
				sfText_setString(data.text, buffer);
				sfRenderWindow_drawText(window, text, NULL);
			}
			sfRenderWindow_display(window);
		//} else
		//	nanosleep((struct timespec[1]){{0, 6666667}}, NULL);
		if (!debug && noEventsLeft(state.events, result->nbOfTracks))
			isEnd = true;
        }
        data.leave = true;
	sfThread_wait(thread);
	sfThread_destroy(thread);
	sfClock_destroy(clock);
	sfClock_destroy(data.clock);
	sfRectangleShape_destroy(data.rect);
	sfView_destroy(data.view);
	sfRenderWindow_destroy(data.window);
	return (returnValue);
}

bool	playFile(char *path, char *progPath, bool debug, sfRenderWindow *window, sfSound ***sounds, sfSoundBuffer *soundBuffers[2][128], sfText *text)
{
	MidiParser	*result;

	printf("Reading file '%s'\n", path);
	sfRenderWindow_setTitle(window, path);
	result = parseMidi(path, false, true);
	if (!result) {
		printf("An error occurred when reading %s\n", path);
	} else {
		printf("Finished to read %s: format %hi, %hi tracks, %i notes, ", path, result->format, result->nbOfTracks, result->nbOfNotes);
		if (result->fps) {
			printf("division: %i FPS and %i ticks/frame\n", result->fps, result->ticks);
		} else
			printf("division: %i ticks / 1/4 note\n", result->ticks);
		if (!displayMidi(progPath, result, debug, window, sounds, soundBuffers, text))
			return false;
		deleteMidiParserStruct(result);
	}
	return (true);
}

FileInfos	*getDirEntries(char *path)
{
	DIR		*dirstream = opendir(path);
	FileInfos	*entries;
	struct dirent	*entry = NULL;
	int		len = 1;
	void		*buff;
	FileInfos	buffer;

	if (!dirstream) {
		printf("Error: %s: %s\n", path, strerror(errno));
		return (NULL);
	}
	entries = malloc(sizeof(*entries));
	entries->isEnd = true;
	for (entry = readdir(dirstream); entry; entry = readdir(dirstream)) {
		entries[len - 1].name = strdup(entry->d_name);
		entries[len - 1].isEnd = false;
		buff = concatf("%s/%s", path, entries[len - 1].name);
		stat(buff, &entries[len - 1].stats);
		buff = realloc(entries, ++len * sizeof(*entries));
		if (!buff) {
			free(entries);
			return (NULL);
		}
		entries = buff;
		entries[len - 1].isEnd = true;
	}
	for (int i = len - 1; i >= 0; i--)
		for (int j = 1; j < i; j++)
			if (
				((entries[j].stats.st_mode & S_IFMT) == S_IFDIR && (entries[j - 1].stats.st_mode & S_IFMT) != S_IFDIR) ||
				(
					(
						((entries[j].stats.st_mode & S_IFMT) != S_IFDIR && (entries[j - 1].stats.st_mode & S_IFMT) != S_IFDIR) ||
						((entries[j].stats.st_mode & S_IFMT) == S_IFDIR && (entries[j - 1].stats.st_mode & S_IFMT) == S_IFDIR)
					) &&
					strcmp(entries[j - 1].name, entries[j].name) > 0
				)
			) {
				buffer = entries[j - 1];
				entries[j - 1] = entries[j];
				entries[j] = buffer;
			}
	return (entries);
}

size_t	strlen_unicode(sfUint32 *str)
{
	size_t i = 0;

	for (; str[i]; i++);
	return (i);
}

float	getWordSize(sfUint32 *string, int start, int end, sfFont *font, int size)
{
	float	result = 0;

	for (int i = start; string[i] && i < end; i++)
		result += sfFont_getGlyph(font, string[i], size, false, 0).advance;
	return (result);
}

int	getWordPosition(sfUint32 *string, float position, sfFont *font, int size)
{
	float	result = 0;

	for (int i = 0; string[i]; i++) {
		if (result >= position)
			return (i);
		result += sfFont_getGlyph(font, string[i], size, false, 0).advance;
	}
	return (strlen_unicode(string));
}

sfUint32	*convertStringToUnicode(unsigned char *str, sfUint32 *buffer)
{
	int	bufferIndex = 0;

	if (!buffer && !(buffer = malloc(strlen(str) * sizeof(*buffer))))
		return (NULL);
	for (int i = 0; str[i]; i++)
		#if defined _WIN32 || defined __WIN32 || defined __WIN32__
		buffer[bufferIndex++] = str[i];
		#else
		if (str[i] >= 240) {
			buffer[bufferIndex++] = ((str[i] - 240) << 18) + ((str[i + 1] - 128) << 12) + ((str[i + 2] - 128) << 6) + str[i + 3] - 128;
			i += 3;
		} else if (str[i] >= 224) {
			buffer[bufferIndex++] = ((str[i] - 224) << 12) + ((str[i + 1] - 128) << 6) + str[i + 2] - 128;
			i += 2;
		} else if (str[i] >= 192) {
			buffer[bufferIndex++] = ((str[i] - 192) << 6) + str[i + 1] - 128;
			i++;
		} else
			buffer[bufferIndex++] = str[i];
		#endif
	buffer[bufferIndex] = 0;
	return (buffer);
}

size_t	calcStringLen(sfUint32 *str)
{
	int	len = 0;

	#if defined _WIN32 || defined __WIN32 || defined __WIN32__
	return (strlen_unicode(str));
	#else
	for (int i = 0; str[i]; i++) {
		if (str[i] < 0x80)
			len++;
		else if (str[i] < 0x800)
			len += 2;
		else if (str[i] < 0x10000)
			len += 3;
		else
			len += 4;
	}
	return (len);
	#endif
}

char	*convertUnicodeToString(sfUint32 *str, char *buffer)
{
	int	bufferIndex = 0;

	if (!buffer)
		buffer = malloc((calcStringLen(str) + 1) * sizeof(*buffer));
	if (!buffer)
		return (NULL);
	for (int i = 0; str[i]; i++)
		#if defined _WIN32 || defined __WIN32 || defined __WIN32__
		buffer[bufferIndex++] = str[i];
		#else
		if (str[i] < 0x80) {
			buffer[bufferIndex++] = str[i];
		} else if (str[i] < 0x800) {
			buffer[bufferIndex++] = (str[i] >> 6) + 192;
			buffer[bufferIndex++] = str[i] % (1 << 6) + 128;
		} else if (str[i] < 0x10000) {
			buffer[bufferIndex++] = (str[i] >> 12) + 224;
			buffer[bufferIndex++] = (str[i] >> 6) % (1 << 12) + 128;
			buffer[bufferIndex++] = str[i]  % (1 << 6) + 128;
		} else {
			buffer[bufferIndex++] = (str[i] >> 18) + 240;
			buffer[bufferIndex++] = (str[i] >> 12) % (1 << 18) + 128;
			buffer[bufferIndex++] = (str[i] >> 6) % (1 << 12) + 128;
			buffer[bufferIndex++] = str[i] % (1 << 6)+ 128;
		}
		#endif
	buffer[bufferIndex] = 0;
	return (buffer);
}

char	*exploreFile(char *path, sfFont *font, Sprite *sprites)
{
	sfVideoMode	mode = {600, 400, 32};
	sfRectangleShape*rect = sfRectangleShape_create();
	sfEvent		event;
	sfRenderWindow	*window = sfRenderWindow_create(mode, "Choose a path", sfClose, NULL);
	float		start = 0;
	int		selected = 0;
	char		*buff;
	void		*buf;
	char		realPath[PATH_MAX + 1];
	char		buffer[PATH_MAX + 2];
	sfUint32	bufferUnicode[PATH_MAX + 1];
	FileInfos	*direntry;
	bool		displayed = false;
	int		scrollBarPressed = 0;
	sfUint32	displayedPath[PATH_MAX + 2];
	int		len;
	int		menuSelected = -1;
	unsigned int	cursorPos = 0;
	sfVector2i	selectedText = {0, 0};
	sfText		*text = sfText_create();
	int		leftButtonIsPressed = 0;

	if (!window || !text || !rect) {
		if (window)
			sfRenderWindow_destroy(window);
		if (rect)
			sfRectangleShape_destroy(rect);
		if (text)
			sfText_destroy(text);
		return (NULL);
	}
	sfRenderWindow_setFramerateLimit(window, 60);
	sfText_setFont(text, font);
	memset(displayedPath, 0, sizeof(displayedPath));
	if (!realpath(path, realPath)) {
		buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
		printf("Error: %s: %s\n", realPath, strerror(errno));
		dispMsg("Error", buf, 0);
		free(buf);
		realpath(".", realPath);
	}
	direntry = getDirEntries(realPath);
	if (!direntry && errno != 0) {
		buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
		printf("Error: %s: %s\n", realPath, strerror(errno));
		dispMsg("Error", buf, 0);
		free(buf);
		realpath(".", realPath);
		direntry = getDirEntries(realPath);
	}
	if (!direntry) {
		sfRectangleShape_destroy(rect);
		sfRenderWindow_destroy(window);
		return (NULL);
	}
	sfText_setCharacterSize(text, 15);
	for (len = 0; !direntry[len].isEnd; len++);
	while (sfRenderWindow_isOpen(window)) {
		sfRectangleShape_setOutlineThickness(rect, 1);
		sfRectangleShape_setOutlineColor(rect, (sfColor){0, 0, 0, 255});
		sfRenderWindow_clear(window, (sfColor){200, 200, 200, 255});
		while (sfRenderWindow_pollEvent(window, &event)) {
			if (event.type == sfEvtClosed)
				sfRenderWindow_close(window);
			else if (event.type == sfEvtMouseWheelScrolled) {
				if (len < 15) continue;
				if (start - event.mouseWheelScroll.delta / 2 < 0)
					start = 0;
				else if (start - event.mouseWheelScroll.delta / 2 >= len - 15)
					start = len - 15;
				else
					start -= event.mouseWheelScroll.delta / 2;
			} else if (event.type == sfEvtMouseMoved && leftButtonIsPressed) {
				if (menuSelected == 1) {
					selectedText.y = getWordPosition(displayedPath, event.mouseMove.x - 30, font, 15);
					cursorPos = selectedText.y;
				}
				if (scrollBarPressed) {
					start = (float)((scrollBarPressed - event.mouseMove.y + 20) * (len - 15) * len) / (6000 - 300 * len);
					if (start < 0)
						start = 0;
					else if (start > len - 15)
						start = len - 15;
				}
			} else if ((event.type == sfEvtMouseButtonReleased && event.mouseButton.button == sfMouseLeft) || event.type == sfEvtMouseLeft) {
				leftButtonIsPressed--;
				scrollBarPressed = false;
				if (leftButtonIsPressed < 0)
					leftButtonIsPressed = 0;
			} else if (event.type == sfEvtMouseButtonPressed && event.mouseButton.button == sfMouseLeft) {
				leftButtonIsPressed++;
				if (
					event.mouseButton.x >= 569 &&
					event.mouseButton.x <= 580 &&
					event.mouseButton.y >= (float)start * 300 / (len - 15) - ((float)start / (len - 15)) * 6000 / len + 20 &&
					event.mouseButton.y <= (float)start * 300 / (len - 15) - ((float)start / (len - 15)) * 6000 / len + 20 + 6000 / len + 1
				)
					scrollBarPressed = event.mouseButton.y - ((float)start * 300 / (len - 15) - ((float)start / (len - 15)) * 6000 / len + 20);
				else if (event.mouseButton.x < 380 && event.mouseButton.x > 20 && event.mouseButton.y > 330 && event.mouseButton.y < 370) {
					if (menuSelected == 1) {
						selectedText.x = getWordPosition(displayedPath, event.mouseButton.x - 30, font, 15);
						selectedText.y = selectedText.x;
						cursorPos = selectedText.x;
					} else {
						menuSelected = 1;
						selectedText.x = 0;
						selectedText.y = strlen_unicode(displayedPath);
					}
				} else if (event.mouseButton.x > 20 && event.mouseButton.x < 570 && event.mouseButton.y > 20 && event.mouseButton.y < 320 && event.mouseButton.y < (len + 1) * 20) {
					if (menuSelected == 0 && (int)((float)event.mouseButton.y / 20 + start - 1) == selected) {
						displayedPath[0] = '\0';
						if ((direntry[selected].stats.st_mode & S_IFMT) == S_IFDIR) {
							buff = strdup(realPath);
							path = concatf("%s/%s", realPath, direntry[selected].name);
							if (!realpath(path, realPath)) {
								free(path);
								buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
								printf("Error: %s: %s\n", realPath, strerror(errno));
								dispMsg("Error", buf, 0);
								free(buf);
								strcpy(realPath, buff);
								free(buff);
								continue;
							}
							free(path);
							buf = getDirEntries(realPath);
							if (!buf) {
								buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
								printf("Error: %s: %s\n", realPath, strerror(errno));
								dispMsg("Error", buf, 0);
								free(buf);
								strcpy(realPath, buff);
								free(buff);
								continue;
							}
							for (len = 0; !direntry[len].isEnd; len++)
								free(direntry[len].name);
							free(direntry);
							direntry = buf;
							free(buff);
							for (len = 0; !direntry[len].isEnd; len++);
							selected = 0;
							start = 0;
							menuSelected = -1;
							if (!direntry) {
								sfRectangleShape_destroy(rect);
								sfRenderWindow_destroy(window);
								return (NULL);
							}
							continue;
						} else {
							path = concatf("%s/%s", realPath, direntry[selected].name);
							for (len = 0; !direntry[len].isEnd; len++)
								free(direntry[len].name);
							free(direntry);
							sfRectangleShape_destroy(rect);
							sfRenderWindow_destroy(window);
							return (path);
						}
					}
					menuSelected = 0;
					selected = (float)event.mouseButton.y / 20 + start - 1;
					convertStringToUnicode(direntry[selected].name, displayedPath);
				} else
					menuSelected = -1;
			} else if (event.type == sfEvtTextEntered) {
				if (event.text.unicode >= ' ' && strlen_unicode(displayedPath) < PATH_MAX - 1 && event.text.unicode != 127) {
					for (int i = selectedText.x > selectedText.y ? selectedText.y : selectedText.x; displayedPath[i + abs(selectedText.x - selectedText.y) - 1]; i++)
						displayedPath[i] = displayedPath[i + abs(selectedText.x - selectedText.y)];
					for (int i = strlen_unicode(displayedPath); i >= (int)cursorPos; i--)
						displayedPath[i + 1] = displayedPath[i];
					displayedPath[cursorPos++] = event.text.unicode;
					selectedText.x = cursorPos;
					selectedText.y = cursorPos;
				}
			} else if (event.type == sfEvtKeyPressed) {
				if (event.key.code == sfKeyA) {
					if (menuSelected == 1 && event.key.control) {
						selectedText.x = 0;
						selectedText.y = strlen_unicode(displayedPath);
						cursorPos = selectedText.y;
					}
				} else if (event.key.code == sfKeyUp) {
					if (menuSelected != 0) {
						menuSelected = 0;
						if (start > selected)
							start = selected;
						else if (start + 15 < selected)
							start = selected - 14;
						continue;
					}
					if (selected <= 0) {
						selected = len - 1;
						start = selected >= 15 ? selected - 14 : 0;
					} else {
						selected -= 1;
						if (start > selected)
							start = selected;
					}
				} else if (event.key.code == sfKeyDown) {
					if (menuSelected != 0) {
						menuSelected = 0;
						if (start > selected)
							start = selected;
						else if (start + 15 < selected)
							start = selected - 14;
						continue;
					}
					if (selected >= len - 1) {
						selected = 0;
						start = 0;
					} else {
						selected += 1;
						if (start + 14 <= selected)
							start = selected - 14;
					}
					convertStringToUnicode(direntry[selected].name, displayedPath);
				} else if (event.key.code == sfKeyLeft) {
					if (menuSelected == 1) {
						if (cursorPos > 0)
							cursorPos -= 1;
						if (!event.key.shift)
							selectedText.x = cursorPos;
						selectedText.y = cursorPos;
					} else if (menuSelected == 0) {
						if (start > selected)
							start = selected;
						else if (start + 15 < selected)
							start = selected - 14;
						convertStringToUnicode(direntry[selected].name, displayedPath);
					}
				} else if (event.key.code == sfKeyRight) {
					if (menuSelected == 1) {
						if (cursorPos < strlen_unicode(displayedPath))
							cursorPos += 1;
						if (!event.key.shift)
							selectedText.x = cursorPos;
						selectedText.y = cursorPos;
					} else if (menuSelected == 0) {
						if (start > selected)
							start = selected;
						else if (start + 15 < selected)
							start = selected - 14;
						convertStringToUnicode(direntry[selected].name, displayedPath);
					}
				} else if (event.key.code == sfKeyHome) {
					if (menuSelected == 1) {
						cursorPos = 0;
						if (!event.key.shift)
							selectedText.x = cursorPos;
						selectedText.y = cursorPos;
					}
				} else if (event.key.code == sfKeyEnd) {
					if (menuSelected == 1) {
						cursorPos = strlen_unicode(displayedPath);
						if (!event.key.shift)
							selectedText.x = cursorPos;
						selectedText.y = cursorPos;
					}
				} else if (event.key.code == sfKeyPageUp) {
					if (menuSelected == 0) {
						if (selected <= 15) {
							selected = 0;
							start = 0;
						} else {
							selected -= 15;
							start = selected;
						}
						convertStringToUnicode(direntry[selected].name, displayedPath);
					} else {
						if (start <= 15)
							start = 0;
						else
							start -= 15;
					}
				} else if (event.key.code == sfKeyPageDown) {
					if (menuSelected == 0) {
						if (selected >= len - 15) {
							selected = len - 1;
							start = len - 15;
						} else {
							selected += 15;
							start = selected - 14;
						}
						convertStringToUnicode(direntry[selected].name, displayedPath);
					} else {
						if (start >= len - 30)
							start = len - 15;
						else
							start += 15;
					}
				} else if (event.key.code == sfKeySpace) {
					if (menuSelected == -1)
						menuSelected = 0;
					if (menuSelected == 0) {
						if (start > selected)
							start = selected;
						else if (start + 15 < selected)
							start = selected - 14;
						convertStringToUnicode(direntry[selected].name, displayedPath);
					}
				}
				#ifndef sfKeyBackspace
				else if (event.key.code == sfKeyBack) {
				#else
				else if (event.key.code == sfKeyBackspace) {
				#endif
					if (menuSelected == 1 && selectedText.x != selectedText.y) {
						for (int i = selectedText.x > selectedText.y ? selectedText.y : selectedText.x; displayedPath[i + abs(selectedText.x - selectedText.y) - 1]; i++)
							displayedPath[i] = displayedPath[i + abs(selectedText.x - selectedText.y)];
						selectedText.x = 0;
						selectedText.y = 0;
					} else if (menuSelected == 1 && cursorPos) {
						for (int i = --cursorPos; i == 0 || displayedPath[i - 1]; i++)
							displayedPath[i] = displayedPath[i + 1];
					}
				} else if (event.key.code == sfKeyDelete) {
					if (menuSelected == 1 && selectedText.x != selectedText.y) {
						for (int i = selectedText.x > selectedText.y ? selectedText.y : selectedText.x; displayedPath[i + abs(selectedText.x - selectedText.y) - 1]; i++)
							displayedPath[i] = displayedPath[i + abs(selectedText.x - selectedText.y)];
						selectedText.x = 0;
						selectedText.y = 0;
					} else if (menuSelected == 1 && cursorPos < strlen_unicode(displayedPath)) {
						for (int i = cursorPos; i == 0 || displayedPath[i - 1]; i++)
							displayedPath[i] = displayedPath[i + 1];
					}
				}
				#ifndef sfKeyEnter
				else if (event.key.code == sfKeyReturn) {
				#else
				else if (event.key.code == sfKeyEnter) {
				#endif
					struct stat	stats;

					if (menuSelected == 0) {
						path = concatf("%s/%s", realPath, direntry[selected].name);
						stats = direntry[selected].stats;
					} else if (menuSelected == 1) {
						#if defined _WIN32 || defined __WIN32 || defined __WIN32__
						if (strlen_unicode(displayedPath) < 2 || displayedPath[1] != ':')
							path = concatf("%s\\%s", realPath, convertUnicodeToString(displayedPath, buffer));
						#else
						if (displayedPath[0] != '/')
							path = concatf("%s/%s", realPath, convertUnicodeToString(displayedPath, buffer));
						#endif
						else
							path = strdup(convertUnicodeToString(displayedPath, buffer));
						if (stat(path, &stats) < 0) {
							buf = concatf("Error: %s: %s\n", path, strerror(errno));
							printf("Error: %s: %s\n", path, strerror(errno));
							dispMsg("Error", buf, 0);
							free(path);
							free(buf);
							continue;
						}
					}
					if (!realpath(path, realPath)) {
						free(path);
						buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
						printf("Error: %s: %s\n", realPath, strerror(errno));
						dispMsg("Error", buf, 0);
						free(buf);
						strcpy(realPath, buff);
						free(buff);
						continue;
					}
					convertStringToUnicode(path, displayedPath);
					selectedText.x = 0;
					selectedText.y = menuSelected == 1 ? strlen_unicode(displayedPath) : 0;
					cursorPos = selectedText.y;
					if ((stats.st_mode & S_IFMT) == S_IFDIR) {
						buff = strdup(realPath);
						free(path);
						buf = getDirEntries(realPath);
						if (!buf) {
							buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
							printf("Error: %s: %s\n", realPath, strerror(errno));
							dispMsg("Error", buf, 0);
							free(buf);
							strcpy(realPath, buff);
							free(buff);
							continue;
						}
						for (len = 0; !direntry[len].isEnd; len++)
							free(direntry[len].name);
						free(direntry);
						direntry = buf;
						free(buff);
						for (len = 0; !direntry[len].isEnd; len++);
						selected = 0;
						start = 0;
						if (!direntry) {
							sfRectangleShape_destroy(rect);
							sfRenderWindow_destroy(window);
							return (NULL);
						}
					} else {
						for (len = 0; !direntry[len].isEnd; len++)
							free(direntry[len].name);
						free(direntry);
						sfRectangleShape_destroy(rect);
						sfRenderWindow_destroy(window);
						return (path);
					}
				}
			}
		}
		sfText_setColor(text, (sfColor){0, 0, 0, 255});
		sfRectangleShape_setPosition(rect, (sfVector2f){20, 20});
		sfRectangleShape_setSize(rect, (sfVector2f){560, 300});
		sfRectangleShape_setFillColor(rect, (sfColor){125, 125, 125, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		for (int i = start; !direntry[i].isEnd && i - start < 15; i++) {
			displayed = false;
			if (menuSelected == 0 && selected == i) {
				sfText_setColor(text, (sfColor){255, 255, 255, 255});
				sfRectangleShape_setFillColor(rect, (sfColor){0, 0, 255, 255});
				sfRectangleShape_setPosition(rect, (sfVector2f){20, 20 + (i - start) * 20});
				sfRectangleShape_setSize(rect, (sfVector2f){560, 20});
				sfRenderWindow_drawRectangleShape(window, rect, NULL);
			} else if (menuSelected == 0 && selected + 1 == i)
				sfText_setColor(text, (sfColor){0, 0, 0, 255});
			for (int j = 0; (j == 0 || sprites[j - 1].extensions) && !displayed; j++)
				for (int k = 0; !k || sprites[j].nbOfExtensions > k; k++) {
					if (!sprites[j].extensions) {
						sfSprite_setPosition(sprites[j].sprite, (sfVector2f){24, 22 + (i - start) * 20});
						sfRenderWindow_drawSprite(window, sprites[j].sprite, NULL);
						displayed = true;
					} else if (strcmp(sprites[j].extensions[k], "folder") == 0 && (direntry[i].stats.st_mode & S_IFMT) == S_IFDIR) {
						sfSprite_setPosition(sprites[j].sprite, (sfVector2f){24, 22 + (i - start) * 20});
						sfRenderWindow_drawSprite(window, sprites[j].sprite, NULL);
						displayed = true;
					} else if (
						strlen(direntry[i].name) >= strlen(sprites[j].extensions[k]) &&
						my_strcmp(
							&direntry[i].name[
								strlen(direntry[i].name) - strlen(sprites[j].extensions[k])
							],
							sprites[j].extensions[k]
						) == 0
					) {
						sfSprite_setPosition(sprites[j].sprite, (sfVector2f){24, 22 + (i - start) * 20});
						sfRenderWindow_drawSprite(window, sprites[j].sprite, NULL);
						displayed = true;
					}
				}
			sfText_setUnicodeString(text, convertStringToUnicode(direntry[i].name, bufferUnicode));
			sfText_setPosition(text, (sfVector2f){40, 20 + (i - start) * 20});
			sfRenderWindow_drawText(window, text, NULL);
		}
		sfText_setColor(text, (sfColor){0, 0, 0, 255});
		sfRectangleShape_setOutlineThickness(rect, 0);
		sfRectangleShape_setPosition(rect, (sfVector2f){0, 0});
		sfRectangleShape_setSize(rect, (sfVector2f){600, 20});
		sfRectangleShape_setFillColor(rect, (sfColor){200, 200, 200, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){0, 320});
		sfRectangleShape_setSize(rect, (sfVector2f){600, 80});
		sfRectangleShape_setFillColor(rect, (sfColor){200, 200, 200, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){20, 330});
		sfRectangleShape_setSize(rect, (sfVector2f){360, 30});
		sfRectangleShape_setFillColor(rect, (sfColor){255, 255, 255, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){580, 20});
		sfRectangleShape_setSize(rect, (sfVector2f){20, 300});
		sfRectangleShape_setFillColor(rect, (sfColor){200, 200, 200, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){569, 20});
		sfRectangleShape_setSize(rect, (sfVector2f){11, 300});
		sfRectangleShape_setFillColor(rect, (sfColor){75, 75, 75, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfText_setUnicodeString(text, displayedPath);
		sfText_setPosition(text, (sfVector2f){30, 335});
		if (menuSelected == 1) {
			if (selectedText.x > selectedText.y) {
				sfRectangleShape_setPosition(rect, (sfVector2f){getWordSize(displayedPath, 0, selectedText.y, font, 15) + 30, 335});
				sfRectangleShape_setSize(rect, (sfVector2f){getWordSize(displayedPath, selectedText.y, selectedText.x, font, 15), 20});
			} else {
				sfRectangleShape_setPosition(rect, (sfVector2f){getWordSize(displayedPath, 0, selectedText.x, font, 15) + 30, 335});
				sfRectangleShape_setSize(rect, (sfVector2f){getWordSize(displayedPath, selectedText.x, selectedText.y, font, 15), 20});
			}
			sfRectangleShape_setFillColor(rect, (sfColor){0, 0, 255, 255});
			sfRenderWindow_drawRectangleShape(window, rect, NULL);
			if (time(NULL) % 2) {
				sfRectangleShape_setOutlineThickness(rect, 1);
				sfRectangleShape_setPosition(rect, (sfVector2f){getWordSize(displayedPath, 0, cursorPos, font, 15) + 30, 335});
				sfRectangleShape_setSize(rect, (sfVector2f){0, 20});
				sfRectangleShape_setFillColor(rect, (sfColor){0, 0, 0, 255});
				sfRenderWindow_drawRectangleShape(window, rect, NULL);
				sfRectangleShape_setOutlineThickness(rect, 0);
			}
		}
		sfRenderWindow_drawText(window, text, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){380, 320});
		sfRectangleShape_setSize(rect, (sfVector2f){220, 80});
		sfRectangleShape_setFillColor(rect, (sfColor){200, 200, 200, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		if (len > 15) {
			sfRectangleShape_setPosition(rect, (sfVector2f){570, (float)(300 * start * len - 6000 * start) / ((len - 15) * len) + 20});
			sfRectangleShape_setSize(rect, (sfVector2f){10, 6000 / len + 1});
			sfRectangleShape_setFillColor(rect, (sfColor){255, 255, 255, 255});
			sfRenderWindow_drawRectangleShape(window, rect, NULL);
		}
		sfRectangleShape_setOutlineThickness(rect, 1);
		sfRectangleShape_setPosition(rect, (sfVector2f){20, 330});
		sfRectangleShape_setSize(rect, (sfVector2f){360, 30});
		sfRectangleShape_setFillColor(rect, (sfColor){255, 255, 255, 0});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){20, 20});
		sfRectangleShape_setSize(rect, (sfVector2f){560, 300});
		sfRectangleShape_setFillColor(rect, (sfColor){200, 200, 200, 0});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfText_setColor(text, (sfColor){0, 0, 0, 255});
		sfText_setUnicodeString(text, convertStringToUnicode(realPath, bufferUnicode));
		sfText_setPosition(text, (sfVector2f){10, 0});
		sfRectangleShape_setPosition(rect, (sfVector2f){400, 330});
		sfRectangleShape_setSize(rect, (sfVector2f){60, 30});
		sfRectangleShape_setFillColor(rect, (sfColor){255, 255, 255, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfText_setColor(text, (sfColor){0, 0, 0, 255});
		sfText_setString(text, "Open");
		sfText_setPosition(text, (sfVector2f){410, 335});
		sfRenderWindow_drawText(window, text, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){470, 330});
		sfRectangleShape_setSize(rect, (sfVector2f){60, 30});
		sfRectangleShape_setFillColor(rect, (sfColor){255, 255, 255, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfText_setColor(text, (sfColor){0, 0, 0, 255});
		sfText_setString(text, "Cancel");
		sfText_setPosition(text, (sfVector2f){475, 335});
		sfRenderWindow_drawText(window, text, NULL);
		sfRenderWindow_display(window);
	}
	sfRectangleShape_destroy(rect);
	sfRenderWindow_destroy(window);
	for (len = 0; !direntry[len].isEnd; len++)
		free(direntry[len].name);
	free(direntry);
	return (NULL);
}

Sprite	*loadConfig(char *path)
{
	Sprite	*array = malloc(sizeof(*array));
	int	len = 1;
	bool	found = false;
	void	*buff;
	char	buffer[PATH_MAX];

	memset(array, 0, sizeof(*array));
	for (int i = 0; ; i++) {
		found = false;
		if (!configs[i].extension) {
			array[len - 1].path = configs[i].path;
			array[len - 1].sprite = sfSprite_create();
			sprintf(buffer, "%s%s", path, configs[i].path);
			array[len - 1].texture = sfTexture_createFromFile(buffer, NULL);
			array[len - 1].extensions = NULL;
			array[len - 1].nbOfExtensions = 0;
			sfSprite_setTexture(array[len - 1].sprite, array[len - 1].texture, sfFalse);
			break;
		} else
			for (int j = 0; j < len - 1; j++) {
				if (strcmp(array[j].path, configs[i].path) == 0) {
					found = true;
					buff = realloc(array[j].extensions, ++array[j].nbOfExtensions * sizeof(*array[j].extensions));
					if (!buff) {
						for (int j = 0; j < len; i++) {
							sfSprite_destroy(array[i].sprite);
							sfTexture_destroy(array[i].texture);
							free(array[i].extensions);
						}
						free(array);
						return (NULL);
					}
					array[j].extensions = buff;
					array[j].extensions[array[j].nbOfExtensions - 1] = configs[i].extension;
					break;
				}
			}
		if (!found) {
			buff = realloc(array, ++len * sizeof(*array));
			if (!buff) {
				for (int j = 0; j < len - 1; i++) {
					sfSprite_destroy(array[i].sprite);
					sfTexture_destroy(array[i].texture);
					free(array[i].extensions);
				}
				free(array);
				return (NULL);
			}
			array = buff;
			array[len - 2].path = configs[i].path;
			array[len - 2].sprite = sfSprite_create();
			sprintf(buffer, "%s%s", path, configs[i].path);
			array[len - 2].texture = sfTexture_createFromFile(buffer, NULL);
			array[len - 2].extensions = malloc(sizeof(*array[len - 2].extensions));
			array[len - 2].nbOfExtensions = 1;
			if (!array[len - 2].extensions) {
				for (int j = 0; j < len - 1; i++) {
					sfSprite_destroy(array[i].sprite);
					sfTexture_destroy(array[i].texture);
					free(array[i].extensions);
				}
				free(array);
				return (NULL);
			}
			*array[len - 2].extensions = configs[i].extension;
			sfSprite_setTexture(array[len - 2].sprite, array[len - 2].texture, sfFalse);
			memset(&array[len - 1], 0, sizeof(*array));
		}
	}
	return (array);
}

char	*loadFile(char *path)
{
	char	*buffer = malloc(PATH_MAX + 1);
	int	fd;

	if (!buffer)
		return (NULL);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (NULL);
	buffer[read(fd, buffer, PATH_MAX)] = '\0';
	close(fd);
	return (buffer);
}

void	saveToFile(char *path, char *content)
{
	int	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if (fd < 0)
		return;
	write(fd, content, strlen(content));
	close(fd);
}

int	main(int argc, char **args)
{
	sfRenderWindow	*window;
	bool		debug = argc > 1 && (strcmp(args[1], "debug") == 0 || strcmp(args[1], "ddebug") == 0);
	sfText		*text = sfText_create();
	sfSound		***sounds;
	sfSoundBuffer	*soundBuffers[2][128];
	sfFont		*font;
	sfVideoMode	mode = {1280, 960, 32};
	char		*buffer;
	char		*path = NULL;
	Sprite		*sprites;
	char		*lastPath = NULL;

	#ifdef _WIN32
		signal(SIGSEGV, sighandler);
		signal(SIGABRT, sighandler);
	#endif
	sounds = malloc(sizeof(*sounds) * 2);
	if (!sounds)
		return EXIT_FAILURE;
	*sounds = malloc(sizeof(**sounds) * 2 * 128);
	if (!*sounds)
		return EXIT_FAILURE;
	sounds[1] = *sounds + 128;
	window = sfRenderWindow_create(mode, args[0], sfClose | sfResize, NULL);
	sfRenderWindow_setFramerateLimit(window, 60);
	for (int i = strlen(args[0]) - 1; i >= 0; i--)
		if (args[0][i] == '/' || args[0][i] == '\\') {
			args[0][i + 1] = 0;
			break;
		} else if (i == 0) {
			args[0][0] = '.';
			args[0][1] = '/';
			args[0][2] = '\0';
		}
	sprites = loadConfig(args[0]);
	buffer = malloc(strlen(args[0]) + 10);
	sprintf(buffer, "%sarial.ttf", args[0]);
	font = sfFont_createFromFile(buffer);
	if (!window || !text)
		return EXIT_FAILURE;
	sfText_setCharacterSize(text, 20);
	sfText_setFont(text, font);
	sfText_setColor(text, (sfColor){255, 255, 255, 255});
	sfText_setPosition(text, (sfVector2f){500, 450});
	sfText_setString(text, "Loading Resources");
	sfRenderWindow_clear(window, (sfColor){50, 155, 155, 255});
	sfRenderWindow_drawText(window, text, NULL);
	sfRenderWindow_display(window);
	loadSounds(args[0], sounds, soundBuffers, debug, PIANO);
	sprintf(buffer, "%slastPath", args[0]);
	if (argc == 1 || (argc == 2 && strcmp(args[1], "debug") == 0)) {
		path = loadFile(buffer) ?: strdup(args[0]);
		do {
			free(lastPath);
			lastPath = path;
			for (int i = strlen(lastPath) - 1; i >= 0; i--) {
				if (lastPath[i] == '/' || lastPath[i] == '\\') {
					lastPath[i + 1] = 0;
					break;
				} else if (i == 0) {
					lastPath[0] = '.';
					lastPath[1] = '/';
					lastPath[2] = '\0';
				}
			}
			path = exploreFile(lastPath, font, sprites);
		} while (path && playFile(path, args[0], debug, window, sounds, soundBuffers, text));
		if (path) {
			for (int i = strlen(path) - 1; i >= 0; i--) {
				if (path[i] == '/' || path[i] == '\\') {
					path[i + 1] = 0;
					break;
				} else if (i == 0) {
					path[0] = '.';
					path[1] = '/';
					path[2] = '\0';
				}
			}
			saveToFile(buffer, path);
		} else if (lastPath) {
			for (int i = strlen(lastPath) - 1; i >= 0; i--) {
				if (lastPath[i] == '/' || lastPath[i] == '\\') {
					lastPath[i + 1] = 0;
					break;
				} else if (i == 0) {
					lastPath[0] = '.';
					lastPath[1] = '/';
					lastPath[2] = '\0';
				}
			}
			saveToFile(buffer, lastPath);
		}
		free(path);
		free(lastPath);
	} else {
		printf("Play list contains:\n");
		for (int i = 1 + debug; i < argc; i++)
			printf("- %s\n", args[i]);
		for (int i = 1 + debug; i < argc && playFile(args[i], args[0], debug, window, sounds, soundBuffers, text); i++);
	}
	sfRenderWindow_destroy(window);
	sfFont_destroy(font);
	sfText_destroy(text);
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 128; j++) {
			sfSound_destroy(sounds[i][j]);
			sfSoundBuffer_destroy(soundBuffers[i][j]);
		}
	free(buffer);
	return (EXIT_SUCCESS);
}
