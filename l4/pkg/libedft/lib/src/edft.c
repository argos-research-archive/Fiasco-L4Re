/**
 * \file
 * \brief  Library for creating deadline-based threads for the EDF scheduler
 *
 * \date   Aug 27th 2014
 * \author Valentin Hauner <vh@hnr.name>
 */
/* 
 * (c) 2014 Valentin Hauner <vh@hnr.name>
 *
 * This file is distributed under the terms of the
 * GNU General Public License 3.
 * Please see the COPYING-GPL-3 file for details.
 */
#include <l4/libedft/edft.h>
#include <l4/libedft/slist.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

Edf_thread     threads[THREAD_MAX_NUM];
l4_umword_t   *thread_stacks[THREAD_MAX_NUM];

unsigned       count = 0;

int edft_no2index(unsigned no)
{
  unsigned i;
  for (i = 0; i < count; i++)
  {
    if (threads[i].no == no)
      return i;
  }
  return -1;
}

Edf_thread * edft_obj(unsigned no)
{
  int index = edft_no2index(no);
  if (index < 0)
    return NULL;
  return &threads[index];
}

l4_cap_idx_t edft_create_l4_task(unsigned map_code, unsigned map_data)
{
  l4_msgtag_t tag;

  l4_cap_idx_t task_cap = l4re_util_cap_alloc();
  if (l4_is_invalid_cap(task_cap))
    return -1;
  // The second argument of l4_fpage denotes the size of the flex-page in log2
  l4_fpage_t task_fpage = l4_fpage(l4re_env()->first_free_utcb, L4_LOG2_PAGESIZE, L4_CAP_FPAGE_RW);
  tag = l4_factory_create_task(l4re_env()->factory, task_cap, task_fpage);
  if (l4_error(tag))
    return -2;

  // Give the new task a basic set of capabilities
  l4_cap_idx_t caps[] = { /*l4re_env()->parent,*/
                           l4re_env()->rm,
                           l4re_env()->mem_alloc,
                           l4re_env()->log,
                           l4re_env()->factory,
                           l4re_env()->scheduler };
  unsigned i;
  for (i = 0; i < sizeof(caps) / sizeof(l4_cap_idx_t); i++)
  {
    tag = l4_task_map(task_cap, L4RE_THIS_TASK_CAP,
                       l4_obj_fpage(caps[i], 0, L4_FPAGE_RO),
                       l4_map_obj_control(caps[i], L4_MAP_ITEM_MAP));
    if (l4_error(tag))
      return -3;
  }

  tag = l4_task_map(task_cap, L4RE_THIS_TASK_CAP,
                     l4_obj_fpage(l4re_env()->parent, 0, L4_FPAGE_RO),
                     l4_map_obj_control(L4RE_THIS_TASK_CAP, L4_MAP_ITEM_MAP));
  if (l4_error(tag))
    return -3;


  // Map the Kernel Info Page (KIP) to the different task
  l4_touch_ro(l4re_kip(), L4_PAGESIZE);
  tag = l4_task_map(task_cap, L4RE_THIS_TASK_CAP,
                     l4_fpage((unsigned long)l4re_kip(), L4_LOG2_PAGESIZE, L4_FPAGE_RO),
                     l4_map_control((l4_umword_t)l4re_kip(), 0, L4_MAP_ITEM_MAP));
  if (l4_error(tag))
    return -4;

  if (map_code || map_data)
  {
    char *it_start, *it_end;
    extern char _start[], _sdata[], _end[];

    if (map_code)
    {
      it_start = _start;
      l4_touch_ro(_start, _sdata - _start + 1);
    }
    else
      it_start = _sdata;

    if (map_data)
    {
      it_end = _end;
      l4_touch_rw(_sdata, _end - _sdata);
    }
    else
      it_end = _sdata - 3;

    for (unsigned long i = ((unsigned long)it_start & L4_PAGEMASK);
          i < ((unsigned long)it_end & L4_PAGEMASK);
          i += L4_PAGESIZE)
    {
      unsigned rights;
      if (i >= ((unsigned long) _sdata & L4_PAGEMASK))
        rights = L4_FPAGE_RW;
      else
        rights = L4_FPAGE_RO;
      tag = l4_task_map(task_cap, L4RE_THIS_TASK_CAP,
                         l4_fpage(i, L4_LOG2_PAGESIZE, rights),
                         l4_map_control(i, 0, L4_MAP_ITEM_MAP));
      if (l4_error(tag))
        return -5;
    }
  }

  return task_cap;
}

