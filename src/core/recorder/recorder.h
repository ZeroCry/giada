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


#include <vector>
#include <functional>
#include "../types.h"
#include "../patch.h"
#include "../midiEvent.h"


class Channel;


namespace giada {
namespace m 
{
class Action;

namespace recorder
{
void debug();
/* init
Initializes the recorder: everything starts from here. */

void init(pthread_mutex_t* mixerMutex);

/* clearAll
Deletes all recorded actions. */

void clearAll();

/* clearChannel
Clears all actions from a channel. */

void clearChannel(int channel);

/* clearActions
Clears the actions by type from a channel. */

void clearActions(int channel, int type);

/* deleteAction
Deletes a specific action. */

void deleteAction(const Action* a);

/* updateKeyFrames_
Update all the key frames in the internal map of actions, according to a lambda 
function 'f'. */

void updateKeyFrames(std::function<Frame(Frame old)> f);

/* updateEvent
Changes the event in action 'a'. */

void updateEvent(const Action* a, MidiEvent e);

void updateSiblings(const Action* a, const Action* prev, const Action* next);

/* hasActions
Checks if the channel has at least one action recorded. */

bool hasActions(int channel, int type=0);

/* isActive
Is recorder recording something? */

bool isActive();

void enable();
void disable();

/* rec
Records an action. */

const Action* rec(int channel, Frame frame, MidiEvent e, const Action* prev, 
    const Action* next=nullptr);

/* forEachAction
Applies a read-only callback on each action recorded. */

void forEachAction(std::function<void(const Action*)> f);

/* getActionsOnFrame
Returns a vector of actions on frame 'f'. */

std::vector<const Action*> getActionsOnFrame(Frame f);

/* getClosestAction
Given a frame 'f' returns the closest action. */

const Action* getClosestAction(int channel, Frame f, int type);

std::vector<const Action*> getActionsOnChannel(int channel);

void writePatch(int chanIndex, std::vector<patch::action_t>& pactions);
void readPatch(const std::vector<patch::action_t>& pactions);

}}}; // giada::m::recorder::

#endif
