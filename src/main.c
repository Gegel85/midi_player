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
