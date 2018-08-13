#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>
#include <math.h>
#include <time.h>
#include "midi_parser.h"
#include "header.h"

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

bool	noEventsLeft(EventList **events, int nbOfTracks)
{
	for (int i = 0; i < nbOfTracks; i++) {
		if (events[i]) {
			return (false);
		}
	}
	return (true);
}

void	displayMidi(MidiParser *result, bool debug, sfRenderWindow *window, sfSound *sounds[2][128], sfText *text)
{
	EventList	*events[result->nbOfTracks];
	sfEvent		event;
	NoteList	notes;
	sfRectangleShape*rect = sfRectangleShape_create();
	char		playingNotes[16][128];
	unsigned short	notesVolume[2][128];
	char		fadeSpeed[2][128];
	double		elapsedTicks = 0;
	char		buffer[1000];
	double		tmp[result->nbOfTracks];
	MidiInfos	infos;
	bool		go = !debug;
	bool		pressed = false;
	double		speed = (float)result->ticks / 1000;
	unsigned int	notesPlayed = 0;
	bool		isEnd = false;
	sfClock		*clock;
	double		time;
	unsigned char	volume = 100;
	sfView		*view = sfView_createFromRect(frect);
	bool		fromEvent = true;
	bool		dontDisplay = false;
	float		seconds;
	double		midiClockTicks = 0;
	bool		displayHUD = true;
	int		nbOfNotesDisplayed = 0;
	
	for (int i = 0; i < 2; i++)
		for (int k = 0; k < 128; k++) {
			notesVolume[i][k] = 0;
			fadeSpeed[i][k] = 0;
		}
	for (int i = 0; i < 16; i++)
		for (int j = 0; j < 128; j++)
			playingNotes[i][j] = 0;
	memset(tmp, 0, sizeof(tmp));
	memset(&infos, 0, sizeof(infos));
	sfRectangleShape_setOutlineColor(rect, (sfColor){0, 0, 0, 255});
	sfRectangleShape_setOutlineThickness(rect, 2);
	infos.signature.ticksPerQuarterNote = 24;
	for (int i = 0; i < result->nbOfTracks; i++)
		events[i] = &result->tracks[i].events;
	sfText_setCharacterSize(text, 10);
	sfText_setPosition(text, (sfVector2f){0, frect.top});
	sfText_setScale(text, (sfVector2f){1, frect.height / 960});
	sfRenderWindow_setView(window, view);
	sfText_setPosition(text, (sfVector2f){0, frect.top});
	sfText_setScale(text, (sfVector2f){1, frect.height / 960});
	clock = sfClock_create();
	while (!isEnd) {
		pressed = false;
		seconds = sfTime_asSeconds(sfClock_getElapsedTime(clock));
		sfClock_restart(clock);
		time = speed * seconds * infos.signature.ticksPerQuarterNote * 128000000 / (infos.tempo ?: 10000000);
		while (sfRenderWindow_isOpen(window) && sfRenderWindow_pollEvent(window, &event)) {
			if (event.type == sfEvtClosed) {
				sfRenderWindow_close(window);
				isEnd = true;
			} else if (event.type == sfEvtKeyPressed) {
				if (event.key.code == sfKeySpace)
					go = !go;
				else if (event.key.code == sfKeyPageUp && volume < 100)
					volume++;
				else if (event.key.code == sfKeyPageDown && volume > 0)
					volume--;
				else if (event.key.code == sfKeyA)
					sfRenderWindow_close(window);
				else if (event.key.code == sfKeyX)
					dontDisplay = !dontDisplay;
				else if (event.key.code == sfKeyM)
					volume = 0;
				else if (!go && event.key.code == sfKeyRight) {
					elapsedTicks += 100 * speed;
					pressed = debug;
					updateEvents(events, tmp, result->nbOfTracks, playingNotes, &infos, sounds, notesVolume, fadeSpeed, debug, &notesPlayed, 100, volume);
					if (!fromEvent)
						displayNotesFromNotesList(&notes, elapsedTicks, rect, window, debug);
				} else if (fromEvent && event.key.code == sfKeyW) {
					displayHUD = !displayHUD;
					/* sfText_setPosition(text, (sfVector2f){500, 450});
					sfText_setCharacterSize(text, 20);
					sfText_setString(text, "Converting midi events");
					sfRenderWindow_clear(window, (sfColor){50, 155, 155, 255});
					sfRenderWindow_drawText(window, text, NULL);
					sfRenderWindow_display(window);
					sfText_setCharacterSize(text, 10);
					notes = eventsToNotes(result);
					fromEvent = false; */
				} else if (event.key.code == sfKeyD)
					pressed = debug;
				else if (event.key.code == sfKeyLeft) {
					elapsedTicks -= 100 * speed;
					for (int i = 0; i < 16; i++)
						for (int j = 0; j < 128; j++)
							playingNotes[i][j] = 0;
					pressed = debug;
					for (int i = 0; i < result->nbOfTracks; i++) {
						tmp[i] = elapsedTicks - 100;
						events[i] = &result->tracks[i].events;
					}
					notesPlayed = 0;
					for (int i = 0; fromEvent && i < result->nbOfTracks; i++)
						while (events[i] && events[i]->data->timeToAppear < tmp[i]) {
							tmp[i] -= events[i]->data->timeToAppear;
							if (events[i]->data->type == MidiNotePressed) {
								notesPlayed++;
								playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch] = ((MidiNote *)events[i]->data->infos)->velocity;
							} else if (events[i]->data->type == MidiNoteReleased)
								playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch] = 0;
							events[i] = events[i]->next;
						}
				} else if (event.key.code == sfKeyUp)
					speed += 0.02;
				else if (event.key.code == sfKeyDown)
					speed -= 0.02;
				else if (event.key.code == sfKeyAdd) {
					frect.height /= 1.1;
					frect.top = 960 - frect.height;
					sfView_reset(view, frect);
					sfRectangleShape_setOutlineThickness(rect, frect.height / 960 > 1 ? 2 : frect.height / 960 * 2);
					sfRenderWindow_setView(window, view);
					sfText_setPosition(text, (sfVector2f){0, frect.top});
					sfText_setScale(text, (sfVector2f){1, frect.height / 960});
				} else if (event.key.code == sfKeySubtract) {
					frect.height *= 1.1;
					frect.top = 960 - frect.height;
					sfView_reset(view, frect);
					sfRectangleShape_setOutlineThickness(rect, frect.height / 960 > 1 ? 2 : frect.height / 960 * 2);
					sfRenderWindow_setView(window, view);
					sfText_setPosition(text, (sfVector2f){0, frect.top});
					sfText_setScale(text, (sfVector2f){1, frect.height / 960});
				} else if (event.key.code == sfKeyHome) {
					for (int i = 0; i < 16; i++)
						memset(playingNotes[i], 0, sizeof(playingNotes[i]));
					pressed = debug;
					elapsedTicks = 0;
					notesPlayed = 0;
					midiClockTicks = 0;
					for (int i = 0; i < result->nbOfTracks; i++) {
						tmp[i] = 0;
						events[i] = &result->tracks[i].events;
					}
				}
			}
		}
		if (go || !sfRenderWindow_isOpen(window)) {
			elapsedTicks += time;
			midiClockTicks += 128 * infos.signature.ticksPerQuarterNote * seconds;
			updateEvents(events, tmp, result->nbOfTracks, playingNotes, &infos, sounds, notesVolume, fadeSpeed, debug, &notesPlayed, time, volume);
			updateSounds(sounds, notesVolume, fadeSpeed, volume, seconds);
		}
		if (sfRenderWindow_isOpen(window)) {
			sfRenderWindow_clear(window, (sfColor){50, 155, 155, 255});
			if (!dontDisplay) {
				if (fromEvent)
					displayNotes(events, tmp, playingNotes, result->nbOfTracks, window, rect, &nbOfNotesDisplayed, pressed);
				else
					displayNotesFromNotesList(&notes, elapsedTicks, rect, window, debug);
			}
			displayPianoKeys(playingNotes, rect, window);
			if (displayHUD) {
				sprintf(buffer,
					"%.3f FPS\nTicks %.3f\nMidiclock ticks: %.3f\nSpeed: %.3f\nMicroseconds / clock tick: %i\nClock ticks / second: %i\nNotes on screen: %i\nNotes played: %u/%u\nZoom level: %.3f%%\nVolume: %u%%\n\n\nControls:%s\n",
					1 / seconds,
					elapsedTicks,
					midiClockTicks,
					speed,
					infos.tempo,
					infos.signature.ticksPerQuarterNote * 128,
					nbOfNotesDisplayed,
					notesPlayed,
					result->nbOfNotes,
					960 * 100 / frect.height,
					volume,
					CONTROLS
				);
				sfText_setString(text, buffer);
				sfRenderWindow_drawText(window, text, NULL);
			}
			sfRenderWindow_display(window);
		} else
			nanosleep((struct timespec[1]){{0, 16666667}}, NULL);
		if (!debug && noEventsLeft(events, result->nbOfTracks))
			isEnd = true;
        }
	sfClock_destroy(clock);
}

