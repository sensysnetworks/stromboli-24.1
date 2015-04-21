/*
COPYRIGHT (C) 2003  Lorenzo Dozio (dozio@aero.polimi.it)
		    Paolo Mantegazza (mantegazza@aero.polimi.it)
		    Roberto Bucher (roberto.bucher@supsi.ch)

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

#include <efltk/Fl.h>
#include <efltk/Fl_Item.h>
#include <efltk/Fl_Window.h>
#include <efltk/Fl_Double_Window.h>
#include <efltk/Fl_Button.h>
#include <efltk/Fl_Check_Button.h>
#include <efltk/Fl_Radio_Button.h>
#include <efltk/Fl_Browser.h>
#include <efltk/Fl_Menu_Bar.h>
#include <efltk/Fl_Main_Window.h>
#include <efltk/Fl_Input.h>
#include <efltk/Fl_Workspace.h>
#include <efltk/Fl_MDI_Window.h>
#include <efltk/Fl_MDI_Bar.h>
#include <efltk/Fl_ListView.h>
#include <efltk/Fl_ListView_Item.h>
#include <efltk/Fl_File_Dialog.h>
#include <efltk/Fl_Stock_Images.h>
#include <efltk/Fl_Radio_Buttons.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl_Input_Browser.h>
#include <efltk/Fl_Config.h>
#include <efltk/Fl_Item.h>
#include <efltk/Fl_Toggle_Item.h>
#include <efltk/Fl_Item_Group.h>
#include <efltk/Fl_Font.h>
#include <efltk/Fl_Widget.h>
#include <efltk/Fl_Dialog.h>
#include <efltk/fl_ask.h>
#include <efltk/Fl_Choice.h>
#include <efltk/Fl_Tabs.h>
#include <efltk/Fl_Int_Input.h>
#include <efltk/Fl_Float_Input.h>
#include <efltk/Fl_Light_Button.h>
#include <efltk/Fl_Color_Chooser.h>
#include <efltk/Fl_Dial.h>
#include <efltk/Fl_Value_Input.h>
#include <efltk/Fl_Button_Group.h>
#include <efltk/Fl_Radio_Buttons.h>

#include "session_open_icon.xpm"
#include "session_save_icon.xpm"
#include "start_icon.xpm"
#include "stop_icon.xpm"
#include "parameters_icon.xpm"
#include "log_icon.xpm"
#include "scope_icon.xpm"
#include "connect_icon.xpm"
#include "disconnect_icon.xpm"
#include "profile_icon.xpm"
#include "folder_small.xpm"
#include "led_icon.xpm"
#include "meter_icon.xpm"
#include "synchronoscope_icon.xpm"

#include <stdio.h>
#include <pthread.h>
#include <sys/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/io.h>
#include <sys/poll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include <net_rpc.h>

#include <Fl_Scope.h>
#include <Fl_Scope_Window.h>
#include <Fl_Led.h>
#include <Fl_Led_Window.h>
#include <Fl_Meter.h>
#include <Fl_Meter_Window.h>
#include <Fl_Synchronoscope.h>
#include <Fl_Synchronoscope_Window.h>

#define DBG

#define MAX_NHOSTS		100
#define MAX_NAMES_SIZE		256
#define MAX_DATA_SIZE		30
#define MAX_RTAI_SCOPES		1000
#define MAX_RTAI_LOGS		1000
#define MAX_RTAI_METERS		1000
#define MAX_RTAI_LEDS		1000
#define MAX_BATCH_PARAMS	1000
#define FLTK_EVENTS_TICK        0.05
#define MAX_MSG_LEN             (MAX_MSG_SIZE - 100)
#define REFRESH_RATE            0.05

#define SCOPE_WIN_T		0
#define LOG_WIN_T		1
#define PARAM_WIN_T		2

#define CONNECT_TO_TARGET       0
#define DISCONNECT_FROM_TARGET  1
#define START_TARGET            2
#define STOP_TARGET             3
#define UPDATE_PARAM            4
#define GET_TARGET_TIME         5
#define BATCH_DOWNLOAD          6
#define GET_PARAMS              7
#define CLOSE                   8

#define msleep(t)       do { poll(0, 0, t); } while (0)

typedef struct Target_Parameters_Struct Target_Parameters_T;
typedef struct Target_Blocks_Struct Target_Blocks_T;
typedef struct Target_Scopes_Struct Target_Scopes_T;
typedef struct Target_Logs_Struct Target_Logs_T;
typedef struct Target_Leds_Struct Target_Leds_T;
typedef struct Target_Meters_Struct Target_Meters_T;
typedef struct Target_Synchronoscope_Struct Target_Synchronoscope_T;
typedef struct Batch_Parameters_Struct Batch_Parameters_T;

typedef struct s_idx_T {
	int scope_idx;
	int trace_idx;
};

typedef struct p_idx_T {
	int block_idx;
	int param_idx;
	int val_idx;
};

struct Target_Parameters_Struct
{
	char model_name[MAX_NAMES_SIZE];
	char block_name[MAX_NAMES_SIZE];
	char param_name[MAX_NAMES_SIZE];
	unsigned int n_rows;
	unsigned int n_cols;
	unsigned int data_type;
	unsigned int data_class;
	double data_value[MAX_DATA_SIZE];
};

struct Target_Blocks_Struct
{
	char name[MAX_NAMES_SIZE];
	int offset;
};

struct Target_Scopes_Struct
{
	char name[MAX_NAMES_SIZE];
	int ntraces;
	int visible;
	float dt;
};

struct Target_Logs_Struct
{
	char name[MAX_NAMES_SIZE];
	int nrow;
	int ncol;
	float dt;
};

struct Target_Leds_Struct
{
	char name[MAX_NAMES_SIZE];
	int n_leds;
	int visible;
	float dt;
};

struct Target_Meters_Struct
{
	char name[MAX_NAMES_SIZE];
	int visible;
	float dt;
};

struct Target_Synchronoscope_Struct
{
	char name[MAX_NAMES_SIZE];
	int visible;
	float dt;
};

struct Batch_Parameters_Struct
{
	int index;
	int mat_index;
	double value;
};

typedef enum {
	rt_SCALAR,
	rt_VECTOR,
	rt_MATRIX_ROW_MAJOR,
	rt_MATRIX_COL_MAJOR,
	rt_MATRIX_COL_MAJOR_ND
} ParamClass;

static int Is_Target_Connected = 0;
static unsigned int Is_Target_Running = 0;
static long Target_Node = 0;
static int Num_Tunable_Parameters = 0;
static int Num_Tunable_Blocks = 0;
static int Num_Scopes = 0;
static int Num_Logs = 0;
static int Num_Leds = 0;
static int Num_Meters = 0;
static int Num_Synchronoscope = 0;
int EndApp = 0;
int EndAll = 0;
Target_Parameters_T *Tunable_Parameters;
Target_Blocks_T *Tunable_Blocks;
Target_Scopes_T *Scopes;
Target_Logs_T *Logs;
Target_Leds_T *Leds;
Target_Meters_T *Meters;
Target_Synchronoscope_T Synchronoscope;
Batch_Parameters_T Batch_Parameters[MAX_BATCH_PARAMS];

Fl_Main_Window *MainWin;
Fl_Menu_Bar *MainMenu;
Fl_Tool_Bar *MainToolBar;
Fl_Workspace *MainWorkSpace;
Fl_Widget *Main_Status;
Fl_Group *Parameters_Group, *Scope_Group, *Log_Group;
Fl_Box *Scope_Group_Box;
Fl_Group **Common_Scope_Settings;

Fl_Dialog *Connect_Dialog;
Fl_Dialog *Edit_Profiles_Dialog;
Fl_Tabs *Profile_Tabs;
Fl_Browser *Profiles_Tree;
Fl_Group *Profiles_Folder;
Fl_Group *Profile_Connection;
Fl_Input *Profile_IP_Addr, *Profile_Task_ID, *Profile_Scope_ID, *Profile_Log_ID;
Fl_Group *Profile_Scopes;

Fl_Dialog *Add_Profile_Dialog;

Fl_Tool_Button *View_Target_Settings;
Fl_Tool_Button *Connect_To_Target, *Disconnect_From_Target, *Edit_Profiles, *Add_Profile;
Fl_Tool_Button *Start_RT, *Stop_RT;
Fl_Tool_Button *Open_Parameters_Manager, *Open_Logs_Manager, *Open_Scopes_Manager, *Open_Leds_Manager, *Open_Meters_Manager;
Fl_Tool_Button *Open_Synchronoscope;
Fl_Choice *Profile_Choice;
Fl_Input *Profile_Input;

Fl_Window *Target_Settings_Win;
Fl_Button *Target_Settings_Ok;
Fl_Button *Target_Settings_Cancel;
Fl_Input *Target_IP_Address;
Fl_Input *Target_Task_ID, *Target_Scope_ID, *Target_Log_ID;
Fl_Input *Target_Led_ID, *Target_Meter_ID, *Target_Synchronoscope_ID;
char *TargetIP;
char *TargetInterfaceTaskName;
char *TargetScopeMbxID;
char *TargetLogMbxID;
char *TargetLedMbxID;
char *TargetMeterMbxID;
char *TargetSynchronoscopeMbxID;

RT_TASK *TargetInterfaceTask;
pthread_t TargetInterfaceThread;
pthread_t *GetScopeDataThread;
pthread_t *GetLogDataThread;
pthread_t *GetLedDataThread;
pthread_t *GetMeterDataThread;
pthread_t GetSynchronoscopeDataThread;

void quit_cb(Fl_Widget*, void*);
void start_stop_cb(Fl_Widget *, void *);
void open_synchronoscope_cb(Fl_Widget *, void *);
void open_meters_manager_cb(Fl_Widget *, void *);
void open_leds_manager_cb(Fl_Widget *, void *);
void open_logs_manager_cb(Fl_Widget *, void *);
void open_scopes_manager_cb(Fl_Widget *, void *);
void open_parameters_manager_cb(Fl_Widget *, void *);
void edit_profiles_cb(Fl_Widget *, void *);
void add_profile_cb(Fl_Widget *, void *);
void connect_cb(Fl_Widget *, void *);
void disconnect_cb(Fl_Widget *, void *);

Fl_Menu_Item MainMenuTable[] = {
	{" &File", FL_ALT+'f', 0, 0, FL_SUBMENU},
		{" Connect... ", FL_ALT+'c', connect_cb, 0},
		{" Disconnect ", FL_ALT+'d', disconnect_cb, 0, FL_MENU_INACTIVE|FL_MENU_DIVIDER},
		{" Edit Profiles... ", 0, edit_profiles_cb, 0},
		{" Add Profile... ", 0, add_profile_cb, 0, FL_MENU_INACTIVE},
		{" Delete Profile... ", 0, 0, 0, FL_MENU_DIVIDER},
		{" Quit ", FL_ALT+'q', quit_cb, 0, 0},
		{0},
	{" &View", FL_ALT+'v', 0, 0, FL_SUBMENU},
		{" Parameters ", FL_ALT+'p', open_parameters_manager_cb, 0, FL_MENU_INACTIVE|FL_MENU_TOGGLE},
		{" Scopes ", FL_ALT+'s', open_scopes_manager_cb, 0, FL_MENU_INACTIVE|FL_MENU_TOGGLE},
		{" Logs ", FL_ALT+'l', open_logs_manager_cb, 0, FL_MENU_INACTIVE|FL_MENU_TOGGLE},
		{" Leds ", FL_ALT+'e', open_leds_manager_cb, 0, FL_MENU_INACTIVE|FL_MENU_TOGGLE},
		{" Meters ", FL_ALT+'m', open_meters_manager_cb, 0, FL_MENU_INACTIVE|FL_MENU_TOGGLE},
		{" Synchronoscope ", FL_ALT+'y', open_synchronoscope_cb, 0, FL_MENU_INACTIVE|FL_MENU_TOGGLE|FL_MENU_DIVIDER},
		{" Close All ", 0, 0, 0, FL_MENU_INACTIVE},
		{0},
	{" &Help", FL_ALT+'h', 0, 0, FL_SUBMENU},
		{" Help contents ", 0, 0},
		{" About RTAI-Lab ", 0, 0},
		{0},
	{0}
};

static volatile int GlobalRet[16];

static inline void RT_RPC(RT_TASK *task, unsigned int msg, unsigned int *reply)
{
	GlobalRet[msg & 0xf] = 0;
	rt_send(task, msg);
	while (!GlobalRet[msg & 0xf]) {
		Fl::wait(FLTK_EVENTS_TICK);
	}
}

static inline void RT_RETURN(RT_TASK *task, unsigned int reply)
{
	GlobalRet[reply] = 1;
}

static Fl_Group* add_folder(Fl_Group* parent, const char* name, int open, Fl_Image* image)
{
	parent->begin();
	Fl_Item_Group* o = new Fl_Item_Group(name);
	o->image(image);
	if (open) o->set_flag(FL_VALUE);
	return o;
}

static Fl_Widget* add_paper(Fl_Group* parent, const char* name, Fl_Image* image)
{
	parent->begin();
	Fl_Item* o = new Fl_Item(name);
	o->image(image);
	return o;
}

class Scopes_Manager
{
	public:
		Scopes_Manager(int w, int h, Fl_MDI_Viewport *s, const char *name);
		void show();
		void hide();
		int is_visible();
		int n_points_to_save(int);
		int start_saving(int);
		void stop_saving(int);
		FILE *save_file(int);
		Fl_Scope_Window **Scope_Windows;
	private:
		Fl_MDI_Window *SWin;
		Fl_Browser *Scopes_Tree;

		Fl_Tabs **Scopes_Tabs;
		Fl_Check_Button **Scope_Show;
		Fl_Check_Button **Scope_Pause;
		Fl_Check_Button **Scope_OneShot;
		Fl_Check_Button **Grid_On;
		Fl_Button **Grid_Color, **Bg_Color;
		Fl_Input_Browser **Sec_Div;
		Fl_Check_Button **Save_Type;
		Fl_Int_Input **Save_Points;
		Fl_Float_Input **Save_Time;
		Fl_Input **Save_File;
		Fl_Light_Button **Save;

		int *Save_Flag;
		FILE **Save_File_Pointer;

		Fl_Group ***Trace_Page;
		Fl_Check_Button ***Trace_Show;
		Fl_Input_Browser ***Units_Div;
		Fl_Button ***Trace_Color;
		Fl_Dial ***Trace_Pos;

		Fl_Button *Help, *Close;

		inline void select_scope_i(Fl_Browser *, void *);
		static void select_scope(Fl_Browser *, void *);
		inline void show_scope_i(Fl_Check_Button *, void *);
		static void show_scope(Fl_Check_Button *, void *);
		inline void pause_scope_i(Fl_Check_Button *, void *);
		static void pause_scope(Fl_Check_Button *, void *);
		inline void oneshot_scope_i(Fl_Check_Button *, void *);
		static void oneshot_scope(Fl_Check_Button *, void *);
		inline void show_grid_i(Fl_Check_Button *, void *);
		static void show_grid(Fl_Check_Button *, void *);
		inline void select_grid_color_i(Fl_Button *, void *);
		static void select_grid_color(Fl_Button *, void *);
		inline void select_bg_color_i(Fl_Button *, void *);
		static void select_bg_color(Fl_Button *, void *);
		inline void enter_secdiv_i(Fl_Input_Browser *, void *);
		static void enter_secdiv(Fl_Input_Browser *, void *);
		inline void select_save_i(Fl_Check_Button *, void *);
		static void select_save(Fl_Check_Button *, void *);
		inline void enable_saving_i(Fl_Light_Button *, void *);
		static void enable_saving(Fl_Light_Button *, void *);
		inline void show_trace_i(Fl_Check_Button *, void *);
		static void show_trace(Fl_Check_Button *, void *);
		inline void enter_unitsdiv_i(Fl_Input_Browser *, void *);
		static void enter_unitsdiv(Fl_Input_Browser *, void *);
		inline void select_trace_color_i(Fl_Button *, void *);
		static void select_trace_color(Fl_Button *, void *);
		inline void change_trace_pos_i(Fl_Dial *, void *);
		static void change_trace_pos(Fl_Dial *, void *);
		inline void close_i(Fl_Button *, void *);
		static void close(Fl_Button *, void *);
};

void Scopes_Manager::show()
{
        SWin->show();
}

void Scopes_Manager::hide()
{
        SWin->hide();
}

int Scopes_Manager::is_visible()
{
        return SWin->visible();
}

int Scopes_Manager::start_saving(int n)
{
	return Save_Flag[n];
}

int Scopes_Manager::n_points_to_save(int n)
{
	int n_points;

	if (Save_Type[n]->value()) {
		n_points = (int)atoi(Save_Points[n]->value());
		if (n_points < 0) return 0;
	} else {
		n_points = (int)(atof(Save_Time[n]->value())*Scope_Windows[n]->Plot->sampling_frequency());
		if (n_points < 0) return 0;
	}
	return n_points;
}

void Scopes_Manager::stop_saving(int n)
{
	fclose(Save_File_Pointer[n]);
	Save_Type[n]->activate();
	if (Save_Type[n]->value()) {
		Save_Points[n]->activate();
	} else {
		Save_Time[n]->activate();
	}
	Save_File[n]->activate();
	Save[n]->activate();
	Save[n]->value(0);
	Save[n]->label("Save");
	Scopes_Tabs[n]->redraw();
	Save_Flag[n] = false;
}

FILE *Scopes_Manager::save_file(int n)
{
	return Save_File_Pointer[n];
}

inline void Scopes_Manager::select_scope_i(Fl_Browser *b, void *v)
{
	int n = b->value();
	for (int i = 0; i < Num_Scopes; i++) {
		Scopes_Tabs[i]->hide();
	}
	Scopes_Tabs[n]->show();
}

void Scopes_Manager::select_scope(Fl_Browser *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->user_data()))->select_scope_i(b,v);
}

inline void Scopes_Manager::show_scope_i(Fl_Check_Button *b, void *v)
{
	int n = (int)v;
	if (b->value()) {
		Scopes[n].visible = true;
		Scope_Pause[n]->activate();
//		Scope_OneShot[n]->activate();
	} else {
		Scopes[n].visible = false;
		Scope_Pause[n]->deactivate();
		Scope_OneShot[n]->deactivate();
	}
}

void Scopes_Manager::show_scope(Fl_Check_Button *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->show_scope_i(b,v);
}

inline void Scopes_Manager::pause_scope_i(Fl_Check_Button *b, void *v)
{
	int n = (int)v;
	if (b->value()) {
		Scope_Windows[n]->Plot->pause(true);
	} else {
		Scope_Windows[n]->Plot->pause(false);
	}
}

void Scopes_Manager::pause_scope(Fl_Check_Button *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->pause_scope_i(b,v);
}

inline void Scopes_Manager::oneshot_scope_i(Fl_Check_Button *b, void *v)
{
	int n = (int)v;
	if (b->value()) {
		Scope_Windows[n]->Plot->oneshot(true);
	} else {
		Scope_Windows[n]->Plot->oneshot(false);
	}
}

void Scopes_Manager::oneshot_scope(Fl_Check_Button *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->oneshot_scope_i(b,v);
}

inline void Scopes_Manager::show_grid_i(Fl_Check_Button *b, void *v)
{
	int n = (int)v;
	if (b->value()) {
		Scope_Windows[n]->Plot->grid_visible(true);
	} else {
		Scope_Windows[n]->Plot->grid_visible(false);
	}
}

void Scopes_Manager::show_grid(Fl_Check_Button *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->show_grid_i(b,v);
}

inline void Scopes_Manager::select_grid_color_i(Fl_Button *bb, void *v)
{
	int n = (int)v;
	uchar r,g,b;
	Fl_Color c;

	c = Scope_Windows[n]->Plot->grid_color();
	fl_get_color(c,r,g,b);
	if (!fl_color_chooser("New color:",r,g,b)) return;
	c = FL_FREE_COLOR;
	Scope_Windows[n]->Plot->grid_free_color();
	fl_set_color(FL_FREE_COLOR, fl_rgb(r,g,b));
	Scope_Windows[n]->Plot->grid_color(r/255.,g/255.,b/255.);
}

void Scopes_Manager::select_grid_color(Fl_Button *bb, void *v)
{
	((Scopes_Manager *)(bb->parent()->parent()->parent()->parent()->user_data()))->select_grid_color_i(bb,v);
}

inline void Scopes_Manager::select_bg_color_i(Fl_Button *bb, void *v)
{
	int n = (int)v;
	uchar r,g,b;
	Fl_Color c;

	c = Scope_Windows[n]->Plot->bg_color();
	fl_get_color(c,r,g,b);
	if (!fl_color_chooser("New color:",r,g,b)) return;
	c = FL_FREE_COLOR;
	Scope_Windows[n]->Plot->bg_free_color();
	fl_set_color(FL_FREE_COLOR, fl_rgb(r,g,b));
	Scope_Windows[n]->Plot->bg_color(r/255.,g/255.,b/255.);
}

void Scopes_Manager::select_bg_color(Fl_Button *bb, void *v)
{
	((Scopes_Manager *)(bb->parent()->parent()->parent()->parent()->user_data()))->select_bg_color_i(bb,v);
}

inline void Scopes_Manager::enter_secdiv_i(Fl_Input_Browser *b, void *v)
{
	int n = (int)v;
	float val = (float)atof(b->value());

	if (val > 0.) {
		Scope_Windows[n]->Plot->time_range(val*NDIV_GRID_X);
	}
}

void Scopes_Manager::enter_secdiv(Fl_Input_Browser *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->enter_secdiv_i(b,v);
}

inline void Scopes_Manager::select_save_i(Fl_Check_Button *b, void *v)
{
	int n = (int)v;
	if (b->value()) {
		Save_Points[n]->activate();
		Save_Time[n]->deactivate();
	} else {
		Save_Points[n]->deactivate();
		Save_Time[n]->activate();
	}
	Scopes_Tabs[n]->redraw();
}

void Scopes_Manager::select_save(Fl_Check_Button *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->select_save_i(b,v);
}

inline void Scopes_Manager::enable_saving_i(Fl_Light_Button *b, void *v)
{
	int n = (int)v;
	if (b->value()) {
		if ((Save_File_Pointer[n] = fopen(Save_File[n]->value(), "a+")) == NULL) {
			fl_alert("Error in opening file %s", Save_File[n]->value());
			return;
		}
		b->deactivate();
		Save_Type[n]->deactivate();
		Save_Time[n]->deactivate();
		Save_Points[n]->deactivate();
		Save_File[n]->deactivate();
		Save[n]->label("Saving...");
		Scopes_Tabs[n]->redraw();
		Save_Flag[n] = true;
	}
}

void Scopes_Manager::enable_saving(Fl_Light_Button *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->enable_saving_i(b,v);
}

inline void Scopes_Manager::show_trace_i(Fl_Check_Button *b, void *v)
{
	s_idx_T *idx = (s_idx_T *)v;
	int scope = idx->scope_idx;
	int trace = idx->trace_idx;
	if (b->value()) {
		Scope_Windows[scope]->Plot->show_trace(trace, true);
	} else {
		Scope_Windows[scope]->Plot->show_trace(trace, false);
	}
}

void Scopes_Manager::show_trace(Fl_Check_Button *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->show_trace_i(b,v);
}

inline void Scopes_Manager::enter_unitsdiv_i(Fl_Input_Browser *b, void *v)
{
	s_idx_T *idx = (s_idx_T *)v;
	int scope = idx->scope_idx;
	int trace = idx->trace_idx;
	float val = (float)atof(b->value());

	if (val > 0.) {
		Scope_Windows[scope]->Plot->y_range_inf(trace, -val*NDIV_GRID_Y/2.);
		Scope_Windows[scope]->Plot->y_range_sup(trace, val*NDIV_GRID_Y/2.);
	}
}

void Scopes_Manager::enter_unitsdiv(Fl_Input_Browser *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->enter_unitsdiv_i(b,v);
}

inline void Scopes_Manager::select_trace_color_i(Fl_Button *bb, void *v)
{
	s_idx_T *idx = (s_idx_T *)v;
	int scope = idx->scope_idx;
	int trace = idx->trace_idx;
	uchar r,g,b;
	Fl_Color c;

	c = Scope_Windows[scope]->Plot->trace_color(trace);
	fl_get_color(c,r,g,b);
	if (!fl_color_chooser("New color:",r,g,b)) return;
	c = FL_FREE_COLOR;
	Scope_Windows[scope]->Plot->trace_free_color(trace);
	fl_set_color(FL_FREE_COLOR, fl_rgb(r,g,b));
	Scope_Windows[scope]->Plot->trace_color(trace, r/255.,g/255.,b/255.);
	Trace_Page[scope][trace]->label_color(fl_rgb(r,g,b));
	Trace_Page[scope][trace]->redraw();
}

void Scopes_Manager::select_trace_color(Fl_Button *bb, void *v)
{
	((Scopes_Manager *)(bb->parent()->parent()->parent()->parent()->user_data()))->select_trace_color_i(bb,v);
}

inline void Scopes_Manager::change_trace_pos_i(Fl_Dial *b, void *v)
{
	s_idx_T *idx = (s_idx_T *)v;
	int scope = idx->scope_idx;
	int trace = idx->trace_idx;

	Scope_Windows[scope]->Plot->trace_offset(trace, (float)b->value());
}

void Scopes_Manager::change_trace_pos(Fl_Dial *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->change_trace_pos_i(b,v);
}

inline void Scopes_Manager::close_i(Fl_Button *b, void *v)
{
	SWin->hide();
	MainMenuTable[10].clear();
	Open_Scopes_Manager->clear();
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
}

void Scopes_Manager::close(Fl_Button *b, void *v)
{
	((Scopes_Manager *)(b->parent()->parent()->user_data()))->close_i(b,v);
}

Scopes_Manager::Scopes_Manager(int width, int height, Fl_MDI_Viewport *s, const char *name)
{
	Fl::lock();

	s->begin();
	Fl_MDI_Window *w = SWin = new Fl_MDI_Window(0, 0, width, height, name);
	w->user_data((void *)this);
	w->resizable(w->view());

	w->titlebar()->close_button()->hide();

	w->view()->begin();

	Scopes_Tabs = new Fl_Tabs*[Num_Scopes];
	Scope_Show = new Fl_Check_Button*[Num_Scopes];
	Scope_Pause = new Fl_Check_Button*[Num_Scopes];
	Scope_OneShot = new Fl_Check_Button*[Num_Scopes];
	Grid_On = new Fl_Check_Button*[Num_Scopes];
	Grid_Color = new Fl_Button*[Num_Scopes];
	Bg_Color = new Fl_Button*[Num_Scopes];
	Sec_Div = new Fl_Input_Browser*[Num_Scopes];
	Save_Type = new Fl_Check_Button*[Num_Scopes];
	Save_Points = new Fl_Int_Input*[Num_Scopes];
	Save_Time = new Fl_Float_Input*[Num_Scopes];
	Save_File = new Fl_Input*[Num_Scopes];
	Save = new Fl_Light_Button*[Num_Scopes];
	Save_Flag = new int[Num_Scopes];
	Save_File_Pointer = new FILE*[Num_Scopes];

	Trace_Page = new Fl_Group**[Num_Scopes];
	Trace_Show = new Fl_Check_Button**[Num_Scopes];
	Units_Div = new Fl_Input_Browser**[Num_Scopes];
	Trace_Color = new Fl_Button**[Num_Scopes];
	Trace_Pos = new Fl_Dial**[Num_Scopes];
	Scope_Windows = new Fl_Scope_Window*[Num_Scopes];

	for (int i = 0; i < Num_Scopes; i++) {
		Save_Flag[i] = false;
		{ Fl_Tabs *o = Scopes_Tabs[i] = new Fl_Tabs(160, 5, width-165, height-40);
		  o->new_page("General");
//		  o->new_page("General", false);
		  { Fl_Check_Button *o = Scope_Show[i] = new Fl_Check_Button(10, 25, 100, 20, "Show/Hide");
		    o->callback((Fl_Callback *)show_scope, (void *)i);
		  }
		  { Fl_Check_Button *o = Scope_Pause[i] = new Fl_Check_Button(10, 50, 100, 20, "Pause/Run");
		    o->deactivate();
		    o->callback((Fl_Callback *)pause_scope, (void *)i);
		  }
		  { Fl_Check_Button *o = Scope_OneShot[i] = new Fl_Check_Button(10, 75, 100, 20, "OneShot/Run");
		    o->deactivate();
		    o->callback((Fl_Callback *)oneshot_scope, (void *)i);
		  }
		  { Fl_Check_Button *o = Grid_On[i] = new Fl_Check_Button(10, 110, 100, 20, "Grid on/off");
		    o->value(1);
		    o->callback((Fl_Callback *)show_grid, (void *)i);
		  }
		  { Fl_Button *o = Grid_Color[i] = new Fl_Button(10, 150, 90, 25, "Grid Color");
		    o->callback((Fl_Callback *)select_grid_color, (void *)i);
		  }
		  { Fl_Button *o = Bg_Color[i] = new Fl_Button(10, 180, 90, 25, "Bg Color");
		    o->callback((Fl_Callback *)select_bg_color, (void *)i);
		  }
		  { Fl_Input_Browser *o = Sec_Div[i] = new Fl_Input_Browser(200, 25, 60, 20, "Sec/Div:  ");
		    o->add("0.001|0.005|0.01|0.05|0.1|0.5|1");
		    o->align(FL_ALIGN_LEFT);
		    o->value("0.1");
		    o->when(FL_WHEN_ENTER_KEY);
		    o->callback((Fl_Callback *)enter_secdiv, (void *)i);
		  }
		  { Fl_Check_Button *o = Save_Type[i] = new Fl_Check_Button(140, 70, 100, 20, "Points/Time");
		    o->value(1);
		    o->callback((Fl_Callback *)select_save, (void *)i);
		  }
		  { Fl_Int_Input *o = Save_Points[i] = new Fl_Int_Input(200, 95, 60, 20, "N Points: ");
		    o->align(FL_ALIGN_LEFT);
		    o->value("1000");
		  }
		  { Fl_Float_Input *o = Save_Time[i] = new Fl_Float_Input(200, 120, 60, 20, "Time [s]:  ");
		    o->align(FL_ALIGN_LEFT);
		    o->value("1.0");
		    o->deactivate();
		  }
		  { Fl_Input *o = Save_File[i] = new Fl_Input(200, 145, 100, 20, "Filename:");
		    char buf[100];
		    o->align(FL_ALIGN_LEFT);
		    sprintf(buf, "%s", Scopes[i].name);
		    o->value(buf);
		  }
		  { Fl_Light_Button *o = Save[i] = new Fl_Light_Button(140, 180, 90, 25, "Save");
		    o->selection_color(FL_BLACK);
		    o->callback((Fl_Callback *)enable_saving, (void *)i);
		  }
		  Trace_Page[i] = new Fl_Group*[Scopes[i].ntraces];
		  Trace_Show[i] = new Fl_Check_Button*[Scopes[i].ntraces];
		  Units_Div[i] = new Fl_Input_Browser*[Scopes[i].ntraces];
		  Trace_Color[i] = new Fl_Button*[Scopes[i].ntraces];
		  Trace_Pos[i] = new Fl_Dial*[Scopes[i].ntraces];
		  for (int j = 0; j < Scopes[i].ntraces; j++) {
			char buf[10];
			s_idx_T *idx = new s_idx_T;
			idx->scope_idx = i;
			idx->trace_idx = j;
			sprintf(buf, "Trace %d", j+1);
		  	Trace_Page[i][j] = o->new_page(buf);
//		  	Trace_Page[i][j] = o->new_page(buf, false);
			Trace_Page[i][j]->label_color(FL_WHITE);
			{ Fl_Check_Button *o = Trace_Show[i][j] = new Fl_Check_Button(10, 25, 100, 20, "Show/Hide");
			  o->value(1);
		    	  o->callback((Fl_Callback *)show_trace, (void *)idx);
		  	}
		  	{ Fl_Input_Browser *o = Units_Div[i][j] = new Fl_Input_Browser(77, 55, 60, 20, "Units/Div:  ");
		    	  o->align(FL_ALIGN_LEFT);
		    	  o->value("2.5");
			  o->add("0.001|0.002|0.005|0.01|0.02|0.05|0.1|0.2|0.5|1|2|5|10|50|100|1000");
		    	  o->when(FL_WHEN_ENTER_KEY);
		    	  o->callback((Fl_Callback *)enter_unitsdiv, (void *)idx);
		  	}
		  	{ Fl_Button *o = Trace_Color[i][j] = new Fl_Button(10, 90, 90, 25, "Trace Color");
		    	  o->callback((Fl_Callback *)select_trace_color, (void *)idx);
		  	}
			{ Fl_Dial *o = Trace_Pos[i][j] = new Fl_Dial(200, 40, 50, 50, "Trace Offset");
			  o->type(Fl_Dial::LINE);
			  o->minimum(0.0);
			  o->maximum(2.0);
			  o->value(1);
		    	  o->callback((Fl_Callback *)change_trace_pos, (void *)idx);
			}
		  }
		  o->end();
		  Fl_Group::current()->resizable(w);
		}
	}
	for (int i = 1; i < Num_Scopes; i++) {
		Scopes_Tabs[i]->hide();
	}
	Scopes_Tabs[0]->show();
	Help = new Fl_Button(width-150, height-30, 70, 25, "Help");
	Close = new Fl_Button(width-75, height-30, 70, 25, "Close");
	Close->callback((Fl_Callback *)close);
	Fl_Browser *o = Scopes_Tree = new Fl_Browser(5, 5, 150, height-10);
	o->indented(1);
	o->callback((Fl_Callback *)select_scope);
	for (int i = 0; i < Num_Scopes; i++) {
		add_paper(Scopes_Tree, Scopes[i].name, Fl_Image::read_xpm(0, scope_icon));
	}

	w->view()->end();

	s->end();

	w->titlebar()->h(15);
	w->titlebar()->color(FL_BLACK);

	w->position(0,290);

	Fl::unlock();
}

Scopes_Manager *Scopes_Manager_Win;

class Parameters_Manager
{
	public:
		Parameters_Manager(int w, int h, Fl_MDI_Viewport *s, const char *name);
		void show();
		void hide();
		int is_visible();
		int is_batch_download();
		int batch_counter();
		int increment_batch_counter();
		int update_parameter(int, int, double);
	private:
		Fl_MDI_Window *PWin;
		Fl_Browser *Parameters_Tree;

		Fl_Tabs **Parameters_Tabs;

		int b_counter;
		Fl_Check_Button *Batch_Download;
		Fl_Button *Download;
		Fl_Button *Save;
		Fl_Button *Help, *Close;

		inline void select_block_i(Fl_Browser *, void *);
		static void select_block(Fl_Browser *, void *);
		inline void batch_download_i(Fl_Check_Button *, void *);
		static void batch_download(Fl_Check_Button *, void *);
		inline void close_i(Fl_Button *, void *);
		static void close(Fl_Button *, void *);
};

Parameters_Manager *Parameters_Manager_Win;

void Parameters_Manager::show()
{
        PWin->show();
}

void Parameters_Manager::hide()
{
        PWin->hide();
}

int Parameters_Manager::is_visible()
{
        return PWin->visible();
}

int Parameters_Manager::is_batch_download()
{
	return (int)(Batch_Download->value());
}

int Parameters_Manager::batch_counter()
{
	return b_counter;
}

int Parameters_Manager::increment_batch_counter()
{
	return b_counter++;
}

int Parameters_Manager::update_parameter(int idx, int mat_idx, double val)
{
	Tunable_Parameters[idx].data_value[mat_idx] = val;
	return 1;
}

inline void Parameters_Manager::select_block_i(Fl_Browser *b, void *v)
{
	int n = b->value();
	for (int i = 0; i < Num_Tunable_Blocks; i++) {
		Parameters_Tabs[i]->hide();
	}
	Parameters_Tabs[n]->show();
}

void Parameters_Manager::select_block(Fl_Browser *b, void *v)
{
	((Parameters_Manager *)(b->parent()->parent()->user_data()))->select_block_i(b,v);
}

inline void Parameters_Manager::batch_download_i(Fl_Check_Button *b, void *v)
{
	if (b->value()) {
		Download->activate();
	} else {
		Download->deactivate();
	}
	b_counter = 0;
}

void Parameters_Manager::batch_download(Fl_Check_Button *b, void *v)
{
	((Parameters_Manager *)(b->parent()->parent()->user_data()))->batch_download_i(b,v);
}

inline void Parameters_Manager::close_i(Fl_Button *b, void *v)
{
	PWin->hide();
	Open_Parameters_Manager->clear();
	MainMenuTable[9].clear();
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
}

void Parameters_Manager::close(Fl_Button *b, void *v)
{
	((Parameters_Manager *)(b->parent()->parent()->user_data()))->close_i(b,v);
}

void batch_update_parameters_cb(Fl_Widget *o, void *v)
{
	int i, n;

	for (i = n = 0; i < Parameters_Manager_Win->batch_counter(); i++) {
		n += Parameters_Manager_Win->update_parameter(Batch_Parameters[i].index, Batch_Parameters[i].mat_index, Batch_Parameters[i].value);
	}
	if (Parameters_Manager_Win->batch_counter() > 0 && n == Parameters_Manager_Win->batch_counter()) {
		RT_RPC(TargetInterfaceTask, BATCH_DOWNLOAD, 0);
	}
}

void update_parameters_cb(Fl_Widget *o, void *v)
{
	p_idx_T *idx = (p_idx_T *)v;
	int blk = idx->block_idx;
	int prm = idx->param_idx;
	int ind = idx->val_idx;
	int map_offset = Tunable_Blocks[blk].offset + prm;
	double value = atof(((Fl_Float_Input*)o)->value());

	if (Parameters_Manager_Win->is_batch_download() && Parameters_Manager_Win->batch_counter() < MAX_BATCH_PARAMS) {
		Batch_Parameters[Parameters_Manager_Win->batch_counter()].index = map_offset;
		Batch_Parameters[Parameters_Manager_Win->batch_counter()].value = value;
		Batch_Parameters[Parameters_Manager_Win->increment_batch_counter()].mat_index = ind;
	} else {
		if (Parameters_Manager_Win->update_parameter(map_offset, ind, value)) {
			RT_RPC(TargetInterfaceTask, (ind << 20) | (map_offset << 4) | UPDATE_PARAM, 0);
//			RT_RPC(TargetInterfaceTask, (map_offset << 4) | UPDATE_PARAM, 0);
//			RT_RPC(TargetInterfaceTask, (map_offset << 16) | UPDATE_PARAM, 0);
		}
	}
}

double get_parameter(Target_Parameters_T p, int nr, int nc, int *val_idx)
{
	switch (p.data_class) {
		case rt_SCALAR:
			*val_idx = 0;
			return (p.data_value[0]);
		case rt_VECTOR:
			*val_idx = nr;
			return (p.data_value[nr]);
		case rt_MATRIX_ROW_MAJOR:
			*val_idx = nr*p.n_cols+nc;
			return (p.data_value[nr*p.n_cols+nc]);
		case rt_MATRIX_COL_MAJOR:
			*val_idx = nc*p.n_rows+nr;
			return (p.data_value[nc*p.n_rows+nr]);
	}
}

Parameters_Manager::Parameters_Manager(int width, int height, Fl_MDI_Viewport *s, const char *name)
{
	Fl::lock();

	s->begin();
	Fl_MDI_Window *w = PWin = new Fl_MDI_Window(0, 0, width, height, name);
	w->user_data((void *)this);
	w->resizable(w->view());

	w->titlebar()->close_button()->hide();

	w->view()->begin();

	Parameters_Tabs = new Fl_Tabs*[Num_Tunable_Blocks];

	for (int i = 0; i < Num_Tunable_Blocks; i++) {
		{ Fl_Tabs *o = Parameters_Tabs[i] = new Fl_Tabs(160, 5, width-165, height-70);
		  o->new_page("Block Parameters");
//		  o->new_page("Block Parameters", false);
		  int tot_rows = 0;
		  if (i == Num_Tunable_Blocks - 1) {
		  	for (int j = 0; j < Num_Tunable_Parameters - Tunable_Blocks[i].offset; j++) {
				char scalar_val[20];
				char param_label[MAX_NAMES_SIZE + 10];
				int val_idx;
				unsigned int ncols = Tunable_Parameters[Tunable_Blocks[i].offset+j].n_cols;
				unsigned int nrows = Tunable_Parameters[Tunable_Blocks[i].offset+j].n_rows;
				sprintf(param_label, "%s", Tunable_Parameters[Tunable_Blocks[i].offset+j].param_name);
				for (int nr = 0; nr < nrows; nr++) {
					for (int nc = 0; nc < ncols; nc++) {
		  				{ Fl_Float_Input *o = new Fl_Float_Input(10 + nc*110, 30 + (j+nr+tot_rows)*40, 100, 20, strdup(param_label));
						  param_label[0] = '\0';
			    	  		  o->align(FL_ALIGN_LEFT|FL_ALIGN_TOP);
				  		  o->when(FL_WHEN_ENTER_KEY);
						  sprintf(scalar_val, "%G", get_parameter(Tunable_Parameters[Tunable_Blocks[i].offset+j], nr, nc, &val_idx));
			    	  		  o->value(strdup(scalar_val));
						  p_idx_T *idx = new p_idx_T;
						  idx->block_idx = i;
						  idx->param_idx = j;
						  idx->val_idx = val_idx;
				  		  o->callback((Fl_Callback *)update_parameters_cb, (void *)idx);
						}
					}
				}
				tot_rows = tot_rows + nrows - 1;
			}
		  } else {
		  	for (int j = 0; j < Tunable_Blocks[i+1].offset-Tunable_Blocks[i].offset; j++) {
				char scalar_val[20];
				char param_label[MAX_NAMES_SIZE + 10];
				int val_idx;
				unsigned int ncols = Tunable_Parameters[Tunable_Blocks[i].offset+j].n_cols;
				unsigned int nrows = Tunable_Parameters[Tunable_Blocks[i].offset+j].n_rows;
				sprintf(param_label, "%s", Tunable_Parameters[Tunable_Blocks[i].offset+j].param_name);
				for (int nr = 0; nr < nrows; nr++) {
					for (int nc = 0; nc < ncols; nc++) {
		  				{ Fl_Float_Input *o = new Fl_Float_Input(10 + nc*110, 30 + (j+nr+tot_rows)*40, 100, 20, strdup(param_label));
						  param_label[0] = '\0';
			    	  		  o->align(FL_ALIGN_LEFT|FL_ALIGN_TOP);
				  		  o->when(FL_WHEN_ENTER_KEY);
						  sprintf(scalar_val, "%G", get_parameter(Tunable_Parameters[Tunable_Blocks[i].offset+j], nr, nc, &val_idx));
			    	  		  o->value(strdup(scalar_val));
						  p_idx_T *idx = new p_idx_T;
						  idx->block_idx = i;
						  idx->param_idx = j;
						  idx->val_idx = val_idx;
				  		  o->callback((Fl_Callback *)update_parameters_cb, (void *)idx);
						}
					}
				}
				tot_rows = tot_rows + nrows - 1;
			}
		  }
		  o->end();
		  Fl_Group::current()->resizable(w);
		}
	}
	for (int i = 1; i < Num_Tunable_Blocks; i++) {
		Parameters_Tabs[i]->hide();
	}
	Parameters_Tabs[0]->show();

	Batch_Download = new Fl_Check_Button(width-270, height-60, 120, 25, "Batch Download");
	Batch_Download->callback((Fl_Callback *)batch_download);
	Download = new Fl_Button(width-150, height-60, 70, 25, "Download");
	Download->callback((Fl_Callback *)batch_update_parameters_cb);
	Download->deactivate();
	Save = new Fl_Button(width-75, height-60, 70, 25, "Save");
	Save->deactivate();
	Help = new Fl_Button(width-150, height-30, 70, 25, "Help");
	Close = new Fl_Button(width-75, height-30, 70, 25, "Close");
	Close->callback((Fl_Callback *)close);

	Fl_Browser *o = Parameters_Tree = new Fl_Browser(5, 5, 150, height-10);
	o->indented(1);
	o->callback((Fl_Callback *)select_block);
	for (int i = 0; i < Num_Tunable_Blocks; i++) {
		add_paper(Parameters_Tree, Tunable_Blocks[i].name, Fl_Image::read_xpm(0, folder_small));
	}

	w->view()->end();

	s->end();

	w->titlebar()->h(15);
	w->titlebar()->color(FL_BLACK);
	w->position(0,0);

	Fl::unlock();
}

class Logs_Manager
{
	public:
		Logs_Manager(int w, int h, Fl_MDI_Viewport *s, const char *name);
		void show();
		void hide();
		int is_visible();
		int n_points_to_save(int);
		int start_saving(int);
		void stop_saving(int);
		FILE *save_file(int);
	private:
		Fl_MDI_Window *LWin;
		Fl_Browser *Logs_Tree;

		Fl_Tabs **Logs_Tabs;
		Fl_Check_Button **Save_Type;
		Fl_Int_Input **Save_Points;
		Fl_Float_Input **Save_Time;
		Fl_Input **Save_File;
		Fl_Light_Button **Save;
		int *Save_Flag;
		FILE **Save_File_Pointer;

		Fl_Button *Help, *Close;

		inline void select_log_i(Fl_Browser *, void *);
		static void select_log(Fl_Browser *, void *);
		inline void select_save_i(Fl_Check_Button *, void *);
		static void select_save(Fl_Check_Button *, void *);
		inline void enable_saving_i(Fl_Light_Button *, void *);
		static void enable_saving(Fl_Light_Button *, void *);
		inline void close_i(Fl_Button *, void *);
		static void close(Fl_Button *, void *);
};

void Logs_Manager::show()
{
        LWin->show();
}

void Logs_Manager::hide()
{
        LWin->hide();
}

int Logs_Manager::is_visible()
{
        return LWin->visible();
}

int Logs_Manager::start_saving(int n)
{
	return Save_Flag[n];
}

int Logs_Manager::n_points_to_save(int n)
{
	int n_points;

	if (Save_Type[n]->value()) {
		n_points = (int)atoi(Save_Points[n]->value());
		if (n_points < 0) return 0;
	} else {
		n_points = (int)(atof(Save_Time[n]->value())/Logs[n].dt);
		if (n_points < 0) return 0;
	}
	return n_points;
}

void Logs_Manager::stop_saving(int n)
{
	fclose(Save_File_Pointer[n]);
	Save_Type[n]->activate();
	if (Save_Type[n]->value()) {
		Save_Points[n]->activate();
	} else {
		Save_Time[n]->activate();
	}
	Save_File[n]->activate();
	Save[n]->activate();
	Save[n]->value(0);
	Save[n]->label("Save");
	Logs_Tabs[n]->redraw();
	Save_Flag[n] = false;
}

FILE *Logs_Manager::save_file(int n)
{
	return Save_File_Pointer[n];
}

inline void Logs_Manager::select_log_i(Fl_Browser *b, void *v)
{
	int n = b->value();
	for (int i = 0; i < Num_Logs; i++) {
		Logs_Tabs[i]->hide();
	}
	Logs_Tabs[n]->show();
}

void Logs_Manager::select_log(Fl_Browser *b, void *v)
{
	((Logs_Manager *)(b->parent()->parent()->user_data()))->select_log_i(b,v);
}

inline void Logs_Manager::select_save_i(Fl_Check_Button *b, void *v)
{
	int n = (int)v;
	if (b->value()) {
		Save_Points[n]->activate();
		Save_Time[n]->deactivate();
	} else {
		Save_Points[n]->deactivate();
		Save_Time[n]->activate();
	}
	Logs_Tabs[n]->redraw();
}

void Logs_Manager::select_save(Fl_Check_Button *b, void *v)
{
	((Logs_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->select_save_i(b,v);
}

inline void Logs_Manager::enable_saving_i(Fl_Light_Button *b, void *v)
{
	int n = (int)v;
	if (b->value()) {
		if ((Save_File_Pointer[n] = fopen(Save_File[n]->value(), "a+")) == NULL) {
			fl_alert("Error in opening file %s", Save_File[n]->value());
			return;
		}
		b->deactivate();
		Save_Type[n]->deactivate();
		Save_Time[n]->deactivate();
		Save_Points[n]->deactivate();
		Save_File[n]->deactivate();
		Save[n]->label("Saving...");
		Logs_Tabs[n]->redraw();
		Save_Flag[n] = true;
	}
}

void Logs_Manager::enable_saving(Fl_Light_Button *b, void *v)
{
	((Logs_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->enable_saving_i(b,v);
}

inline void Logs_Manager::close_i(Fl_Button *b, void *v)
{
	LWin->hide();
	Open_Logs_Manager->clear();
	MainMenuTable[11].clear();
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
}

void Logs_Manager::close(Fl_Button *b, void *v)
{
	((Logs_Manager *)(b->parent()->parent()->user_data()))->close_i(b,v);
}

Logs_Manager::Logs_Manager(int width, int height, Fl_MDI_Viewport *s, const char *name)
{
	Fl::lock();

	s->begin();
	Fl_MDI_Window *w = LWin = new Fl_MDI_Window(0, 0, width, height, name);
	w->user_data((void *)this);
	w->resizable(w->view());

	w->titlebar()->close_button()->hide();

	w->view()->begin();

	Logs_Tabs = new Fl_Tabs*[Num_Logs];
	Save_Type = new Fl_Check_Button*[Num_Logs];
	Save_Points = new Fl_Int_Input*[Num_Logs];
	Save_Time = new Fl_Float_Input*[Num_Logs];
	Save_File = new Fl_Input*[Num_Logs];
	Save = new Fl_Light_Button*[Num_Logs];
	Save_Flag = new int[Num_Logs];
	Save_File_Pointer = new FILE*[Num_Logs];

	for (int i = 0; i < Num_Logs; i++) {
		Save_Flag[i] = false;
		{ Fl_Tabs *o = Logs_Tabs[i] = new Fl_Tabs(160, 5, width-165, height-40);
		  o->new_page("Saving");
//		  o->new_page("Saving", false);
		  { Fl_Check_Button *o = Save_Type[i] = new Fl_Check_Button(10, 25, 100, 20, "Points/Time");
		    o->value(1);
		    o->callback((Fl_Callback *)select_save, (void *)i);
		  }
		  { Fl_Int_Input *o = Save_Points[i] = new Fl_Int_Input(70, 50, 60, 20, "N Points: ");
		    o->align(FL_ALIGN_LEFT);
		    o->value("1000");
		  }
		  { Fl_Float_Input *o = Save_Time[i] = new Fl_Float_Input(70, 75, 60, 20, "Time [s]:  ");
		    o->align(FL_ALIGN_LEFT);
		    o->value("1.0");
		    o->deactivate();
		  }
		  { Fl_Input *o = Save_File[i] = new Fl_Input(70, 100, 120, 20, "Filename:");
		    char buf[100];
		    o->align(FL_ALIGN_LEFT);
		    sprintf(buf, "%s", Logs[i].name);
		    o->value(buf);
		  }
		  { Fl_Light_Button *o = Save[i] = new Fl_Light_Button(10, 130, 90, 25, "Save");
		    o->selection_color(FL_BLACK);
		    o->callback((Fl_Callback *)enable_saving, (void *)i);
		  }
		  o->end();
		  Fl_Group::current()->resizable(w);
		}
	}
	for (int i = 1; i < Num_Logs; i++) {
		Logs_Tabs[i]->hide();
	}
	Logs_Tabs[0]->show();

	Help = new Fl_Button(width-150, height-30, 70, 25, "Help");
	Close = new Fl_Button(width-75, height-30, 70, 25, "Close");
	Close->callback((Fl_Callback *)close);

	Fl_Browser *o = Logs_Tree = new Fl_Browser(5, 5, 150, height-10);
	o->indented(1);
	o->callback((Fl_Callback *)select_log);
	for (int i = 0; i < Num_Logs; i++) {
		add_paper(Logs_Tree, Logs[i].name, Fl_Image::read_xpm(0, folder_small));
	}

	w->view()->end();

	s->end();

	w->titlebar()->h(15);
	w->titlebar()->color(FL_BLACK);
	w->position(440,0);

	Fl::unlock();
}

Logs_Manager *Logs_Manager_Win;

class Leds_Manager
{
	public:
		Leds_Manager(int w, int h, Fl_MDI_Viewport *s, const char *name);
		void show();
		void hide();
		int is_visible();
		Fl_Led_Window **Led_Windows;
	private:
		Fl_MDI_Window *LWin;
		Fl_Browser *Leds_Tree;

		Fl_Tabs **Leds_Tabs;
		Fl_Check_Button **Led_Show;

		Fl_Radio_Buttons **Led_Colors;

		Fl_Value_Input **Led_Layout;

		Fl_Button *Help, *Close;

		inline void select_led_i(Fl_Browser *, void *);
		static void select_led(Fl_Browser *, void *);
		inline void show_led_i(Fl_Check_Button *, void *);
		static void show_led(Fl_Check_Button *, void *);
		inline void select_led_color_i(Fl_Button_Group *, void *);
		static void select_led_color(Fl_Button_Group *, void *);
		inline void led_layout_i(Fl_Value_Input *, void *);
		static void led_layout(Fl_Value_Input *, void *);
		inline void close_i(Fl_Button *, void *);
		static void close(Fl_Button *, void *);
};

void Leds_Manager::show()
{
        LWin->show();
}

void Leds_Manager::hide()
{
        LWin->hide();
}

int Leds_Manager::is_visible()
{
        return LWin->visible();
}

inline void Leds_Manager::select_led_i(Fl_Browser *b, void *v)
{
	int n = b->value();
	for (int i = 0; i < Num_Leds; i++) {
		Leds_Tabs[i]->hide();
	}
	Leds_Tabs[n]->show();
}

void Leds_Manager::select_led(Fl_Browser *b, void *v)
{
	((Leds_Manager *)(b->parent()->parent()->user_data()))->select_led_i(b,v);
}

inline void Leds_Manager::show_led_i(Fl_Check_Button *b, void *v)
{
	int n = (int)v;
	if (b->value()) {
		Leds[n].visible = true;
	} else {
		Leds[n].visible = false;
	}
}

void Leds_Manager::show_led(Fl_Check_Button *b, void *v)
{
	((Leds_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->show_led_i(b,v);
}

inline void Leds_Manager::select_led_color_i(Fl_Button_Group *bg, void *v)
{
	int n = (int)v;
	Fl_String s = bg->value();

	Led_Windows[n]->led_color(s);
}

void Leds_Manager::select_led_color(Fl_Button_Group *bg, void *v)
{
	((Leds_Manager *)(bg->parent()->parent()->parent()->parent()->user_data()))->select_led_color_i(bg,v);
}

inline void Leds_Manager::led_layout_i(Fl_Value_Input *b, void *v)
{
	int n = (int)v;
	int value = (int)(b->value());

	Led_Windows[n]->layout(value);
}

void Leds_Manager::led_layout(Fl_Value_Input *b, void *v)
{
	((Leds_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->led_layout_i(b,v);
}

inline void Leds_Manager::close_i(Fl_Button *b, void *v)
{
	LWin->hide();
	Open_Leds_Manager->clear();
	MainMenuTable[12].clear();
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
}

void Leds_Manager::close(Fl_Button *b, void *v)
{
	((Leds_Manager *)(b->parent()->parent()->user_data()))->close_i(b,v);
}

Leds_Manager::Leds_Manager(int width, int height, Fl_MDI_Viewport *s, const char *name)
{
	Fl::lock();

	s->begin();
	Fl_MDI_Window *w = LWin = new Fl_MDI_Window(0, 0, width, height, name);
	w->user_data((void *)this);
	w->resizable(w->view());

	w->titlebar()->close_button()->hide();

	w->view()->begin();

	Leds_Tabs = new Fl_Tabs*[Num_Leds];
	Led_Show = new Fl_Check_Button*[Num_Leds];
	Led_Colors = new Fl_Radio_Buttons*[Num_Leds];
	Led_Layout = new Fl_Value_Input *[Num_Leds];
	Led_Windows = new Fl_Led_Window*[Num_Leds];

	for (int i = 0; i < Num_Leds; i++) {
		{ Fl_Tabs *o = Leds_Tabs[i] = new Fl_Tabs(160, 5, width-165, height-40);
		  o->new_page("Leds");
//		  o->new_page("Leds", false);
		  { Fl_Check_Button *o = Led_Show[i] = new Fl_Check_Button(10, 25, 100, 20, "Show/Hide");
		    o->callback((Fl_Callback *)show_led, (void *)i);
		  }
		  { Fl_Radio_Buttons *o = Led_Colors[i] = new Fl_Radio_Buttons(10, 80, 90, 75, "Led Colors");
		    Fl_String_List colors("green,red,yellow", ",");
		    o->buttons(colors);
		    o->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
		    o->value("green");
		    o->callback((Fl_Callback *)select_led_color, (void *)i);
		  }
		  o->end();
		  Fl_Group::current()->resizable(w);
		}
	}
	for (int i = 1; i < Num_Leds; i++) {
		Leds_Tabs[i]->hide();
	}
	Leds_Tabs[0]->show();

	Help = new Fl_Button(width-150, height-30, 70, 25, "Help");
	Close = new Fl_Button(width-75, height-30, 70, 25, "Close");
	Close->callback((Fl_Callback *)close);

	Fl_Browser *o = Leds_Tree = new Fl_Browser(5, 5, 150, height-10);
	o->indented(1);
	o->callback((Fl_Callback *)select_led);
	for (int i = 0; i < Num_Leds; i++) {
		add_paper(Leds_Tree, Leds[i].name, Fl_Image::read_xpm(0, led_icon));
	}

	w->view()->end();

	s->end();

	w->titlebar()->h(15);
	w->titlebar()->color(FL_BLACK);
	w->position(500,290);

	Fl::unlock();
}

Leds_Manager *Leds_Manager_Win;

class Meters_Manager
{
	public:
		Meters_Manager(int w, int h, Fl_MDI_Viewport *s, const char *name);
		void show();
		void hide();
		int is_visible();
		Fl_Meter_Window **Meter_Windows;

	private:
		Fl_MDI_Window *MWin;
		Fl_Browser *Meters_Tree;
		Fl_Tabs **Meters_Tabs;
		Fl_Check_Button **Meter_Show;
		Fl_Float_Input **Min_Val;
		Fl_Float_Input **Max_Val;
		Fl_Button **Bg_Color;
		Fl_Button **Arrow_Color;
		Fl_Button **Grid_Color;
		Fl_Button *Help, *Close;
		inline void select_meter_i(Fl_Browser *, void *);
		static void select_meter(Fl_Browser *, void *);
		inline void show_meter_i(Fl_Check_Button *, void *);
		static void show_meter(Fl_Check_Button *, void *);
		inline void enter_minval_i(Fl_Float_Input *, void *);
		static void enter_minval(Fl_Float_Input *, void *);
		inline void enter_maxval_i(Fl_Float_Input *, void *);
		static void enter_maxval(Fl_Float_Input *, void *);
		inline void select_bg_color_i(Fl_Button *, void *);
		static void select_bg_color(Fl_Button *, void *);
		inline void select_grid_color_i(Fl_Button *, void *);
		static void select_grid_color(Fl_Button *, void *);
		inline void select_arrow_color_i(Fl_Button *, void *);
		static void select_arrow_color(Fl_Button *, void *);
		inline void close_i(Fl_Button *, void *);
		static void close(Fl_Button *, void *);
};

void Meters_Manager::show()
{
        MWin->show();
}

void Meters_Manager::hide()
{
        MWin->hide();
}

int Meters_Manager::is_visible()
{
        return MWin->visible();
}

inline void Meters_Manager::select_meter_i(Fl_Browser *b, void *v)
{
	int n = b->value();
	for (int i = 0; i < Num_Meters; i++) {
		Meters_Tabs[i]->hide();
	}
	Meters_Tabs[n]->show();
}

void Meters_Manager::select_meter(Fl_Browser *b, void *v)
{
	((Meters_Manager *)(b->parent()->parent()->user_data()))->select_meter_i(b,v);
}

inline void Meters_Manager::show_meter_i(Fl_Check_Button *b, void *v)
{
	int n = (int)v;
	if (b->value()) {
		Meters[n].visible = true;
	} else {
		Meters[n].visible = false;
	}
}

void Meters_Manager::show_meter(Fl_Check_Button *b, void *v)
{
	((Meters_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->show_meter_i(b,v);
}

inline void Meters_Manager::enter_minval_i(Fl_Float_Input *b, void *v)
{
	int n = (int)v;
	float val = (float)atof(b->value());
	Meter_Windows[n]->Meter->minimum_value(val);
}

void Meters_Manager::enter_minval(Fl_Float_Input *b, void *v)
{
	((Meters_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->enter_minval_i(b,v);
}

inline void Meters_Manager::enter_maxval_i(Fl_Float_Input *b, void *v)
{
	int n = (int)v;
	float val = (float)atof(b->value());
	Meter_Windows[n]->Meter->maximum_value(val);
}

void Meters_Manager::enter_maxval(Fl_Float_Input *b, void *v)
{
	((Meters_Manager *)(b->parent()->parent()->parent()->parent()->user_data()))->enter_maxval_i(b,v);
}

inline void Meters_Manager::select_bg_color_i(Fl_Button *bb, void *v)
{
	int n = (int)v;
	uchar r,g,b;
	Fl_Color c;

	c = Meter_Windows[n]->Meter->bg_color();
	fl_get_color(c,r,g,b);
	if (!fl_color_chooser("New color:",r,g,b)) return;
	c = FL_FREE_COLOR;
	Meter_Windows[n]->Meter->bg_free_color();
	fl_set_color(FL_FREE_COLOR, fl_rgb(r,g,b));
	Meter_Windows[n]->Meter->bg_color(r/255.,g/255.,b/255.);
}

void Meters_Manager::select_bg_color(Fl_Button *bb, void *v)
{
	((Meters_Manager *)(bb->parent()->parent()->parent()->parent()->user_data()))->select_bg_color_i(bb,v);
}

inline void Meters_Manager::select_grid_color_i(Fl_Button *bb, void *v)
{
	int n = (int)v;
	uchar r,g,b;
	Fl_Color c;

	c = Meter_Windows[n]->Meter->grid_color();
	fl_get_color(c,r,g,b);
	if (!fl_color_chooser("New color:",r,g,b)) return;
	c = FL_FREE_COLOR;
	Meter_Windows[n]->Meter->grid_free_color();
	fl_set_color(FL_FREE_COLOR, fl_rgb(r,g,b));
	Meter_Windows[n]->Meter->grid_color(r/255.,g/255.,b/255.);
}

void Meters_Manager::select_grid_color(Fl_Button *bb, void *v)
{
	((Meters_Manager *)(bb->parent()->parent()->parent()->parent()->user_data()))->select_grid_color_i(bb,v);
}

inline void Meters_Manager::select_arrow_color_i(Fl_Button *bb, void *v)
{
	int n = (int)v;
	uchar r,g,b;
	Fl_Color c;

	c = Meter_Windows[n]->Meter->arrow_color();
	fl_get_color(c,r,g,b);
	if (!fl_color_chooser("New color:",r,g,b)) return;
	c = FL_FREE_COLOR;
	Meter_Windows[n]->Meter->arrow_free_color();
	fl_set_color(FL_FREE_COLOR, fl_rgb(r,g,b));
	Meter_Windows[n]->Meter->arrow_color(r/255.,g/255.,b/255.);
}

void Meters_Manager::select_arrow_color(Fl_Button *bb, void *v)
{
	((Meters_Manager *)(bb->parent()->parent()->parent()->parent()->user_data()))->select_arrow_color_i(bb,v);
}

inline void Meters_Manager::close_i(Fl_Button *b, void *v)
{
	MWin->hide();
	Open_Meters_Manager->clear();
	MainMenuTable[13].clear();
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
}

void Meters_Manager::close(Fl_Button *b, void *v)
{
	((Meters_Manager *)(b->parent()->parent()->user_data()))->close_i(b,v);
}

Meters_Manager::Meters_Manager(int width, int height, Fl_MDI_Viewport *s, const char *name)
{
	Fl::lock();

	s->begin();
	Fl_MDI_Window *w = MWin = new Fl_MDI_Window(0, 0, width, height, name);
	w->user_data((void *)this);
	w->resizable(w->view());

	w->titlebar()->close_button()->hide();

	w->view()->begin();

	Meters_Tabs = new Fl_Tabs*[Num_Meters];
	Meter_Show = new Fl_Check_Button*[Num_Meters];
	Min_Val = new Fl_Float_Input*[Num_Meters];
	Max_Val = new Fl_Float_Input*[Num_Meters];
	Bg_Color = new Fl_Button*[Num_Meters];
	Arrow_Color = new Fl_Button*[Num_Meters];
	Grid_Color = new Fl_Button*[Num_Meters];
	Meter_Windows = new Fl_Meter_Window*[Num_Meters];

	for (int i = 0; i < Num_Meters; i++) {
		{ Fl_Tabs *o = Meters_Tabs[i] = new Fl_Tabs(160, 5, width-165, height-40);
		  o->new_page("Meter");
//		  o->new_page("Meter", false);
		  { Fl_Check_Button *o = Meter_Show[i] = new Fl_Check_Button(10, 10, 100, 20, "Show/Hide");
		    o->value(0);
		    o->callback((Fl_Callback *)show_meter, (void *)i);
		  }
		  { Fl_Float_Input *o = Min_Val[i] = new Fl_Float_Input(77, 35, 60, 20, "Minimum:   ");
		    o->align(FL_ALIGN_LEFT);
		    o->value("-1.2");
		    o->when(FL_WHEN_ENTER_KEY);
		    o->callback((Fl_Callback *)enter_minval, (void *)i);
		  }
		  { Fl_Float_Input *o = Max_Val[i] = new Fl_Float_Input(77, 60, 60, 20, "Maximum:  ");
		    o->align(FL_ALIGN_LEFT);
		    o->value("1.2");
		    o->when(FL_WHEN_ENTER_KEY);
		    o->callback((Fl_Callback *)enter_maxval, (void *)i);
		  }
		  { Fl_Button *o = Bg_Color[i] = new Fl_Button(10, 90, 90, 25, "Bg Color");
		    o->callback((Fl_Callback *)select_bg_color, (void *)i);
		  }
		  { Fl_Button *o = Arrow_Color[i] = new Fl_Button(10, 120, 90, 25, "Arrow Color");
		    o->callback((Fl_Callback *)select_arrow_color, (void *)i);
		  }
		  { Fl_Button *o = Grid_Color[i] = new Fl_Button(10, 150, 90, 25, "Grid Color");
		    o->callback((Fl_Callback *)select_grid_color, (void *)i);
		  }
		  o->end();
		  Fl_Group::current()->resizable(w);
		}
	}
	for (int i = 1; i < Num_Meters; i++) {
		Meters_Tabs[i]->hide();
	}
	Meters_Tabs[0]->show();

	Help = new Fl_Button(width-150, height-30, 70, 25, "Help");
	Close = new Fl_Button(width-75, height-30, 70, 25, "Close");
	Close->callback((Fl_Callback *)close);

	Fl_Browser *o = Meters_Tree = new Fl_Browser(5, 5, 150, height-10);
	o->indented(1);
	o->callback((Fl_Callback *)select_meter);
	for (int i = 0; i < Num_Meters; i++) {
		add_paper(Meters_Tree, Meters[i].name, Fl_Image::read_xpm(0, meter_icon));
	}

	w->view()->end();

	s->end();

	w->titlebar()->h(15);
	w->titlebar()->color(FL_BLACK);
	w->position(530,320);

	Fl::unlock();
}

Meters_Manager *Meters_Manager_Win;

Fl_Synchronoscope_Window *Synchronoscope_Win;

unsigned long get_an_id(const char *root)
{
	int i;
	char name[7];
	for (i = 0; i < MAX_NHOSTS; i++) {
		sprintf(name, "%s%d", root, i);
		if (!rt_get_adr(nam2num(name))) {
			return nam2num(name);
		}
	}
	return 0;
}

void read_target_settings(void)
{
	Fl_Config Session("RTAI-Lab", "RTAI-Lab", Fl_Config::USER);

	Session.set_section("Target");
	Session.read("IPaddress", TargetIP, "127.0.0.1");
	Target_IP_Address->value(TargetIP);
	Session.read("TaskID", TargetInterfaceTaskName, "IFTASK");
	Target_Task_ID->value(TargetInterfaceTaskName);
	Session.read("ScopeID", TargetScopeMbxID, "RTS");
	Target_Scope_ID->value(TargetScopeMbxID);
	Session.read("LogID", TargetLogMbxID, "RTL");
	Target_Log_ID->value(TargetLogMbxID);
	Session.read("LedID", TargetLedMbxID, "RTE");
	Target_Led_ID->value(TargetLedMbxID);
	Session.read("MeterID", TargetMeterMbxID, "RTM");
	Target_Meter_ID->value(TargetMeterMbxID);
	Session.read("SynchID", TargetSynchronoscopeMbxID, "RTY");
	Target_Synchronoscope_ID->value(TargetSynchronoscopeMbxID);
}

void write_target_settings(void)
{
	Fl_Config Session("RTAI-Lab", "RTAI-Lab", Fl_Config::USER);

	Session.set_section("Target");
	Session.write("IPaddress", Target_IP_Address->value());
	Session.write("TaskID", Target_Task_ID->value());
	Session.write("ScopeID", Target_Scope_ID->value());
	Session.write("LogID", Target_Log_ID->value());
	Session.write("LedID", Target_Led_ID->value());
	Session.write("MeterID", Target_Meter_ID->value());
	Session.write("SynchID", Target_Synchronoscope_ID->value());
}

void quit_cb(Fl_Widget*, void*)
{
	EndApp = 1;
	for (int n = 0; n < Num_Scopes; n++) {
		pthread_join(GetScopeDataThread[n], NULL);
	}
	for (int n = 0; n < Num_Logs; n++) {
		pthread_join(GetLogDataThread[n], NULL);
	}
	for (int n = 0; n < Num_Leds; n++) {
		pthread_join(GetLedDataThread[n], NULL);
	}
	for (int n = 0; n < Num_Meters; n++) {
		pthread_join(GetMeterDataThread[n], NULL);
	}
	if (Num_Synchronoscope > 0) {
		pthread_join(GetSynchronoscopeDataThread, NULL);
	}
	if (Parameters_Manager_Win) Parameters_Manager_Win->hide();
	if (Scopes_Manager_Win) Scopes_Manager_Win->hide();
	if (Logs_Manager_Win) Logs_Manager_Win->hide();
	if (Leds_Manager_Win) Leds_Manager_Win->hide();
	if (Meters_Manager_Win) Meters_Manager_Win->hide();
	if (Synchronoscope_Win) Synchronoscope_Win->hide();
	rt_send(TargetInterfaceTask, CLOSE);
	pthread_join(TargetInterfaceThread, NULL);
	EndAll = 1;
	return;
}

void start_stop_cb(Fl_Widget *, void *)
{
	if (Is_Target_Running) {
		if (!fl_ask("Are you sure you want to stop the real time control?")) {
			return;
		}
		RT_RPC(TargetInterfaceTask, STOP_TARGET, 0);
	} else {
		RT_RPC(TargetInterfaceTask, START_TARGET, 0);
	}
}

void open_synchronoscope_cb(Fl_Widget *, void *)
{
	if (Synchronoscope_Win) {
		Synchronoscope_Win->show();
		Synchronoscope.visible = true;
	}
	MainMenuTable[14].set();
	MainMenuTable[14].deactivate();
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
	Open_Synchronoscope->deactivate();
}

void open_meters_manager_cb(Fl_Widget *, void *)
{
	if (MainMenuTable[13].checked()) {
		if (Meters_Manager_Win) Meters_Manager_Win->hide();
		MainMenuTable[13].clear();
		Open_Meters_Manager->clear();
	}
	else {
		if (Meters_Manager_Win) Meters_Manager_Win->show();
		MainMenuTable[13].set();
		Open_Meters_Manager->set();
	}
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
}

void open_leds_manager_cb(Fl_Widget *, void *)
{
	if (MainMenuTable[12].checked()) {
		if (Leds_Manager_Win) Leds_Manager_Win->hide();
		MainMenuTable[12].clear();
		Open_Leds_Manager->clear();
	}
	else {
		if (Leds_Manager_Win) Leds_Manager_Win->show();
		MainMenuTable[12].set();
		Open_Leds_Manager->set();
	}
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
}

void open_logs_manager_cb(Fl_Widget *, void *)
{
	if (MainMenuTable[11].checked()) {
		if (Logs_Manager_Win) Logs_Manager_Win->hide();
		MainMenuTable[11].clear();
		Open_Logs_Manager->clear();
	}
	else {
		if (Logs_Manager_Win) Logs_Manager_Win->show();
		MainMenuTable[11].set();
		Open_Logs_Manager->set();
	}
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
}

void open_scopes_manager_cb(Fl_Widget *, void *)
{
	if (MainMenuTable[10].checked()) {
		if (Scopes_Manager_Win) Scopes_Manager_Win->hide();
		MainMenuTable[10].clear();
		Open_Scopes_Manager->clear();
	}
	else {
		if (Scopes_Manager_Win) Scopes_Manager_Win->show();
		MainMenuTable[10].set();
		Open_Scopes_Manager->set();
	}
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
}

void open_parameters_manager_cb(Fl_Widget *, void *)
{
	if (MainMenuTable[9].checked()) {
		if (Parameters_Manager_Win) Parameters_Manager_Win->hide();
		MainMenuTable[9].clear();
		Open_Parameters_Manager->clear();
	}
	else {
		if (Parameters_Manager_Win) Parameters_Manager_Win->show();
		MainMenuTable[9].set();
		Open_Parameters_Manager->set();
	}
	MainMenu->menu(MainMenuTable);
	MainMenu->redraw();
	MainWin->redraw();
}

void edit_profiles_cb(Fl_Widget *, void *)
{
	Fl_Dialog& dialog = *Edit_Profiles_Dialog;
	int xpos = MainWin->x();
	int ypos = MainWin->y();
	dialog.x(xpos + (int)((MainWin->w()-dialog.w())/2));
	dialog.y(ypos + (int)((MainWin->h()-dialog.h())/2));
	switch (dialog.show_modal()) {
        	case FL_DLG_OK:
			break;
        	case FL_DLG_CANCEL:
			break;
	}
}

void add_profile_cb(Fl_Widget *, void *)
{
	Fl_Dialog& dialog = *Add_Profile_Dialog;
	int xpos = MainWin->x();
	int ypos = MainWin->y();
	dialog.x(xpos + (int)((MainWin->w()-dialog.w())/2));
	dialog.y(ypos + (int)((MainWin->h()-dialog.h())/2));
	switch (dialog.show_modal()) {
        	case FL_DLG_OK:
			Profile_Choice->add(Profile_Input->value());
	  		add_paper(Profiles_Folder, Profile_Input->value(), 0);
			MainWin->redraw();
			break;
        	case FL_DLG_CANCEL:
			break;
	}
}

void connect_cb(Fl_Widget *, void *)
{
	Fl_Dialog& dialog = *Connect_Dialog;
	int xpos = MainWin->x();
	int ypos = MainWin->y();
	dialog.x(xpos + (int)((MainWin->w()-dialog.w())/2));
	dialog.y(ypos + (int)((MainWin->h()-dialog.h())/2));
	read_target_settings();
	switch (dialog.show_modal()) {
        	case FL_DLG_OK:
			write_target_settings();
			RT_RPC(TargetInterfaceTask, CONNECT_TO_TARGET, 0);
			break;
        	case FL_DLG_CANCEL:
			break;
	}
}

void disconnect_cb(Fl_Widget *, void *)
{
	if (!fl_ask("Do you want to disconnect?")) {
		return;
	}
	RT_RPC(TargetInterfaceTask, DISCONNECT_FROM_TARGET, 0);
}

static void *rt_get_synchronoscope_data(void *arg)
{
	RT_TASK *GetSynchronoscopeDataTask;
	MBX *GetSynchronoscopeDataMbx;
	char GetSynchronoscopeDataMbxName[7];
	long GetSynchronoscopeDataPort;
	int MsgData = 0, MsgLen, MaxMsgLen, DataBytes;
	float MsgBuf[MAX_MSG_LEN/sizeof(float)];
	int n;

	rt_allow_nonroot_hrt();
	if (!(GetSynchronoscopeDataTask = rt_task_init_schmod(get_an_id("HGY"), 99, 0, 0, SCHED_RR, 0xFF))) {
		printf("Cannot init Host GetSynchronoscopeData Task\n");
		return (void *)1;
	}
	GetSynchronoscopeDataPort = rt_request_port(Target_Node);
	sprintf(GetSynchronoscopeDataMbxName, "%s", TargetSynchronoscopeMbxID);
	if (!(GetSynchronoscopeDataMbx = (MBX *)RT_get_adr(Target_Node, GetSynchronoscopeDataPort, GetSynchronoscopeDataMbxName))) {
		printf("Error in getting %s mailbox address\n", GetSynchronoscopeDataMbxName);
		exit(1);
	}
	DataBytes = sizeof(float);
	MaxMsgLen = (MAX_MSG_LEN/DataBytes)*DataBytes;
	MsgLen = (((int)(DataBytes*REFRESH_RATE*(1./Synchronoscope.dt)))/DataBytes)*DataBytes;
	if (MsgLen < DataBytes) MsgLen = DataBytes;
	if (MsgLen > MaxMsgLen) MsgLen = MaxMsgLen;
	MsgData = MsgLen/DataBytes;

	Synchronoscope_Win = new Fl_Synchronoscope_Window(0, 0, 260, 200, MainWorkSpace->viewport(), Synchronoscope.name);

	rt_send(TargetInterfaceTask, 0);
	mlockall(MCL_CURRENT | MCL_FUTURE);

	while (true) {
		if (EndApp || !Is_Target_Connected) break;
		while (RT_mbx_receive_if(Target_Node, GetSynchronoscopeDataPort, GetSynchronoscopeDataMbx, &MsgBuf, MsgLen)) {
			if (EndApp || !Is_Target_Connected) goto end;
			if (!Synchronoscope_Win->is_visible()) {
				Fl::lock();
				Open_Synchronoscope->activate();
				Synchronoscope.visible = false;
/*
				MainMenuTable[14].clear();
				MainMenuTable[14].activate();
				MainMenu->menu(MainMenuTable);
				MainMenu->redraw();
				MainWin->redraw();
*/
				Fl::unlock();
			}
			if (Synchronoscope.visible) {
				Fl::lock();
				Synchronoscope_Win->show();
				Fl::unlock();
			} else {
				Fl::lock();
				Synchronoscope_Win->hide();
				Fl::unlock();
			}
			msleep(10);
		}
		Fl::lock();
		for (n = 0; n < MsgData; n++) {
			Synchronoscope_Win->Synchronoscope->value(MsgBuf[n]);
			Synchronoscope_Win->Synchronoscope->redraw();
		}
		Fl::unlock();
	}
