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
/* @CFILE ini.c
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2013/01/16 12:03:42 Alf*/

#include "ini.h"
#include "util.h"

#include <glib.h>

ini_t *g_ini;

static void ini_free (ini_t *);

ini_t *
ini_new ()
{
  ini_t *ini;

  const gchar *const isuffix
      = "bmp,gif,jpeg,jpg,jpe,png,pcx,pnm,tif,tga,xpm,ico,cur,ani";
  const gchar *const vsuffix = "avi,mp4,mpg,rmvb,rm,mov,mkv,m4v,mpg,mpeg,vob,"
                               "asf,wmv,3gp,flv,mod,swf,mts,m2ts,ts";
  const gchar *const asuffix = "mp3,wma,wav,ogg,amr,m4a,mka,aac";
  const gchar *const esuffix = "pdf,xps,epub,mobi,fb2,cbz,svg,txt";

  ini = g_malloc0 (sizeof (ini_t));
  g_return_val_if_fail (ini, NULL);

  if (g_ini == NULL)
    {
      g_ini = ini;
    }

  ini->keyfile = g_key_file_new ();

  ini->verbose = FALSE;

  ini->proc_image = FALSE;
  ini->image_suffix = g_strsplit (isuffix, ",", -1);

  ini->proc_video = TRUE;
  ini->video_suffix = g_strsplit (vsuffix, ",", -1);

  ini->proc_audio = TRUE;
  ini->audio_suffix = g_strsplit (asuffix, ",", -1);

  ini->proc_ebook = FALSE;
  ini->ebook_suffix = g_strsplit (esuffix, ",", -1);

  ini->proc_other = FALSE;

  ini->compare_area = 0;

  ini->filter_time_rate = 0;

  ini->compare_count = 4;

  ini->same_image_distance = 6;
  ini->same_video_distance = 8;
  ini->same_audio_distance = 2;

  ini->threads_count = 1;
  ini->ebook_viewer = g_strdup ("apvlv");

  ini->thumb_size[0] = 512;
  ini->thumb_size[1] = 384;

  ini->video_timers[0][0] = 10;
  ini->video_timers[0][1] = 120;
  ini->video_timers[0][2] = 4;
  ini->video_timers[1][0] = 60;
  ini->video_timers[1][1] = 600;
  ini->video_timers[1][2] = 25;
  ini->video_timers[2][0] = 300;
  ini->video_timers[2][1] = 3000;
  ini->video_timers[2][2] = 120;
  ini->video_timers[3][0] = 1500;
  ini->video_timers[3][1] = 28800;
  ini->video_timers[3][2] = 600;
  ini->video_timers[4][0] = 0;

  ini->directories = NULL;

  ini->cache_file
      = g_build_filename (g_get_home_dir (), ".cache", "fdupves", NULL);

  return ini;
}

ini_t *
ini_new_with_file (const gchar *file)
{
  ini_t *ini;

  ini = ini_new ();
  g_return_val_if_fail (ini, NULL);

  if (ini_load (ini, file) == FALSE)
    {
      ini_free (ini);
      return NULL;
    }

  return ini;
}

