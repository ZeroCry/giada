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


#ifndef G_GLUE_ACTION_EDITOR_H
#define G_GLUE_ACTION_EDITOR_H


#include <vector>
#include "../core/types.h"


class SampleChannel;
class MidiChannel;
class Channel;


namespace giada {
namespace m
{
class Action;
}
namespace c {
namespace actionEditor 
{
/* MIDI actions.  */

void recordMidiAction(MidiChannel* ch, int note, int velocity, Frame f1, Frame f2=0);
void deleteMidiAction(MidiChannel* ch, const m::Action* a);
void updateMidiAction(MidiChannel* ch, const m::Action* a, int note, int velocity, 
    Frame f1, Frame f2);
std::vector<const m::Action*> getMidiActions(const MidiChannel* ch);
void updateVelocity(const MidiChannel* ch, const m::Action* a, int value);

/* Sample Actions. */

void recordSampleAction(const SampleChannel* ch, int type, Frame f1, Frame f2=0);
std::vector<const m::Action*> getSampleActions(const SampleChannel* ch);
void deleteSampleAction(SampleChannel* ch, const m::Action* a);
void updateSampleAction(SampleChannel* ch, const m::Action* a, int type, Frame f1, Frame f2=0);

/* Envelope actions (only volume for now). */

std::vector<const m::Action*> getEnvelopeActions(const Channel* ch);
void recordEnvelopeAction(Channel* ch, int frame, int value);
void deleteEnvelopeAction(Channel* ch, const m::Action* a);
void updateEnvelopeAction(Channel* ch, const m::Action* a, int frame, int value);
}}}; // giada::c::actionEditor::

#endif
