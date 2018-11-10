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


#ifndef G_GLUE_RECORDER_H
#define G_GLUE_RECORDER_H


#include <vector>
#include "../core/recorder.h"


class SampleChannel;
class MidiChannel;
class geChannel;


namespace giada {
namespace m
{
class Action;
}
namespace c {
namespace recorder 
{
void clearAllActions(geChannel* gch);
void clearVolumeActions(geChannel* gch);
void clearStartStopActions(geChannel* gch);



/* MOVE ALL THESE FUNCTIONS TO c::actionEditor*/


bool midiActionCanFit(int chan, int note, int frame_a, int frame_b);
bool sampleActionCanFit(const SampleChannel* ch, int frame_a, int frame_b);

/* recordMidiAction
Records a new MIDI action at frame_a. If frame_b == 0, uses the default action
size. This function is designed for the Piano Roll (not for live recording). */

void recordMidiAction(MidiChannel* ch, int note, int velocity, Frame f1, Frame f2=0);

void deleteMidiAction(MidiChannel* ch, const m::Action* a);

void updateMidiAction(MidiChannel* ch, const m::Action* a, int note, int velocity, 
    Frame f1, Frame f2);

/* getMidiActions
Returns a list of Composite actions, ready to be displayed in a MIDI note
editor as pairs of NoteOn+NoteOff. */

std::vector<const m::Action*> getMidiActions(int channel);

void recordEnvelopeAction(Channel* ch, int type, int frame, float fValue);

void recordSampleAction(SampleChannel* ch, int type, int frame_a, int frame_b=0);

void setVelocity(const Channel* ch, m::recorder_DEPR_::action a, int value);

std::vector<m::recorder_DEPR_::action> getEnvelopeActions(const Channel* ch, int type);

/* getSampleActions
Returns a list of Composite actions, ready to be displayed in a Sample Action
Editor. If actions are not keypress+keyrelease combos, the second action in
the Composite struct if left empty (with action2.frame = -1). */

std::vector<m::recorder_DEPR_::Composite> getSampleActions(const SampleChannel* ch);

void deleteSampleAction(SampleChannel* ch, m::recorder_DEPR_::action a1,
	m::recorder_DEPR_::action a2);

void deleteEnvelopeAction(Channel* ch, m::recorder_DEPR_::action a, bool moved);

}}} // giada::c::recorder::

#endif
