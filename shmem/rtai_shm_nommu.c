/*
COPYRIGHT (C) 2001  Lineo Inc. (Author: bkuhn@lineo.com)

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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/


#include <linux/version.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <rtai.h>
#include <rtai_shm.h>

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#endif

static struct desc_struct sidt;

DEFINE_SHM_HANDLER

struct rtai_kmalloc_node {
  struct rtai_kmalloc_node *next;
  struct rtai_kmalloc_node *prev;
  int name;
  int addr;
  int size;
  int used;
  char alignfill[8];
};


struct rtai_kmalloc_node rtai_kmalloc_node_root = {
  &rtai_kmalloc_node_root,&rtai_kmalloc_node_root,0,0,0};


struct rtai_kmalloc_node*
rtai_kmalloc_node_find(int name) {
  struct rtai_kmalloc_node* node = rtai_kmalloc_node_root.next;
  while(node != &rtai_kmalloc_node_root) {
    if(node->name == name) return node;
    node=node->next;
  };
  return 0;
};


struct rtai_kmalloc_node*
rtai_kmalloc_node_new(int name,int size) {
  struct rtai_kmalloc_node *node = (struct rtai_kmalloc_node*)
    kmalloc(sizeof(struct rtai_kmalloc_node)+size,GFP_KERNEL);

  node->name=name;
  node->addr=(int)node+sizeof(struct rtai_kmalloc_node);
  node->size=size;
  node->used=1;

  node->next = &rtai_kmalloc_node_root;
  node->prev = rtai_kmalloc_node_root.prev;
  rtai_kmalloc_node_root.prev->next = node;
  rtai_kmalloc_node_root.prev = node;

  MOD_INC_USE_COUNT;
  return node;
};


// struct rtai_kmalloc_node*
void
rtai_kmalloc_node_delete(struct rtai_kmalloc_node* node) {
  node->next->prev = node->prev;
  node->prev->next = node->next;
  kfree((void*)node->addr);
  MOD_DEC_USE_COUNT;
};


void *rtai_kmalloc(int name, int size) {
  struct rtai_kmalloc_node* node = rtai_kmalloc_node_find(name);
  if(node) 
    if(node->size == size)
      node->used++;
    else
      printk("rtai_kmalloc: requested size (%i) missmatches existing size (%i) for name %x\n",size,node->size,name);
  else node = rtai_kmalloc_node_new(name,size);
  return (void*)node->addr;
};


void rtai_kfree(int name) {
  struct rtai_kmalloc_node* node = rtai_kmalloc_node_find(name);
  if(node && !(--node->used)) rtai_kmalloc_node_delete(node);
};


void* shm_handler(unsigned int srq, unsigned long arg) {
  switch (srq) {
  case CMD_RTAI_KMALLOC:
    do {
      struct rtai_kmalloc_desc *mem = (struct rtai_kmalloc_desc*)arg;
      return rtai_kmalloc(mem->name,mem->size);
    } while(0);
    break;
  case CMD_RTAI_KFREE:
    do {
      struct rtai_kfree_desc *mem = (struct rtai_kfree_desc*)arg;    
      rtai_kfree(mem->name);
      return 0;
    } while(0);    
    break;
  default:
    break;
  };
  return (void*)-EINVAL;
};

#ifdef CONFIG_PROC_FS
static int rtai_proc_shm_register(void);
static void rtai_proc_shm_unregister(void);
#endif

int init_module(void) {
#ifdef CONFIG_PROC_FS
  rtai_proc_shm_register();
#endif
  sidt = rt_set_full_intr_vect(RTAI_SHM_VECTOR,15,3,(void *)RTAI_SHM_HANDLER);
  return 0;
};


void cleanup_module(void) {
#ifdef CONFIG_PROC_FS
  rtai_proc_shm_unregister();
#endif
  rt_reset_full_intr_vect(RTAI_SHM_VECTOR, sidt);
};


#ifdef CONFIG_PROC_FS
/* ----------------------< proc filesystem section >----------------------*/

static int rtai_read_shm(char *page, char **start, off_t off, int count,
			 int *eof, void *data) {
  PROC_PRINT_VARS;
  struct rtai_kmalloc_node* node = rtai_kmalloc_node_root.next;

  PROC_PRINT("\nRTAI Shared Memory Manager for uClinux.\n\n");
  PROC_PRINT("Name        Address     Size    Used\n");
  PROC_PRINT("------------------------------------\n" );

  while(node != &rtai_kmalloc_node_root) {
    int flags,name,addr,size,used;
    save_and_cli(flags);
    name=node->name;
    addr=node->addr;
    size=node->size;
    used=node->used;
    node=node->next;
    restore_flags(flags);
    PROC_PRINT("0x%-9x 0x%-9x %-7i %-4i\n",name,addr,size,used);    
  };
  PROC_PRINT_DONE;
  
}  /* End function - rtai_read_shm */


static int rtai_proc_shm_register(void) {
  struct proc_dir_entry *proc_shm_ent;
  
  proc_shm_ent = create_proc_entry("shmem", S_IFREG|S_IRUGO|S_IWUSR, rtai_proc_root);
  if (!proc_shm_ent) {
    printk("Unable to initialize /proc/rtai/shmem\n");
    return(-1);
  }
  proc_shm_ent->read_proc = rtai_read_shm;
  return(0);
}  /* End function - rtai_proc_sched_register */


static void rtai_proc_shm_unregister(void) {
  remove_proc_entry("shmem", rtai_proc_root);
}  /* End function - rtai_proc_sched_unregister */

/* ------------------< end of proc filesystem section >------------------*/
#endif /* CONFIG_PROC_FS */
