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

#include "Fl_Meter.h"

void Fl_Meter::value(float v)
{
	if (v < Minimum_Value) {
		Value = Minimum_Value;
		overload = 1;
		return;
	}
	if (v > Maximum_Value) {
		Value = Maximum_Value;
		overload = 1;
		return;
	}
	overload = 0;
	Value = v;
}

float Fl_Meter::value()
{
	return Value;
}

void Fl_Meter::minimum_value(float v)
{
	Minimum_Value = v;
}

void Fl_Meter::maximum_value(float v)
{
	Maximum_Value = v;
}

void Fl_Meter::bg_color(float r, float g, float b)
{
	Bg_rgb[0] = r;
	Bg_rgb[1] = g;
	Bg_rgb[2] = b;
}

Fl_Color Fl_Meter::bg_color()
{
	return Bg_Color;
}

void Fl_Meter::bg_free_color()
{
	Bg_Color = FL_FREE_COLOR;
}

void Fl_Meter::grid_color(float r, float g, float b)
{
	Grid_rgb[0] = r;
	Grid_rgb[1] = g;
	Grid_rgb[2] = b;
}

Fl_Color Fl_Meter::grid_color()
{
	return Grid_Color;
}

void Fl_Meter::grid_free_color()
{
	Grid_Color = FL_FREE_COLOR;
}

void Fl_Meter::arrow_color(float r, float g, float b)
{
	Arrow_rgb[0] = r;
	Arrow_rgb[1] = g;
	Arrow_rgb[2] = b;
}

Fl_Color Fl_Meter::arrow_color()
{
	return Arrow_Color;
}

void Fl_Meter::arrow_free_color()
{
	Arrow_Color = FL_FREE_COLOR;
}

void Fl_Meter::initgl()
{
	glViewport(0, 0, w(), h());
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-w()/2., w()/2., -h()/3., 2.*h()/3., -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);
	gl_font(FL_HELVETICA_BOLD, 12);
	glClearColor(Bg_rgb[0], Bg_rgb[1], Bg_rgb[2], 0.0);
	glClear(GL_COLOR_BUFFER_BIT);	
}

void Fl_Meter::drawgrid()
{
	char value_label[20];
	char tick_label[20];
	float tick_value;

	glLineWidth(0.1);
	glColor3f(Grid_rgb[0], Grid_rgb[1], Grid_rgb[2]);
	glLineWidth(2.0);
	for (int i = 0; i < N_Ticks; i++) {
		glBegin(GL_LINES);
		glVertex2f((Radius + Radius/10.)*cos(Start_Rad + i*Big_Ticks_Angle), (Radius + Radius/10.)*sin(Start_Rad + i*Big_Ticks_Angle));
		glVertex2f((Radius - Radius/10.)*cos(Start_Rad + i*Big_Ticks_Angle), (Radius - Radius/10.)*sin(Start_Rad + i*Big_Ticks_Angle));
		glEnd();
	}
	glLineWidth(0.1);
	for (int i = 0; i < (int)((N_Ticks - 1)*N_Ticks_Per_Div); i++) {
		glBegin(GL_LINES);
		glVertex2f((Radius + Radius/10.)*cos(Start_Rad + i*Small_Ticks_Angle), (Radius + Radius/10.)*sin(Start_Rad + i*Small_Ticks_Angle));
		glVertex2f((Radius)*cos(Start_Rad + i*Small_Ticks_Angle), (Radius)*sin(Start_Rad + i*Small_Ticks_Angle));
		glEnd();
	}

	glDisable(GL_DEPTH_TEST);

	gl_font(FL_TIMES, (int)(w()/20.));
	if (overload) {
		gl_color(fl_darker(FL_RED));
		gl_draw("overload", -(w()/12.), -(h()/8.));
	} else {
		gl_color(fl_rgb((unsigned char)(Grid_rgb[0]*255.), (unsigned char)(Grid_rgb[1]*255.), (unsigned char)(Grid_rgb[2]*255.)));
		sprintf(value_label, "%1.3f", Value);
		gl_draw(value_label, -(w()/20.), -(h()/8.));
	}

	gl_color(fl_rgb((unsigned char)(Grid_rgb[0]*255.), (unsigned char)(Grid_rgb[1]*255.), (unsigned char)(Grid_rgb[2]*255.)));
	gl_font(FL_TIMES, (int)(w()/25.));
	for (int i = 0; i < (int)(N_Ticks/2.); i++) {
		tick_value = Maximum_Value - i*(Maximum_Value - Minimum_Value)/(N_Ticks - 1);
		sprintf(tick_label, "%1.1f", tick_value);
		gl_draw(tick_label, (Radius - Radius/4.)*cos(Start_Rad + i*Big_Ticks_Angle), (Radius - Radius/4.)*sin(Start_Rad + i*Big_Ticks_Angle));
	}
	tick_value = Maximum_Value - ((N_Ticks - 1)/2.)*(Maximum_Value - Minimum_Value)/(N_Ticks - 1);
	sprintf(tick_label, "%1.1f", tick_value);
	gl_draw(tick_label, -(float(w())/40.), (Radius - Radius/4.));
	for (int i = (int)(N_Ticks/2.) + 1; i < N_Ticks; i++) {
		tick_value = Maximum_Value - i*(Maximum_Value - Minimum_Value)/(N_Ticks - 1);
		sprintf(tick_label, "%1.1f", tick_value);
		gl_draw(tick_label, (Radius - Radius/4.)*cos(Start_Rad + i*Big_Ticks_Angle) - w()/20., (Radius - Radius/4.)*sin(Start_Rad + i*Big_Ticks_Angle));
	}

	glEnable(GL_DEPTH_TEST);
}

