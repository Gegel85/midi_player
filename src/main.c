#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>
#include <math.h>
#include "midi_parser.h"

#define NOTE_STEP (float)1280 / 74
#define	TICKS 32

const sfColor	channelColors[16] = {
	{255, 0  , 0  , 255},
	{0  , 0  , 255, 255},
	{0  , 255, 0  , 255},
	{255, 255, 0  , 255},
	{255, 0  , 255, 255},
	{0  , 255, 255, 255},
	{100, 100, 100, 255},
	{255, 120, 0  , 255},
	{255, 120, 120, 255},
	{120, 0  , 255, 255},
	{0  , 80 , 80 , 255},
	{0  , 0  , 120, 255},
	{120, 120, 255, 255},
	{120, 255, 120, 255},
	{116 , 26 , 26 , 255},
	{200, 200, 200, 255},
};

typedef struct {
	int	tempo;
	
} MidiInfos;

float	getPosForNote(unsigned char pitch)
{
	switch (pitch % 12) {
	case 0:
		return (pitch / 12 * NOTE_STEP * 7);
	case 1:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP - 6);
	case 2:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP);
	case 3:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 2 - 6);
	case 4:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 2);
	case 5:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 3);
	case 6:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 4 - 6);
	case 7:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 4);
	case 8:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 5 - 6);
	case 9:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 5);
	case 10:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 6 - 6);
	case 11:
		return (pitch / 12 * NOTE_STEP * 7 + NOTE_STEP * 6);
	}
	return (pitch / 12 * NOTE_STEP * 7);
}

