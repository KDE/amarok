/* GStreamer
 * Copyright (C) 2004 Benjamin Otte <otte@gnome.org>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include "gstkiosrc.h"
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <qeventloop.h>

// #include "gst/gst-i18n-plugin.h"

/**********************************************************************
 * GStreamer Default Kio Source
 * Theory of Operation
 *
 * FIXME
 */
static KApplication *our_app = NULL;

GST_DEBUG_CATEGORY_STATIC (gst_kiosrc_debug);
#define GST_CAT_DEFAULT gst_kiosrc_debug

enum
{
  ARG_0,
  ARG_LOCATION,
};

static void gst_kiosrc_dispose (GObject * object);
static void gst_kiosrc_finalize (GObject * object);

static void gst_kiosrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_kiosrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstData *gst_kiosrc_get (GstPad * pad);
static gboolean gst_kiosrc_srcpad_event (GstPad * pad, GstEvent * event);
static gboolean gst_kiosrc_srcpad_query (GstPad * pad, GstQueryType type,
    GstFormat * format, gint64 * value);

static GstElementStateReturn gst_kiosrc_change_state (GstElement * element);

#if 0
static void gst_kiosrc_uri_handler_init (gpointer g_iface,
    gpointer iface_data);
#endif

static void
_do_init (GType kiosrc_type)
{
#if 0
  static const GInterfaceInfo urihandler_info = {
    gst_kiosrc_uri_handler_init,
    NULL,
    NULL
  };

  g_type_add_interface_static (kiosrc_type, GST_TYPE_URI_HANDLER,
      &urihandler_info);
#endif
}

GST_BOILERPLATE_FULL (GstKioSrc, gst_kiosrc, GstElement, GST_TYPE_ELEMENT, _do_init);

static void
gst_kiosrc_base_init (gpointer g_class)
{
  GstElementDetails gst_kiosrc_details = GST_ELEMENT_DETAILS ("Kio Source",
      "Source",
      "Read from URLs using KDEs I/O mechanims (KIO)",
      "Tim Jansen <tim@tjansen.de>, "
      "Benjamin Otte <otte@gnome.org>");

  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details (gstelement_class, &gst_kiosrc_details);
}
static void
gst_kiosrc_class_init (GstKioSrcClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_LOCATION,
      g_param_spec_string ("location", "Location",
          "URL of the kio to read", NULL, (GParamFlags) G_PARAM_READWRITE));

  gobject_class->dispose = gst_kiosrc_dispose;
  gobject_class->finalize = gst_kiosrc_finalize;
  gobject_class->set_property = gst_kiosrc_set_property;
  gobject_class->get_property = gst_kiosrc_get_property;

  gstelement_class->change_state = gst_kiosrc_change_state;
}

static void
gst_kiosrc_init (GstKioSrc * src)
{
  src->srcpad = gst_pad_new ("src", GST_PAD_SRC);
  gst_pad_set_get_function (src->srcpad, gst_kiosrc_get);
  gst_pad_set_event_function (src->srcpad, gst_kiosrc_srcpad_event);
  gst_pad_set_query_function (src->srcpad, gst_kiosrc_srcpad_query);
  gst_element_add_pad (GST_ELEMENT (src), src->srcpad);

  src->filename = NULL;
  src->uri = NULL;

  src->seek_happened = FALSE;
  src->receiver = new KioReceiver ();
}