void Fl_Meter::draw()
{
	float degree = End_Deg + (Start_Deg - End_Deg)*((Value-Minimum_Value)/(Maximum_Value-Minimum_Value));
	float rad = degree*(M_PI/180.);

	if (!valid()) {
		initgl();
		Radius = (float)(w()/2. - (w()/2.)/4.);
		Arrow_Length = Radius + Radius/10.;
	}
	glClearColor(Bg_rgb[0], Bg_rgb[1], Bg_rgb[2], 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	drawgrid();
	glLineWidth(3.0);
	glColor3f(Arrow_rgb[0], Arrow_rgb[1], Arrow_rgb[2]);
	glBegin(GL_LINES);
	glVertex2f(0., 0.);
	glVertex2f(Arrow_Length*cos(rad), Arrow_Length*sin(rad));
	glEnd();
}

Fl_Meter::Fl_Meter(int x, int y, int w, int h, const char *title):Fl_Gl_Window(x,y,w,h,title)
{
	Start_Deg = -30;
	End_Deg   = 210;
	Start_Rad = (float)Start_Deg*(M_PI/180.);
	End_Rad   = (float)End_Deg*(M_PI/180.);
	Radius = (float)(w/2. - (w/2.)/4.);
	Arrow_Length = Radius + Radius/10.;
	N_Ticks = 13;
	Big_Ticks_Angle = ((End_Deg - Start_Deg)*(M_PI/180.))/(N_Ticks - 1);
	N_Ticks_Per_Div = 5;
	Small_Ticks_Angle = Big_Ticks_Angle/N_Ticks_Per_Div;

	Minimum_Value = -1.2;
	Maximum_Value = 1.2;

	Value = 0.;
	overload = 0;

	Bg_rgb[0] = 1.0;
	Bg_rgb[1] = 1.0;
	Bg_rgb[2] = 1.0;
	Bg_Color = FL_GRAY;
	fl_set_color(FL_FREE_COLOR, fl_rgb((unsigned char)(Bg_rgb[0]*255.), (unsigned char)(Bg_rgb[1]*255.), (unsigned char)(Bg_rgb[2]*255.)));
	Grid_Color = FL_FREE_COLOR;
	Grid_rgb[0] = 0.650;
	Grid_rgb[1] = 0.650;
	Grid_rgb[2] = 0.650;
	Grid_Color = FL_GRAY;
	fl_set_color(FL_FREE_COLOR, fl_rgb((unsigned char)(Grid_rgb[0]*255.), (unsigned char)(Grid_rgb[1]*255.), (unsigned char)(Grid_rgb[2]*255.)));
	Grid_Color = FL_FREE_COLOR;
	Arrow_rgb[0] = 0.;
	Arrow_rgb[1] = 0.;
	Arrow_rgb[2] = 0.;
	Arrow_Color = FL_GRAY;
	fl_set_color(FL_FREE_COLOR, fl_rgb((unsigned char)(Arrow_rgb[0]*255.), (unsigned char)(Arrow_rgb[1]*255.), (unsigned char)(Arrow_rgb[2]*255.)));
	Arrow_Color = FL_FREE_COLOR;
}
