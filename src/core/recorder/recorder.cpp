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


/* -------------------------------------------------------------------------- */

/* optimize
Removes frames without actions. */

void optimize_()
{
	for (auto it = actions.cbegin(); it != actions.cend();)
	  it->second.size() == 0 ? it = actions.erase(it) : ++it;
}


/* -------------------------------------------------------------------------- */


void debug_()
{
	for (auto& kv : actions)
	{
		printf("frame: %d\n", kv.first);
		for (const Action& action : kv.second)
			printf(" channel=%d, value=0x%X\n", action.channel, action.event.getRaw());	
	}
}
} // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


void init()
{
	active = false;
	clearAll();	
}


/* -------------------------------------------------------------------------- */


void clearAll()
{
	actions.clear();
}


/* -------------------------------------------------------------------------- */



void clearChannel(int channel)
{
	for (auto& kv : actions)
	{
		vector<Action>& as = kv.second;
		as.erase(std::remove_if(as.begin(), as.end(), [=](const Action& a) { return a.channel == channel; }), as.end());
	}
	optimize_();
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


bool isActive()
{
	return active;	
}


void enable()
{
	active = true;
}


/* -------------------------------------------------------------------------- */


Action* rec(int channel, int frame, uint32_t value)
{
	if (!active) return nullptr;

	Action action(channel, frame, value);

	/* If key frame doesn't exist yet, create it brand new with an empty
	vector. */

	if (actions.count(frame) == 0)
		actions[frame] = {};
	
	actions[frame].push_back(action);

	return &actions[frame].back();
}


/* -------------------------------------------------------------------------- */


void forEachAction(std::function<void(const Action&)> f)
{
	for (auto& kv : actions)
		for (const Action& action : kv.second)
			f(action);
}
}}}; // giada::m::recorder::
