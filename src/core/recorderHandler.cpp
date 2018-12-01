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


#include <cassert>
#include "recorder/recorder.h"
#include "action.h"
#include "recorderHandler.h"


namespace giada {
namespace m {
namespace recorderHandler
{
bool isBoundaryEnvelopeAction(const Action* a)
{
    assert(a->prev != nullptr);
    assert(a->next != nullptr);
    return a->prev->frame > a->frame || a->next->frame < a->frame;
}


/* -------------------------------------------------------------------------- */


void expand(int old_fpb, int new_fpb)
{
    /* This algorithm requires multiple passages if we expand from e.g. 2 to 16 
    beats, precisely 16 / 2 - 1 = 7 times (-1 is the first group, which exists 
    yet). If we expand by a non-multiple, the result is zero, due to float->int 
    implicit cast. */
#if 0
    int pass = (int) (new_fpb / old_fpb) - 1;
    if (pass == 0) pass = 1;

    size_type init_fs = actions.size();

    for (unsigned z=1; z<=pass; z++) {
        for (unsigned i=0; i<init_fs; i++) {
            unsigned newframe = frames.at(i) + (old_fpb*z);
            frames.push_back(newframe);
            global.push_back(actions);
            for (unsigned k=0; k<global.at(i).size(); k++) {
                action* a = global.at(i).at(k);
                rec(a->chan, a->type, newframe, a->iValue, a->fValue);
            }
        }
    }
#endif
}


/* -------------------------------------------------------------------------- */


void shrink(int new_fpb)
{

}


}}}; // giada::m::recorderHandler::


