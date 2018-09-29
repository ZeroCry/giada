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
#include <cassert>
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
/* actions
The big map of actions {frame : actions[]}. This belongs to Recorder, but it
is often parsed by Mixer. So every "write" action performed on it (add, 
remove, ...) must be guarded by a trylock on the mixerMutex. */

map<Frame, vector<Action>> actions;

/* mixerMutex */

pthread_mutex_t* mixerMutex = nullptr;
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


void removeIf_(std::function<bool(const Action&)> f)
{
	assert(mixerMutex != nullptr);
	while (pthread_mutex_trylock(mixerMutex) == 0) {
		for (auto& kv : actions) {
			vector<Action>& as = kv.second;
			as.erase(std::remove_if(as.begin(), as.end(), f), as.end());
		}
		optimize_();
		pthread_mutex_unlock(mixerMutex);
		break;		
	}
}


/* -------------------------------------------------------------------------- */


void debug_()
{
	for (auto& kv : actions) {
		printf("frame: %d\n", kv.first);
		for (const Action& action : kv.second)
			printf(" %p - frame=%d, channel=%d, value=0x%X\n", 
				(void*) &action, action.frame, action.channel, action.event.getRaw());	
	}
}
} // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


void init(pthread_mutex_t* m)
{
	mixerMutex = m;
	active = false;
	clearAll();
}


/* -------------------------------------------------------------------------- */


void clearAll()
{
	assert(mixerMutex != nullptr);
	while (pthread_mutex_trylock(mixerMutex) == 0) {
		actions.clear();
		pthread_mutex_unlock(mixerMutex);
		break;
	}
}


/* -------------------------------------------------------------------------- */



void clearChannel(int channel)
{
	removeIf_([=](const Action& a) { return a.channel == channel; });
}


/* -------------------------------------------------------------------------- */


void clearAction(int channel, ActionType t)
{
	removeIf_([=](const Action& a)
	{ 
		return a.channel == channel && a.event.getStatus() == static_cast<int>(t);
	});
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


bool isActive() { return active; }
void enable()   { active = true; }
void disable()  { active = false; }


/* -------------------------------------------------------------------------- */


Action* rec(int channel, int frame, MidiEvent event)
{
	if (!active) return nullptr;

	Action action{ channel, frame, event };

	/* If key frame doesn't exist yet, create it brand new with an empty
	vector. */

	if (actions.count(frame) == 0)
		actions[frame] = {};
	
	actions[frame].push_back(action);

	return &actions[frame].back();
}


/* -------------------------------------------------------------------------- */


void updateBpm(float oldval, float newval, int oldquanto)
{
	/* TODO - This algorithm might be slow as f**k. Instead of updating keys in
	the existing map, we create a temporary map with the updated keys. Then we 
	swap it with the original one (moved, not copied). */

	map<Frame, vector<Action>> tmp;

	for (auto& kv : actions)
	{
		/* The frame computation cannot be precise (float to int conversion). A new 
		frame can be 44099 and the quantizer set to 44100. That would mean two recs 
		completely useless. So we compute a reject value ('delta'): if it's lower 
		than 6 frames the new frame is collapsed with a quantized frame. 
		TODO - maybe 6 frames are too low */

		Frame frame = ((float) kv.first / newval) * oldval;
		if (frame != 0) {
			Frame delta = oldquanto % frame;
			if (delta > 0 && delta <= 6)
				frame = frame + delta;
		}

		/* The value is the original array of actions stored in the old map. An
		update to all actions is required. Don't copy the vector, move it: we want
		to keep the original references. */

		tmp[frame] = std::move(kv.second);
		for (Action& action : tmp[frame])
			action.frame = frame;
	}

	actions = std::move(tmp);
}


/* -------------------------------------------------------------------------- */


void forEachAction(std::function<void(const Action&)> f)
{
	for (auto& kv : actions)
		for (const Action& action : kv.second)
			f(action);
}
}}}; // giada::m::recorder::
