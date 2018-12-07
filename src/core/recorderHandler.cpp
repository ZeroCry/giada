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
#include "clock.h"
#include "recorderHandler.h"


namespace giada {
namespace m {
namespace recorderHandler
{
namespace
{
const Action* noteOn_ = nullptr;


/* -------------------------------------------------------------------------- */


const Action* getActionById_(int id, const recorder::ActionMap& source)
{
    for (auto& kv : source)
        for (const Action* action : kv.second)
            if (action->id == id)
                return action;
    return nullptr;
}


/* -------------------------------------------------------------------------- */

/* isRingLoop_
Ring loop: a composite action with key_press at frame N and key_release at 
frame M, with M <= N. */

bool isRingLoop_(Frame noteOffFrame)
{
    return noteOn_ != nullptr && noteOn_->frame > noteOffFrame;
}

/* isNullLoop_
Null loop: a composite action that begins and ends on the very same frame, i.e. 
with 0 size. Very unlikely. */

bool isNullLoop_(Frame noteOffFrame)
{
    return noteOn_ != nullptr && noteOn_->frame == noteOffFrame;
}


/* -------------------------------------------------------------------------- */


void recordLiveNoteOn_(int channel, MidiEvent e)
{
    assert(noteOn_ == nullptr);
    noteOn_ = recorder::rec(channel, clock::getCurrentFrame(), e, nullptr, nullptr);
}


void recordLiveNoteOff_(int channel, MidiEvent e)
{
    assert(noteOn_ != nullptr);

    Frame frame = clock::getCurrentFrame(); 

    /* If ring loop: record the note off at the end of the sequencer. If 
    null loop: remove previous action and do nothing. */

    if (isRingLoop_(frame))
        frame = clock::getFramesInLoop();
    
    if (isNullLoop_(frame))
        recorder::deleteAction(noteOn_);
    else {
        const Action* noteOff = recorder::rec(channel, frame, e, nullptr, nullptr);
        recorder::updateSiblings(noteOff, noteOn_, nullptr);        
    }

    noteOn_ = nullptr;
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


void recordLiveAction(int channel, MidiEvent e)
{
    if (!e.isNoteOnOff()) // Can't record other kind of events right now
        return;
    if (e.getStatus() == MidiEvent::NOTE_ON)
        recordLiveNoteOn_(channel, e);
    else
        recordLiveNoteOff_(channel, e);
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