static void
gst_kiosrc_dispose (GObject * object)
{
  GstKioSrc *src = GST_KIOSRC (object);

  g_free (src->filename);
  g_free (src->uri);

  /* dispose may be called multiple times */
  src->filename = NULL;
  src->uri = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_kiosrc_finalize (GObject * object)
{
  GstKioSrc *src = GST_KIOSRC (object);

  delete src->receiver;
}

static gboolean
gst_kiosrc_set_location (GstKioSrc * src, const gchar * location)
{
  /* the element must be stopped in order to do this */
  if (GST_STATE (src) != GST_STATE_READY && GST_STATE (src) != GST_STATE_NULL)
    return FALSE;

  g_free (src->filename);
  g_free (src->uri);

  /* clear the filename if we get a NULL (is that possible?) */
  if (location == NULL) {
    src->filename = NULL;
    src->uri = NULL;
  } else {
    src->filename = g_strdup (location);
    src->uri = gst_uri_construct ("kio", src->filename);
  }
  g_object_notify (G_OBJECT (src), "location");
  //gst_uri_handler_new_uri (GST_URI_HANDLER (src), src->uri);

  return TRUE;
}

static void
gst_kiosrc_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstKioSrc *src;

  /* it's not null if we got it, but it might not be ours */
  g_return_if_fail (GST_IS_KIOSRC (object));

  src = GST_KIOSRC (object);

  switch (prop_id) {
    case ARG_LOCATION:
      gst_kiosrc_set_location (src, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_kiosrc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstKioSrc *src;

  /* it's not null if we got it, but it might not be ours */
  g_return_if_fail (GST_IS_KIOSRC (object));

  src = GST_KIOSRC (object);

  switch (prop_id) {
    case ARG_LOCATION:
      g_value_set_string (value, src->filename);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
process_events (bool block)
{
  if (our_app) {
    /* FIXME: need some kind of lock here? */
    QApplication::eventLoop()->processEvents (QEventLoop::ExcludeUserInput | 
	(block ? QEventLoop::WaitForMore : QEventLoop::ExcludeUserInput));
  } else if (block) {
    if (gst_thread_get_current ()) {
      /* FIXME: now we need to block somehow until data becomes available */
      /* for now we just sleep 1/100th of a second */
      g_usleep (G_USEC_PER_SEC / 100);
    } else {
      /* FIXME: does this break when we're inside a KDE thread?
         if so, how can we figure that out? */
      QApplication::eventLoop()->processEvents (QEventLoop::WaitForMore);
    }
  }
}

static GstData *
gst_kiosrc_get (GstPad * pad)
{
  GstKioSrc *src;
  GstBuffer *buf = NULL;
  guint readsize;
  int got = 0;
  long long file_size;

  g_return_val_if_fail (pad != NULL, NULL);
  src = GST_KIOSRC (gst_pad_get_parent (pad));
  g_return_val_if_fail (GST_FLAG_IS_SET (src, GST_KIOSRC_OPEN), NULL);

  process_events (false);
  /* check for flush */
  if (src->need_flush) {
    src->need_flush = FALSE;
    GST_DEBUG_OBJECT (src, "sending flush");
    return GST_DATA (gst_event_new_flush ());
  }
  /* check for seek */
  if (src->seek_happened) {
    GstEvent *event;

    src->seek_happened = FALSE;
    GST_DEBUG_OBJECT (src, "sending discont");
    event =
        gst_event_new_discontinuous (FALSE, GST_FORMAT_BYTES, src->curoffset,
        NULL);
    return GST_DATA (event);
  }

  process_events (false);
  /* check for EOF */
  file_size = src->receiver->fileSize ();
  if (file_size >= 0 &&
      src->curoffset >= file_size) {
    GST_DEBUG_OBJECT (src, "eos %" G_GINT64_FORMAT " %" G_GINT64_FORMAT,
	src->curoffset, file_size);
    gst_element_set_eos (GST_ELEMENT (src));
    return GST_DATA (gst_event_new (GST_EVENT_EOS));
  }

  readsize = 4096; //src->receiver->maxRead ();
  if (file_size >= 0 && 
      src->curoffset + readsize > file_size) {
    readsize = file_size - src->curoffset;
  }

  buf = gst_buffer_new_and_alloc (readsize);
  g_return_val_if_fail (buf != NULL, NULL);
  GST_BUFFER_MAXSIZE (buf) = readsize;

  while (got == 0) {
    got = readsize;
    if (!src->receiver->read ((void *) GST_BUFFER_DATA (buf), got)) {
      GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL), (NULL));
      return NULL;
    }
    if (got == 0)
      process_events (true);
  }
  g_print ("got %d bytes", got);

  GST_BUFFER_SIZE (buf) = got;
  GST_BUFFER_OFFSET (buf) = src->curoffset;
  GST_BUFFER_OFFSET_END (buf) = src->curoffset + got;
  src->curoffset += got;

  return GST_DATA (buf);
}

/* open the kio and mmap it, necessary to go to READY state */
static gboolean
gst_kiosrc_open_kio (GstKioSrc * src)
{
  g_return_val_if_fail (!GST_FLAG_IS_SET (src, GST_KIOSRC_OPEN), FALSE);

  if (src->filename == NULL || src->filename[0] == '\0') {
    GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND,
        (("No kio name specified for reading.")), (NULL));
    return FALSE;
  }

  GST_INFO_OBJECT (src, "opening kio %s", src->filename);
  QApplication::postEvent (src->receiver, new OpenFileEvent (src->filename));
  src->curoffset = 0;
  GST_FLAG_SET (src, GST_KIOSRC_OPEN);
  return TRUE;
}

/* unmap and close the kio */
static void
gst_kiosrc_close_kio (GstKioSrc * src)
{
  g_return_if_fail (GST_FLAG_IS_SET (src, GST_KIOSRC_OPEN));

  /* close the kio */
  QApplication::postEvent (src->receiver, new CloseFileEvent ());

  process_events (false);
  GST_FLAG_UNSET (src, GST_KIOSRC_OPEN);
}


static GstElementStateReturn
gst_kiosrc_change_state (GstElement * element)
{
  GstKioSrc *src = GST_KIOSRC (element);

  switch (GST_STATE_TRANSITION (element)) {
    case GST_STATE_NULL_TO_READY:
      break;
    case GST_STATE_READY_TO_NULL:
      break;
    case GST_STATE_READY_TO_PAUSED:
      if (!GST_FLAG_IS_SET (element, GST_KIOSRC_OPEN)) {
        if (!gst_kiosrc_open_kio (GST_KIOSRC (element)))
          return GST_STATE_FAILURE;
      }
      break;
    case GST_STATE_PAUSED_TO_READY:
      if (GST_FLAG_IS_SET (element, GST_KIOSRC_OPEN))
        gst_kiosrc_close_kio (GST_KIOSRC (element));
      src->seek_happened = TRUE;
      break;
    default:
      break;
  }

  if (GST_ELEMENT_CLASS (parent_class)->change_state)
    return GST_ELEMENT_CLASS (parent_class)->change_state (element);

  return GST_STATE_SUCCESS;
}

static gboolean
gst_kiosrc_srcpad_query (GstPad * pad, GstQueryType type,
    GstFormat * format, gint64 * value)
{
  GstKioSrc *src = GST_KIOSRC (GST_PAD_PARENT (pad));

  process_events (false);
  
  switch (type) {
    case GST_QUERY_TOTAL:
      if (*format != GST_FORMAT_BYTES &&
	  *format != GST_FORMAT_DEFAULT) {
        return FALSE;
      }
      
      src->receiver->fileSize ();
      *value = src->receiver->fileSize ();
      break;
    case GST_QUERY_POSITION:
      switch (*format) {
        case GST_FORMAT_BYTES:
        case GST_FORMAT_DEFAULT:
          *value = src->curoffset;
          break;
        case GST_FORMAT_PERCENT:
          if (src->receiver->fileSize () == 0)
            return FALSE;
          *value = src->curoffset * GST_FORMAT_PERCENT_MAX / src->receiver->fileSize ();
          break;
        default:
          return FALSE;
      }
      break;
    default:
      return FALSE;
      break;
  }
  return TRUE;
}

static gboolean
gst_kiosrc_srcpad_event (GstPad * pad, GstEvent * event)
{
  GstKioSrc *src = GST_KIOSRC (GST_PAD_PARENT (pad));

  GST_DEBUG_OBJECT (src, "event %d", GST_EVENT_TYPE (event));
  process_events (false);
  
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
    {
      gint64 offset;

      if (GST_EVENT_SEEK_FORMAT (event) != GST_FORMAT_BYTES &&
	  GST_EVENT_SEEK_FORMAT (event) != GST_FORMAT_DEFAULT) {
        goto error;
      }

      offset = GST_EVENT_SEEK_OFFSET (event);

      switch (GST_EVENT_SEEK_METHOD (event)) {
        case GST_SEEK_METHOD_SET:
          if (offset > src->receiver->fileSize () && (!src->receiver->fileSize ()
                  || offset > src->receiver->fileSize ())) {
            goto error;
          }
          src->curoffset = offset;
          GST_DEBUG_OBJECT (src, "seek set pending to %" G_GINT64_FORMAT,
              src->curoffset);
          break;
        case GST_SEEK_METHOD_CUR:
          if (offset + src->curoffset > src->receiver->fileSize ())
            goto error;
          src->curoffset += offset;
          GST_DEBUG_OBJECT (src, "seek cur pending to %" G_GINT64_FORMAT,
              src->curoffset);
          break;
        case GST_SEEK_METHOD_END:
          if (ABS (offset) > src->receiver->fileSize ()) {
            goto error;
          }
          src->curoffset = src->receiver->fileSize () - ABS (offset);
          GST_DEBUG_OBJECT (src, "seek end pending to %" G_GINT64_FORMAT,
              src->curoffset);
          break;
        default:
          goto error;
          break;
      }
      src->seek_happened = TRUE;
      src->need_flush = GST_EVENT_SEEK_FLAGS (event) & GST_SEEK_FLAG_FLUSH;
      break;
    }
    case GST_EVENT_FLUSH:
      src->need_flush = TRUE;
      break;
    default:
      goto error;
      break;
  }
  gst_event_unref (event);
  return TRUE;

error:
  gst_event_unref (event);
  return FALSE;
}