int	main(int argc, char **args)
{
	MidiParser	*result;
	sfRenderWindow	*window;
	bool		debug = argc > 1 && (strcmp(args[1], "debug") == 0 || strcmp(args[1], "ddebug") == 0);
	sfSound		*sounds[2][128];
	sfSoundBuffer	*soundBuffers[2][128];
	sfText		*text = sfText_create();
	sfFont		*font;
	sfVideoMode	mode = {1280, 960, 32};
	char		*buffer;

	if (argc < 2) {
		printf("Usage: %s [ddebug] <file.mid>\n", args[0]);
		return (EXIT_FAILURE);
	}
	window = sfRenderWindow_create(mode, args[0], sfClose | sfResize, NULL);
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
	sfText_setString(text, "Loading Ressources");
	sfRenderWindow_clear(window, (sfColor){50, 155, 155, 255});
	sfRenderWindow_drawText(window, text, NULL);
	sfRenderWindow_display(window);
	loadSounds(args[0], sounds, soundBuffers, debug);
	for (int i = 1 + debug; i < argc; i++) {
		sfRenderWindow_setTitle(window, args[i]);
		result = parseMidi(args[i], strcmp(args[1], "ddebug") == 0);
		if (!result) {
			printf("An error occurred when reading %s\nExit in 10 seconds\n", args[1]);
			nanosleep((struct timespec[1]){{10, 0}}, NULL);
		} else {
			printf("Finished to read %s: format %hi, %hi tracks, %i notes, ", args[1], result->format, result->nbOfTracks, result->nbOfNotes);
			if (result->fps) {
				printf("division: %i FPS and %i ticks/frame\n", result->fps, result->ticks);
			} else
				printf("division: %i ticks / 1/4 note\n", result->ticks);
			displayMidi(result, debug, window, sounds, text);
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