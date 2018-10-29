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


#include <thread>
#include <mutex>
#include <condition_variable>
#include "channel.h"
#include "mixer.h"
#include "audioBuffer.h"
#include "const.h"
#include "queue.h"
#include "renderer.h"


namespace giada {
namespace m {
namespace renderer
{
namespace
{
} // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


AudioBuffer in, out;
std::atomic<RenderData*> renderData;


/* -------------------------------------------------------------------------- */


void update()
{
	RenderData* oldRenderData = renderData.load();
	RenderData* newRenderData = new RenderData();

	newRenderData->channels = mixer::channels;

	while (!renderData.compare_exchange_weak(oldRenderData, newRenderData))
	{
		std::cout << "ui: can't swap, BUSY!\n";
	}

	delete oldRenderData;	
}


/* -------------------------------------------------------------------------- */


void init(Frame bufferSize)
{
	out.alloc(bufferSize, G_MAX_IO_CHANS);
	in.alloc(bufferSize, G_MAX_IO_CHANS);

	RenderData* rd = new RenderData();
	rd->channels = mixer::channels;

	renderData.store(rd);
}


/* -------------------------------------------------------------------------- */


void shutdown()
{
	delete renderData;
	/* Trigger the rendering one last time. 'running' is false now so the 
	renderer loop will quit. */

	//trigger();
	//thread.join();	
}


/* -------------------------------------------------------------------------- */


void render()
{
	out.clear();

	RenderData* rd = renderData.load();
	for (Channel* channel : rd->channels) {
		channel->prepareBuffer(false);
		channel->process(out, in, true, false);
	}
}


/* -------------------------------------------------------------------------- */


void lock(std::function<void()> f)
{
	//mutex.lock();
	//f();
	//mutex.unlock();
}


}}} // giada::m::renderer::;