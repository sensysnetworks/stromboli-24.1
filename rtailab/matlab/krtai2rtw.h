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

#define MAX_NAME_SIZE  50

typedef struct rtTargetParamInfo {
    char modelName[MAX_NAME_SIZE];
    char blockName[MAX_NAME_SIZE];
    char paramName[MAX_NAME_SIZE];
    unsigned int dataType;
    unsigned int dataClass;
    double dataValue;
} rtTargetParamInfo;

void start_rtw();
void update_rtw();
void stop_rtw();
long long get_t_samp();
int get_nBlockParams();
int rt_GetParameterInfo(rtTargetParamInfo * rtpi, int i);
int rt_ModifyParameterValue(int i, void *_newVal);
float rtGetT();
int rtGetNumInpP_scope(int idx);
int * rtGetInpPDim_log(int idx);
char * rtGetModelName_scope(int idx);
char * rtGetModelName_log(int idx);
float rtGetSampT_scope(int idx);
float rtGetSampT_log(int idx);




