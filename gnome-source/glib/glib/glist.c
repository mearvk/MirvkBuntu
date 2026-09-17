/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Vendored from GNOME/glib. Upstream path: glib/glist.c.
 */

#include "config.h"
#include "glist.h"
#include "gslice.h"
#include "gmessages.h"
#include "gtestutils.h"

#define _g_list_alloc()         g_slice_new (GList)
#define _g_list_alloc0()        g_slice_new0 (GList)
#define _g_list_free1(list)     g_slice_free (GList, list)

GList *
g_list_alloc (void)
{
  return _g_list_alloc0 ();
}

void
g_list_free (GList *list)
{
  g_slice_free_chain (GList, list, next);
}

void
g_list_free_1 (GList *list)
{
  _g_list_free1 (list);
}

void
g_list_free_full (GList *list, GDestroyNotify free_func)
{
  g_list_foreach (list, (GFunc) free_func, NULL);
  g_list_free (list);
}

GList *
g_list_append (GList *list, gpointer data)
{
  GList *new_list = _g_list_alloc ();
  GList *last;
  new_list->data = data;
  new_list->next = NULL;
  if (list)
    {
      last = g_list_last (list);
      last->next = new_list;
      new_list->prev = last;
      return list;
    }
  new_list->prev = NULL;
  return new_list;
}

GList *
g_list_prepend (GList *list, gpointer data)
{
  GList *new_list = _g_list_alloc ();
  new_list->data = data;
  new_list->next = list;
  if (list)
    {
      new_list->prev = list->prev;
      if (list->prev)
        list->prev->next = new_list;
      list->prev = new_list;
    }
  else
    new_list->prev = NULL;
  return new_list;
}

GList *
g_list_insert (GList *list, gpointer data, gint position)
{
  GList *new_list;
  GList *tmp_list;
  if (position < 0)
    return g_list_append (list, data);
  if (position == 0)
    return g_list_prepend (list, data);
  tmp_list = g_list_nth (list, position);
  if (!tmp_list)
    return g_list_append (list, data);
  new_list = _g_list_alloc ();
  new_list->data = data;
  new_list->prev = tmp_list->prev;
  tmp_list->prev->next = new_list;
  new_list->next = tmp_list;
  tmp_list->prev = new_list;
  return list;
}

GList *
g_list_concat (GList *list1, GList *list2)
{
  GList *tmp_list;
  if (list2)
    {
      tmp_list = g_list_last (list1);
      if (tmp_list)
        tmp_list->next = list2;
      else
        list1 = list2;
      list2->prev = tmp_list;
    }
  return list1;
}