end:
#ifdef DBG
	printf("Deleting synchronoscope thread number\n");
#endif
	Synchronoscope_Win->hide();
	rt_release_port(Target_Node, GetSynchronoscopeDataPort);
	rt_task_delete(GetSynchronoscopeDataTask);

	return 0;
}

static void *rt_get_meter_data(void *arg)
{
	RT_TASK *GetMeterDataTask;
	MBX *GetMeterDataMbx;
	char GetMeterDataMbxName[7];
	long GetMeterDataPort;
	int MsgData = 0, MsgLen, MaxMsgLen, DataBytes;
	float MsgBuf[MAX_MSG_LEN/sizeof(float)];
	int n, index = *((int *)arg);

	rt_allow_nonroot_hrt();
	if (!(GetMeterDataTask = rt_task_init_schmod(get_an_id("HGM"), 99, 0, 0, SCHED_RR, 0xFF))) {
		printf("Cannot init Host GetMeterData Task\n");
		return (void *)1;
	}
	GetMeterDataPort = rt_request_port(Target_Node);
	sprintf(GetMeterDataMbxName, "%s%d", TargetMeterMbxID, index);
	if (!(GetMeterDataMbx = (MBX *)RT_get_adr(Target_Node, GetMeterDataPort, GetMeterDataMbxName))) {
		printf("Error in getting %s mailbox address\n", GetMeterDataMbxName);
		exit(1);
	}
	DataBytes = sizeof(float);
	MaxMsgLen = (MAX_MSG_LEN/DataBytes)*DataBytes;
	MsgLen = (((int)(DataBytes*REFRESH_RATE*(1./Meters[index].dt)))/DataBytes)*DataBytes;
	if (MsgLen < DataBytes) MsgLen = DataBytes;
	if (MsgLen > MaxMsgLen) MsgLen = MaxMsgLen;
	MsgData = MsgLen/DataBytes;

	Fl_Meter_Window *Meter_Win = new Fl_Meter_Window(0, 0, 300, 200, MainWorkSpace->viewport(), Meters[index].name);
	Meters_Manager_Win->Meter_Windows[index] = Meter_Win;

	rt_send(TargetInterfaceTask, 0);
	mlockall(MCL_CURRENT | MCL_FUTURE);

	while (true) {
		if (EndApp || !Is_Target_Connected) break;
		while (RT_mbx_receive_if(Target_Node, GetMeterDataPort, GetMeterDataMbx, &MsgBuf, MsgLen)) {
			if (EndApp || !Is_Target_Connected) goto end;
			if (!Meters_Manager_Win->is_visible()) {
				Fl::lock();
				Open_Meters_Manager->activate();
				Fl::unlock();
			}
			if (Meters[index].visible) {
				Fl::lock();
				Meter_Win->show();
				Fl::unlock();
			} else {
				Fl::lock();
				Meter_Win->hide();
				Fl::unlock();
			}
			msleep(10);
		}
		Fl::lock();
		for (n = 0; n < MsgData; n++) {
			Meter_Win->Meter->value(MsgBuf[n]);
			Meter_Win->Meter->redraw();
		}
		Fl::unlock();
	}
end:
#ifdef DBG
	printf("Deleting meter thread number...%d\n", index);
#endif
	Meter_Win->hide();
	rt_release_port(Target_Node, GetMeterDataPort);
	rt_task_delete(GetMeterDataTask);

	return 0;
}

