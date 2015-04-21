/*
 * Project: rtai_cpp - RTAI C++ Framework 
 *
 * File: $Id: rtai_wrapper.c,v 1.1.1.1 2004/06/06 14:03:07 rpm Exp $
 *
 * Copyright: (C) 2001,2002 Erwin Rol <erwin@muffin.org>
 *
 * Licence:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */
#include "rtai_wrapper.h"

#ifdef RTASK
#undef RTASK
#endif

#ifdef SEM
#undef SEM
#endif

#ifdef CND
#undef CND
#endif

#ifdef MBX
#undef MBX
#endif

#ifdef TBX
#undef TBX
#endif

#ifdef BITS
#undef BITS
#endif

#include <rtai_sched.h>
#include <rtai.h>
#include <rtai_fifos.h>
#include <rtai_nam2num.h>
#include <rtai_tbx.h>
#include <rtai_bits.h>
#include <rtai_trace.h>
#include <../lxrt/registry.h>

#ifdef NR_RT_CPUS
#if RTAI_NR_CPUS != NR_RT_CPUS
#error RTAI_NR_CPUS is not the same as NR_RT_CPUS, check rtai_wrapper.h
#endif
#endif


void __rt_get_global_lock(void){
	rt_get_global_lock();
}
 
void __rt_release_global_lock(void){
	rt_release_global_lock();
}

int __hard_cpu_id( void ){
	return hard_cpu_id();
}


int cpp_key;

// task functions

RT_TASK * __rt_task_init(void (*rt_thread)(int), int data,
			int stack_size, int priority, int uses_fpu, void(*signal)(void))
{
	RT_TASK * task;

	task = rt_malloc( sizeof(RT_TASK) );

	if(task == 0)
		return 0;

	memset(task,0,sizeof(RT_TASK));
	
	rt_task_init(task,rt_thread,data,stack_size,priority,uses_fpu,signal);

	__rt_tld_set_data(task,cpp_key,(void*)data);
			
	return task;
}

RT_TASK * __rt_task_init_cpuid(void (*rt_thread)(int), int data,
				int stack_size, int priority, int uses_fpu,
				void(*signal)(void), unsigned int run_on_cpu)
{
	RT_TASK * task;
	
	task = rt_malloc(sizeof(RT_TASK));
	
	if(task == 0)
		return 0;

	memset(task,0,sizeof(RT_TASK));
		
	rt_task_init_cpuid(task,rt_thread,data,stack_size,priority,uses_fpu,signal,run_on_cpu);

	__rt_tld_set_data(task,cpp_key,(void*)data);
	
	return task;
}

int __rt_task_delete(RT_TASK *task)
{
	int result;
	rt_printk("__rt_task_delete(%p)\n",task);

	if(task == 0)
		return -1;
	
	rt_task_suspend(task);
	
	result = rt_task_delete(task);
	
	rt_free(task);
	
	return result;
}


RT_TASK * __rt_named_task_init(const char* name, void (*rt_thread)(int), int data,
			int stack_size, int priority, int uses_fpu, void(*signal)(void))
{
	RT_TASK * task;

	if ( rt_get_adr( nam2num(name) ) ) {
		// already a object with this name
		// we should use open in this case
		return NULL;
	}

	task = rt_named_task_init(name,rt_thread,data,stack_size,priority,uses_fpu,signal);

	__rt_tld_set_data(task,cpp_key,(void*)data);
			
	return task;
}

RT_TASK * __rt_named_task_init_cpuid(const char* name, void (*rt_thread)(int), int data,
				int stack_size, int priority, int uses_fpu,
				void(*signal)(void), unsigned int run_on_cpu)
{
	RT_TASK * task;
	
	if ( rt_get_adr( nam2num(name) ) ) {
		// already a object with this name
		// we should use open in this case
		return NULL;
	}

	
	task = rt_named_task_init_cpuid(name,rt_thread,data,stack_size,priority,uses_fpu,signal,run_on_cpu);

	__rt_tld_set_data(task,cpp_key,(void*)data);
	
	return task;
}

RT_TASK* __rt_get_named_task( const char* name )
{
	unsigned long num = nam2num(name);
	if(  rt_get_type( num ) == IS_TASK ){
		return rt_get_adr( num  );
	} else {
		return 0;
	}
}

int __rt_named_task_delete(RT_TASK *task)
{
	int result;
	rt_printk("__rt_named_task_delete(%p)\n",task);

	if(task == 0)
		return -1;
	
	rt_task_suspend(task);
	
	result = rt_named_task_delete(task);
	
	return result;
}
// get a new key and mark it in tld_used_mask 
int __rt_tld_create_key(void){
#if 1
	return 0;
#else
	return rt_tld_create_key();
#endif
}

