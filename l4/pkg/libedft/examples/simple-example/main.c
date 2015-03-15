/**
 * \file
 * \brief  Example of creating threads for the EDF scheduler
 *
 * This example shows how threads owning a deadline are created for the EDF scheduler.
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

#include <stdio.h>

void thread_func(l4_umword_t);

char *thread_color[] = { ANSI_COLOR_BLUE, ANSI_COLOR_GREEN, ANSI_COLOR_MAGENTA, ANSI_COLOR_YELLOW, ANSI_COLOR_CYAN };

void thread_func(l4_umword_t no)
{
  unsigned i, k;
  k = edft_obj(no)->wcet;
  for (i = 1; i <= k; i++)
    printf("%sThread %lu: Hello World %d!" ANSI_RESET "\n", thread_color[no % 5], no, i);
  edft_exit_thread();
}

int main(void)
{
  l4_debugger_set_object_name(l4re_env()->main_thread, "main_thread");

  l4_cap_idx_t task0_cap = edft_create_l4_task(1, 1);

  // Initialization: no - name - wcet - dl - func - arg - suc - thread cap - task cap
  Edf_thread thread0 = { 0, "thread0",  5, -1, thread_func, 0, NULL, 0, 0 };
  Edf_thread thread1 = { 1, "thread1", 10, -1, thread_func, 1, NULL, 0, 0 };
  Edf_thread thread2 = { 2, "thread2", 15, -1, thread_func, 2, NULL, 0, 0 };
  Edf_thread thread3 = { 3, "thread3", 20, -1, thread_func, 3, NULL, 0, task0_cap };
  Edf_thread thread4 = { 4, "thread4", 25, -1, thread_func, 4, NULL, 0, task0_cap };

  // Set up precedence relations
  Edf_suc suc0f = { &thread2, NULL };
  Edf_suc suc0  = { &thread1, &suc0f };

  Edf_suc suc1  = { &thread3, NULL };
  Edf_suc suc2  = { &thread4, NULL };
  Edf_suc suc3  = { &thread4, NULL };

  thread0.suc = &suc0;
  thread1.suc = &suc1;
  thread2.suc = &suc2;
  thread3.suc = &suc3;

  // Create threads according to their precedence relations
  edft_create_dependent_l4_threads(&thread0, 0, 0);

  // Print precedence graph
  edft_print_precedence_graph(&thread0);

  // Set a breakpoint here to enable the user to take a look at the kernel's thread list with command 'lp'
  enter_kdebug();

  // Pass the threads to the kernel
  edft_run_l4_threads();

  /*
  // Example for creating threads manually (w/o precedence relations)
  // Don't forget to set the deadlines of the threads manually

  edft_create_l4_thread(thread0, 1);
  edft_create_l4_thread(thread1, 1);
  edft_create_l4_thread(thread2, 1);
  edft_create_l4_thread(thread3, 1);
  */

  return 0;
}