#define __ini_field(a, b) ini->a##b
#define __ini_load_type(type)                                                 \
  do                                                                          \
    {                                                                         \
      if (g_key_file_has_key (ini->keyfile, "_", "proc_" #type, NULL))        \
        {                                                                     \
          __ini_field (proc_, type) = g_key_file_get_boolean (                \
              ini->keyfile, "_", "proc_" #type, NULL);                        \
        }                                                                     \
      tmpstr = g_key_file_get_string (ini->keyfile, "_", #type "_ext", NULL); \
      if (tmpstr != NULL)                                                     \
        {                                                                     \
          g_strfreev (__ini_field (type, _suffix));                           \
          __ini_field (type, _suffix) = g_strsplit (tmpstr, ",", 0);          \
        }                                                                     \
    }                                                                         \
  while (0)

gboolean
ini_load (ini_t *ini, const gchar *file)
{
  gchar *path, *tmpstr;
  gint level, count;
  GError *err;

  path = fd_realpath (file);
  g_return_val_if_fail (path, FALSE);

  err = NULL;
  g_key_file_load_from_file (ini->keyfile, path, 0, &err);
  g_free (path);
  if (err)
    {
      g_warning ("load configuration file: %s failed: %s", file, err->message);
      g_error_free (err);
      return FALSE;
    }

  __ini_load_type (image);
  __ini_load_type (video);
  __ini_load_type (audio);
  __ini_load_type (ebook);

  level = g_key_file_get_integer (ini->keyfile, "_", "same_image_rate", &err);
  if (err)
    {
      g_warning ("configuration file: %s value error: %s, set as default.",
                 file, err->message);
      g_error_free (err);
    }
  else
    {
      ini->same_image_distance = SAME_RATE_MAX - level;
    }

  level = g_key_file_get_integer (ini->keyfile, "_", "same_video_rate", &err);
  if (err)
    {
      g_warning ("configuration file: %s value error: %s, set as default.",
                 file, err->message);
      g_error_free (err);
    }
  else
    {
      ini->same_video_distance = SAME_RATE_MAX - level;
    }
  level = g_key_file_get_integer (ini->keyfile, "_", "same_audio_rate", &err);
  if (err)
    {
      g_warning ("configuration file: %s value error: %s, set as default.",
                 file, err->message);
      g_error_free (err);
    }
  else
    {
      ini->same_audio_distance = SAME_RATE_MAX - level;
    }

  count = g_key_file_get_integer (ini->keyfile, "_", "threads_count", &err);
  if (err)
    {
      g_warning ("configuration file: %s value error: %s, set as default.",
                 file, err->message);
      g_error_free (err);
    }
  else
    {
      ini->threads_count = count;
    }

  tmpstr = g_key_file_get_string (ini->keyfile, "_", "ebook_viewer", &err);
  if (err)
    {
      g_warning ("configuration file: %s value error: %s, set as default.",
                 file, err->message);
      g_error_free (err);
    }
  else
    {
      g_free (ini->ebook_viewer);
      ini->ebook_viewer = g_strdup (tmpstr);
    }

  if (g_key_file_has_key (ini->keyfile, "_", "compare_area", NULL))
    {
      ini->compare_area
          = g_key_file_get_integer (ini->keyfile, "_", "compare_area", NULL);
    }

  if (g_key_file_has_key (ini->keyfile, "_", "filter_time_rate", NULL))
    {
      ini->filter_time_rate = g_key_file_get_integer (
          ini->keyfile, "_", "filter_time_rate", NULL);
    }

  if (g_key_file_has_key (ini->keyfile, "_", "compare_count", NULL))
    {
      ini->compare_count
          = g_key_file_get_integer (ini->keyfile, "_", "compare_count", NULL);
    }

  if (g_key_file_has_key (ini->keyfile, "_", "directories", NULL))
    {
      ini->directories = g_key_file_get_string_list (
          ini->keyfile, "_", "directories", &ini->directory_count, NULL);
    }

  return TRUE;
}

#define __ini_save_type(type)                                                 \
  do                                                                          \
    {                                                                         \
      g_key_file_set_boolean (ini->keyfile, "_", "proc_" #type,               \
                              __ini_field (proc_, type));                     \
      tmpstr = g_strjoinv (",", __ini_field (type, _suffix));                 \
      if (tmpstr)                                                             \
        {                                                                     \
          g_key_file_set_string (ini->keyfile, "_", #type "_ext", tmpstr);    \
          g_free (tmpstr);                                                    \
        }                                                                     \
    }                                                                         \
  while (0)

gboolean
ini_save (ini_t *ini, const gchar *file)
{
  gchar *data, *path, *tmpstr;
  gsize len;

  path = fd_realpath (file);
  g_return_val_if_fail (path, FALSE);

  __ini_save_type (image);
  __ini_save_type (video);
  __ini_save_type (audio);
  __ini_save_type (ebook);

  g_key_file_set_integer (ini->keyfile, "_", "same_image_rate",
                          SAME_RATE_MAX - ini->same_image_distance);
  g_key_file_set_integer (ini->keyfile, "_", "same_video_rate",
                          SAME_RATE_MAX - ini->same_video_distance);
  g_key_file_set_integer (ini->keyfile, "_", "same_audio_rate",
                          SAME_RATE_MAX - ini->same_audio_distance);

  g_key_file_set_integer (ini->keyfile, "_", "threads_count",
                          ini->threads_count);

  g_key_file_set_string (ini->keyfile, "_", "ebook_viewer", ini->ebook_viewer);

  g_key_file_set_integer (ini->keyfile, "_", "compare_area",
                          ini->compare_area);
  g_key_file_set_integer (ini->keyfile, "_", "filter_time_rate",
                          ini->filter_time_rate);
  g_key_file_set_integer (ini->keyfile, "_", "compare_count",
                          ini->compare_count);

  g_key_file_set_string_list (ini->keyfile, "_", "directories",
                              (const gchar *const *)ini->directories,
                              ini->directory_count);

  data = g_key_file_to_data (ini->keyfile, &len, NULL);
  g_file_set_contents (path, data, len, NULL);
  g_free (path);
  g_free (data);

  return TRUE;
}

static void
ini_free (ini_t *ini)
{
  if (g_ini == ini)
    {
      g_ini = NULL;
    }

  g_key_file_free (ini->keyfile);
  g_free (ini);
}
