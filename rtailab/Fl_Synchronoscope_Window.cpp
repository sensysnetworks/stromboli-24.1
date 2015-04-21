/*
COPYRIGHT (C) 2003  Lorenzo Dozio (dozio@aero.polimi.it)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#include "Fl_Synchronoscope_Window.h"

void Fl_Synchronoscope_Window::show()
{
	SWin->show();
}

void Fl_Synchronoscope_Window::hide()
{
	SWin->hide();
}

int Fl_Synchronoscope_Window::is_visible()
{
	return SWin->visible();
}

Fl_Synchronoscope_Window::Fl_Synchronoscope_Window(int x, int y, int width, int height, Fl_MDI_Viewport *s, const char *name)
{
	Fl::lock();
	s->begin();
	Fl_MDI_Window *w = SWin = new Fl_MDI_Window(0, 0, width, height, name);
	w->resizable(w->view());
	w->view()->begin();
	Fl_Synchronoscope *o = Synchronoscope = new Fl_Synchronoscope(0, 0, width, height, name);
	o->mode(FL_DOUBLE|FL_RGB);
	w->view()->end();
	s->end();
	w->titlebar()->h(15);
	w->titlebar()->color(FL_BLACK);
	w->position(x, y);
	Fl::unlock();
}
