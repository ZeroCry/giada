/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2018 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------- */


#include <cassert>
#include "../gui/dialogs/gd_warnings.h"
#include "../gui/elems/mainWindow/keyboard/channel.h"
#include "../gui/elems/mainWindow/keyboard/sampleChannel.h"
#include "../core/const.h"
#include "../core/clock.h"
#include "../core/kernelMidi.h"
#include "../core/channel.h"
#include "../core/recorder.h"
#include "../core/recorder/recorder.h"
#include "../core/action.h"
#include "../core/mixer.h"
#include "../core/sampleChannel.h"
#include "../core/midiChannel.h"
#include "../utils/gui.h"
#include "../utils/log.h"
#include "recorder.h"


using std::vector;
using namespace giada;


namespace giada {
namespace c     {
namespace recorder 
{
namespace
{
void updateChannel_(geChannel* gch, bool refreshActionEditor=true)
{
	gch->ch->hasActions = m::recorder::hasActions(gch->ch->index);
	if (gch->ch->type == ChannelType::SAMPLE) {
		geSampleChannel* gsch = static_cast<geSampleChannel*>(gch);
		gsch->ch->hasActions ? gsch->showActionButton() : gsch->hideActionButton();
	}
	if (refreshActionEditor)
		gu_refreshActionEditor(); // refresh a.editor window, it could be open
}


/* -------------------------------------------------------------------------- */


bool isBoundaryEnvelopeAction_(const m::Action* a)
{
	assert(a->prev != nullptr);
	assert(a->next != nullptr);
	return a->prev->frame > a->frame || a->next->frame < a->frame;
}
}; // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


void clearAllActions(geChannel* gch)
{
	if (!gdConfirmWin("Warning", "Clear all actions: are you sure?"))
		return;
	m::recorder::clearChannel(gch->ch->index);
	updateChannel_(gch);
}


/* -------------------------------------------------------------------------- */


void clearVolumeActions(geChannel* gch)
{
	if (!gdConfirmWin("Warning", "Clear all volume actions: are you sure?"))
		return;
	m::recorder_DEPR_::clearAction(gch->ch->index, G_ACTION_VOLUME);
	updateChannel_(gch);
}


/* -------------------------------------------------------------------------- */


void clearStartStopActions(geChannel* gch)
{
	if (!gdConfirmWin("Warning", "Clear all start/stop actions: are you sure?"))
		return;
	m::recorder_DEPR_::clearAction(gch->ch->index, G_ACTION_KEYPRESS | G_ACTION_KEYREL | G_ACTION_KILL);
	updateChannel_(gch);
}


/* -------------------------------------------------------------------------- */


vector<const m::Action*> getMidiActions(const MidiChannel* ch)
{
	return m::recorder::getActionsOnChannel(ch->index);
}


/* -------------------------------------------------------------------------- */


void recordMidiAction(MidiChannel* ch, int note, int velocity, Frame f1, Frame f2)
{
	if (f2 == 0)
		f2 = f1 + G_DEFAULT_ACTION_SIZE;

	/* Avoid frame overflow. */

	int overflow = f2 - (m::clock::getFramesInLoop());
	if (overflow > 0) {
		f2 -= overflow;
		f1 -= overflow;
	}

	m::MidiEvent e1 = m::MidiEvent(m::MidiEvent::NOTE_ON,  note, velocity);
	m::MidiEvent e2 = m::MidiEvent(m::MidiEvent::NOTE_OFF, note, velocity);

	const m::Action* a = m::recorder::rec(ch->index, f1, e1, nullptr, nullptr);
	                     m::recorder::rec(ch->index, f2, e2, a, nullptr);

	updateChannel_(ch->guiChannel, /*refreshActionEditor=*/false);
}


/* -------------------------------------------------------------------------- */


void deleteMidiAction(MidiChannel* ch, const m::Action* a)
{
	namespace mr = m::recorder;

	assert(a != nullptr);
	assert(a->event.getStatus() == m::MidiEvent::NOTE_ON);
	assert(a->next != nullptr);

	/* Send a note-off first in case we are deleting it in a middle of a 
	key_on/key_off sequence. */
	
	ch->sendMidi(a->next->event.getRaw());
	mr::deleteAction(a->next);
	mr::deleteAction(a);

	updateChannel_(ch->guiChannel, /*refreshActionEditor=*/false);
}

/* -------------------------------------------------------------------------- */


void updateMidiAction(MidiChannel* ch, const m::Action* a, int note, int velocity, 
	Frame f1, Frame f2)
{
	namespace mr = m::recorder;

	mr::deleteAction(a->next);
	mr::deleteAction(a);
	
	recordMidiAction(ch, note, velocity, f1, f2);
}


/* -------------------------------------------------------------------------- */


void recordSampleAction(const SampleChannel* ch, int type, Frame f1, Frame f2)
{
	namespace mr = m::recorder;
	
	if (ch->mode == ChannelMode::SINGLE_PRESS) {
		m::MidiEvent e1 = m::MidiEvent(m::MidiEvent::NOTE_ON, 0, 0);
		m::MidiEvent e2 = m::MidiEvent(m::MidiEvent::NOTE_OFF, 0, 0);
		const m::Action* a = mr::rec(ch->index, f1, e1, nullptr);
		                     mr::rec(ch->index, f2 == 0 ? f1 + G_DEFAULT_ACTION_SIZE : f2, e2, a);
	}
	else {
		m::MidiEvent e1 = m::MidiEvent(type, 0, 0);
		mr::rec(ch->index, f1, e1, nullptr);
	}
	
	updateChannel_(ch->guiChannel, /*refreshActionEditor=*/false);
}


/* -------------------------------------------------------------------------- */


void updateSampleAction(SampleChannel* ch, const m::Action* a, int type, Frame f1, 
	Frame f2)
{
	namespace mr = m::recorder;	

	if (ch->mode == ChannelMode::SINGLE_PRESS)
		mr::deleteAction(a->next);
	mr::deleteAction(a);

	recordSampleAction(ch, type, f1, f2);
}


/* -------------------------------------------------------------------------- */


void recordEnvelopeAction(Channel* ch, int type, int frame, int value)
{
	/* TODO - this function assumes we are working with ENVELOPE events. This
	should be generalized. */
	
	namespace mr = m::recorder;

	assert(value >= 0 && value <= G_MAX_VELOCITY);

	m::MidiEvent e2 = m::MidiEvent(m::MidiEvent::ENVELOPE, 0, value);
	
	/* First action ever? Add actions at boundaries. Else, find action right
	before frame 'f' and inject a new action in there. */

	if (!mr::hasActions(ch->index, type)) {
		m::MidiEvent e1 = m::MidiEvent(m::MidiEvent::ENVELOPE, 0, G_MAX_VELOCITY);
		const m::Action* a1 = mr::rec(ch->index, 0, e1, nullptr, nullptr);	
		const m::Action* a2 = mr::rec(ch->index, frame, e2, nullptr, nullptr);
		const m::Action* a3 = mr::rec(ch->index, m::clock::getFramesInLoop() - 1, e1, nullptr, nullptr);
		mr::updateSiblings(a1, a3, a2);
		mr::updateSiblings(a2, a1, a3);
		mr::updateSiblings(a3, a2, a1);
	}
	else {
		const m::Action* a1 = mr::getActionInFrameRange(ch->index, frame, m::MidiEvent::ENVELOPE);
		const m::Action* a3 = a1->next;
		assert(a1 != nullptr);
		assert(a3 != nullptr);
		mr::rec(ch->index, frame, e2, a1, a3);
	}

	updateChannel_(ch->guiChannel, /*refreshActionEditor=*/false);
}


/* -------------------------------------------------------------------------- */


void deleteEnvelopeAction(Channel* ch, const m::Action* a)
{
	namespace mr = m::recorder;

	assert(a != nullptr);

	/* Delete a boundary action wipes out everything. */

	if (isBoundaryEnvelopeAction_(a)) {
		mr::clearActions(ch->index, a->event.getStatus());
		return;
	}

	const m::Action* a1 = a->prev;
	const m::Action* a3 = a->next; 

	/* Original status:   a1--->a--->a3
	   Modified status:   a1-------->a3 */

	mr::deleteAction(a);
	mr::updateSiblings(a1, a1->prev, a3);
	mr::updateSiblings(a3, a1, a3->next);

	updateChannel_(ch->guiChannel, /*refreshActionEditor=*/false);
}


/* -------------------------------------------------------------------------- */


void updateEnvelopeAction(Channel* ch, const m::Action* a, int frame, int value)
{
	namespace mr = m::recorder;

	assert(a != nullptr);

	/* Update the action directly if it is a boundary one. Else, delete the
	previous one and record a new action. */

	int type = a->event.getStatus();

	if (isBoundaryEnvelopeAction_(a))
		mr::updateEvent(a, m::MidiEvent(type, 0, value));
	else {
		deleteEnvelopeAction(ch, a);
		recordEnvelopeAction(ch, type, frame, value); 
	}

	updateChannel_(ch->guiChannel, /*refreshActionEditor=*/false);	
}


/* -------------------------------------------------------------------------- */


void deleteSampleAction(SampleChannel* ch, const m::Action* a)
{
	namespace mr = m::recorder;

	assert(a != nullptr);

	/* if SINGLE_PRESS delete the keyrelease first. */

	if (ch->mode == ChannelMode::SINGLE_PRESS) {
		assert(a->next != nullptr);
		mr::deleteAction(a->next);
	}
	mr::deleteAction(a);

  updateChannel_(ch->guiChannel, /*refreshActionEditor=*/false);
}


/* -------------------------------------------------------------------------- */


vector<const m::Action*> getSampleActions(const SampleChannel* ch)
{
	return m::recorder::getActionsOnChannel(ch->index);
}


/* -------------------------------------------------------------------------- */


vector<const m::Action*> getEnvelopeActions(const Channel* ch, int type)
{
	return m::recorder::getActionsOnChannel(ch->index);
}


/* -------------------------------------------------------------------------- */


void updateVelocity(const MidiChannel* ch, const m::Action* a, int value)
{
	namespace mr = m::recorder;
	
	m::MidiEvent event(a->event);
	event.setVelocity(value);

	mr::updateEvent(a, event);
}

}}} // giada::c::recorder::