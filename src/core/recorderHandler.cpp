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


#include <cmath>
#include <cassert>
#include "recorder/recorder.h"
#include "action.h"
#include "recorderHandler.h"


namespace giada {
namespace m {
namespace recorderHandler
{
namespace
{
const Action* getActionById_(int id, const recorder::ActionMap& source)
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


bool isBoundaryEnvelopeAction(const Action* a)
{
    assert(a->prev != nullptr);
    assert(a->next != nullptr);
    return a->prev->frame > a->frame || a->next->frame < a->frame;
}


/* -------------------------------------------------------------------------- */


void updateBpm(float oldval, float newval, int oldquanto)
{
    recorder::updateKeyFrames([=](Frame old) 
    {
        /* The division here cannot be precise. A new frame can be 44099 and the 
        quantizer set to 44100. That would mean two recs completely useless. So we 
        compute a reject value ('scarto'): if it's lower than 6 frames the new frame 
        is collapsed with a quantized frame. FIXME - maybe 6 frames are too low. */
        Frame frame = (old / newval) * oldval;
        if (frame != 0) {
            Frame delta = oldquanto % frame;
            if (delta > 0 && delta <= 6)
                frame = frame + delta;
        }
        return frame;
    });
}


/* -------------------------------------------------------------------------- */


void updateSamplerate(int systemRate, int patchRate)
{
    if (systemRate == patchRate)
        return;

    float ratio = systemRate / (float) patchRate;

    recorder::updateKeyFrames([=](Frame old) { return floorf(old * ratio); });
}


/* -------------------------------------------------------------------------- */


bool cloneActions(int chanIndex, int newChanIndex)
{
    recorder::ActionMap temp = recorder::getActionMap();

    bool cloned   = false;
    int  actionId = recorder::getLatestActionId(); 

    recorder::forEachAction([&](const Action* a) 
    {
        if (a->channel == chanIndex) {
            Action* clone = new Action(*a);
            clone->id      = ++actionId;
            clone->channel = newChanIndex;
            temp[clone->frame].push_back(clone);
            cloned = true;
        }
    });

    recorder::updateActionId(actionId);
    recorder::updateActionMap(std::move(temp));

    return cloned;
}


/* -------------------------------------------------------------------------- */


void writePatch(int chanIndex, std::vector<patch::action_t>& pactions)
{
    recorder::forEachAction([&] (const Action* a) 
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


void readPatch(const std::vector<patch::action_t>& pactions)
{
    recorder::ActionMap temp = recorder::getActionMap();

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
        recorder::updateActionId(paction.id + 1);
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

    recorder::updateActionMap(std::move(temp));
}
}}}; // giada::m::recorderHandler::


