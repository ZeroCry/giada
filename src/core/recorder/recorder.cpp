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
#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>
#include "../../utils/log.h"
#include "../action.h"
#include "../channel.h"
#include "recorder.h"


using std::map;
using std::vector;


namespace giada {
namespace m {
namespace recorder
{
using ActionMap = map<Frame, vector<const Action*>>;

namespace
{
/* actions
The big map of actions {frame : actions[]}. This belongs to Recorder, but it
is often parsed by Mixer. So every "write" action performed on it (add, 
remove, ...) must be guarded by a trylock on the mixerMutex. Until a proper
lock-free solution will be implemented. */

ActionMap actions;

pthread_mutex_t* mixerMutex = nullptr;
bool             active     = false;
int              actionId   = 0;


/* -------------------------------------------------------------------------- */


void trylock_(std::function<void()> f)
{
	assert(mixerMutex != nullptr);
	pthread_mutex_lock(mixerMutex);
	f();
	pthread_mutex_unlock(mixerMutex);
	/*
	while (pthread_mutex_trylock(mixerMutex) == 0) {
		f();
		pthread_mutex_unlock(mixerMutex);
		break;
	}*/
}


/* -------------------------------------------------------------------------- */

/* optimize
Removes frames without actions. */

void optimize_(ActionMap& map)
{
	for (auto it = map.cbegin(); it != map.cend();)
		it->second.size() == 0 ? it = map.erase(it) : ++it;
}


/* -------------------------------------------------------------------------- */


void removeIf_(std::function<bool(const Action*)> f)
{
	ActionMap temp = actions;

	/*
	for (auto& kv : temp) {
		vector<Action*>& as = kv.second;
		as.erase(std::remove_if(as.begin(), as.end(), f), as.end());
	}*/
	for (auto& kv : temp) {
		auto i = std::begin(kv.second);
		while (i != std::end(kv.second)) {
			if (f(*i)) {
				delete *i;
				i = kv.second.erase(i);
			}
			else
				++i;
		}
	}
	optimize_(temp);

	trylock_([&](){ actions = std::move(temp); });
}


/* -------------------------------------------------------------------------- */


const Action* getActionById_(int id, const ActionMap& source)
{
	for (auto& kv : source)
		for (const Action* action : kv.second)
			if (action->id == id)
				return action;
	return nullptr;
}
} // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


void init(pthread_mutex_t* m)
{
	mixerMutex = m;
	active     = false;
	actionId   = 0;
	clearAll();
}


/* -------------------------------------------------------------------------- */


void debug()
{
	int total = 0;
	puts("-------------");
	for (auto& kv : actions) {
		printf("frame: %d\n", kv.first);
		for (const Action* a : kv.second) {
			total++;
			printf(" this=%p - id=%d, frame=%d, channel=%d, value=0x%X, prev=%p, next=%p\n", 
				(void*) a, a->id, a->frame, a->channel, a->event.getRaw(), (void*) a->prev, (void*) a->next);	
		}
	}
	printf("TOTAL: %d\n", total);
	puts("-------------");
}


/* -------------------------------------------------------------------------- */


void clearAll()
{
	removeIf_([=](const Action* a) { return true; }); // TODO optimize this
}


/* -------------------------------------------------------------------------- */


void clearChannel(int channel)
{
	removeIf_([=](const Action* a) { return a->channel == channel; });
}


/* -------------------------------------------------------------------------- */


void clearActions(int channel, int type)
{
	removeIf_([=](const Action* a)
	{ 
		return a->channel == channel && a->event.getStatus() == type;
	});
}


/* -------------------------------------------------------------------------- */


void deleteAction(const Action* target)
{
	removeIf_([=](const Action* a) { return a == target; });
}


/* -------------------------------------------------------------------------- */


void updateKeyFrames(std::function<Frame(Frame old)> f)
{
	/* This stuff must be performed in a lock, because we are moving the vector
	of actions from the real ActionMap to the temporary one. */
	
	ActionMap temp;

	trylock_([&]()
	{ 
		for (auto& kv : actions) {
			Frame frame = f(kv.first);
			temp[frame] = std::move(kv.second);  // Move std::vector<Action*>
			for (const Action* action : temp[frame])
				const_cast<Action*>(action)->frame = frame;
			gu_log("[recorder::updateKeyFrames] %d -> %d\n", kv.first, frame);
		}
		actions = std::move(temp); 
	});
}


/* -------------------------------------------------------------------------- */


void updateEvent(const Action* a, MidiEvent e)
{
	assert(a != nullptr);
	trylock_([&] { const_cast<Action*>(a)->event = e; });
}


/* -------------------------------------------------------------------------- */


void updateSiblings(const Action* a, const Action* prev, const Action* next)
{
	assert(a != nullptr);
	trylock_([&] 
	{ 
		const_cast<Action*>(a)->prev = prev;
		const_cast<Action*>(a)->next = next;
	});
}


/* -------------------------------------------------------------------------- */


bool hasActions(int channel, int type)
{
	for (auto& kv : actions)
		for (const Action* action : kv.second)
			if (action->channel == channel && (type == 0 || type == action->event.getStatus()))
				return true;
	return false;
}


/* -------------------------------------------------------------------------- */


bool isActive() { return active; }
void enable()   { active = true; }
void disable()  { active = false; }


/* -------------------------------------------------------------------------- */


const Action* rec(int channel, Frame frame, MidiEvent event, const Action* prev,
	const Action* next)
{
	ActionMap temp = actions;

	/* If key frame doesn't exist yet, the [] operator in std::map is smart 
	enough to insert a new item first. Then swap the temp map with the original 
	one. No plug-in data for now. */
	
	temp[frame].push_back(new Action{ actionId++, channel, frame, event, -1, -1, prev, next });

	/* Update linked actions, if any. */
	
	const Action* curr = temp[frame].back();
	if (prev != nullptr)
		const_cast<Action*>(prev)->next = curr;
	if (next != nullptr)
		const_cast<Action*>(next)->prev = curr;
	
	trylock_([&](){ actions = std::move(temp); });

	return curr;
}


/* -------------------------------------------------------------------------- */


vector<const Action*> getActionsOnFrame(Frame frame)
{
	return actions.count(frame) ? actions[frame] : vector<const Action*>();
}


/* -------------------------------------------------------------------------- */


const Action* getClosestAction(int channel, Frame f, int type)
{
	const Action* out = nullptr;
	forEachAction([&](const Action* a)
	{
		if (a->event.getStatus() != type || a->channel != channel)
			return;
		if (out == nullptr || (a->frame <= f && a->frame > out->frame))
			out = a;
	});
	return out;
}


/* -------------------------------------------------------------------------- */


vector<const Action*> getActionsOnChannel(int channel)
{
	vector<const Action*> out;
	forEachAction([&](const Action* a)
	{
		if (a->channel == channel)
			out.push_back(a);
	});
	return out;
}


/* -------------------------------------------------------------------------- */


void forEachAction(std::function<void(const Action*)> f)
{
	for (auto& kv : actions)
		for (const Action* action : kv.second)
			f(action);
}


/* -------------------------------------------------------------------------- */


void writePatch(int chanIndex, std::vector<patch::action_t>& pactions)
{
	forEachAction([&] (const Action* a) 
	{
		if (a->channel != chanIndex) 
			return;
		pactions.push_back(patch::action_t { 
			a->id, 
			a->channel, 
			a->frame, 
			a->event.getRaw(), 
			a->prev != nullptr ? a->prev->id : -1,
			a->next != nullptr ? a->next->id : -1
		});
	});
}


/* -------------------------------------------------------------------------- */


void readPatch(const vector<patch::action_t>& pactions)
{
	ActionMap temp = actions;

	/* First pass: add actions with no relationship (no prev/next). */

	for (const patch::action_t paction : pactions) {
		temp[paction.frame].push_back(new Action{ 
			paction.id, 
			paction.channel, 
			paction.frame, 
			MidiEvent(paction.event), 
			-1, // No plug-in data so far
			-1, // No plug-in data so far
			nullptr, 
			nullptr 
		});
		if (actionId <= paction.id) actionId = paction.id + 1; // Update id generator
	}

	/* Second pass: fill in previous and next actions, if any. */

	for (const patch::action_t paction : pactions) {
		if (paction.next == -1 && paction.prev == -1) 
			continue;
		Action* curr = const_cast<Action*>(getActionById_(paction.id, temp));
		assert(curr != nullptr);
		if (paction.next != -1) {
			curr->next = getActionById_(paction.next, temp);
			assert(curr->next != nullptr);
		}
		if (paction.prev != -1) {
			curr->prev = getActionById_(paction.prev, temp);
			assert(curr->prev != nullptr);
		}
	}

	trylock_([&](){ actions = std::move(temp); });
}

}}}; // giada::m::recorder::
