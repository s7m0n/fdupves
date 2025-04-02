/*
 * This file is part of the fdupves package
 * Copyright (C) <2010>  <Alf>
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
/* @CFILE gui.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2013/01/16 11:35:09 Alf*/

#ifndef _FDUPVES_GUI_H_
#define _FDUPVES_GUI_H_

#include <gtk/gtk.h>

typedef struct file_node_s file_node;

struct gui_s
{
  gboolean quit;

  GtkWidget *widget;
  GtkWidget *main_vbox;
  GtkWidget *progress;

  GtkToolItem *but_add;
  GtkToolItem *but_find;
  GtkToolItem *but_del;

  GtkListStore *dir_store;

  GPtrArray *images;
  GPtrArray *videos;
  GPtrArray *audios;
  GPtrArray *ebooks;
  GSList *same_images;
  GSList *same_videos;
  GSList *same_audios;
  GSList *same_ebooks;
  GSList *same_list;

  GtkWidget *log_tree;
  GtkListStore *log_store;

  GtkWidget *result_tree;
  GtkTreeStore *result_store;
  GtkTreeSelection *result_select;
  GtkTreeIter *result_select_iters;
  file_node **result_file_nodes;

  GAsyncQueue *step_queue;
  GAsyncQueue *log_queue;
  guint queue_timer;
};

typedef struct gui_s gui_t;

gboolean gui_init (int argc, char *argv[]);

#endif
