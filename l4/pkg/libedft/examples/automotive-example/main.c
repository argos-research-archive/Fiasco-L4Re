/**
 * \file
 * \brief  Example of executing safety-relevant operations in automotive systems
 *
 * This example shows how safety-relevant and non-safety-relevant operations are executed
 * in different tasks of automotive systems using the EDF scheduler.
 *
 * \date   Sep 21th 2014
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
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

char *thread_color[] = { ANSI_BOLD, ANSI_COLOR_BLUE, ANSI_COLOR_GREEN };

typedef struct Op
{
  unsigned op_code;
  char     op_name[40];
} Op;

Op       *ops[2];
unsigned  ops_count[2];

static void thread_printf(int no, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (no == -2)
      // Set formatting for errors
      printf(ANSI_BOLD ANSI_COLOR_RED);
    else
    {
      printf(thread_color[no + 1]);
      if (no == -1)
        // -1 is the code for our main thread
        printf("[main_thread] ");
      else
        printf("[%s] ", edft_obj(no)->name);
    }
    vprintf(format, args);
    printf(ANSI_RESET "\n");
    va_end(args);
}

static void execute_ops(l4_umword_t no)
{
  l4_msgtag_t tag;
  l4_umword_t label;
  l4_msg_regs_t mr;
  unsigned num_failed, i, j;

  thread_printf(no, "Ready\n");

  tag = l4_ipc_wait(l4_utcb(), &label, L4_IPC_NEVER);
  while (1)
  {
    if (l4_msgtag_has_error(tag))
    {
      thread_printf(no, "IPC receive error");
      return;
    }

    memcpy(&mr, l4_utcb_mr(), sizeof(mr));
    num_failed = 0;
 
    thread_printf(no, "Number of operations received: %d\n", l4_msgtag_words(tag));

    for (i = 0; i < l4_msgtag_words(tag); i++)
    {
      thread_printf(no, "Received at #%u: %lu", i, mr.mr[i]);
      for (j = 0; j < ops_count[no]; j++)
      {
        if (mr.mr[i] == ops[no][j].op_code)
        {
          thread_printf(no, "Operation %u: %s", ops[no][j].op_code, ops[no][j].op_name);
          break;
        }
        if (j + 1 == ops_count[no])
        {
          thread_printf(-2, "SECURITY VIOLATION: No operation with code %lu specified for this thread", mr.mr[i]);
          l4_utcb_mr()->mr[num_failed++] = mr.mr[i];
        }
      }
      printf("\n");
    }

    printf("\n");

    tag = l4_ipc_reply_and_wait(l4_utcb(), l4_msgtag(0, num_failed, 0, 0), &label, L4_IPC_NEVER);
  }
}

static int map_ops_to_task(unsigned no, l4_cap_idx_t task_cap)
{
  unsigned ops_size = ops_count[no] * sizeof(Op);
  unsigned fpage_size = (unsigned)log2(ceil((double)ops_size / L4_PAGESIZE) * L4_PAGESIZE);
  l4_msgtag_t tag;

  l4_touch_ro(ops[no], ops_size);
  tag = l4_task_map(task_cap, L4RE_THIS_TASK_CAP,
                     l4_fpage((unsigned long)ops[no], fpage_size, L4_FPAGE_RO),
                     l4_map_control((l4_umword_t)ops[no], 0, L4_MAP_ITEM_MAP));
  if (l4_error(tag))
    return -1;

  return 0;
}

static l4_msgtag_t send_ops(Edf_thread *thread, l4_umword_t *data, unsigned num)
{
  unsigned i;
  l4_msg_regs_t *mr = l4_utcb_mr();

  for (i = 0; i < num; i++)
    mr->mr[i] = data[i];

  return l4_ipc_call(thread->thread_cap, l4_utcb(), l4_msgtag(0, num, 0, 0), L4_IPC_NEVER);
}

int main(void)
{
  int ret;
  l4_msgtag_t tag;

  l4_debugger_set_object_name(l4re_env()->main_thread, "main_thread");

  // ops[0]: Critical operations
  ops_count[0] = 4;
  // We have to align the memory allocated for ops[0] to guarantee that it can be accessed properly
  ret = posix_memalign((void**)&ops[0], L4_PAGESIZE, ops_count[0] * sizeof(Op));
  if (ret != 0)
  {
    thread_printf(-1, "Could not alloc memory for ops[0]: %d", ret);
    return -1;
  }
  ops[0][0] = (Op){ 5, "ENABLE_ALL_WHEEL_DRIVE" };
  ops[0][1] = (Op){ 6, "DISABLE_ALL_WHEEL_DRIVE" };
  ops[0][2] = (Op){ 7, "ENABLE_ELECTRONIC_STABILITY_CONTROL" };
  ops[0][3] = (Op){ 8, "DISABLE_ELECTRONIC_STABILITY_CONTROL" };

  // ops[1]: Uncritical operations
  ops_count[1] = 4;
  // We have to align the memory allocated for ops[1] to guarantee that it can be accessed properly
  ret = posix_memalign((void**)&ops[1], L4_PAGESIZE, ops_count[1] * sizeof(Op));
  if (ret != 0)
  {
    thread_printf(-1, "Could not alloc memory for ops[1]: %d", ret);
    return -1;
  }
  ops[1][0] = (Op){ 10, "ENABLE_WIRELESS_LAN" }; 
  ops[1][1] = (Op){ 11, "DISABLE_WIRELESS_LAN" };
  ops[1][2] = (Op){ 12, "ENABLE_TRAFFIC_LIGHT_FEEDBACK" };
  ops[1][3] = (Op){ 13, "DISABLE_TRAFFIC_LIGHT_FEEDBACK" };

  // All critical operations are executed in a separate task: task_crit
  l4_cap_idx_t task_crit_cap = edft_create_l4_task(1, 1);
  if (l4_is_invalid_cap(task_crit_cap))
  {
    thread_printf(-1, "Could not create task_crit");
    return -1;
  }

  // We have to map the database containing the critical operations to task_crit,
  // making sure that this sort of operations is mapped to it ONLY
  if (map_ops_to_task(0, task_crit_cap) < 0)
  {
    thread_printf(-1, "Could not map ops[0] to task_crit");
    return -1;
  }

  // Initialize a new thread for the critical operations that is bound to to task_crit
  // Order: no - name - wcet - dl - func - arg - suc - thread cap - task cap
  Edf_thread thread0 = { 0, "thread0", 2, 5, execute_ops, 0, NULL, 0, task_crit_cap };

  // All uncritical operations are executed in another separate task: task_uncrit
  // Executing them in this main task is no good idea since the executing
  // thread would have access to all critical operations, too
  l4_cap_idx_t task_uncrit_cap = edft_create_l4_task(1, 1);
  if (l4_is_invalid_cap(task_uncrit_cap))
  {
    thread_printf(-1, "Could not create task_uncrit");
    return -1;
  }

  // We have to map the database containing the uncritical operations to task_uncrit,
  // making sure that this sort of operations is mapped to it ONLY
  if (map_ops_to_task(1, task_uncrit_cap) < 0)
  {
    thread_printf(-1, "Could not map ops[1] to task_uncrit");
    return -1;
  }

  // Initialize a new thread for the uncritical operations that is bound to to task_uncrit
  Edf_thread thread1 = { 1, "thread1", 4, 20, execute_ops, 1, NULL, 0, task_uncrit_cap };  

  ret = edft_create_l4_thread(&thread0, 0);
  if (ret < 0)
  {
    thread_printf(-1, "Could not create thread0: %d", ret);
    return -1;
  }

  ret = edft_create_l4_thread(&thread1, 0);
  if (ret < 0)
  {
    thread_printf(-1, "Could not create thread1: %d", ret);
    return -1;
  }

  ret = edft_run_l4_threads();
  if (ret < 0)
  {
    thread_printf(-1, "Could not pass the threads to the kernel: %d", ret);
    return -1;
  }

  // Communicate with thread0
  l4_umword_t data0[2] = { 6, 8 };
  tag = send_ops(&thread0, data0, 2);
  if (l4_msgtag_has_error(tag))
  {
    thread_printf(-1, "Could not send operations to thread0");
    return -1;
  }
  thread_printf(-1, "Finished IPC with thread0 successfully");
  if (l4_msgtag_words(tag) > 0)
    thread_printf(-1, "Number of invalid operations in thread0: %d", l4_msgtag_words(tag));

  printf("\n\n");

  sleep(5);

  // Communicate with thread1
  // 10 and 12 are valid operations for thread1, while 5 is not (ENABLE_ALL_WHEEL_DRIVE)
  // This critical operation is exclusive for thread0 that is bound to task_crit due to security reasons
  // So we will see a huge error message here
  l4_umword_t data1[3] = { 10, 12, 5 };
  tag = send_ops(&thread1, data1, 3);
  if (l4_msgtag_has_error(tag))
  {
    thread_printf(-1, "Could not send operations to thread1");
    return -1;
  }
  thread_printf(-1, "Finished IPC with thread1 successfully");
  if (l4_msgtag_words(tag) > 0)
    thread_printf(-1, "Number of invalid operations in thread1: %d", l4_msgtag_words(tag));

  return 0;
}
