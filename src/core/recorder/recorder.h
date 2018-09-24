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


class Channel;


namespace giada {
namespace m 
{
class Action;

namespace recorder
{
/* init
Initializes the recorder: everything starts from here. */

void init();

/* clearAll
Deletes all recorded actions. */

void clearAll();

/* hasActions
Checks if the channel has at least one action recorded. */

bool hasActions(int channel);

/* canRecord
Can a channel record an action? Call this one BEFORE rec(). */

bool canRecord(const Channel* ch);

/* record
 * Records an action. */

void record(int channel, int frame, uint32_t value);

/* forEachAction
Applies a read-only callback on each action recorded. */

void forEachAction(std::function<void(const Action&)> f);

}}}; // giada::m::recorder::

#endif