int edft_create_l4_thread(Edf_thread *thread, unsigned run)
{
  l4_msgtag_t tag;
  unsigned tid = count++;

  thread->thread_cap = l4re_util_cap_alloc();
  if (l4_is_invalid_cap(thread->thread_cap))
    return -1;

  tag = l4_factory_create_thread(l4re_env()->factory, thread->thread_cap);
  if (l4_error(tag))
    return -2;

  // Handle the default value for the task capability of the thread
  if (thread->task_cap == 0)
    thread->task_cap = L4RE_THIS_TASK_CAP;

  // We have to align the memory allocated for the thread stack to guarantee that it can be accessed properly
  if (posix_memalign((void**)&thread_stacks[tid], L4_PAGESIZE, THREAD_STACK_SIZE) != 0)
    return -3;
  // For x86-32, the arguments of functions are passed via the stack
  // At the moment, this is implemented for x86 only
  unsigned stack_idx_max = THREAD_STACK_SIZE / sizeof(l4_umword_t) - 1;
  thread_stacks[tid][stack_idx_max] = thread->arg;
  thread_stacks[tid][stack_idx_max - 1] = 0;

  if (thread->task_cap != L4RE_THIS_TASK_CAP)
  {
    // Map the thread stack which has been allocated on the heap to the different task
    l4_touch_rw(thread_stacks[tid], THREAD_STACK_SIZE);
    tag = l4_task_map(thread->task_cap, L4RE_THIS_TASK_CAP,
                       l4_fpage((unsigned long)thread_stacks[tid], log2(THREAD_STACK_SIZE), L4_FPAGE_RW),
                       l4_map_control((l4_umword_t)thread_stacks[tid], 0, L4_MAP_ITEM_MAP));
    if (l4_error(tag))
      return -4;
  }

  l4_thread_control_start();
  l4_thread_control_pager(l4re_env()->rm);
  l4_thread_control_exc_handler(l4re_env()->rm);
  l4_thread_control_bind((l4_utcb_t *)l4re_env()->first_free_utcb, thread->task_cap);
  tag = l4_thread_control_commit(thread->thread_cap);
  if (l4_error(tag))
    return -5;

  // Set the stack pointer to the second last element in the stack array for passing the arguments to the function
  tag = l4_thread_ex_regs(thread->thread_cap,
                          (l4_umword_t)thread->func,
                          (l4_umword_t)&thread_stacks[tid][stack_idx_max - 1], 0);
  if (l4_error(tag))
    return -6;

  threads[tid] = *thread;

  // Immediate call of l4_scheduler_run_thread?
  if (run)
  {
    if (edft_run_l4_thread(thread->no) < 0)
      return -7;
  }

  l4_debugger_set_object_name(thread->thread_cap, thread->name);

  return tid;
}

int edft_run_l4_thread(unsigned no)
{
  l4_msgtag_t tag;
  int index = edft_no2index(no);
  if (index < 0)
    return -1;

  // Pass the deadline of the thread to the L4 system
  l4_sched_param_t sp = l4_sched_param_by_type(Deadline, threads[index].dl, 0);
  // Let the L4 system tell the kernel to enqueue the thread in its (deadline-based) ready queue
  tag = l4_scheduler_run_thread(l4re_env()->scheduler, threads[index].thread_cap, &sp);
  if (l4_error(tag))
    return -2;

  return 0;
}

