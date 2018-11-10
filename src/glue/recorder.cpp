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
void updateChannel(geChannel* gch, bool refreshActionEditor=true)
{
	gch->ch->hasActions = m::recorder_DEPR_::hasActions(gch->ch->index);
	if (gch->ch->type == ChannelType::SAMPLE) {
		geSampleChannel* gsch = static_cast<geSampleChannel*>(gch);
		gsch->ch->hasActions ? gsch->showActionButton() : gsch->hideActionButton();
	}
	if (refreshActionEditor)
		gu_refreshActionEditor(); // refresh a.editor window, it could be open
}
}; // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


void clearAllActions(geChannel* gch)
{
	if (!gdConfirmWin("Warning", "Clear all actions: are you sure?"))
		return;
	m::recorder_DEPR_::clearChan(gch->ch->index);
	updateChannel(gch);
}


/* -------------------------------------------------------------------------- */


void clearVolumeActions(geChannel* gch)
{
	if (!gdConfirmWin("Warning", "Clear all volume actions: are you sure?"))
		return;
	m::recorder_DEPR_::clearAction(gch->ch->index, G_ACTION_VOLUME);
	updateChannel(gch);
}


/* -------------------------------------------------------------------------- */


void clearStartStopActions(geChannel* gch)
{
	if (!gdConfirmWin("Warning", "Clear all start/stop actions: are you sure?"))
		return;
	m::recorder_DEPR_::clearAction(gch->ch->index, G_ACTION_KEYPRESS | G_ACTION_KEYREL | G_ACTION_KILL);
	updateChannel(gch);
}


/* -------------------------------------------------------------------------- */


bool midiActionCanFit(int chan, int note, int frame_a, int frame_b)
{
 	assert(false); // TODO 

 #if 0
	namespace mr = m::recorder_DEPR_;

	/* TODO - This is insane, to say the least. Let's wait for recorder refactoring... */

	vector<mr::Composite> comps = getMidiActions(chan);
	for (mr::Composite c : comps)
    if (frame_b >= c.a1.frame && c.a2.frame >= frame_a && m::MidiEvent(c.a1.iValue).getNote() == note)
			return false;
	return true;
#endif
}


bool sampleActionCanFit(const SampleChannel* ch, int frame_a, int frame_b)
{
	namespace mr = m::recorder_DEPR_;

	/* TODO - Even more insanity... Let's wait for recorder refactoring... */

	vector<mr::Composite> comps = getSampleActions(ch);
	for (mr::Composite c : comps)
    if (frame_b >= c.a1.frame && c.a2.frame >= frame_a)
			return false;
	return true;
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

	const m::Action* a = m::recorder::rec(ch->index, f1, e1, nullptr);
	const m::Action* b = m::recorder::rec(ch->index, f2, e2, a);

	ch->hasActions = m::recorder::hasActions(ch->index);

	gu_log("[c::recordMidiAction] record null.a=%d, null.b%d\n", a == nullptr, b == nullptr);
}


/* -------------------------------------------------------------------------- */


void deleteMidiAction(MidiChannel* ch, const m::Action* a)
{
	namespace mr = m::recorder;

	/* If action is not orphaned, send a note-off first in case we are deleting 
	it in a middle of a key_on/key_off sequence. Conversely, orphaned actions
	should not play, so no need to fire the note-off. */
	
	if (a->next != nullptr) {
		ch->sendMidi(a->next->event.getRaw());
		mr::deleteAction(a->next);
	}

	mr::deleteAction(a);

	ch->hasActions = mr::hasActions(ch->index);
}

/* -------------------------------------------------------------------------- */


void updateMidiAction(MidiChannel* ch, const m::Action* a, int note, int velocity, 
	Frame f1, Frame f2)
{
	namespace mr = m::recorder;

	//if (!::canFit())
	//	return; 

	mr::deleteAction(a->next);
	mr::deleteAction(a);
	
	recordMidiAction(ch, note, velocity, f1, f2);
}


/* -------------------------------------------------------------------------- */


void recordSampleAction(SampleChannel* ch, int type, int frame_a, int frame_b)
{
	if (ch->mode == ChannelMode::SINGLE_PRESS) {
		m::recorder_DEPR_::rec(ch->index, G_ACTION_KEYPRESS, frame_a);
		m::recorder_DEPR_::rec(ch->index, G_ACTION_KEYREL, frame_b == 0 ? frame_a + G_DEFAULT_ACTION_SIZE : frame_b);
	}
	else
		m::recorder_DEPR_::rec(ch->index, type, frame_a);

	updateChannel(ch->guiChannel, /*refreshActionEditor=*/false);
}


/* -------------------------------------------------------------------------- */