// free the bit in tld_used_mask
int __rt_tld_free_key(int key){
#if 1
	return 0;
#else	
	return rt_tld_free_key(key);
#endif
}

// set the data in the currents RT_TASKS tld_data array
int __rt_tld_set_data(RT_TASK *task,int key,void* data){
#if 1
	task->system_data_ptr = (void*)data;
	return 0;
#else
	return rt_tld_set_data(task,key,data);
#endif
}

// get the data from the current task
void* __rt_tld_get_data(RT_TASK *task,int key){
#if 1
	return (void*)task->system_data_ptr;
#else
	return rt_tld_get_data(task,key);
#endif
}

/* semaphore */

SEM* __rt_typed_sem_init(int value, int type){
	SEM * sem;
	
	sem = rt_malloc(sizeof(SEM));
	
	if(sem == 0)
		return 0;
		

	memset(sem,0,sizeof(SEM));

	rt_typed_sem_init(sem,value,type);

	return sem;
}

int __rt_sem_delete(SEM* sem){
	int result;
	
	if(sem == 0)
		return -1;

	result =  rt_sem_delete(sem);

	rt_free(sem);

	return result;
}

SEM* __rt_typed_named_sem_init(const char* name,int value, int type){
	SEM * sem;
	
	if ( rt_get_adr( nam2num(name) ) ) {
		// already a object with this name
		// we should use open in this case
		return NULL;
	}
	
	sem = rt_typed_named_sem_init(name,value,type);

	return sem;
}

SEM* __rt_get_named_sem( const char* name )
{
	unsigned long num = nam2num(name);
	if(  rt_get_type( num ) == IS_SEM ){
		return rt_get_adr( num  );
	} else {
		return 0;
	}
}

int __rt_named_sem_delete(SEM* sem){
	int result;
	
	if(sem == 0)
		return -1;

	result =  rt_named_sem_delete(sem);

	return result;
}

int __rt_cond_wait(CND *cnd, SEM *mtx){
	return rt_cond_wait(cnd, mtx);
}

int __rt_cond_wait_until(CND *cnd, SEM *mtx, RTIME time){
	return rt_cond_wait_until(cnd, mtx, time);
}

int __rt_cond_wait_timed(CND *cnd, SEM *mtx, RTIME delay){
	return rt_cond_wait_timed(cnd, mtx, delay);
}

/* mailbox functions */

MBX* __rt_mbx_init(int size){
	MBX * mbx;
	
	mbx = rt_malloc(sizeof(MBX));
	
	if(mbx == 0)
		return 0;
		

	memset(mbx,0,sizeof(MBX));

	rt_mbx_init(mbx,size);

	return mbx;
}

int __rt_mbx_delete(MBX* mbx){
	int result;
	
	if(mbx == 0)
		return -1;

	result =  rt_mbx_delete(mbx);

	rt_free(mbx);

	return result;
}

MBX* __rt_named_mbx_init(const char* name,int size){
	MBX * mbx;
	
	if ( rt_get_adr( nam2num(name) ) ) {
		// already a object with this name
		// we should use open in this case
		return NULL;
	}

	mbx = rt_named_mbx_init(name,size);

	return mbx;
}


MBX* __rt_get_named_mbx( const char* name )
{
	unsigned long num = nam2num(name);
	if(  rt_get_type( num ) == IS_MBX ){
		return rt_get_adr( num  );
	} else {
		return 0;
	}
}


int __rt_named_mbx_delete(MBX* mbx){
	int result;
	
	if(mbx == 0)
		return -1;

	result =  rt_named_mbx_delete(mbx);

	return result;
}

#ifdef CONFIG_RTAI_TRACE

int __trace_destroy_event( int id ){
	return trace_destroy_event( id );
}

int __trace_create_event( const char* name, void* p1, void* p2){
	return trace_create_event( name, p1, p2);
}

int __trace_raw_event( int id, int size, void* p){
	return trace_raw_event( id, size, p);
}

#endif

/* By Peter Soetens */
RTIME __timeval2count(struct timeval *t)
{
    return timeval2count(t);
}

void  __count2timeval(RTIME rt, struct timeval *t)
{
    return count2timeval(rt, t);
}

RTIME __timespec2count(const struct timespec *t)
{
    return timespec2count(t);
}

void __count2timespec(RTIME rt, struct timespec *t)
{
    return count2timespec(rt, t);
}