#if 0
/*** GSTURIHANDLER INTERFACE *************************************************/

static guint
gst_kiosrc_uri_get_type (void)
{
  return GST_URI_SRC;
}
static gchar **
gst_kiosrc_uri_get_protocols (void)
{
  static gchar *protocols[] = { "kio", NULL };

  return protocols;
}
static const gchar *
gst_kiosrc_uri_get_uri (GstURIHandler * handler)
{
  GstKioSrc *src = GST_KIOSRC (handler);

  return src->uri;
}

static gboolean
gst_kiosrc_uri_set_uri (GstURIHandler * handler, const gchar * uri)
{
  gchar *protocol, *location;
  gboolean ret;
  GstKioSrc *src = GST_KIOSRC (handler);

  protocol = gst_uri_get_protocol (uri);
  if (strcmp (protocol, "kio") != 0) {
    g_free (protocol);
    return FALSE;
  }
  g_free (protocol);
  location = gst_uri_get_location (uri);
  ret = gst_kiosrc_set_location (src, location);
  g_free (location);

  return ret;
}

static void
gst_kiosrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_kiosrc_uri_get_type;
  iface->get_protocols = gst_kiosrc_uri_get_protocols;
  iface->get_uri = gst_kiosrc_uri_get_uri;
  iface->set_uri = gst_kiosrc_uri_set_uri;
}
#endif

extern "C" {
  
static gboolean
plugin_init (GstPlugin *plugin)
{
  if (!gst_element_register (plugin, "kiosrc", GST_RANK_SECONDARY,
          GST_TYPE_KIOSRC))
    return FALSE;

  GST_DEBUG_CATEGORY_INIT (gst_kiosrc_debug, "kiosrc", 0, "KIO source");

  if (qApp == 0) {
    GST_INFO ("No Qt running, we run it instead.");

    const char *args[] = { "kiosrc", "" };
    KCmdLineArgs::init(1, (char **) args, "GStreamer kio plugin", "GStreamer kio plugin",
	"GStreamer kio plugin", VERSION, false);
    our_app = new KApplication (false, false);
  } else {
    GST_INFO ("We're inside a Qt application here.");
  }

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "kio",
    "plugins for KDE's I/O system (KIO)",
    plugin_init, VERSION, "LGPL", "GST", "http://gstreamer.net")

}
