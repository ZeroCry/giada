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


#ifndef G_RECORDER_HANDLER_H
#define G_RECORDER_HANDLER_H


namespace giada {
namespace m 
{
class Action;

namespace recorderHandler
{
bool isBoundaryEnvelopeAction(const Action* a);

/* TODO - move here from ::recorder:

void updateBpm(float oldval, float newval, int oldquanto);
void updateSamplerate(int systemRate, int patchRate);
void writePatch(int chanIndex, std::vector<patch::action_t>& pactions);
void readPatch(const std::vector<patch::action_t>& pactions);
void expand(int old_fpb, int new_fpb);
void shrink(int new_fpb);
*/


}}}; // giada::m::recorderHandler::


#endif
