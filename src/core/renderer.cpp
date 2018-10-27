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
#include "queue.h"
#include "renderer.h"


namespace giada {
namespace m {
namespace renderer
{
namespace
{
std::condition_variable cond;
std::thread thread;
bool running = false;


/* -------------------------------------------------------------------------- */


void render_()
{
	AudioBuffer in, out;

	out.alloc(4096, 2);
	in.alloc(4096, 2);

	while (running)
	{
		out.clear();

		int full = 0;

		{ // lock scope
			std::unique_lock<std::mutex> lock(mutex);
			cond.wait(lock);

			for (Channel* channel : mixer::channels)
			{
				channel->prepareBuffer(false);
				channel->process(out, in, true, false);
			}
		}

		for (int i = 0; i < out.countFrames(); i++)
		{
			bool done = queue.push(out[i][0]); 
			if (!done) full++; 
		}

		if (full > 0)
			printf("%d times queue full!\n", full);
	}
}

} // {anonymous}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


Queue<float, 8192> queue;
std::mutex         mutex;
bool               ready;


/* -------------------------------------------------------------------------- */


void init()
{
	running = true;
	thread  = std::thread(render_);
}


/* -------------------------------------------------------------------------- */


void shutdown()
{
	/* Trigger the rendering one last time. 'running' is false now so the 
	renderer loop will quit. */

	running = false;
	trigger();
	thread.join();	
}


/* -------------------------------------------------------------------------- */


void trigger()
{
	cond.notify_one();
}


/* -------------------------------------------------------------------------- */



}}} // giada::m::renderer::;