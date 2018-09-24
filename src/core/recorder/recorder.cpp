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


#include <map>
#include <vector>
#include <algorithm>
#include "../action.h"
#include "../channel.h"
#include "recorder.h"


using std::map;
using std::vector;


namespace giada {
namespace m {
namespace recorder
{
namespace
{
	map<Frame, vector<Action>> actions;
	bool active = false;
	bool sorted = false;
} // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


void init()
{
	active = false;
	sorted = false;
	clearAll();	
}


/* -------------------------------------------------------------------------- */


void clearAll()
{
	actions.clear();
}


/* -------------------------------------------------------------------------- */


bool hasActions(int channel)
{
	for (auto& kv : actions)
		for (const Action& action : kv.second)
			if (action.channel == channel)
				return true;
	return false;
}


/* -------------------------------------------------------------------------- */


bool canRecord(const Channel* ch)
{
	/* Can record on a channel if the recorder is active and ((channel is MIDI) or 
	(SAMPLE type with data in it)). */
	return active && (ch->type == ChannelType::MIDI || (ch->type == ChannelType::SAMPLE && ch->hasData()));	
}


/* -------------------------------------------------------------------------- */


void record(int channel, int frame, uint32_t value)
{
	Action action(channel, frame, value);

	/* If key frame doesn't exist yet, create it brand new with an already filled
	vector (with one action). std::initializer_list is currently the only way to 
	push back a vector in a map. It hurts, I know. */

	if (actions.count(frame) == 0)
		actions.emplace(frame, std::initializer_list<Action>{ action });
	else
		actions[frame].push_back(action);
}


/* -------------------------------------------------------------------------- */


void forEachAction(std::function<void(const Action&)> f)
{
	for (auto& kv : actions)
		for (const Action& action : kv.second)
			f(action);
}
}}}; // giada::m::recorder::