int edft_run_l4_threads(void)
{
  unsigned i;
  int ret;

  for (i = 0; i < count; i++)
  {
    ret = edft_run_l4_thread(threads[i].no);
    if (ret < 0)
      return ret;
  }

  return 0;
}

void edft_exit_thread(void)
{
  // Since none of the thread functions has a caller, there is no return address on the stack
  // An infinite sleep stops the execution of the thread
  while (1)
    l4_ipc_sleep(L4_IPC_NEVER);
}

static void edft_create_dependent_l4_threads_helper(Slist_elem *root, Edf_thread *pred, Edf_thread *suc)
{
  // Trying to insert the successive thread into the list ...
  if (!slist_is_elem(root, suc))
    // Thread has not been processed yet, so let's insert it at the back of the list
    root = slist_push_back(root, suc);
  else if (pred != NULL && slist_indexof(root, pred) > slist_indexof(root, suc))
    // Thread has already been processed, so we have to make sure that the successor follows its predecessor
    root = slist_move_after(root, pred, suc);
 
  // Iterate through all successors of the thread by depth-first search
  Edf_suc *it = suc->suc;
  while (it != NULL)
  {
    // Each call of edft_create_dependent_l4_threads_helper processes a vertex in the precedence graph,
    // represented by a predecessor-successor-tuple
    edft_create_dependent_l4_threads_helper(root, suc, it->data);
    it = it->next;
  }
}

void edft_create_dependent_l4_threads(Edf_thread *thread, unsigned offset, unsigned run)
{
  // We will iterate through the precedence graph and flat it to a list
  // The order of the items in this list will satisfy the precedence relations
  Slist_elem *head = slist_elem(thread);
  // edft_create_dependent_l4_threads_helper is handed over a pointer to the head of the list
  // and a vertex of the precedence graph, represented by a predecessor-successor-tuple
  // At the beginning (starting with the root vertex), there is no predecessor (therefore NULL)
  edft_create_dependent_l4_threads_helper(head, NULL, thread);

  printf("[edft_create_dependent_l4_threads] Sequential representation of the precedence graph:\n");

  Slist_elem *it = head;
  do
  {
    if (it != head)
      printf(" =>");
    printf(" %s", it->thread->name);

    // Calculate absolute deadline by adding current offset and worst case execution time
    offset += it->thread->wcet;
    it->thread->dl = offset;
    edft_create_l4_thread(it->thread, run);
  }
  while ((it = it->next) != NULL);

  printf("\n");
}

static int edft_calc_indent(unsigned level)
{
  if (level <= 1)
    return 0;

  return edft_calc_indent(level - 1) + (level - 1) + 3;
}

static void edft_print_precedence_graph_helper(Edf_thread *thread, unsigned level)
{
  if (level == 0)
    printf(ANSI_BOLD "\n=== PRECEDENCE GRAPH ===\n" ANSI_RESET);

  printf("\n");

  unsigned i;
  unsigned indent = edft_calc_indent(level);
  for (i = 0; i < indent; i++)
    printf(" ");
  for (i = 0; i < level; i++)
    printf("|");

  if (level > 0)
    printf("=> ");

  printf(ANSI_COLOR_BLUE    "#%u-%s"  ANSI_RESET "("
          ANSI_COLOR_YELLOW "wcet:%u" ANSI_RESET ";"
          ANSI_COLOR_RED    "dl:%u"   ANSI_RESET ") ",
          thread->no,
          thread->name,
          thread->wcet,
          thread->dl);
  Edf_suc *it = thread->suc;
  while (it != NULL)
  {
    edft_print_precedence_graph_helper(it->data, level + 1);
    it = it->next;
  }

  if (level == 0)
    printf(ANSI_BOLD "\n\n========================\n\n" ANSI_RESET);
}

void edft_print_precedence_graph(Edf_thread *thread)
{
  edft_print_precedence_graph_helper(thread, 0);
}