static void *rt_get_led_data(void *arg)
{
	RT_TASK *GetLedDataTask;
	MBX *GetLedDataMbx;
	char GetLedDataMbxName[7];
	long GetLedDataPort;
	int MsgData = 0, MsgLen, MaxMsgLen, DataBytes;
	unsigned int MsgBuf[MAX_MSG_LEN/sizeof(unsigned int)];
	int n, index = *((int *)arg);
	unsigned int Led_Mask = 0;

	rt_allow_nonroot_hrt();
	if (!(GetLedDataTask = rt_task_init_schmod(get_an_id("HGE"), 99, 0, 0, SCHED_RR, 0xFF))) {
		printf("Cannot init Host GetLedData Task\n");
		return (void *)1;
	}
	GetLedDataPort = rt_request_port(Target_Node);
	sprintf(GetLedDataMbxName, "%s%d", TargetLedMbxID, index);
	if (!(GetLedDataMbx = (MBX *)RT_get_adr(Target_Node, GetLedDataPort, GetLedDataMbxName))) {
		printf("Error in getting %s mailbox address\n", GetLedDataMbxName);
		exit(1);
	}
	DataBytes = sizeof(unsigned int);
	MaxMsgLen = (MAX_MSG_LEN/DataBytes)*DataBytes;
	MsgLen = (((int)(DataBytes*REFRESH_RATE*(1./Leds[index].dt)))/DataBytes)*DataBytes;
	if (MsgLen < DataBytes) MsgLen = DataBytes;
	if (MsgLen > MaxMsgLen) MsgLen = MaxMsgLen;
	MsgData = MsgLen/DataBytes;

	Fl_Led_Window *Led_Win = new Fl_Led_Window(500 + 20*index, 290 + 20*index, 250, 250, MainWorkSpace->viewport(), Leds[index].name, Leds[index].n_leds);
	Leds_Manager_Win->Led_Windows[index] = Led_Win;

	rt_send(TargetInterfaceTask, 0);
	mlockall(MCL_CURRENT | MCL_FUTURE);

	while (true) {
		if (EndApp || !Is_Target_Connected) break;
		while (RT_mbx_receive_if(Target_Node, GetLedDataPort, GetLedDataMbx, &MsgBuf, MsgLen)) {
			if (EndApp || !Is_Target_Connected) goto end;
			if (!Leds_Manager_Win->is_visible()) {
				Fl::lock();
				Open_Leds_Manager->activate();
				Fl::unlock();
			}
			if (Leds[index].visible) {
				Fl::lock();
				Led_Win->show();
				Fl::unlock();
			} else {
				Fl::lock();
				Led_Win->hide();
				Fl::unlock();
			}
			msleep(10);
		}
		Fl::lock();
		for (n = 0; n < MsgData; n++) {
			Led_Mask = MsgBuf[n];
			Led_Win->led_mask(Led_Mask);
			Led_Win->led_on_off();
			Led_Win->update();
		}
		Fl::unlock();
	}
end:
#ifdef DBG
	printf("Deleting led thread number...%d\n", index);
#endif
	Led_Win->hide();
	rt_release_port(Target_Node, GetLedDataPort);
	rt_task_delete(GetLedDataTask);

	return 0;
}

