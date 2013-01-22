/*
 * This file is part of the fdupves package
 * Copyright (C) <2008> Alf
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
/* @CFILE ihash.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "hash.h"
#include "ini.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

static hash_t pixbuf_hash (GdkPixbuf *);

hash_t
file_hash (const char *file)
{
  GdkPixbuf *buf;
  GError *err;
  hash_t h;

  err = NULL;
  buf = gdk_pixbuf_new_from_file_at_scale (file,
					   g_ini->hash_size[0],
					   g_ini->hash_size[1],
					   FALSE,
					   &err);
  if (err)
    {
      g_error ("Load file: %s to pixbuf failed: %s", file, err->message);
      g_error_free (err);
      return 0;
    }

  h = pixbuf_hash (buf);
  g_object_unref (buf);

  return h;
}

hash_t
buffer_hash (const char *buffer, int size)
{
  GdkPixbuf *buf;
  GError *err;
  hash_t h;

  err = NULL;
  buf = gdk_pixbuf_new_from_inline (size,
				    (const guint8 *) buffer,
				    FALSE,
				    &err);
  if (err)
    {
      g_error ("Load inline data to pixbuf failed: %s", err->message);
      g_error_free (err);
      return 0;
    }

  h = pixbuf_hash (buf);
  g_object_unref (buf);

  return h;
}

static hash_t
pixbuf_hash (GdkPixbuf *pixbuf)
{
  int width, height, rowstride, n_channels;
  guchar *pixels, *p;
  guint *grays, sum, avg;
  size_t x, y, off;
  hash_t hash;

  n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
  g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  grays = g_new0 (guint, width * height);
  off = 0;
  for (y = 0; y < width; ++ y)
    {
      for (x = 0; x < height; ++ x)
	{
	  p = pixels + y * rowstride + x * n_channels;
	  grays[off] = (p[0] * 30 + p[1] * 59 + p[2] * 11) / 100;
	  ++ off;
	}
    }

  sum = 0;
  for (x = 0; x < off; ++ x)
    {
      sum += grays[x];
    }
  avg = sum / off;

  hash = 0;
  for (x = 0; x < off; ++ x)
    {
      if (grays[x] >= avg)
	{
	  hash |= (x << x);
	}
    }

  g_free (grays);

  return hash;
}

int
hash_cmp (hash_t a, hash_t b)
{
  hash_t c;
  int cmp;

  if (!a || !b)
    {
      return g_ini->hash_size[0] * g_ini->hash_size[1]; /* max invalid distance */
    }

  c = a ^ b;
  for (cmp = 0; c; c = c >> 1)
    {
      if (c & 1)
	{
	  ++ cmp;
	}
      c = c >> 1;
    }

  return cmp;
}

hash_t
video_time_hash (const char *file, double time)
{
  hash_t v;

  v = 0;

  return v;
}