void	displayPianoKeys(char playingNotes[16][128], sfRectangleShape *rec, sfRenderWindow *win)
{
	bool	drawOneBefore = false;
	float	last = 0;
	sfColor	color;

	sfRectangleShape_setOrigin(rec, (sfVector2f){0, 0});
	if (!rec)
		return;
	for (int i = 0; i < 127; i++) {
		switch (i % 12) {
		case 0:
		case 2:
		case 4:
		case 5:
		case 7:
		case 9:
		case 11:
			sfRectangleShape_setPosition(rec, (sfVector2f){last + 1, 880});
			color = (sfColor){255, 255, 255, 255};
			for (int j = 0; j < 16; j++)
				if (playingNotes[j][i]) {
					color = channelColors[j];
					break;
				}
			sfRectangleShape_setFillColor(rec, color);
			sfRectangleShape_setSize(rec, (sfVector2f){NOTE_STEP - 2, 80});
			sfRenderWindow_drawRectangleShape(win, rec, NULL);
			if (drawOneBefore) {
				sfRectangleShape_setPosition(rec, (sfVector2f){last - 6, 880});
				color = (sfColor){0, 0, 0, 255};
				for (int j = 0; j < 16; j++)
					if (playingNotes[j][i - 1]) {
						color = (sfColor){
							channelColors[j].r * 0.5,
							channelColors[j].g * 0.5,
							channelColors[j].b * 0.5,
							channelColors[j].a
						};
						break;
					}
				sfRectangleShape_setFillColor(rec, color);
				sfRectangleShape_setSize(rec, (sfVector2f){(NOTE_STEP - 2) / 1.5, 40});
				sfRenderWindow_drawRectangleShape(win, rec, NULL);
			}
			drawOneBefore = false;
			last += (float)1280 / 74;
			break;
		case 1:
		case 3:
		case 6:
		case 8:
		case 10:
			drawOneBefore = true;
			break;
		}
	}
	if (drawOneBefore) {
		sfRectangleShape_setPosition(rec, (sfVector2f){last - 6, 880});
		sfRectangleShape_setFillColor(rec, (sfColor){0, 0, 0, 255});
		sfRectangleShape_setSize(rec, (sfVector2f){(NOTE_STEP - 2) / 1.5, 40});
		sfRenderWindow_drawRectangleShape(win, rec, NULL);
	}
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

void	displayNote(unsigned char channel, unsigned char pitch, int startTime, int currentTime, sfRectangleShape *rec, sfRenderWindow *win, bool debug)
{
	if (debug)
		printf("Displaying %s from channel %i (startTime: %i, currentTime: %i)\n", getNoteString(pitch), channel, startTime, currentTime);
	switch (pitch % 12) {
	case 0:
	case 2:
	case 4:
	case 5:
	case 7:
	case 9:
	case 11:
		if (debug)
			printf("Size (%f, %f)\nPosition (%f, %f)\n", ((float)1280 / 74 - 2), (float)(currentTime - startTime), (float)getPosForNote(pitch), (float)(880 - startTime));
		sfRectangleShape_setFillColor(rec, channelColors[channel]);
		sfRectangleShape_setSize(rec, (sfVector2f){(float)1280 / 74 - 2, currentTime - startTime});
		sfRectangleShape_setOrigin(rec, (sfVector2f){0, currentTime - startTime});
		sfRectangleShape_setPosition(rec, (sfVector2f){getPosForNote(pitch), 880 - startTime});
		sfRenderWindow_drawRectangleShape(win, rec, NULL);
		break;
	case 1:
	case 3:
	case 6:
	case 8:
	case 10:
		if (debug)
			printf("Size (%f, %f)\nPosition (%f, %f)\n", ((float)1280 / 74 - 2) / 1.5, (float)(currentTime - startTime), (float)getPosForNote(pitch), (float)(880 - startTime));
		sfRectangleShape_setFillColor(rec, (sfColor){
			channelColors[channel].r * 0.5,
			channelColors[channel].g * 0.5,
			channelColors[channel].b * 0.5,
			channelColors[channel].a
		});
		sfRectangleShape_setSize(rec, (sfVector2f){((float)1280 / 74 - 2) / 1.5, currentTime - startTime});
		sfRectangleShape_setOrigin(rec, (sfVector2f){0, currentTime - startTime});
		sfRectangleShape_setPosition(rec, (sfVector2f){getPosForNote(pitch), 880 - startTime});
		sfRenderWindow_drawRectangleShape(win, rec, NULL);
		break;
	}
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

void	displayNotes(EventList **allevents, int *allticks, char playingNotes[16][128], int nbOfTracks, sfRenderWindow *win, sfRectangleShape *rec, bool debug)
{
	int		time = 0;
	int		rectSize[16][128];
	int		ticks;
	EventList	*events;

	for (int i = 0; i < 16; i++)
		memset(rectSize[i], -1, sizeof(rectSize[i]));
	for (int i = 0; i < nbOfTracks; i++) {
		events = allevents[i];
		ticks = allticks[i];
		time = 0;
		for (; events && (int)(time += events->data->timeToAppear) - (int)ticks < 1900; events = events->next) {
			if (!events->data)
				continue;
			if (events->data->type == MidiNotePressed) {
				if(debug)
					printf("Note %s is pressed on channel %i ! (%i)\n", 
						getNoteString(((MidiNote *)events->data->infos)->pitch),
						((MidiNote *)events->data->infos)->channel,
						time
					);
				rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] = time - ticks;
			} else if (events->data->type == MidiNoteReleased) {
				displayNote(
					((MidiNote *)events->data->infos)->channel,
					((MidiNote *)events->data->infos)->pitch,
					rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch],
					time - ticks,
					rec,
					win,
					debug
				);
				rectSize[((MidiNote *)events->data->infos)->channel][((MidiNote *)events->data->infos)->pitch] = -2;
			}
		}
		if(debug)
			printf(!events ? "End of events list !\n\n" : "Stopped because %i + %i - %i () >= 1900 (Next event: %s)\n\n",
				time,
				events->data->timeToAppear,
				ticks,
				getEventString(events ? events->data : NULL)
			);
		for (unsigned char i = 0; i < 16; i++)
			for (unsigned char j = 0; j < 128; j++)
				if (rectSize[i][j] >= 0) {
					displayNote(i, j, rectSize[i][j], time, rec, win, debug);
					rectSize[i][j] = -2;
				}
	}
	for (unsigned char i = 0; i < 16; i++)
		for (unsigned char j = 0; j < 128; j++)
			if (rectSize[i][j] != -2 && playingNotes[i][j])
				displayNote(i, j, 0, 880, rec, win, debug);
	if(debug)printf("\n\n");
}

