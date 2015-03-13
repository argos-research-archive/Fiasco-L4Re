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
#ifndef __LIBEDFT_EDFT_H__
#define __LIBEDFT_EDFT_H__

#include <l4/sys/ipc.h>
#include <l4/sys/task.h>
#include <l4/sys/thread.h>

#include <l4/util/util.h>
#include <l4/sys/factory.h>
#include <l4/sys/utcb.h>
#include <l4/re/env.h>
#include <l4/re/c/util/cap_alloc.h>

#include <l4/sys/kdebug.h>
#include <l4/sys/debugger.h>

#define THREAD_MAX_NUM     20
#define THREAD_STACK_SIZE  4096

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_BOLD          "\x1b[1m"
#define ANSI_RESET         "\x1b[0m"

typedef struct Edf_thread Edf_thread;
typedef struct Edf_suc    Edf_suc;

typedef struct Edf_thread
{
  unsigned     no;         // Unique number
  char         name[20];   // Name
  unsigned     wcet;       // Worst case execution time
  unsigned     dl;         // Deadline
  void        *func;       // EIP (aka: thread function)
  l4_umword_t  arg;        // Argument to pass to the thread function
  Edf_suc     *suc;        // Successor(s) in precedence graph
  l4_cap_idx_t thread_cap; // L4 thread capability
  l4_cap_idx_t task_cap;   // L4 task capability
} Edf_thread;

typedef struct Edf_suc
{
  Edf_thread *data;
  Edf_suc    *next;
} Edf_suc;

/**
 * \brief Returns the index of the given thread at which it is stored in the thread array
 *
 * \param no Number of the thread
 * \return Index of the thread at which it is stored in the thread array
 */
int edft_no2index(unsigned no);

/**
 * \brief Returns the Edf_thread object with the given number stored in the thread array
 *
 * \param no Number of the thread
 * \return Edf_thread object stored in the thread array
 */
Edf_thread * edft_obj(unsigned no);

/**
 * \brief Creates a new L4 task with a basic set of capabilities and returns its capability index
 *
 * \param map_code 1 for mapping the code segment of the current task to the new task, 0 otherwise
 * \param map_data 1 for mapping the data segment of the current task to the new task, 0 otherwise
 * \return Capability index > 0 for success, error code < 0 for failure
 */
l4_cap_idx_t edft_create_l4_task(unsigned map_code, unsigned map_data);

/**
 * \brief Copies the given Edf_thread object into the thread array and creates a corresponding L4 thread
 *
 * \param thread Edf_thread object
 * \param run    1 for telling the kernel to enqueue the thread at once, 0 otherwise
 * \return Thread index > 0 for success, error code < 0 for failure
 */
int edft_create_l4_thread(Edf_thread *thread, unsigned run);

/**
 * \brief Tells the L4 system to enqueue the given thread
 *
 * \param no Number of the thread
 * \return 0 for success, !0 for failure
 */
int edft_run_l4_thread(unsigned no);

/**
 * \brief Tells the L4 system to enqueue all threads stored in the thread array
 *
 * \return 0 for success, !0 for failure
 */
int edft_run_l4_threads(void);

/**
 * \brief Stops the execution of that thread this function is called by
 */
void edft_exit_thread(void);

/**
 * \brief Assigns deadlines to the threads according to their precedence relations
 *
 * \param thread Edf_thread_object to start (usually root node)
 * \param offset Offset to add to all calculated deadlines
 * \param run    1 for telling the kernel to enqueue the threads at once, 0 otherwise
 */
void edft_create_dependent_l4_threads(Edf_thread *thread, unsigned offset, unsigned run);

/**
 * \brief Prints a graph showing the precedence relations of the threads
 *
 * \param thread Edf_thread_object to start (usually root node)
 */
void edft_print_precedence_graph(Edf_thread *thread);

#endif /* __LIBEDFT_EDFT_H__ */
