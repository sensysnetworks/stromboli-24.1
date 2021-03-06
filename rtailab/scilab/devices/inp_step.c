/*
COPYRIGHT (C) 2003  Roberto Bucher (roberto.bucher@die.supsi.ch)

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

#include "devstruct.h"

extern devStr inpDevStr[];

void inp_step_init(int port,int nch,char * sName,double p1,
                   double p2, double p3, double p4, double p5)
{
    inpDevStr[port-1].dParam[0]=p1;
    inpDevStr[port-1].dParam[1]=p2;
}

void inp_step_input(int port, double * y, double t)
{
    if(t>=inpDevStr[port-1].dParam[1]) y[0]=inpDevStr[port-1].dParam[0];
    else                               y[0]=0.0;
}

void inp_step_update()
{
}

void inp_step_end(int port)
{
}