void	updateEvents(EventList **events, int *tmp, int nbOfTracks, char playingNotes[16][128], MidiInfos *infos, sfSound *sounds[16][128], bool debug, unsigned int speed)
{
	for (int i = 0; i < nbOfTracks; i++)
		tmp[i] += speed;
	for (int i = 0; i < nbOfTracks; i++)
		while (events[i] && events[i]->data->timeToAppear < tmp[i]) {
			tmp[i] -= events[i]->data->timeToAppear;
			if (events[i]->data->type == MidiNotePressed) {
				playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch] = ((MidiNote *)events[i]->data->infos)->velocity;
				if (sounds[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch]) {
					if (debug)
						printf("Playing note %s on channel %i\n", getNoteString(((MidiNote *)events[i]->data->infos)->pitch), ((MidiNote *)events[i]->data->infos)->channel);
					sfSound_setVolume(sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch], (float)((MidiNote *)events[i]->data->infos)->velocity * 100 / 127);
					sfSound_play(sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch]);
				}
			} else if (events[i]->data->type == MidiNoteReleased) {
				playingNotes[((MidiNote *)events[i]->data->infos)->channel][((MidiNote *)events[i]->data->infos)->pitch] = 0;
				if (sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch]) {
					for (int k = ((MidiNote *)events[i]->data->infos)->channel % 2; k < 18; k += 2)
						if (k >= 16)
							sfSound_stop(sounds[((MidiNote *)events[i]->data->infos)->channel % 2][((MidiNote *)events[i]->data->infos)->pitch]);
						else if (playingNotes[k][((MidiNote *)events[i]->data->infos)->pitch])
							break;
				}
			} else if (events[i]->data->type == MidiTempoChanged)
				infos->tempo = *(int *)events[i]->data->infos;
			events[i] = events[i]->next;
		}
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

double	getNoteFrequency(char note)
{
	return (pow(2, (double)(note - 69) / 12.0) * 440);
}

int	findClosestBuffer(sfSoundBuffer *buffers[128], int startIndex)
{
	for (int i = 0; startIndex + i < 128 || startIndex - i >= 0; i++) {
		if (startIndex + i < 128 && buffers[startIndex + i])
			return (startIndex + i);
		if (startIndex - i >= 0 && buffers[startIndex - i])
			return (startIndex - i);
	}
	return (-1);
}

void	loadSounds(char *path, sfSound *sounds[16][128], sfSoundBuffer *soundBuffers[16][128], bool debug)
{
	char		buffer[1024];
	char		*note;
	double		pitch[128];	
	sfSoundBuffer	*buffers[128];

	for (int i = 0; i < 16; i++)
		for (int j = 0; j < 128; j++) {
			sounds[i][j] = NULL;
			soundBuffers[i][j] = NULL;
		}
	memset(pitch, 0, sizeof(pitch));
	for (int j = 0; j < 128; j++) {
		note = getNoteString(j);
		note[0] += 'a' - 'A';
		sprintf(buffer, "%ssounds/Grand Piano4/%smmell.wav", path, note);
		soundBuffers[0][j] = sfSoundBuffer_createFromFile(buffer);
		if (soundBuffers[0][j]) {
			sounds[0][j] = sfSound_create();
			if (sounds[0][j])
				sfSound_setBuffer(sounds[0][j], soundBuffers[0][j]);
		} else
			sounds[0][j] = NULL;
	}
	for (int i = 0; i < 128; i++)
		buffers[i] = soundBuffers[0][i];
	for (int j = 0, i = 0; j < 128; j++) {
		if (!soundBuffers[0][j]) {
			i = findClosestBuffer(buffers, j);
			if (i < 0) {
				printf("No sound could be loaded\n");
				return;
			}
			soundBuffers[0][j] = soundBuffers[0][i];
			if (soundBuffers[0][j]) {
				sounds[0][j] = sfSound_create();
				if (sounds[0][j]) {
					sfSound_setBuffer(sounds[0][j], soundBuffers[0][j]);
					sfSound_setPitch(sounds[0][j], getNoteFrequency(j) / getNoteFrequency(i));
					pitch[j] = getNoteFrequency(j) / getNoteFrequency(i);
					if (debug)printf("Creating %i from %i (Ratio: %f)\n", j, i, pitch[j]);
				}
			}
		}
	}
	for (int i = 1; i < 16; i++)
		for (int j = 0; j < 128; j++) {
			soundBuffers[i][j] = soundBuffers[i - 1][j] ? sfSoundBuffer_copy(soundBuffers[i - 1][j]) : NULL;
			if (soundBuffers[i - 1][j]) {
				sounds[i][j] = sfSound_create();
				if (sounds[i][j])
					sfSound_setBuffer(sounds[i][j], soundBuffers[i - 1][j]);
				if (pitch[j])
					sfSound_setPitch(sounds[i][j], pitch[j]);
			}
		}
	if (debug) {
		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 128; j++) {
				if (!sounds[i][j]) {
					debug = false;
					printf("Sound[%i][%i] is not set !\n", i, j);
				}
			}
		}
		if (debug)
			printf("No sound problems found\n");
	}
}