void recordEnvelopeAction(Channel* ch, int type, int frame, float fValue)
{
	namespace mr = m::recorder_DEPR_;

	if (!mr::hasActions(ch->index, type)) {  // First action ever? Add actions at boundaries.
		mr::rec(ch->index, type, 0, 0, 1.0);	
		mr::rec(ch->index, type, m::clock::getFramesInLoop() - 1, 0, 1.0);	
	}
	mr::rec(ch->index, type, frame, 0, fValue);

	updateChannel(ch->guiChannel, /*refreshActionEditor=*/false);
}


/* -------------------------------------------------------------------------- */


void deleteEnvelopeAction(Channel* ch, m::recorder_DEPR_::action a, bool moved)
{
	namespace mr = m::recorder_DEPR_;

	/* Deleting first or last action: clear everything. Otherwise delete the 
	selected action only. */

	if (!moved && (a.frame == 0 || a.frame == m::clock::getFramesInLoop() - 1))
		mr::clearAction(ch->index, a.type);
	else
		mr::deleteAction(ch->index, a.frame, a.type, false, &m::mixer::mutex);

	updateChannel(ch->guiChannel, /*refreshActionEditor=*/false);
}


/* -------------------------------------------------------------------------- */


void deleteSampleAction(SampleChannel* ch, m::recorder_DEPR_::action a1,
	m::recorder_DEPR_::action a2)
{
	namespace mr = m::recorder_DEPR_;

	/* if SINGLE_PRESS delete both the keypress and the keyrelease pair. */

	if (ch->mode == ChannelMode::SINGLE_PRESS) {
		mr::deleteAction(ch->index, a1.frame, G_ACTION_KEYPRESS, false, &m::mixer::mutex);
		mr::deleteAction(ch->index, a2.frame, G_ACTION_KEYREL, false, &m::mixer::mutex);
	}
	else
		mr::deleteAction(ch->index, a1.frame, a1.type, false, &m::mixer::mutex);

  updateChannel(ch->guiChannel, /*refreshActionEditor=*/false);
}


/* -------------------------------------------------------------------------- */


vector<m::recorder_DEPR_::Composite> getSampleActions(const SampleChannel* ch)
{
	namespace mr = m::recorder_DEPR_;

	vector<mr::Composite> out;

	mr::sortActions();
	mr::forEachAction([&](const mr::action* a1)
	{
		/* Exclude:
		- actions beyond clock::getFramesInLoop();
		- actions that don't belong to channel ch;
		- actions != G_ACTION_KEYPRESS, G_ACTION_KEYREL or G_ACTION_KILL;
		- G_ACTION_KEYREL actions in a SINGLE_PRESS context. */

		if (a1->frame > m::clock::getFramesInLoop() || 
			  a1->chan != ch->index                   || 
			  a1->type & ~(G_ACTION_KEYPRESS | G_ACTION_KEYREL | G_ACTION_KILL) || 
			  (ch->mode == ChannelMode::SINGLE_PRESS && a1->type == G_ACTION_KEYREL))
			return;

		mr::Composite cmp; 
		cmp.a1 = *a1;
		cmp.a2.frame = -1;

		/* If SINGLE_PRESS mode and the current action is G_ACTION_KEYPRESS, let's
		fetch the corresponding G_ACTION_KEYREL. */

		if (ch->mode == ChannelMode::SINGLE_PRESS && a1->type == G_ACTION_KEYPRESS) {
			m::recorder_DEPR_::action* a2 = nullptr;
			mr::getNextAction(ch->index, G_ACTION_KEYREL, a1->frame, &a2);
			if (a2 != nullptr)
				cmp.a2 = *a2;
		}

		out.push_back(cmp);
	});

	return out;
}


/* -------------------------------------------------------------------------- */


vector<m::recorder_DEPR_::action> getEnvelopeActions(const Channel* ch, int type)
{
	namespace mr = m::recorder_DEPR_;

	vector<mr::action> out;

	mr::sortActions();
	mr::forEachAction([&](const mr::action* a)
	{
		/* Exclude:
		- actions beyond clock::getFramesInLoop();
		- actions that don't belong to channel ch;
		- actions with wrong type. */

		if (a->frame > m::clock::getFramesInLoop() || 
			  a->chan != ch->index                   || 
			  a->type != type)
			return;

		out.push_back(*a);
	});

	return out;
}


/* -------------------------------------------------------------------------- */


void setVelocity(const Channel* ch, m::recorder_DEPR_::action a, int value)
{
	/* TODO - this is super ugly: delete the action and add a new one with the
	modified values. This shit will go away as soon as we'll refactor m::recorder
	for good. */

	m::MidiEvent event = m::MidiEvent(a.iValue);
	event.setVelocity(value);

	m::recorder_DEPR_::deleteAction(ch->index, a.frame, G_ACTION_MIDI, true,
		&m::mixer::mutex, a.iValue, 0.0);
	m::recorder_DEPR_::rec(ch->index, G_ACTION_MIDI, a.frame, event.getRaw());
}


/* -------------------------------------------------------------------------- */


vector<const m::Action*> getMidiActions(int chan)
{
	return m::recorder::getActionsOnChannel(chan);
}

}}} // giada::c::recorder::