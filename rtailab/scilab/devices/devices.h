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

void inp_mem_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void inp_mem_input(int port, double * y, double t);
void inp_mem_update();
void inp_mem_end(int port);

void out_mem_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void out_mem_output(int port, double * u,double t);
void out_mem_end(int port);

void inp_square_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void inp_square_input(int port, double * y, double t);
void inp_square_update();
void inp_square_end(int port);

void inp_step_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void inp_step_input(int port, double * y, double t);
void inp_step_update();
void inp_step_end(int port);

void out_rtai_scope_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void out_rtai_scope_output(int port, double * u,double t);
void out_rtai_scope_end(int port);

void inp_rtai_comedi_data_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void inp_rtai_comedi_data_input(int port, double * y, double t);
void inp_rtai_comedi_data_update();
void inp_rtai_comedi_data_end(int port);

void out_rtai_comedi_data_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void out_rtai_comedi_data_output(int port, double * u,double t);
void out_rtai_comedi_data_end(int port);

void inp_rtai_comedi_dio_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void inp_rtai_comedi_dio_input(int port, double * y, double t);
void inp_rtai_comedi_dio_update();
void inp_rtai_comedi_dio_end(int port);

void out_rtai_comedi_dio_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void out_rtai_comedi_dio_output(int port, double * u,double t);
void out_rtai_comedi_dio_end(int port);



void inp_rtai_led_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void inp_rtai_led_input(int port, double * y, double t);
void inp_rtai_led_update();
void inp_rtai_led_end(int port);

void out_rtai_led_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void out_rtai_led_output(int port, double * u,double t);
void out_rtai_led_end(int port);



void inp_rtai_meter_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void inp_rtai_meter_input(int port, double * y, double t);
void inp_rtai_meter_update();
void inp_rtai_meter_end(int port);

void out_rtai_meter_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void out_rtai_meter_output(int port, double * u,double t);
void out_rtai_meter_end(int port);

void inp_pcan_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void inp_pcan_input(int port, double * y, double t);
void inp_pcan_update();
void inp_pcan_end(int port);

void out_pcan_init(int port,int nch,char * sName,double p1,
                  double p2, double p3, double p4, double p5);
void out_pcan_output(int port, double * u,double t);
void out_pcan_end(int port);




