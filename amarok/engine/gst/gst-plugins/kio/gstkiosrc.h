/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *
 * gstkiosrc.h: 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef __GST_KIOSRC_H__
#define __GST_KIOSRC_H__

#include <gst/gst.h>
#include "kioreceiver.h"

G_BEGIN_DECLS


#define GST_TYPE_KIOSRC \
  (gst_kiosrc_get_type())
#define GST_KIOSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_KIOSRC,GstKioSrc))
#define GST_KIOSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_KIOSRC,GstKioSrcClass)) 
#define GST_IS_KIOSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_KIOSRC))
#define GST_IS_KIOSRC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_KIOSRC))

typedef enum {
  GST_KIOSRC_OPEN              = GST_ELEMENT_FLAG_LAST,

  GST_KIOSRC_FLAG_LAST = GST_ELEMENT_FLAG_LAST + 2
} GstKioSrcFlags;

typedef struct _GstKioSrc GstKioSrc;
typedef struct _GstKioSrcClass GstKioSrcClass;

struct _GstKioSrc {
  GstElement element;
  GstPad *srcpad;

  guint pagesize;			/* system page size*/
 
  gchar *filename;			/* kioname */
  gchar *uri;				/* caching the URI */

  long long curoffset;			/* current offset in kio */

  gboolean seek_happened;
  gboolean need_flush;

  KioReceiver *receiver;
};

struct _GstKioSrcClass {
  GstElementClass parent_class;
};

GType gst_kiosrc_get_type(void);

G_END_DECLS

#endif /* __GST_KIOSRC_H__ */