static void *rt_get_log_data(void *arg)
{
	RT_TASK *GetLogDataTask;
	MBX *GetLogDataMbx;
	char GetLogDataMbxName[7];
	long GetLogDataPort;
	int MsgData = 0, MsgLen, MaxMsgLen, DataBytes;
	float MsgBuf[MAX_MSG_LEN/sizeof(float)];
	int n, i, j, k, DataCnt = 0;
	int index = *((int *)arg);

	rt_allow_nonroot_hrt();
	if (!(GetLogDataTask = rt_task_init_schmod(get_an_id("HGL"), 99, 0, 0, SCHED_RR, 0xFF))) {
		printf("Cannot init Host GetLogData Task\n");
		return (void *)1;
	}
	GetLogDataPort = rt_request_port(Target_Node);
	sprintf(GetLogDataMbxName, "%s%d", TargetLogMbxID, index);
	if (!(GetLogDataMbx = (MBX *)RT_get_adr(Target_Node, GetLogDataPort, GetLogDataMbxName))) {
		printf("Error in getting %s mailbox address\n", GetLogDataMbxName);
		exit(1);
	}
	DataBytes = (Logs[index].nrow*Logs[index].ncol)*sizeof(float);
	MaxMsgLen = (MAX_MSG_LEN/DataBytes)*DataBytes;
	MsgLen = (((int)(DataBytes*REFRESH_RATE*(1./Logs[index].dt)))/DataBytes)*DataBytes;
	if (MsgLen < DataBytes) MsgLen = DataBytes;
	if (MsgLen > MaxMsgLen) MsgLen = MaxMsgLen;
	MsgData = MsgLen/DataBytes;

	rt_send(TargetInterfaceTask, 0);
	mlockall(MCL_CURRENT | MCL_FUTURE);

	while (true) {
		if (EndApp || !Is_Target_Connected) break;
		while (RT_mbx_receive_if(Target_Node, GetLogDataPort, GetLogDataMbx, &MsgBuf, MsgLen)) {
			if (EndApp || !Is_Target_Connected) goto end;
			if (!Logs_Manager_Win->is_visible()) {
				Fl::lock();
				Open_Logs_Manager->activate();
				Fl::unlock();
			}
			msleep(10);
		}
		if (Logs_Manager_Win->start_saving(index)) {
			for (n = 0; n < MsgData; n++) {
				fprintf(Logs_Manager_Win->save_file(index), "Data # %d\n", ++DataCnt);
				for (i = 0; i < Logs[index].nrow; i++) {
					j = n*Logs[index].nrow*Logs[index].ncol + i;
					for (k = 0; k < Logs[index].ncol; k++) {
						fprintf(Logs_Manager_Win->save_file(index), "%1.5f ", MsgBuf[j]);
						j += Logs[index].nrow;
					}
					fprintf(Logs_Manager_Win->save_file(index), "\n");
				}
				if (DataCnt == Logs_Manager_Win->n_points_to_save(index)) {
					Logs_Manager_Win->stop_saving(index);
					DataCnt = 0;
					break;
				}
			}
		}
	}
end:
#ifdef DBG
	printf("Deleting log thread number...%d\n", index);
#endif
	rt_release_port(Target_Node, GetLogDataPort);
	rt_task_delete(GetLogDataTask);

	return 0;
}

