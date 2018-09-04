#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>
#include <math.h>
#include <time.h>
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
#endif

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
		if (sfRenderWindow_hasFocus(window)) {
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
		} else
			nanosleep((struct timespec[1]){{0, 6666667}}, NULL);
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

int	main(int argc, char **args)
{
	MidiParser	*result;
	sfRenderWindow	*window;
	bool		debug = argc > 1 && (strcmp(args[1], "debug") == 0 || strcmp(args[1], "ddebug") == 0);
	sfText		*text = sfText_create();
	sfSound		***sounds;
	sfSoundBuffer	*soundBuffers[2][128];
	sfFont		*font;
	sfVideoMode	mode = {1280, 960, 32};
	char		*buffer;

	if (argc < 2) {
		printf("Usage: %s [ddebug] <file.mid>\n", args[0]);
		return (EXIT_FAILURE);
	}
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
	buffer = malloc(strlen(args[0]) + 10);
	sprintf(buffer, "%sarial.ttf", args[0]);
	font = sfFont_createFromFile(buffer);
	free(buffer);
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
//	sfRenderWindow_close(window);
	printf("Play list contains:\n");
	for (int i = 1 + debug; i < argc; i++)
		printf("- %s\n", args[i]);
	for (int i = 1 + debug; i < argc; i++) {
		sfRenderWindow_setTitle(window, args[i]);
		result = parseMidi(args[i], strcmp(args[1], "ddebug") == 0, true);
		if (!result) {
			printf("An error occurred when reading %s\nExit in 10 seconds\n", args[i]);
			nanosleep((struct timespec[1]){{10, 0}}, NULL);
		} else {
			printf("Finished to read %s: format %hi, %hi tracks, %i notes, ", args[i], result->format, result->nbOfTracks, result->nbOfNotes);
			if (result->fps) {
				printf("division: %i FPS and %i ticks/frame\n", result->fps, result->ticks);
			} else
				printf("division: %i ticks / 1/4 note\n", result->ticks);
			if (!displayMidi(args[0], result, debug, window, sounds, soundBuffers, text))
				i = argc;
			deleteMidiParserStruct(result);
		}
	}
	sfRenderWindow_destroy(window);
	sfFont_destroy(font);
	sfText_destroy(text);
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 128; j++) {
			sfSound_destroy(sounds[i][j]);
			sfSoundBuffer_destroy(soundBuffers[i][j]);
		}
	return (EXIT_SUCCESS);
}
