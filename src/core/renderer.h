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


#ifndef G_RENDERER_H
#define G_RENDERER_H


#include <functional>
#include <atomic>


namespace giada {
namespace m {
namespace renderer
{
struct RenderData
{
	std::vector<Channel*> channels;
};


extern AudioBuffer in, out;
extern std::atomic<RenderData*> renderData;


void init(Frame bufferSize);
void shutdown();

void update();


/* trigger
Asks the renderer to produce some data (i.e. to fill the FIFO). The renderer
is dormant otherwise (no busy waits). */

void render();

/* lock
Performs callback 'f' by locking the mutex. WARNING: call this from the UI 
thread or from non-realtime ones! */

void lock(std::function<void()> f);

}}} // giada::m::renderer::;


#endif
