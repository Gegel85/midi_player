//
// Created by andgel on 28/12/18.
//

#include <midi_parser.h>
#include <stdio.h>

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