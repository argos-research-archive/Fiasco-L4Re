/**
 * \file
 * \brief  Singly linked list, optimized for storing the sequentialization of a precedence graph
 *
 * \date   Sep 14th 2014
 * \author Valentin Hauner <vh@hnr.name>
 */
/* 
 * (c) 2014 Valentin Hauner <vh@hnr.name>
 *
 * This file is distributed under the terms of the
 * GNU General Public License 3.
 * Please see the COPYING-GPL-3 file for details.
 */
#include <l4/libedft/slist.h>
#include <l4/libedft/edft.h>

#include <stdio.h>
#include <stdlib.h>

Slist_elem * slist_elem(Edf_thread *thread)
{
  Slist_elem *new = malloc(sizeof(Slist_elem));
  new->thread = thread;
  new->next   = NULL;
  return new;
}

int slist_indexof(Slist_elem *root, Edf_thread *thread)
{
  if (root == NULL)
    return -1;

  Slist_elem *it = root;
  unsigned index = 0;

  do
  {
    if (it->thread == thread)
      return index;
    index++;
  }
  while ((it = it->next) != NULL);

  return -1;
}

unsigned slist_is_elem(Slist_elem *root, Edf_thread *thread)
{
  if (slist_indexof(root, thread) > -1)
    return 1;

  return 0;
}

Slist_elem * slist_push_back(Slist_elem *root, Edf_thread *thread)
{
  Slist_elem *new = slist_elem(thread);

  if (root == NULL)
    return new;

  Slist_elem *it = root;
  while (it->next != NULL)
    it = it->next;
  it->next = new;

  return root;
}

Slist_elem * slist_remove(Slist_elem *root, Edf_thread *thread)
{
  Slist_elem *it = root;
  Slist_elem *prev = NULL;

  do
  {
    if (it->thread == thread)
    {
      if (prev == NULL)
        // Remove the head of the list
        return it->next;
      else
        prev->next = it->next;
    }
    prev = it;
  }
  while((it = it->next) != NULL);

  return root;
}

Slist_elem * slist_insert_after(Slist_elem *root, Edf_thread *pred, Edf_thread *suc_to_insert)
{
  Slist_elem *new = slist_elem(suc_to_insert);

  if (root == NULL)
    return new;

  Slist_elem *it = root;

  do
  {
    if (it->thread == pred)
    {
      Slist_elem *temp = it->next;
      it->next  = new;
      new->next = temp;
      return root;
    }
  }
  while ((it = it->next) != NULL);

  return root;
}

Slist_elem * slist_move_after(Slist_elem *root, Edf_thread *pred, Edf_thread *suc_to_move)
{
  return slist_insert_after(slist_remove(root, suc_to_move), pred, suc_to_move);
}

void slist_print(Slist_elem *root)
{
  printf("slist: ");
  Slist_elem *it = root;
  do
  {
    printf("%s => ", it->thread->name);
  }
  while ((it = it->next) != NULL);
  printf("end\n");
}