static void *rt_get_scope_data(void *arg)
{
	RT_TASK *GetScopeDataTask;
	MBX *GetScopeDataMbx;
	char GetScopeDataMbxName[7];
	long GetScopeDataPort;
	int MsgData = 0, MsgLen, MaxMsgLen, TracesBytes;
	float MsgBuf[MAX_MSG_LEN/sizeof(float)];
	int n, nn, js, jl;
	int index = *((int *)arg);
	int stop_draw = false;
	int save_idx = 0;

	rt_allow_nonroot_hrt();
	if (!(GetScopeDataTask = rt_task_init_schmod(get_an_id("HGS"), 99, 0, 0, SCHED_RR, 0xFF))) {
		printf("Cannot init Host GetScopeData Task\n");
		return (void *)1;
	}
	GetScopeDataPort = rt_request_port(Target_Node);
	sprintf(GetScopeDataMbxName, "%s%d", TargetScopeMbxID, index);
	if (!(GetScopeDataMbx = (MBX *)RT_get_adr(Target_Node, GetScopeDataPort, GetScopeDataMbxName))) {
		printf("Error in getting %s mailbox address\n", GetScopeDataMbxName);
		return (void *)1;
	}
	TracesBytes = (Scopes[index].ntraces + 1)*sizeof(float);
	MaxMsgLen = (MAX_MSG_LEN/TracesBytes)*TracesBytes;
	MsgLen = (((int)(TracesBytes*REFRESH_RATE*(1./Scopes[index].dt)))/TracesBytes)*TracesBytes;
	if (MsgLen < TracesBytes) MsgLen = TracesBytes;
	if (MsgLen > MaxMsgLen) MsgLen = MaxMsgLen;
	MsgData = MsgLen/TracesBytes;

	Fl_Scope_Window *Scope_Win = new Fl_Scope_Window(500 + 20*index, 290 + 20*index, 250, 250, MainWorkSpace->viewport(), Scopes[index].name, Scopes[index].ntraces, Scopes[index].dt);
	Scopes_Manager_Win->Scope_Windows[index] = Scope_Win;

	rt_send(TargetInterfaceTask, 0);
	mlockall(MCL_CURRENT | MCL_FUTURE);

	while (true) {
		if (EndApp || !Is_Target_Connected) break;
		while (RT_mbx_receive_if(Target_Node, GetScopeDataPort, GetScopeDataMbx, &MsgBuf, MsgLen)) {
			if (EndApp || !Is_Target_Connected) goto end;
			if (!Scopes_Manager_Win->is_visible()) {
				Fl::lock();
				Open_Scopes_Manager->activate();
				Fl::unlock();
			}
			if (Scopes[index].visible) {
				Fl::lock();
				Scope_Win->show();
				Fl::unlock();
			} else {
				Fl::lock();
				Scope_Win->hide();
				Fl::unlock();
			}
			msleep(10);
		}
		Fl::lock();
		js = 1;
		for (n = 0; n < MsgData; n++) {
			for (nn = 0; nn < Scopes[index].ntraces; nn++) {
				Scope_Win->Plot->add_to_trace(nn, MsgBuf[js++]);
			}
			js++;
		}
		if (Scope_Win->is_visible() && (!stop_draw && !Scope_Win->Plot->pause())) {
			Scope_Win->Plot->redraw();
		}
		if (Scopes_Manager_Win->start_saving(index)) {
			jl = 0;
			for (n = 0; n < MsgData; n++) {
				for (nn = 0; nn < Scopes[index].ntraces + 1; nn++) {
					fprintf(Scopes_Manager_Win->save_file(index), "%1.5f ", MsgBuf[jl++]);
				}
				fprintf(Scopes_Manager_Win->save_file(index), "\n");
				save_idx++;
				if (save_idx == Scopes_Manager_Win->n_points_to_save(index)) {
					Scopes_Manager_Win->stop_saving(index);
					save_idx = 0;
					break;
				}
			}
		}
		Fl::unlock();
	}

end:
#ifdef DBG
	printf("Deleting scope thread number...%d\n", index);
#endif
	Scope_Win->hide();
	rt_release_port(Target_Node, GetScopeDataPort);
	rt_task_delete(GetScopeDataTask);

	return 0;
}

