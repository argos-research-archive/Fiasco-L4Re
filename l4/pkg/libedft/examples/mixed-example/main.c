/**
 * \file
 * \brief  Example of creating a mixed set of FP and EDF threads
 *
 * This example shows the coexistence of FP and EDF threads.
 *
 * \date   Sep 30th 2014
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

#include <stdio.h>
#include <unistd.h>

void thread_func(l4_umword_t);

char *thread_color[] = { ANSI_COLOR_BLUE, ANSI_COLOR_GREEN, ANSI_COLOR_MAGENTA, ANSI_COLOR_YELLOW };

void thread_func(l4_umword_t no)
{
  unsigned i, k;
  k = edft_obj(no)->wcet;
  for (i = 1; i <= k; i++)
    printf("%sThread %lu: Hello World %d!" ANSI_RESET "\n", thread_color[no % 4], no, i);
  edft_exit_thread();
}

int main(void)
{
  l4_debugger_set_object_name(l4re_env()->main_thread, "main_thread");

  l4_cap_idx_t task0_cap = edft_create_l4_task(1, 1);

  // Initialization: no - name - wcet - dl - func - arg - suc - thread cap - task cap
  Edf_thread thread0 = { 0, "thread0",  5, -1, thread_func, 0, NULL, 0, 0 };
  Edf_thread thread1 = { 1, "thread1", 10, 10, thread_func, 1, NULL, 0, 0 };
  Edf_thread thread2 = { 2, "thread2", 15, -1, thread_func, 2, NULL, 0, task0_cap };
  Edf_thread thread3 = { 3, "thread3", 20, -1, thread_func, 3, NULL, 0, task0_cap };

  edft_create_l4_thread(&thread0, 0);
  edft_create_l4_thread(&thread1, 0);
  edft_create_l4_thread(&thread2, 0);
  edft_create_l4_thread(&thread3, 0);

  // Set a breakpoint here to enable the user to take a look at the kernel's thread list with command 'lp'
  enter_kdebug();

  // Mixed FP and EDF example
  // The only thread that will be enqueued in the EDF ready queue is thread1
  edft_run_l4_thread(1);

  // All others will be enqueued in the ordinary FP ready queue
  l4_sched_param_t sp = l4_sched_param_by_type(Fixed_prio, 1, 0);

  if (l4_error(l4_scheduler_run_thread(l4re_env()->scheduler, thread3.thread_cap, &sp)))
    return -1;

  if (l4_error(l4_scheduler_run_thread(l4re_env()->scheduler, thread2.thread_cap, &sp)))
    return -1;

  if (l4_error(l4_scheduler_run_thread(l4re_env()->scheduler, thread0.thread_cap, &sp)))
    return -1;

  return 0;
}
