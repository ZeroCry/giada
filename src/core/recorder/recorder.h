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


#ifndef G_RECORDER_H
#define G_RECORDER_H


#include <functional>
#include "../types.h"
#include "../midiEvent.h"


class Channel;


namespace giada {
namespace m 
{
class Action;

namespace recorder
{
/* init
Initializes the recorder: everything starts from here. */

void init(pthread_mutex_t* mixerMutex);

/* clearAll
Deletes all recorded actions. */

void clearAll();

/* clearChannel
Clears all actions from a channel. */

void clearChannel(int channel);

/* clearAction
Clears the actions by type from a channel. */

void clearAction(int channel, ActionType t);

/* hasActions
Checks if the channel has at least one action recorded. */

bool hasActions(int channel);

/* isActive
Is recorder active? Call this one BEFORE rec(). */

bool isActive();

void enable();
void disable();

/* rec
Records an action. */

const Action* rec(int channel, int frame, MidiEvent e, const Action* prev);

/* forEachAction
Applies a read-only callback on each action recorded. */

void forEachAction(std::function<void(const Action&)> f);

/* updateBpm
Changes actions position by calculating the new bpm value. */

void updateBpm(float oldval, float newval, int oldquanto);

/* updateSamplerate
Changes actions position by taking in account the new samplerate. If 
f_system == f_patch nothing will change, otherwise the conversion is 
mandatory. */

void updateSamplerate(int systemRate, int patchRate);

void expand(int old_fpb, int new_fpb);
void shrink(int new_fpb);


}}}; // giada::m::recorder::

#endif