static void *rt_target_interface(void *args)
{
	unsigned int code, U_Request;
	char C_Request;
	int I_Request, Msg;
	int blk_index = 0;
	char buf[100];
	struct sockaddr_in Target_Addr;
	long Target_Port = 0;
	int Connect_Poll_Cnt = 0;
	int Connect_Poll_Max = 5;
	RT_TASK *IfTask = NULL, *task;

	rt_allow_nonroot_hrt();
	if (!(TargetInterfaceTask = rt_task_init_schmod(get_an_id("HTI"), 98, 0, 0, SCHED_FIFO, 0xFF))) {
		printf("Cannot init TargetInterfaceTask\n");
		exit(1);
	}

	while (!EndApp) {
		if (!(task = rt_receive(0, &code))) continue;
#ifdef DBG
		printf("Received code %d from task %p\n", code, task);
#endif
		switch (code & 0xf) {
//		switch (code & 0xffff) {
			case CONNECT_TO_TARGET:
#ifdef DBG
				printf("Reading target settings\n");
#endif
				read_target_settings();
				if (!strcmp(TargetIP, "0")) {
					Target_Node = 0;
#ifdef DBG
					printf("Local execution of the RTAI APIs\n");
#endif
				} else {
					sprintf(buf, "Trying to connect to %s", TargetIP);
					Main_Status->label(buf);
					Connect_To_Target->deactivate();
					MainWin->redraw();
#ifdef DBG
					printf("%s...", buf);
					fflush(stdout);
#endif
					inet_aton(TargetIP, &Target_Addr.sin_addr);
					Target_Node = Target_Addr.sin_addr.s_addr;
					while ((Target_Port = rt_request_port(Target_Node)) <= 0 && Connect_Poll_Cnt++ <= Connect_Poll_Max) {
						msleep(100);
					}
					Connect_To_Target->activate();
					if (Connect_Poll_Cnt >= Connect_Poll_Max) {
						Connect_Poll_Cnt = 0;
						Main_Status->label("Sorry, no route to target");
						MainWin->redraw();
#ifdef DBG
						printf(" Sorry, no route to target\n");
#endif
						RT_RETURN(task, CONNECT_TO_TARGET);
						break;
					}
#ifdef DBG
					printf(" Ok\n");
#endif
				}
				Connect_Poll_Cnt = 0;
				if (!(IfTask = (RT_TASK *)RT_get_adr(Target_Node, Target_Port, TargetInterfaceTaskName))) {
					Main_Status->label("No target or bad interface task identifier");
					MainWin->redraw();
#ifdef DBG
					printf("No target or bad interface task identifier\n");
#endif
					RT_RETURN(task, CONNECT_TO_TARGET);
					break;
				}
/* Getting running state and number of tunable parameters */
				U_Request = 'c';
				RT_rpc(Target_Node, Target_Port, IfTask, U_Request, &Is_Target_Running);
				Num_Tunable_Parameters = Is_Target_Running & 0xffff;
				Is_Target_Running >>= 16;
#ifdef DBG
				printf("Target is running...%s\n", Is_Target_Running ? "yes" : "no");
				printf("Number of target tunable parameters...%d\n", Num_Tunable_Parameters);
#endif
				if (Num_Tunable_Parameters > 0) Tunable_Parameters = new Target_Parameters_T [Num_Tunable_Parameters];
/* Getting tunable parameters info */
				C_Request = 'i';
				for (int n = 0; n < Num_Tunable_Parameters; n++) {
					RT_rpcx(Target_Node, Target_Port, IfTask, &C_Request, &Tunable_Parameters[n], sizeof(char), sizeof(Target_Parameters_T));
					if (n > 0) {
						if (strcmp(Tunable_Parameters[n-1].block_name, Tunable_Parameters[n].block_name)) {
							Num_Tunable_Blocks++;
#ifdef DBG
							printf("P offset %d\n", n);
#endif
						}
					} else {
						Num_Tunable_Blocks = 1;
					}
#ifdef DBG
					printf("Block: %s\n", Tunable_Parameters[n].block_name);
					printf(" Parameter: %s\n", Tunable_Parameters[n].param_name);
					printf(" Number of rows: %d\n", Tunable_Parameters[n].n_rows);
					printf(" Number of cols: %d\n", Tunable_Parameters[n].n_cols);
					for (int nr = 0; nr < Tunable_Parameters[n].n_rows; nr++) {
						for (int nc = 0; nc < Tunable_Parameters[n].n_cols; nc++) {
							printf(" Value    : %f\n", Tunable_Parameters[n].data_value[nr*Tunable_Parameters[n].n_cols+nc]);
						}
					}
#endif
				}
				if (Num_Tunable_Blocks > 0) Tunable_Blocks = new Target_Blocks_T [Num_Tunable_Blocks];
				blk_index = 0;
				for (int n = 0; n < Num_Tunable_Parameters; n++) {
					if (n > 0) {
						if (strcmp(Tunable_Parameters[n-1].block_name, Tunable_Parameters[n].block_name)) {
							blk_index++;
							strncpy(Tunable_Blocks[blk_index].name, Tunable_Parameters[n].block_name + strlen(Tunable_Parameters[0].model_name) + 1, MAX_NAMES_SIZE);
							Tunable_Blocks[blk_index].offset = n;
						}
					} else {
						strncpy(Tunable_Blocks[0].name, Tunable_Parameters[0].block_name + strlen(Tunable_Parameters[0].model_name) + 1, MAX_NAMES_SIZE);
						Tunable_Blocks[0].offset = 0;
					}
				}
					
/* Getting scope blocks info */
				for (int n = 0; n < MAX_RTAI_SCOPES; n++) {
					char mbx_name[7];
					sprintf(mbx_name, "%s%d", TargetScopeMbxID, n);
					if (!RT_get_adr(Target_Node, Target_Port, mbx_name)) {
						Num_Scopes = n;
#ifdef DBG
						printf("Number of target real time scopes: %d\n", Num_Scopes);
#endif
						break;
					}
				}
				if (Num_Scopes > 0) Scopes = new Target_Scopes_T [Num_Scopes];
				for (int n = 0; n < Num_Scopes; n++) {
					char scope_name[MAX_NAMES_SIZE];
					Scopes[n].visible = false;
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &Scopes[n].ntraces, sizeof(int), sizeof(int));
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &scope_name, sizeof(int), sizeof(scope_name));
					strncpy(Scopes[n].name, scope_name, MAX_NAMES_SIZE);
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &Scopes[n].dt, sizeof(int), sizeof(float));
#ifdef DBG
					printf("Scope: %s\n", Scopes[n].name);
					printf(" Number of traces...%d\n", Scopes[n].ntraces);
					printf(" Sampling time...%f\n", Scopes[n].dt);
#endif
				}
				I_Request = -1;
				RT_rpcx(Target_Node, Target_Port, IfTask, &I_Request, &Msg, sizeof(int), sizeof(int));
