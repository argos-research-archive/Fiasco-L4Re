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
#ifndef __LIBEDFT_SLIST_H__
#define __LIBEDFT_SLIST_H__

#include <l4/libedft/edft.h>

typedef struct Slist_elem Slist_elem;

typedef struct Slist_elem
{
  Edf_thread *thread;
  Slist_elem *next;
} Slist_elem;

Slist_elem * slist_elem(Edf_thread *thread);

int slist_indexof(Slist_elem *root, Edf_thread *thread);

unsigned slist_is_elem(Slist_elem *root, Edf_thread *thread);

Slist_elem * slist_push_back(Slist_elem *root, Edf_thread *thread);

Slist_elem * slist_remove(Slist_elem *root, Edf_thread *thread);

Slist_elem * slist_insert_after(Slist_elem *root, Edf_thread *pred, Edf_thread *suc_to_insert);

Slist_elem * slist_move_after(Slist_elem *root, Edf_thread *pred, Edf_thread *suc_to_move);

void slist_print(Slist_elem *root);

#endif /* __LIBEDFT_SLIST_H__ */
