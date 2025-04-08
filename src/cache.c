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
/* @CFILE cache.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "cache.h"
#include "audio.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <errno.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cache_t *g_cache;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef WIN32
#define strtouq _strtoui64
#endif

struct cache_s
{
  sqlite3 *db;
  gchar *file;
};

static gboolean cache_exec (cache_t *cache,
                            int (*cb) (void *, int, char **, char **),
                            void *arg, const char *fmt, ...);

const char *init_text
    = "create table media(id INTEGER PRIMARY KEY AUTOINCREMENT, path text, "
      "size bigint, mtime bigint);"
      "create table hash(id INTEGER PRIMARY KEY AUTOINCREMENT, media_id "
      "integer, alg int, offset real, hash varchar(32));"
      "create unique index index_path on media (path);"
      "create table ebook(id INTEGER PRIMARY KEY AUTOINCREMENT, media_id "
      "integer, hash varchar(32), title "
      "varchar(1024), author varchar(256), "
      "producer varchar(256), pubdate_year integer, pubdate_mon integer, "
      "pubdate_day integer, isbn varchar(128));";

static void
cache_init (cache_t *cache)
{
  cache_exec (cache, NULL, NULL, init_text);
}

cache_t *
cache_open (const gchar *file)
{
  cache_t *cache;
  gchar *dirname;
  gboolean needInit;

  cache = g_malloc (sizeof (cache_t));
  g_return_val_if_fail (cache, NULL);

  cache->file = g_strdup (file);

  needInit = FALSE;
  if (g_file_test (file, G_FILE_TEST_EXISTS) == FALSE)
    {
      needInit = TRUE;
      dirname = g_path_get_dirname (file);
      g_mkdir_with_parents (dirname, 0755);
      g_free (dirname);
    }

  if (sqlite3_open (file, &cache->db) != 0)
    {
      g_warning ("Open cache file: %s failed:%s.", file, strerror (errno));
      g_free (cache);
      return NULL;
    }

  if (needInit)
    {
      cache_init (cache);
    }

  if (g_cache == NULL)
    {
      g_cache = cache;
    }

  return cache;
}

void
cache_close (cache_t *cache)
{
  sqlite3_close (cache->db);
  g_free (cache->file);
  g_free (cache);
}

static gboolean
cache_exec (cache_t *cache, int (*cb) (void *, int, char **, char **),
            void *arg, const char *fmt, ...)
{
  int rc;
  va_list ap;
  char text[1024], *errMsg;

  va_start (ap, fmt);
  vsnprintf (text, sizeof text, fmt, ap);
  va_end (ap);

  rc = sqlite3_exec (cache->db, text, cb, arg, &errMsg);
  if (rc != SQLITE_OK)
    {
      g_warning ("SQL error: %s in [%s]", errMsg, text);
      sqlite3_free (errMsg);
      return FALSE;
    }

  return TRUE;
}

static int
get_id_callback (void *para, int n_column, char **column_value,
                 char **column_name)
{
  int *idp = (int *)para;
  if (column_value[0] != NULL)
    *idp = atoi (column_value[0]);
  return 0;
}

static int
get_hash_callback (void *para, int n_column, char **column_value,
                   char **column_name)
{
  hash_t *hp = (hash_t *)para;
  if (column_value[0] != NULL)
    {
      *hp = strtoull (column_value[0], NULL, 10);
    }
  return 0;
}

static int
get_hash_array_callback (void *para, int n_column, char **column_value,
                         char **column_name)
{
  audio_peak_hash hash;
  hash_array_t **pHashArray = (hash_array_t **)para;

  if (column_value[1] != NULL)
    {
      if (*pHashArray == NULL)
        {
          *pHashArray = hash_array_new ();
        }
      if (*pHashArray == NULL)
        {
          g_warning ("hash array new error: %s", strerror (errno));
          return -1;
        }

      hash.offset = (int)strtof (column_value[0], NULL);
      snprintf (hash.hash, sizeof hash.hash, "%s", column_value[1]);
      hash_array_append (*pHashArray, &hash, sizeof (audio_peak_hash));
    }

  return 0;
}

static int
cache_get_media_id (cache_t *cache, const gchar *file)
{
  int media_id;
  gboolean ret;
  const gchar *p;
  gchar trans_file[PATH_MAX];
  gsize trans_len;

  for (trans_len = 0, p = file; *p && trans_len + 2 < sizeof (trans_file);
       trans_len++, p++)
    {
      trans_file[trans_len] = *p;
      if (*p == '\'')
        {
          trans_file[trans_len + 1] = *p;
          trans_len++;
        }
    }
  g_return_val_if_fail (trans_len + 2 < sizeof (trans_file), -1);
  trans_file[trans_len] = '\0';

  media_id = -1;

  ret = cache_exec (cache, get_id_callback, &media_id,
                    "select id from media where path='%s';", trans_file);
  g_return_val_if_fail (ret, -1);

  if (media_id == -1)
    {
      GStatBuf buf[1];
      if (g_stat (file, buf) != 0)
        {
          g_warning ("stat error: %s", strerror (errno));
          return -1;
        }

#if WIN32
      ret = cache_exec (
          cache, NULL, NULL,
          "insert into media(path, size, mtime) values('%s', %ld, %ld);",
          trans_file, buf->st_size, buf->st_mtime);
#else
      ret = cache_exec (
          cache, NULL, NULL,
          "insert into media(path, size, mtime) values('%s', %ld, %ld);",
          trans_file, buf->st_size, buf->st_mtim.tv_sec);
#endif
      g_return_val_if_fail (ret, -1);
    }

  ret = cache_exec (cache, get_id_callback, &media_id,
                    "select id from media where path='%s';", trans_file);
  g_return_val_if_fail (ret, -1);

  return media_id;
}

gboolean
cache_get (cache_t *cache, const gchar *file, float off, int alg, hash_t *hp)
{
  int media_id;
  gboolean ret;

  media_id = cache_get_media_id (cache, file);
  g_return_val_if_fail (media_id != -1, FALSE);

  *hp = 0;
  ret = cache_exec (
      cache, get_hash_callback, hp,
      "select hash from hash where offset=%f and alg=%d and media_id=%d", off,
      alg, media_id);
  g_return_val_if_fail (ret, FALSE);

  return *hp != 0;
}

gboolean
cache_set (cache_t *cache, const gchar *file, float off, int alg, hash_t h)
{
  int media_id;
  gboolean ret;

  media_id = cache_get_media_id (cache, file);
  g_return_val_if_fail (media_id != -1, FALSE);

  ret = cache_exec (cache, NULL, NULL,
                    "insert into hash(media_id, offset, alg, hash) values(%d, "
                    "%f, %d, '%lld')",
                    media_id, off, alg, h);
  g_return_val_if_fail (ret, FALSE);

  return TRUE;
}

gboolean
cache_gets (cache_t *cache, const gchar *file, int alg,
            hash_array_t **pHashArray)
{
  int media_id;
  gboolean ret;

  media_id = cache_get_media_id (cache, file);
  g_return_val_if_fail (media_id != -1, FALSE);

  *pHashArray = NULL;
  ret = cache_exec (
      cache, get_hash_array_callback, pHashArray,
      "select offset, hash from hash where alg = %d and media_id = %d", alg,
      media_id);
  g_return_val_if_fail (ret, FALSE);

  return (*pHashArray != NULL);
}

gboolean
cache_sets (cache_t *cache, const gchar *file, int alg,
            hash_array_t *hashArray)
{
  int media_id, i;
  audio_peak_hash *hash;
  gboolean ret;

  media_id = cache_get_media_id (cache, file);
  g_return_val_if_fail (media_id != -1, FALSE);

  for (i = 0; i < hash_array_size (hashArray); ++i)
    {
      hash = hash_array_index (hashArray, i);
      ret = cache_exec (cache, NULL, NULL,
                        "insert into hash(media_id, offset, alg, hash) "
                        "values(%d, %d, %d, '%s')",
                        media_id, hash->offset, alg, hash->hash);
      g_return_val_if_fail (ret, FALSE);
    }

  return TRUE;
}

gboolean
cache_set_ebook (cache_t *cache, const char *file, ebook_hash_t *h)
{
  int media_id;

  media_id = cache_get_media_id (cache, file);
  g_return_val_if_fail (media_id != -1, FALSE);

  return cache_exec (
      cache, NULL, NULL,
      "insert into ebook(media_id, hash, title, author, producer, "
      "pubdate_year, pubdate_mon, pubdate_day, isbn) values(%d, "
      "%lld, '%s', '%s', '%s', %d, %d, %d, '%s')",
      media_id, h->cover_hash, h->title, h->author, h->producer,
      h->public_date.year, h->public_date.month, h->public_date.day, h->isbn);
}

struct ebook_result
{
  int got;
  ebook_hash_t *hash;
};

static int
get_ebook_callback (void *para, int n_column, char **column_value,
                    char **column_name)
{
  struct ebook_result *result = para;
  ebook_hash_t *h = result->hash;
  result->got = 1;

  if (column_value[2] != NULL)
    {
      h->cover_hash = strtoull (column_value[2], NULL, 10);
    }
  if (column_value[3] != NULL)
    {
      snprintf (h->title, sizeof (h->title), "%s", column_value[3]);
    }
  if (column_value[4] != NULL)
    {
      snprintf (h->author, sizeof (h->author), "%s", column_value[4]);
    }
  if (column_value[5] != NULL)
    {
      snprintf (h->producer, sizeof (h->producer), "%s", column_value[5]);
    }
  if (column_value[6] != NULL)
    {
      h->public_date.year = strtol (column_value[6], NULL, 10);
    }
  if (column_value[7] != NULL)
    {
      h->public_date.month = strtol (column_value[7], NULL, 10);
    }
  if (column_value[8] != NULL)
    {
      h->public_date.day = strtol (column_value[8], NULL, 10);
    }
  if (column_value[9] != NULL)
    {
      snprintf (h->isbn, sizeof (h->isbn), "%s", column_value[9]);
    }
  return 0;
}

gboolean
cache_get_ebook (cache_t *cache, const char *file, ebook_hash_t *h)
{
  struct ebook_result result[1];
  int media_id;
  gboolean ret;

  media_id = cache_get_media_id (cache, file);
  g_return_val_if_fail (media_id != -1, FALSE);

  result->got = 0;
  result->hash = h;
  ret = cache_exec (cache, get_ebook_callback, result,
                    "select * from ebook where media_id=%d", media_id);
  g_return_val_if_fail (ret, FALSE);
  return result->got == 1;
}

gboolean
cache_remove (cache_t *cache, const gchar *file)
{
  int media_id;

  media_id = cache_get_media_id (cache, file);
  g_return_val_if_fail (media_id != -1, FALSE);

  cache_exec (cache, NULL, NULL, "delete from hash where media_id=%d;",
              media_id);

  cache_exec (cache, NULL, NULL, "delete from media where id=%d;", media_id);

  return TRUE;
}

static int
cache_remove_if_no_exists_callback (void *para, int n_column,
                                    char **column_value, char **column_name)
{
  cache_t *cache = (cache_t *)para;
  char *path;
  int media_id;

  if (column_value[0] != NULL)
    {
      path = column_value[1];
      if (g_file_test (path, G_FILE_TEST_EXISTS) == FALSE)
        {
          media_id = atoi (column_value[0]);
          cache_exec (cache, NULL, NULL, "delete from hash where media_id=%d;",
                      media_id);
          cache_exec (cache, NULL, NULL, "delete from media where id=%d;",
                      media_id);
        }
    }
  return 0;
}

void
cache_cleanup (cache_t *cache)
{
  cache_exec (cache, cache_remove_if_no_exists_callback, cache,
              "select id, path from media;");
}