/* Getting log blocks info */
				for (int n = 0; n < MAX_RTAI_LOGS; n++) {
					char mbx_name[7];
					sprintf(mbx_name, "%s%d", TargetLogMbxID, n);
					if (!RT_get_adr(Target_Node, Target_Port, mbx_name)) {
						Num_Logs = n;
#ifdef DBG
						printf("Number of target real time logs: %d\n", Num_Logs);
#endif
						break;
					}
				}
				if (Num_Logs > 0) Logs = new Target_Logs_T [Num_Logs];
				for (int n = 0; n < Num_Logs; n++) {
					char log_name[MAX_NAMES_SIZE];
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &Logs[n].nrow, sizeof(int), sizeof(int));
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &Logs[n].ncol, sizeof(int), sizeof(int));
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &log_name, sizeof(int), sizeof(log_name));
					strncpy(Logs[n].name, log_name, MAX_NAMES_SIZE);
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &Logs[n].dt, sizeof(int), sizeof(float));
#ifdef DBG
					printf("Log: %s\n", Logs[n].name);
					printf(" Number of rows...%d\n", Logs[n].nrow);
					printf(" Number of cols...%d\n", Logs[n].ncol);
					printf(" Sampling time...%f\n", Logs[n].dt);
#endif
				}
				I_Request = -1;
				RT_rpcx(Target_Node, Target_Port, IfTask, &I_Request, &Msg, sizeof(int), sizeof(int));
/* Getting led blocks info */
				for (int n = 0; n < MAX_RTAI_LEDS; n++) {
					char mbx_name[7];
					sprintf(mbx_name, "%s%d", TargetLedMbxID, n);
					if (!RT_get_adr(Target_Node, Target_Port, mbx_name)) {
						Num_Leds = n;
#ifdef DBG
						printf("Number of target real time leds: %d\n", Num_Leds);
#endif
						break;
					}
				}
				if (Num_Leds > 0) Leds = new Target_Leds_T [Num_Leds];
				for (int n = 0; n < Num_Leds; n++) {
					char led_name[MAX_NAMES_SIZE];
					Leds[n].visible = false;
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &Leds[n].n_leds, sizeof(int), sizeof(int));
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &led_name, sizeof(int), sizeof(led_name));
					strncpy(Leds[n].name, led_name, MAX_NAMES_SIZE);
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &Leds[n].dt, sizeof(int), sizeof(float));
#ifdef DBG
					printf("Led: %s\n", Leds[n].name);
					printf(" Number of leds...%d\n", Leds[n].n_leds);
					printf(" Sampling time...%f\n", Leds[n].dt);
#endif
				}
				I_Request = -1;
				RT_rpcx(Target_Node, Target_Port, IfTask, &I_Request, &Msg, sizeof(int), sizeof(int));
/* Getting meter blocks info */
				for (int n = 0; n < MAX_RTAI_METERS; n++) {
					char mbx_name[7];
					sprintf(mbx_name, "%s%d", TargetMeterMbxID, n);
					if (!RT_get_adr(Target_Node, Target_Port, mbx_name)) {
						Num_Meters = n;
#ifdef DBG
						printf("Number of target real time meters: %d\n", Num_Meters);
#endif
						break;
					}
				}
				if (Num_Meters > 0) Meters = new Target_Meters_T [Num_Meters];
				for (int n = 0; n < Num_Meters; n++) {
					char meter_name[MAX_NAMES_SIZE];
					Meters[n].visible = false;
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &meter_name, sizeof(int), sizeof(meter_name));
					strncpy(Meters[n].name, meter_name, MAX_NAMES_SIZE);
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &Meters[n].dt, sizeof(int), sizeof(float));
#ifdef DBG
					printf("Meter: %s\n", Meters[n].name);
					printf(" Sampling time...%f\n", Meters[n].dt);
#endif
				}
				I_Request = -1;
				RT_rpcx(Target_Node, Target_Port, IfTask, &I_Request, &Msg, sizeof(int), sizeof(int));
/* Getting synchronoscope block info */
				{
					char mbx_name[7];
					sprintf(mbx_name, "%s", TargetSynchronoscopeMbxID);
					if (RT_get_adr(Target_Node, Target_Port, mbx_name)) {
						Num_Synchronoscope = 1;
					}
#ifdef DBG
					printf("Number of target real time synchronoscopes: %d\n", Num_Synchronoscope);
#endif
				}
				if (Num_Synchronoscope > 0) {
					int n = 0;
					char synchronoscope_name[MAX_NAMES_SIZE];
					Synchronoscope.visible = false;
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &synchronoscope_name, sizeof(int), sizeof(synchronoscope_name));
					strncpy(Synchronoscope.name, synchronoscope_name, MAX_NAMES_SIZE);
					RT_rpcx(Target_Node, Target_Port, IfTask, &n, &Synchronoscope.dt, sizeof(int), sizeof(float));
#ifdef DBG
					printf("Synchronoscope: %s\n", Synchronoscope.name);
					printf(" Sampling time...%f\n", Synchronoscope.dt);
#endif
				}
				I_Request = -1;
				RT_rpcx(Target_Node, Target_Port, IfTask, &I_Request, &Msg, sizeof(int), sizeof(int));
/* Ok, got all informations from the target */
				Is_Target_Connected = 1;
				if (Num_Tunable_Parameters > 0) {
					Parameters_Manager_Win = new Parameters_Manager(430, 260, MainWorkSpace->viewport(), "Parameters Manager");
					Parameters_Manager_Win->show();
					Parameters_Manager_Win->hide();
				}
				if (Num_Scopes > 0) {
					Scopes_Manager_Win = new Scopes_Manager(480, 300, MainWorkSpace->viewport(), "Scopes Manager");
					Scopes_Manager_Win->show();
					Scopes_Manager_Win->hide();
				}
				if (Num_Logs > 0) {
					Logs_Manager_Win = new Logs_Manager(380, 250, MainWorkSpace->viewport(), "Logs Manager");
					Logs_Manager_Win->show();
					Logs_Manager_Win->hide();
				}
				if (Num_Leds > 0) {
					Leds_Manager_Win = new Leds_Manager(320, 250, MainWorkSpace->viewport(), "Leds Manager");
					Leds_Manager_Win->show();
					Leds_Manager_Win->hide();
				}
				if (Num_Meters > 0) {
					Meters_Manager_Win = new Meters_Manager(320, 250, MainWorkSpace->viewport(), "Meters Manager");
					Meters_Manager_Win->show();
					Meters_Manager_Win->hide();
				}

#ifdef DBG
				printf("Target %s is correctly connected\n", Tunable_Parameters[0].model_name);
#endif
/* Creation of the threads receiving data from the target scope blocks */
				if (Num_Scopes > 0) GetScopeDataThread = new pthread_t [Num_Scopes];
				for (int n = 0; n < Num_Scopes; n++) {
					unsigned int msg;
					pthread_create(&GetScopeDataThread[n], NULL, rt_get_scope_data, &n);
					rt_receive(0, &msg);
				}
/* Creation of the threads receiving data from the target log blocks */
				if (Num_Logs > 0) GetLogDataThread = new pthread_t [Num_Logs];
				for (int n = 0; n < Num_Logs; n++) {
					unsigned int msg;
					pthread_create(&GetLogDataThread[n], NULL, rt_get_log_data, &n);
					rt_receive(0, &msg);
				}
/* Creation of the threads receiving data from the target led blocks */
				if (Num_Leds > 0) GetLedDataThread = new pthread_t [Num_Leds];
				for (int n = 0; n < Num_Leds; n++) {
					unsigned int msg;
					pthread_create(&GetLedDataThread[n], NULL, rt_get_led_data, &n);
					rt_receive(0, &msg);
				}
/* Creation of the threads receiving data from the target meter blocks */
				if (Num_Meters > 0) GetMeterDataThread = new pthread_t [Num_Meters];
				for (int n = 0; n < Num_Meters; n++) {
					unsigned int msg;
					pthread_create(&GetMeterDataThread[n], NULL, rt_get_meter_data, &n);
					rt_receive(0, &msg);
				}
/* Creation of the thread receiving data from the target synchronoscope block */
				if (Num_Synchronoscope > 0) {
					unsigned int msg;
					pthread_create(&GetSynchronoscopeDataThread, NULL, rt_get_synchronoscope_data, NULL);
					rt_receive(0, &msg);
				}
/* Creation of the thread getting and displaying the target time */
//				pthread_create(&GetTargetTimeThread, NULL, rt_get_target_time, NULL);
/* Updating the GUI */
				Fl::lock();
				Edit_Profiles->activate();
				Add_Profile->activate();
				Open_Parameters_Manager->activate();
				Open_Scopes_Manager->activate();
				Open_Logs_Manager->activate();
				Open_Leds_Manager->activate();
				Open_Meters_Manager->activate();
				Open_Synchronoscope->activate();
				Is_Target_Running ? Stop_RT->activate() : Start_RT->activate();
				Connect_To_Target->deactivate();
				Disconnect_From_Target->activate();
				MainMenuTable[1].deactivate();
				MainMenuTable[2].activate();
				MainMenuTable[4].activate();
				for (int i = 9; i <= 14; i++) MainMenuTable[i].activate();
				sprintf(buf, "Target: %s.", Tunable_Parameters[0].model_name);
				Main_Status->label(buf);
				MainMenu->menu(MainMenuTable);
				MainMenu->redraw();
				MainWin->redraw();
				Fl::unlock();
				RT_RETURN(task, CONNECT_TO_TARGET);
				break;
			case DISCONNECT_FROM_TARGET:
#ifdef DBG
				printf("Disconnecting from target %s\n", Tunable_Parameters[0].model_name);
#endif
				Is_Target_Connected = 0;
				for (int n = 0; n < Num_Scopes; n++) {
					pthread_join(GetScopeDataThread[n], NULL);
				}
				for (int n = 0; n < Num_Logs; n++) {
					pthread_join(GetLogDataThread[n], NULL);
				}
				for (int n = 0; n < Num_Leds; n++) {
					pthread_join(GetLedDataThread[n], NULL);
				}
				for (int n = 0; n < Num_Meters; n++) {
					pthread_join(GetMeterDataThread[n], NULL);
				}
				if (Num_Synchronoscope > 0) {
					pthread_join(GetSynchronoscopeDataThread, NULL);
				}
