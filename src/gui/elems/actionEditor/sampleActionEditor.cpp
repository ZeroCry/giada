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


#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include "../../../core/recorder/recorder.h"
#include "../../../core/const.h"
#include "../../../core/conf.h"
#include "../../../core/action.h"
#include "../../../core/sampleChannel.h"
#include "../../../utils/log.h"
#include "../../../glue/recorder.h"
#include "../../dialogs/actionEditor/baseActionEditor.h"
#include "sampleAction.h"
#include "sampleActionEditor.h"


using std::vector;


namespace giada {
namespace v
{
geSampleActionEditor::geSampleActionEditor(Pixel x, Pixel y, SampleChannel* ch)
: geBaseActionEditor(x, y, 200, m::conf::sampleActionEditorH, ch)
{
	rebuild();
}


/* -------------------------------------------------------------------------- */


geSampleActionEditor::~geSampleActionEditor()
{
	m::conf::sampleActionEditorH = h();
}


/* -------------------------------------------------------------------------- */


void geSampleActionEditor::rebuild()
{
	namespace mr = m::recorder;
	namespace cr = c::recorder;

	const SampleChannel* ch = static_cast<const SampleChannel*>(m_ch);

	/* Remove all existing actions and set a new width, according to the current
	zoom level. */

	clear();
	size(m_base->fullWidth, h());

	for (const m::Action* a1 : cr::getSampleActions(ch)) {

		if (a1->event.getStatus() == m::MidiEvent::ENVELOPE || isNoteOffSinglePress(a1))
			continue;

		const m::Action* a2 = a1->next;

		Pixel px = x() + m_base->frameToPixel(a1->frame);
		Pixel py = y() + 4;
		Pixel pw = 0;
		Pixel ph = h() - 8;
		if (a2 != nullptr && ch->mode == ChannelMode::SINGLE_PRESS)
			pw = m_base->frameToPixel(a2->frame - a1->frame);

		geSampleAction* gsa = new geSampleAction(px, py, pw, ph, ch, a1, a2);
		add(gsa);
		resizable(gsa);
	}

	/* If channel is LOOP_ANY, deactivate it: a loop mode channel cannot hold 
	keypress/keyrelease actions. */
	
	ch->isAnyLoopMode() ? deactivate() : activate();

	redraw();
}


/* -------------------------------------------------------------------------- */


void geSampleActionEditor::draw()
{
	/* Draw basic boundaries (+ beat bars) and hide the unused area. Then draw 
	children (the actions). */

	baseDraw();

	/* Print label. */

	fl_color(G_COLOR_GREY_4);
	fl_font(FL_HELVETICA, G_GUI_FONT_SIZE_BASE);
	if (active())
		fl_draw("start/stop", x()+4, y(), w(), h(), (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_CENTER));
	else
		fl_draw("start/stop (disabled)", x()+4, y(), w(), h(), (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_CENTER));

	draw_children();
}


/* -------------------------------------------------------------------------- */


void geSampleActionEditor::onAddAction()     
{
	Frame f = m_base->pixelToFrame(Fl::event_x() - x());
	c::recorder::recordSampleAction(static_cast<SampleChannel*>(m_ch), 
		m_base->getActionType(), f);
	rebuild();
}


/* -------------------------------------------------------------------------- */


void geSampleActionEditor::onDeleteAction()  
{
	c::recorder::deleteSampleAction(static_cast<SampleChannel*>(m_ch), m_action->a1);
	rebuild();
}


/* -------------------------------------------------------------------------- */


void geSampleActionEditor::onMoveAction()    
{
	Pixel ex = Fl::event_x() - m_action->pick;

	Pixel x1 = x();
	Pixel x2 = m_base->loopWidth + x() - m_action->w();

	if (ex < x1) ex = x1; else if (ex > x2) ex = x2;

	m_action->setPosition(ex);
}


/* -------------------------------------------------------------------------- */


void geSampleActionEditor::onResizeAction()  
{
	Pixel ex = Fl::event_x();

	Pixel x1 = x();
	Pixel x2 = m_base->loopWidth + x();

	if (ex < x1) ex = x1; else if (ex > x2) ex = x2;

	if (m_action->onRightEdge) 
		m_action->setRightEdge(ex - m_action->x());
	else
		m_action->setLeftEdge(ex);
}


/* -------------------------------------------------------------------------- */


void geSampleActionEditor::onRefreshAction() 
{
	namespace cr = c::recorder;

	SampleChannel* ch = static_cast<SampleChannel*>(m_ch);

	Pixel p1   = m_action->x() - x();
	Pixel p2   = m_action->x() + m_action->w() - x();
	Frame f1   = 0;
	Frame f2   = 0;
	int   type = m_action->a1->event.getStatus();

	if (!m_action->isOnEdges()) {
		f1 = m_base->pixelToFrame(p1);
		f2 = m_base->pixelToFrame(p2, /*snap=*/false) - (m_base->pixelToFrame(p1, /*snap=*/false) - f1);
	}	
	else if (m_action->onLeftEdge) {
		f1 = m_base->pixelToFrame(p1);
		f2 = m_action->a2->frame;
	}
	else if (m_action->onRightEdge) {
		f1 = m_action->a1->frame;
		f2 = m_base->pixelToFrame(p2);
	}

	cr::updateSampleAction(ch, m_action->a1, type, f1, f2);
			
	rebuild();
}


/* -------------------------------------------------------------------------- */


bool geSampleActionEditor::isNoteOffSinglePress(const m::Action* a)
{
	const SampleChannel* ch = static_cast<const SampleChannel*>(m_ch);
	return ch->mode == ChannelMode::SINGLE_PRESS && a->event.getStatus() == m::MidiEvent::NOTE_OFF;
}

}} // giada::v::