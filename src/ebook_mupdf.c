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

#include <mupdf/fitz.h>

#include "ebook.h"
#include "util.h"

static void
pdf_get_cover_hash (fz_context *ctx, fz_document *doc, ebook_hash_t *ehash)
{
  GdkPixbuf *pixbuf;
  GdkPixbuf *hashbuf;
  fz_matrix scale = fz_scale (1.0, 1.0);
  fz_matrix mat = fz_pre_rotate (scale, 0.0);
  fz_colorspace *color = fz_device_rgb (ctx);
  fz_pixmap *pixmap
      = fz_new_pixmap_from_page_number (ctx, doc, 0, mat, color, 0);
  if (pixmap == NULL)
    {
      return;
    }

  pixbuf = gdk_pixbuf_new_from_data (pixmap->samples, GDK_COLORSPACE_RGB, FALSE, 8, pixmap->w, pixmap->h, pixmap->stride, NULL, NULL);
  if (pixbuf == NULL)
    {
      fz_drop_pixmap (ctx, pixmap);
      return;
    }

  hashbuf = gdk_pixbuf_scale_simple (pixbuf, 8, 8, GDK_INTERP_BILINEAR);
  g_object_unref (pixbuf);

  ehash->cover_hash = image_buffer_hash (gdk_pixbuf_get_pixels (hashbuf),
                                         pixmap->w * pixmap->h * pixmap->n);
  g_object_unref (hashbuf);
  fz_drop_pixmap (ctx, pixmap);
}

static void
pdf_get_isbn (fz_context *ctx, fz_document *doc, ebook_hash_t *ehash)
{
  fz_lookup_metadata (ctx, doc, "info:ISBN", ehash->isbn, sizeof ehash->isbn);
}

int
ebook_hash (const char *file, ebook_hash_t *ehash)
{
  fz_context *ctx;
  fz_document *doc;

  ctx = fz_new_context (NULL, NULL, FZ_STORE_UNLIMITED);
  g_return_val_if_fail (ctx != NULL, -1);

  fz_register_document_handlers (ctx);

  fz_try (ctx)
  {
    doc = fz_open_document (ctx, file);
    if (doc == NULL)
      {
        g_warning ("open pdf file error: %s", ctx->error.message);
        fz_drop_context (ctx);
        return -1;
      }

    pdf_get_cover_hash (ctx, doc, ehash);
    pdf_get_isbn (ctx, doc, ehash);
  }
  fz_catch (ctx)
  {
    g_warning ("mupdf occured error: %s", ctx->error.message);
    fz_drop_context (ctx);
    return -1;
  }

#define _pdf_get_value_to_ehash(key, area)                                    \
  do                                                                          \
    {                                                                         \
      fz_lookup_metadata (ctx, doc, key, ehash->area, sizeof ehash->area);    \
    }                                                                         \
  while (0)

  _pdf_get_value_to_ehash (FZ_META_INFO_TITLE, title);
  _pdf_get_value_to_ehash (FZ_META_INFO_AUTHOR, author);
  _pdf_get_value_to_ehash (FZ_META_INFO_PRODUCER, producer);
  printf ("%s title: %s\n", file, ehash->title);
  printf ("%s author: %s\n", file, ehash->author);
  printf ("%s producer: %s\n", file, ehash->producer);
  printf ("%s isbn: %s\n", file, ehash->isbn);
  fz_drop_document (ctx, doc);
  fz_drop_context (ctx);

  return 0;
}
