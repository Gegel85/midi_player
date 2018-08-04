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

void	displayMidi(MidiParser *result, char *path, char *progPath, bool debug)
{
	EventList	*events[result->nbOfTracks];
	sfVideoMode	mode = {1280, 960, 32};
	sfEvent		event;
	NoteList	notes;
	sfRectangleShape*rect = sfRectangleShape_create();
	sfRenderWindow	*window = sfRenderWindow_create(mode, path, sfClose | sfResize, NULL);
	char		playingNotes[16][128];
	double		elapsedTicks = 0;
	char		buffer[1000];
	sfText		*text = sfText_create();
	sfFont		*font;
	double		tmp[result->nbOfTracks];
	MidiInfos	infos;
	bool		go = !debug;
	sfSound		*sounds[2][128];
	sfSoundBuffer	*soundBuffers[2][128];
	bool		pressed = false;
	unsigned int	speed = TICKS;
	unsigned int	notesPlayed = 0;
	bool		isEnd = false;
	sfClock		*clock;
	double		time;
	unsigned char	volume = 100;
	sfView		*view = sfView_createFromRect(frect);
	bool		fromEvent = true;
	bool		dontDisplay = false;
	
	for (int i = strlen(progPath) - 1; i > 0; i--)
		if (progPath[i] == '/' || progPath[i] == '\\') {
			progPath[i + 1] = 0;
			break;
		}
	sprintf(buffer, "%sarial.ttf", progPath);
	font = sfFont_createFromFile(buffer);
	if (!window || !text)
		return;
	for (int i = 0; i < 16; i++)
		memset(playingNotes[i], 0, sizeof(playingNotes[i]));
	memset(tmp, 0, sizeof(tmp));
	memset(&infos, 0, sizeof(infos));
	for (int i = 0; i < result->nbOfTracks; i++)
		events[i] = &result->tracks[i].events;
	sfText_setCharacterSize(text, 20);
	sfRectangleShape_setOutlineColor(rect, (sfColor){0, 0, 0, 255});
	sfRectangleShape_setOutlineThickness(rect, 2);
	sfText_setFont(text, font);
	sfText_setColor(text, (sfColor){255, 255, 255, 255});
	//sfRenderWindow_setFramerateLimit(window, 60);
	sfText_setPosition(text, (sfVector2f){500, 450});
	sfText_setString(text, "Loading Ressources");
	sfRenderWindow_clear(window, (sfColor){50, 155, 155, 255});
	sfRenderWindow_drawText(window, text, NULL);
	sfRenderWindow_display(window);
	loadSounds(progPath, sounds, soundBuffers, debug);
	sfText_setCharacterSize(text, 10);
	sfText_setPosition(text, (sfVector2f){0, frect.top});
	sfText_setScale(text, (sfVector2f){1, frect.height / 960});
	sfRenderWindow_setView(window, view);
	sfText_setPosition(text, (sfVector2f){0, frect.top});
	sfText_setScale(text, (sfVector2f){1, frect.height / 960});
	clock = sfClock_create();
	while (!isEnd) {
		pressed = false;
		time = speed * sfTime_asSeconds(sfClock_getElapsedTime(clock)) * 60;
		sfClock_restart(clock);
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
				else if (!go && event.key.code == sfKeyRight) {
					elapsedTicks += time;
					pressed = debug;
					updateEvents(events, tmp, result->nbOfTracks, playingNotes, &infos, sounds, debug, &speed, &notesPlayed, time, volume);
					if (!fromEvent)
						displayNotesFromNotesList(&notes, elapsedTicks, rect, window, debug);
				} else if (fromEvent && event.key.code == sfKeyW) {
					sfText_setPosition(text, (sfVector2f){500, 450});
					sfText_setCharacterSize(text, 20);
					sfText_setString(text, "Converting midi events");
					sfRenderWindow_clear(window, (sfColor){50, 155, 155, 255});
					sfRenderWindow_drawText(window, text, NULL);
					sfRenderWindow_display(window);
					sfText_setCharacterSize(text, 10);
					notes = eventsToNotes(result);
					fromEvent = false;
				} else if (event.key.code == sfKeyLeft) {
					elapsedTicks -= speed;
					for (int i = 0; i < 16; i++)
						memset(playingNotes[i], 0, sizeof(playingNotes[i]));
					pressed = debug;
					for (int i = 0; i < result->nbOfTracks; i++) {
						tmp[i] = elapsedTicks - speed;
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
					speed++;
				else if (event.key.code == sfKeyDown)
					speed--;
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
					for (int i = 0; i < result->nbOfTracks; i++) {
						tmp[i] = 0;
						events[i] = &result->tracks[i].events;
					}
				}
			}
		}
		if (go || !sfRenderWindow_isOpen(window)) {
			elapsedTicks += time;
			updateEvents(events, tmp, result->nbOfTracks, playingNotes, &infos, sounds, debug, &speed, &notesPlayed, time, volume);
		}
		if (sfRenderWindow_isOpen(window)) {
			sfRenderWindow_clear(window, (sfColor){50, 155, 155, 255});
			if (!dontDisplay) {
				if (fromEvent)
					displayNotes(events, tmp, playingNotes, result->nbOfTracks, window, rect, pressed);
				else
					displayNotesFromNotesList(&notes, elapsedTicks, rect, window, debug);
			}
			displayPianoKeys(playingNotes, rect, window);
			sprintf(buffer, "Ticks %.3f\nTicks per frame: %u\nNotes played: %u/%u\nZoom level: %.3f%%\nVolume: %u%%\n", elapsedTicks, speed, notesPlayed, result->nbOfNotes,  960 * 100 / frect.height, volume);
			sfText_setString(text, buffer);
			sfRenderWindow_drawText(window, text, NULL);
			sfRenderWindow_display(window);
		} else
			nanosleep((struct timespec[1]){{0, 16666667}}, NULL);
		if (!debug && noEventsLeft(events, result->nbOfTracks)) {
			sfRenderWindow_close(window);
			isEnd = true;
		}
        }
	sfRenderWindow_destroy(window);
	sfClock_destroy(clock);
	sfFont_destroy(font);
	sfText_destroy(text);
}

int	main(int argc, char **args)
{
	MidiParser	*result;

	if (argc < 2) {
		printf("Usage: %s <file.mid> [debug]\n", args[0]);
		return (EXIT_FAILURE);
	}
	result = parseMidi(args[1], argc >= 3 && strcmp(args[2], "debug") == 0);
	if (!result)
		printf("An error occured when reading %s\n", args[1]);
	else {
		printf("Finished to read %s: format %hi, %hi tracks, %i notes, ", args[1], result->format, result->nbOfTracks, result->nbOfNotes);
		if (result->fps) {
			printf("division: %i FPS and %i ticks/frame\n", result->fps, result->ticks);
		} else
			printf("division: %i ticks / 1/4 note\n", result->ticks);
		displayMidi(result, args[1], args[0], argc >= 4 && strcmp(args[3], "debug") == 0);
		deleteMidiParserStruct(result);
	}
	return (EXIT_SUCCESS);
}