//				pthread_join(GetTargetTimeThread, NULL);
				Fl::lock();
				if (Parameters_Manager_Win) Parameters_Manager_Win->hide();
				if (Scopes_Manager_Win) Scopes_Manager_Win->hide();
				if (Logs_Manager_Win) Logs_Manager_Win->hide();
				if (Leds_Manager_Win) Leds_Manager_Win->hide();
				if (Meters_Manager_Win) Meters_Manager_Win->hide();
				if (Synchronoscope_Win) Synchronoscope_Win->hide();
				MainWin->redraw();
				Connect_To_Target->activate();
				Disconnect_From_Target->deactivate();
				MainMenuTable[1].activate();
				MainMenuTable[2].deactivate();
				MainMenuTable[4].deactivate();
				for (int i = 9; i <= 14; i++) MainMenuTable[i].deactivate();
				MainMenu->menu(MainMenuTable);
				MainMenu->redraw();
				Start_RT->deactivate();
				Stop_RT->deactivate();
				Edit_Profiles->deactivate();
				Add_Profile->deactivate();
				Open_Parameters_Manager->deactivate();
				Open_Scopes_Manager->deactivate();
				Open_Logs_Manager->deactivate();
				Open_Leds_Manager->deactivate();
				Open_Meters_Manager->deactivate();
				Open_Synchronoscope->deactivate();
				Fl::unlock();
				rt_release_port(Target_Node, Target_Port);
				Target_Port = 0;
				free(Tunable_Parameters);
				Main_Status->label("Ready...");
				MainWin->redraw();
#ifdef DBG
				printf("Disconnected succesfully.\n");
#endif
				RT_RETURN(task, DISCONNECT_FROM_TARGET);
				break;
			case START_TARGET:
				if (!Is_Target_Running) {
#ifdef DBG
					printf("Starting real time code...");
#endif
					U_Request = 's';
					RT_rpc(Target_Node, Target_Port, IfTask, U_Request, &Is_Target_Running);
					Start_RT->deactivate();
					Stop_RT->activate();
#ifdef DBG
					printf("ok\n");
#endif
				}
				RT_RETURN(task, START_TARGET);
				break;
			case STOP_TARGET:
				if (Is_Target_Running) {
					U_Request = 't';
					Is_Target_Connected = 0;
					for (int n = 0; n < Num_Scopes; n++) {
						pthread_join(GetScopeDataThread[n], NULL);
					}
					for (int n = 0; n < Num_Logs; n++) {
						pthread_join(GetLogDataThread[n], NULL);
					}
					for (int n = 0; n < Num_Leds; n++) {
						pthread_join(GetLedDataThread[n], NULL);
					}
					for (int n = 0; n < Num_Meters; n++) {
						pthread_join(GetMeterDataThread[n], NULL);
					}
					if (Num_Synchronoscope > 0) {
						pthread_join(GetSynchronoscopeDataThread, NULL);
					}
//					pthread_join(GetTargetTimeThread, NULL);
#ifdef DBG
					printf("Stopping real time code...");
#endif
					RT_rpc(Target_Node, Target_Port, IfTask, U_Request, &Is_Target_Running);
					rt_release_port(Target_Node, Target_Port);
					Connect_Poll_Cnt = 0;
					Target_Node = 0;
					Target_Port = 0;	
					Fl::lock();
					if (Parameters_Manager_Win) Parameters_Manager_Win->hide();
					if (Scopes_Manager_Win) Scopes_Manager_Win->hide();
					if (Logs_Manager_Win) Logs_Manager_Win->hide();
					if (Leds_Manager_Win) Leds_Manager_Win->hide();
					if (Meters_Manager_Win) Meters_Manager_Win->hide();
					if (Synchronoscope_Win) Synchronoscope_Win->hide();
					Connect_To_Target->activate();
					Disconnect_From_Target->deactivate();
					MainMenuTable[1].activate();
					MainMenuTable[2].deactivate();
					MainMenuTable[4].deactivate();
					for (int i = 9; i <= 14; i++) MainMenuTable[i].deactivate();
					MainMenu->menu(MainMenuTable);
					MainMenu->redraw();
					Start_RT->deactivate();
					Stop_RT->deactivate();
					Edit_Profiles->deactivate();
					Add_Profile->deactivate();
					Open_Parameters_Manager->deactivate();
					Open_Scopes_Manager->deactivate();
					Open_Logs_Manager->deactivate();
					Open_Leds_Manager->deactivate();
					Open_Meters_Manager->deactivate();
					Open_Synchronoscope->deactivate();
					Main_Status->label("Ready...");
					MainWin->redraw();
					Fl::unlock();
#ifdef DBG
					printf("ok\n");
#endif
				}
				RT_RETURN(task, STOP_TARGET);
				break;
			case UPDATE_PARAM: {
				U_Request = 'p';
				int Map_Offset = (code >> 4) & 0xffff;
				int Mat_Idx = (code >> 20) & 0xfff;
//				int Map_Offset = code >> 4;
//				int Map_Offset = code >> 16;
				int Is_Param_Updated;
				double new_value = (double)Tunable_Parameters[Map_Offset].data_value[Mat_Idx];
				RT_rpc(Target_Node, Target_Port, IfTask, U_Request, &Is_Target_Running);
				RT_rpcx(Target_Node, Target_Port, IfTask, &Map_Offset, &Is_Param_Updated, sizeof(int), sizeof(int));
				RT_rpcx(Target_Node, Target_Port, IfTask, &new_value, &Is_Param_Updated, sizeof(double), sizeof(int));
				RT_rpcx(Target_Node, Target_Port, IfTask, &Mat_Idx, &Is_Param_Updated, sizeof(int), sizeof(int));
				RT_RETURN(task, UPDATE_PARAM);
				break;
			}
			case BATCH_DOWNLOAD: {
				U_Request = 'd';
				int Is_Param_Updated;
				int Counter = Parameters_Manager_Win->batch_counter();
				RT_rpc(Target_Node, Target_Port, IfTask, U_Request, &Is_Target_Running);
				RT_rpcx(Target_Node, Target_Port, IfTask, &Counter, &Is_Param_Updated, sizeof(int), sizeof(int));
				RT_rpcx(Target_Node, Target_Port, IfTask, &Batch_Parameters, &Is_Param_Updated, sizeof(Batch_Parameters_T)*Counter, sizeof(int));
				RT_RETURN(task, BATCH_DOWNLOAD);
				break;
			}
/*
			case GET_PARAMS: {
				unsigned int Request = 'g';
				int n;
				RT_rpc(TargetNode, TargetPort, IfTask, Request, &IsTargetRunning);
				for (n = 0; n < NumTunableParameters; n++) {
					RT_rpcx(TargetNode, TargetPort, IfTask, &Request, &TunableParameters[n].dataValue, sizeof(int), sizeof(double));
				}
				ParametersManager->GetParameters();
				RT_RETURN(task, GET_PARAMS);
				break;
			}

			case GET_TARGET_TIME: {
				unsigned int Request = 'm';
				int Msg;
				RT_rpc(TargetNode, TargetPort, IfTask, Request, &IsTargetRunning);
				RT_rpcx(TargetNode, TargetPort, IfTask, &Msg, &TargetTime, sizeof(int), sizeof(float));
				break;
			}
*/
			case CLOSE:
				rt_task_delete(TargetInterfaceTask);
				return 0;
			default:
				break;
		}
	}

	rt_task_delete(TargetInterfaceTask);

	return 0;
}

int main(int argc, char **argv)
{
	RT_TASK *MainTask;
	char *lang_env;

	lang_env = getenv("LANG");
	setenv("LANG", "en_US", 1);

	rt_allow_nonroot_hrt();
	if (!(MainTask = rt_task_init_schmod(get_an_id("HMN"), 99, 0, 0, SCHED_FIFO, 0xFF))) {
		printf("Cannot init HostMainTask\n");
		return 1;
	}

	MainWin = new Fl_Main_Window((int)(Fl::w()), (int)(Fl::h()), "RTAI-Lab Graphical User Interface");

	MainMenu = MainWin->menu();
	MainMenu->menu(MainMenuTable);
	MainMenu->box(FL_THIN_UP_BOX);

	MainToolBar = MainWin->toolbar();
	MainToolBar->box(FL_THIN_UP_BOX);
	Connect_To_Target = MainToolBar->add_button(Fl_Image::read_xpm(0, connect_icon), 0, "Connect", "Connect to target...");
	Disconnect_From_Target = MainToolBar->add_button(Fl_Image::read_xpm(0, disconnect_icon), 0, "Disconnect", "Disconnect from target...");
	Edit_Profiles = MainToolBar->add_button(Fl_Image::read_xpm(0, session_open_icon), 0, "Edit Profiles", "Edit Profiles...");
	Add_Profile = MainToolBar->add_button(Fl_Image::read_xpm(0, session_save_icon), 0, "Add Profile", "Add Profile...");
	MainToolBar->add_divider();
	Start_RT = MainToolBar->add_button(Fl_Image::read_xpm(0, start_icon), 0, "Start", "Start real time code");
	Stop_RT = MainToolBar->add_button(Fl_Image::read_xpm(0, stop_icon), 0, "Stop", "Stop real time code");
	MainToolBar->add_divider();
	Open_Parameters_Manager = MainToolBar->add_toggle(Fl_Image::read_xpm(0, parameters_icon), 0, "Parameters", "Open parameters manager");
	Open_Scopes_Manager = MainToolBar->add_toggle(Fl_Image::read_xpm(0, scope_icon), 0, "Scopes", "Open scope manager");
	Open_Logs_Manager = MainToolBar->add_toggle(Fl_Image::read_xpm(0, log_icon), 0, "Logs", "Open log manager");
	Open_Leds_Manager = MainToolBar->add_toggle(Fl_Image::read_xpm(0, led_icon), 0, "Leds", "Open led manager");
	Open_Meters_Manager = MainToolBar->add_toggle(Fl_Image::read_xpm(0, meter_icon), 0, "Meters", "Open meter manager");
	Open_Synchronoscope = MainToolBar->add_button(Fl_Image::read_xpm(0, synchronoscope_icon), 0, "Synchronoscope", "Open synchronoscope");
	MainToolBar->add_divider();
	Profile_Choice = new Fl_Choice(0, 0, 120, 20);
        Profile_Choice->align(FL_ALIGN_RIGHT);
	Profile_Choice->tooltip("Profiles");
	Profile_Choice->add("Default Profile");
	Profile_Choice->deactivate();
	MainToolBar->add(Profile_Choice);

	Connect_To_Target->callback((Fl_Callback *)connect_cb);
	Disconnect_From_Target->callback((Fl_Callback *)disconnect_cb);
	Start_RT->callback((Fl_Callback *)start_stop_cb);
	Stop_RT->callback((Fl_Callback *)start_stop_cb);
	Open_Parameters_Manager->callback((Fl_Callback *)open_parameters_manager_cb);
	Open_Scopes_Manager->callback((Fl_Callback *)open_scopes_manager_cb);
	Open_Logs_Manager->callback((Fl_Callback *)open_logs_manager_cb);
	Open_Leds_Manager->callback((Fl_Callback *)open_leds_manager_cb);
	Open_Meters_Manager->callback((Fl_Callback *)open_meters_manager_cb);
	Open_Synchronoscope->callback((Fl_Callback *)open_synchronoscope_cb);

	Disconnect_From_Target->deactivate();
	Edit_Profiles->deactivate();
	Add_Profile->deactivate();
	Start_RT->deactivate();
	Stop_RT->deactivate();
	Open_Parameters_Manager->deactivate();
	Open_Scopes_Manager->deactivate();
	Open_Logs_Manager->deactivate();
	Open_Leds_Manager->deactivate();
	Open_Meters_Manager->deactivate();
	Open_Synchronoscope->deactivate();

	MainWorkSpace = new Fl_Workspace(0, 0, (int)(Fl::w()), (int)(Fl::h()));
	MainWin->view(MainWorkSpace);

	Main_Status = MainWin->status();
	Main_Status->box(FL_THIN_UP_BOX);
	Main_Status->label("Ready...");

	MainWin->end();
	MainWin->show(argc, argv);

	Connect_Dialog = new Fl_Dialog(250, 290, "Connect to Target");
	Connect_Dialog->new_group("Connect");
	{ Fl_Input *o = Target_IP_Address = new Fl_Input(10, 20, 120, 20, " IP Address");
	  o->align(FL_ALIGN_RIGHT);
	  o->maximum_size(15);
	}
	{ Fl_Input *o = Target_Task_ID = new Fl_Input(10, 20 + 27, 70, 20, " Task Identifier");
	  o->align(FL_ALIGN_RIGHT);
	  o->maximum_size(6);
	}
	{ Fl_Input *o = Target_Scope_ID = new Fl_Input(10, 20 + 27*2, 70, 20, " Scope Identifier");
	  o->align(FL_ALIGN_RIGHT);
	  o->maximum_size(3);
	}
	{ Fl_Input *o = Target_Log_ID = new Fl_Input(10, 20 + 27*3, 70, 20, " Log Identifier");
	  o->align(FL_ALIGN_RIGHT);
	  o->maximum_size(3);
	}
	{ Fl_Input *o = Target_Led_ID = new Fl_Input(10, 20 + 27*4, 70, 20, " Led Identifier");
	  o->align(FL_ALIGN_RIGHT);
	  o->maximum_size(3);
	}
	{ Fl_Input *o = Target_Meter_ID = new Fl_Input(10, 20 + 27*5, 70, 20, " Meter Identifier");
	  o->align(FL_ALIGN_RIGHT);
	  o->maximum_size(3);
	}
	{ Fl_Input *o = Target_Synchronoscope_ID = new Fl_Input(10, 20 + 27*6, 70, 20, " Synch Identifier");
	  o->align(FL_ALIGN_RIGHT);
	  o->maximum_size(3);
	}
	read_target_settings();
	Connect_Dialog->end();
	Connect_Dialog->buttons(FL_DLG_OK|FL_DLG_CANCEL,FL_DLG_OK);

	Add_Profile_Dialog = new Fl_Dialog(250, 100, "Add Profile");
	Add_Profile_Dialog->new_group("Add Profile");
	{ Fl_Input *o = Profile_Input = new Fl_Input(10, 20, 120, 20, "Profile Name");
	  o->align(FL_ALIGN_RIGHT);
	}
	Add_Profile_Dialog->end();
	Add_Profile_Dialog->buttons(FL_DLG_OK|FL_DLG_CANCEL,FL_DLG_OK);

	Edit_Profiles_Dialog = new Fl_Dialog(450, 300, "Edit Profiles");
	Edit_Profiles_Dialog->new_group("Edit Profiles");
	{ Fl_Tabs* o = Profile_Tabs = new Fl_Tabs(160, 5, 280, 240);
	    { Fl_Group* o = Profile_Connection = new Fl_Group(0, 20, 280, 180, "Connection");
		{ Fl_Input *o = Profile_IP_Addr = new Fl_Input(10, 20, 120, 20, " IP Address");
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum_size(15);
		}
		{ Fl_Input *o = Profile_Task_ID = new Fl_Input(10, 20 + 27, 70, 20, " Task Identifier");
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum_size(6);
		}
		{ Fl_Input *o = Profile_Scope_ID = new Fl_Input(10, 20 + 27*2, 70, 20, " Scope Identifier");
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum_size(3);
		}
		{ Fl_Input *o = Profile_Log_ID = new Fl_Input(10, 20 + 27*3, 70, 20, " Log Identifier");
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum_size(3);
		}
                o->end();
            }
	    o->deactivate();
	    o->end();
	}
	Profiles_Tree = new Fl_Browser(5, 5, 150, 240);
	Profiles_Folder = add_folder(Profiles_Tree, "Profiles", 1, Fl_Image::read_xpm(0, profile_icon));
	add_paper(Profiles_Folder, "Default Profile", 0);
	Edit_Profiles_Dialog->end();
	Edit_Profiles_Dialog->buttons(FL_DLG_OK|FL_DLG_CANCEL,FL_DLG_OK);

	Fl::lock();
	pthread_create(&TargetInterfaceThread, NULL, rt_target_interface, NULL);

	while (!EndAll) {
		Fl::wait(FLTK_EVENTS_TICK);
	}

	rt_task_delete(MainTask);

	setenv("LANG", lang_env, 1);

	return 0;
}