void	displayMidi(MidiParser *result, char *path, char *progPath, bool debug)
{
	EventList	*events[result->nbOfTracks];
	sfVideoMode	mode = {1280, 960, 32};
	sfEvent		event;
	sfRectangleShape*rect = sfRectangleShape_create();
	sfRenderWindow	*window = sfRenderWindow_create(mode, path, sfClose, NULL);
	char		playingNotes[16][128];
	unsigned int	elapsedTicks = 0;
	char		buffer[1000];
	sfText		*text = sfText_create();
	sfFont		*font;
	int		tmp[result->nbOfTracks];
	MidiInfos	infos;
	bool		go = !debug;
	sfSound		*sounds[16][128];
	sfSoundBuffer	*soundBuffers[16][128];
	bool		pressed = false;
	unsigned int	speed = TICKS;
	
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
	sfRenderWindow_setFramerateLimit(window, 60);
	sfText_setPosition(text, (sfVector2f){500, 450});
	sfText_setString(text, "Loading Ressources");
	sfRenderWindow_clear(window, (sfColor){50, 155, 155, 255});
	sfRenderWindow_drawText(window, text, NULL);
	sfRenderWindow_display(window);
	sfText_setCharacterSize(text, 10);
	loadSounds(progPath, sounds, soundBuffers, debug);
	sfText_setPosition(text, (sfVector2f){0, 0});
	while (sfRenderWindow_isOpen(window)) {
		pressed = false;
		while (sfRenderWindow_pollEvent(window, &event)) {
			if (event.type == sfEvtClosed)
				sfRenderWindow_close(window);
			if (event.type == sfEvtKeyPressed) {
				if (event.key.code == sfKeySpace)
					go = !go;
				else if (!go && event.key.code == sfKeyRight) {
					elapsedTicks += speed;
					pressed = debug;
					updateEvents(events, tmp, result->nbOfTracks, playingNotes, &infos, sounds, debug, speed);
				} else if (event.key.code == sfKeyLeft) {
					elapsedTicks -= speed;
					for (int i = 0; i < 16; i++)
						memset(playingNotes[i], 0, sizeof(playingNotes[i]));
					pressed = debug;
					for (int i = 0; i < result->nbOfTracks; i++) {
						tmp[i] = elapsedTicks - speed;
						events[i] = &result->tracks[i].events;
					}
					for (int i = 0; i < result->nbOfTracks; i++)
						while (events[i] && events[i]->data->timeToAppear < tmp[i]) {
							tmp[i] -= events[i]->data->timeToAppear;
							events[i] = events[i]->next;
						}
				} else if (event.key.code == sfKeyUp)
					speed++;
				else if (event.key.code == sfKeyDown)
					speed--;
				else if (event.key.code == sfKeyHome) {
					for (int i = 0; i < 16; i++)
						memset(playingNotes[i], 0, sizeof(playingNotes[i]));
					pressed = debug;
					elapsedTicks = 0;
					for (int i = 0; i < result->nbOfTracks; i++) {
						tmp[i] = 0;
						events[i] = &result->tracks[i].events;
					}
				}
			}
		}
		if (go) {
			elapsedTicks += speed;
			updateEvents(events, tmp, result->nbOfTracks, playingNotes, &infos, sounds, debug, speed);
		}
		sfRenderWindow_clear(window, (sfColor){50, 155, 155, 255});
		displayNotes(events, tmp, playingNotes, result->nbOfTracks, window, rect, pressed);
		displayPianoKeys(playingNotes, rect, window);
		sprintf(buffer, "Ticks %u\nTicks per frame: %u\n", elapsedTicks, speed);
		sfText_setString(text, buffer);
		sfRenderWindow_drawText(window, text, NULL);
		sfRenderWindow_display(window);
		if (!debug && noEventsLeft(events, result->nbOfTracks))
			sfRenderWindow_close(window);
        }
	sfRenderWindow_destroy(window);
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