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
#include "audio.h"
#include "cache.h"
#include "find.h"
#include "image.h"
#include "ini.h"
#include "util.h"
#include "video.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

const char *hash_phrase[] = {
  "image_hash",
  "image_phash",
  "audio_hash",
};

static hash_t pixbuf_hash (GdkPixbuf *);

#define FDUPVES_HASH_LEN 8

hash_t
image_file_hash (const char *file)
{
  GdkPixbuf *buf;
  hash_t h;
  GError *err;

  if (g_cache)
    {
      if (cache_get (g_cache, file, 0, FDUPVES_IMAGE_HASH, &h))
        {
          return h;
        }
    }

  buf = fdupves_gdkpixbuf_load_file_at_size (file, FDUPVES_HASH_LEN,
                                             FDUPVES_HASH_LEN, &err);
  if (err)
    {
      g_warning ("Load file: %s to pixbuf failed: %s", file, err->message);
      g_error_free (err);
      return 0;
    }

  h = pixbuf_hash (buf);
  g_object_unref (buf);

  if (g_cache)
    {
      if (h)
        {
          cache_set (g_cache, file, 0, FDUPVES_IMAGE_HASH, h);
        }
    }

  return h;
}

hash_t
image_buffer_hash (const char *buffer, int size)
{
  GdkPixbuf *buf;
  GError *err;
  hash_t h;

  err = NULL;
  buf = gdk_pixbuf_new_from_data ((const guchar *)buffer, GDK_COLORSPACE_RGB,
                                  FALSE, 8, FDUPVES_HASH_LEN, FDUPVES_HASH_LEN,
                                  FDUPVES_HASH_LEN * 3, NULL, &err);
  if (err)
    {
      g_warning ("Load inline data to pixbuf failed: %s", err->message);
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
  int *grays, sum, avg, x, y, off;
  hash_t hash;

  n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
  g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  grays = g_new0 (int, width *height);
  off = 0;
  for (y = 0; y < height; ++y)
    {
      for (x = 0; x < width; ++x)
        {
          p = pixels + y * rowstride + x * n_channels;
          grays[off] = (p[0] * 30 + p[1] * 59 + p[2] * 11) / 100;
          ++off;
        }
    }

  sum = 0;
  for (x = 0; x < off; ++x)
    {
      sum += grays[x];
    }
  avg = sum / off;

  hash = 0;
  for (x = 0; x < off; ++x)
    {
      if (grays[x] >= avg)
        {
          hash |= ((hash_t)1 << x);
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
      return FDUPVES_HASH_LEN * FDUPVES_HASH_LEN; /* max invalid distance */
    }

  c = a ^ b;
  switch (g_ini->compare_area)
    {
    case FD_COMPARE_TOP:
      c = c & 0xFFFFFF00ULL;
      break;

    case FD_COMPARE_BOTTOM:
      c = c & 0x00FFFFFFULL;
      break;

    case FD_COMPARE_LEFT:
      c = c & 0xFCFCFCFCULL;
      break;

    case FD_COMPARE_RIGHT:
      c = c & 0x3F3F3F3FULL;
      break;

    default:
      break;
    }
  for (cmp = 0; c; c = c >> 1)
    {
      if (c & 1)
        {
          ++cmp;
        }
    }

  return cmp;
}

hash_t
video_time_hash (const char *file, float offset)
{
  hash_t h;
  gchar *buffer;
  gsize len;

  if (g_cache)
    {
      if (cache_get (g_cache, file, offset, FDUPVES_IMAGE_HASH, &h))
        {
          return h;
        }
    }

  len = FDUPVES_HASH_LEN * FDUPVES_HASH_LEN * 3;
  buffer = g_malloc (len);
  g_return_val_if_fail (buffer, 0);

  video_time_screenshot (file, offset, FDUPVES_HASH_LEN, FDUPVES_HASH_LEN,
                         buffer, len);

  h = image_buffer_hash (buffer, len);
  g_free (buffer);

  if (g_cache)
    {
      if (h)
        {
          cache_set (g_cache, file, offset, FDUPVES_IMAGE_HASH, h);
        }
    }

  return h;
}

hash_array_t *
audio_hashes (const char *path)
{
  hash_array_t *hashArray;

  if (g_cache)
    {
      if (cache_gets (g_cache, path, 0xFFFF, &hashArray))
        {
          g_debug ("got %s cached peak hashes: %lu", path,
                   hash_array_size (hashArray));
          return hashArray;
        }
    }

  g_debug ("get %s peak hashes ...", path);
  hashArray = audio_fingerprint (path);
  g_debug ("get %s peak hashes: %lu", path,
           hashArray ? hash_array_size (hashArray) : 0);

  if (g_cache)
    {
      if (hashArray)
        {
          cache_sets (g_cache, path, 0xFFFF, hashArray);
        }
    }

  return hashArray;
}

hash_array_t *
hash_array_new ()
{
  hash_array_t *hashArray;

  hashArray = g_new0 (hash_array_t, 1);
  g_return_val_if_fail (hashArray, NULL);

  hashArray->array = g_ptr_array_new_with_free_func (g_free);
  if (hashArray->array == NULL)
    {
      g_free (hashArray);
      return NULL;
    }

  return hashArray;
}

void
hash_array_free (hash_array_t *hashArray)
{
  g_ptr_array_free (hashArray->array, TRUE);
  g_free (hashArray);
}

gsize
hash_array_size (hash_array_t *hashArray)
{
  return hashArray->array->len;
}

void *
hash_array_index (hash_array_t *hashArray, int index)
{
  void *hashp = g_ptr_array_index (hashArray->array, index);
  return hashp;
}

void
hash_array_append (hash_array_t *hashArray, void *hash, size_t size)
{
  hash_t *nhash = g_malloc (size);
  g_return_if_fail (nhash);
  memcpy (nhash, hash, size);
  g_ptr_array_add (hashArray->array, nhash);
}
