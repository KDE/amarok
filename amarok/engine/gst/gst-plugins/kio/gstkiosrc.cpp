/*
* GStreamer KIO source (based on GnomeVFS source)
* Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
*                    2000 Wim Taymans <wtay@chello.be>
*                    2001 Bastien Nocera <hadess@hadess.net>
*                    2002 Tim Jansen <tim@tjansen.de>
*                    2004 Mark Kretschmann <markey@web.de>
*
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

//uncomment to activate code
//#define AMAROK_WITH_GSTKIOSRC
#ifdef  AMAROK_WITH_GSTKIOSRC


#include "kioreceiver.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <qevent.h>
#include <qthread.h>

#include <kapplication.h>

extern "C" {

#include <gst/gst.h>

    extern GstPluginDesc plugin_desc;

    GstElementDetails kiosrc_details = {
                                           ( gchar* ) "Kio Source",
                                           ( gchar* ) "Source/File",
                                           ( gchar* ) "Read from any Kio file",
                                           ( gchar* ) "0.1.0",
                                           ( gchar* ) "Tim Jansen <tim@tjansen.de>",
                                           ( gchar* ) "(C) 2002",
                                       };

#define GST_TYPE_KIOSRC \
  (kiosrc_get_type())
#define KIOSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_KIOSRC,KioSrc))
#define KIOSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_KIOSRC,KioSrcClass))
#define GST_IS_KIOSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_KIOSRC))
#define GST_IS_KIOSRC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_KIOSRC))

    static GStaticMutex count_lock = G_STATIC_MUTEX_INIT;
    static gint ref_count = 0;

    enum KioSrcFlags {
        KIOSRC_OPEN = GST_ELEMENT_FLAG_LAST,
        KIOSRC_FLAG_LAST = GST_ELEMENT_FLAG_LAST + 2
    };

    typedef struct _KioSrc KioSrc;
    typedef struct _KioSrcClass KioSrcClass;

    struct _KioSrc {
        GstElement element;
        /* pads */
        GstPad *srcpad;

        /* filename */
        gchar *filename;

        /* Class that manages communication with KIO */
        KioReceiver *receiver;
    };

    struct _KioSrcClass {
        GstElementClass parent_class;
    };

    /* KioSrc signals and args */
    enum {
        LAST_SIGNAL
    };

    enum {
        ARG_0,
        ARG_LOCATION,
        ARG_MAXBYTESPERREAD,
        ARG_OFFSET,
        ARG_PROGRESSGUI
    };

    GType kiosrc_get_type( void );

    static gboolean plugin_init ( GModule *, GstPlugin * plugin );
    static void kiosrc_class_init	( KioSrcClass * klass );
    static void kiosrc_init	( KioSrc * kiosrc );
    static void kiosrc_dispose	( GObject * object );

    static void kiosrc_set_property	( GObject * object, guint prop_id,
                                      const GValue * value, GParamSpec * pspec );
    static void kiosrc_get_property	( GObject * object, guint prop_id,
                                      GValue * value, GParamSpec * pspec );

    static GstBuffer*	kiosrc_get	( GstPad * pad );

    static GstElementStateReturn
    kiosrc_change_state	( GstElement * element );

    static gboolean kiosrc_srcpad_query ( GstPad * pad, GstPadQueryType type,
                                          GstFormat * format, gint64 * value );


    static GstElementClass *parent_class = NULL;

} // end of C stuff


class EventThread : public QThread {
    public:
        EventThread() {}
        void run() {
            KApplication a;
            a.enter_loop();
        }
};


static EventThread *eventThread = 0;


GType kiosrc_get_type( void ) {
    static GType kiosrc_type = 0;

    if ( !kiosrc_type ) {
        static const GTypeInfo kiosrc_info = {
                                                 sizeof( KioSrcClass ), NULL,
                                                 NULL,
                                                 ( GClassInitFunc ) kiosrc_class_init,
                                                 NULL,
                                                 NULL,
                                                 sizeof( KioSrc ),
                                                 0,
                                                 ( GInstanceInitFunc ) kiosrc_init,
                                             };
        kiosrc_type =
            g_type_register_static( GST_TYPE_ELEMENT,
                                    "KioSrc",
                                    &kiosrc_info,
                                    ( GTypeFlags ) 0 );
    }
    return kiosrc_type;
}


static void kiosrc_class_init( KioSrcClass *klass ) {
    GObjectClass * gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = ( GObjectClass * ) klass;
    gstelement_class = ( GstElementClass * ) klass;

    parent_class = ( GstElementClass* ) g_type_class_ref( GST_TYPE_ELEMENT );

    gst_element_class_install_std_props (
        GST_ELEMENT_CLASS ( klass ),
        "offset", ARG_OFFSET, G_PARAM_READABLE,
        "maxbytesperread", ARG_MAXBYTESPERREAD, G_PARAM_READWRITE,
        "location", ARG_LOCATION, G_PARAM_READWRITE,
        "progressgui", ARG_PROGRESSGUI, G_PARAM_READWRITE,
        NULL );

    gobject_class->dispose = kiosrc_dispose;

    gstelement_class->set_property = kiosrc_set_property;
    gstelement_class->get_property = kiosrc_get_property;

    gstelement_class->change_state = kiosrc_change_state;

}


static void kiosrc_init( KioSrc *kiosrc ) {
    kiosrc->srcpad = gst_pad_new( "src", GST_PAD_SRC );
    gst_pad_set_get_function( kiosrc->srcpad, kiosrc_get );
    gst_pad_set_query_function ( kiosrc->srcpad,
                                 kiosrc_srcpad_query );
    gst_element_add_pad( GST_ELEMENT( kiosrc ), kiosrc->srcpad );

    kiosrc->filename = NULL;

    g_static_mutex_lock ( &count_lock );
    if ( ref_count == 0 ) {
        if ( !KApplication::kApplication() )
            eventThread = new EventThread();
    }
    ref_count++;
    g_static_mutex_unlock ( &count_lock );

    kiosrc->receiver = new KioReceiver();
}


static void kiosrc_dispose( GObject *object ) {
    KioSrc * src = KIOSRC( object );
    g_static_mutex_lock ( &count_lock );
    ref_count--;
    if ( ref_count == 0 ) {
        if ( eventThread )
            QThread::postEvent( src->receiver, new QuitEvent() );
        else
            QThread::postEvent( src->receiver, new CloseFileEvent() );
    }
    g_static_mutex_unlock ( &count_lock );
    delete src->receiver;

    G_OBJECT_CLASS ( parent_class ) ->dispose ( object );
}


static void kiosrc_set_property( GObject *object, guint prop_id, const GValue *value, GParamSpec * ) {
    KioSrc * src;
    const gchar *location;
    gchar cwd[ PATH_MAX ];

    /* it's not null if we got it, but it might not be ours */
    if ( !GST_IS_KIOSRC( object ) )
        return ;

    src = KIOSRC( object );

    switch ( prop_id ) {
    case ARG_LOCATION:
        /* the element must be stopped or paused in order to do this */
        g_return_if_fail( ( GST_STATE( src ) < GST_STATE_PLAYING )
                          || ( GST_STATE( src ) == GST_STATE_PAUSED ) );

        g_free( src->filename );

        /* clear the filename if we get a NULL (is that possible?) */
        if ( g_value_get_string ( value ) == NULL ) {
            gst_element_set_state( GST_ELEMENT( object ), GST_STATE_NULL );
            src->filename = NULL;
        } else {
            /* otherwise set the new filename */
            location = g_value_get_string ( value );
            /* if it's not a proper uri, default to file: -- this
             * is a crude test */
            if ( !strchr ( location, ':' ) )
                if ( *location == '/' )
                    src->filename = g_strdup_printf ( "file://%s", location );
                else
                    src->filename = g_strdup_printf ( "file://%s/%s", getcwd( cwd, PATH_MAX ), location );
            else
                src->filename = g_strdup ( g_value_get_string ( value ) );
        }

        if ( ( GST_STATE( src ) == GST_STATE_PAUSED )
                && ( src->filename != NULL ) ) {
            QThread::postEvent( src->receiver, new OpenFileEvent( src->filename ) );
        }
        break;
    case ARG_MAXBYTESPERREAD:
        src->receiver->setMaxRead( g_value_get_int( value ) );
        break;
    case ARG_PROGRESSGUI:
        src->receiver->setProgressInfo(
            g_value_get_boolean( value ) ? true : false );
    default:
        break;
    }
}


static void kiosrc_get_property( GObject *object, guint prop_id, GValue *value, GParamSpec * ) {
    KioSrc * src = KIOSRC( object );

    /* it's not null if we got it, but it might not be ours */
    if ( !GST_IS_KIOSRC( object ) )
        return ;

    switch ( prop_id ) {
    case ARG_LOCATION:
        g_value_set_string( value, src->filename );
        break;
    case ARG_MAXBYTESPERREAD:
        g_value_set_int( value, src->receiver->maxRead() );
        break;
    case ARG_OFFSET:
        g_value_set_int64( value, src->receiver->currentPosition() );
        break;
    case ARG_PROGRESSGUI:
        g_value_set_boolean( value, src->receiver->progressInfo() );
        break;
    default:
        // TODO: warn here
        break;
    }
}


/**
 * kiosrc_get:
 * @pad: #GstPad to push a buffer from
 *
 * Push a new buffer from the kiosrc at the current offset.
 */
static GstBuffer *kiosrc_get( GstPad *pad ) {
    KioSrc * src;
    GstBuffer *buf;
    int readbytes;
    void *bufferPtr;

    if ( !pad )
        return 0;
    src = KIOSRC( gst_pad_get_parent( pad ) );
    if ( !GST_FLAG_IS_SET( src, KIOSRC_OPEN ) )
        return NULL;

    /* deal with EOF state */
    if ( !src->receiver->read( bufferPtr, readbytes ) ) {
        gst_element_set_eos ( GST_ELEMENT ( src ) );
        return GST_BUFFER ( gst_event_new ( GST_EVENT_EOS ) );
    }

    buf = gst_buffer_new();
    g_return_val_if_fail( buf, NULL );

    GST_BUFFER_DATA( buf ) = ( guchar* ) bufferPtr;
    GST_BUFFER_OFFSET( buf ) = src->receiver->currentPosition();
    GST_BUFFER_SIZE( buf ) = readbytes;
    GST_BUFFER_FLAG_SET( buf, GST_BUFFER_DONTFREE );
    GST_BUFFER_TIMESTAMP( buf ) = -1;

    g_object_notify ( ( GObject* ) src, "offset" );

    /* we're done, return the buffer */
    return buf;
}


static GstElementStateReturn kiosrc_change_state( GstElement *element ) {
    KioSrc * src = KIOSRC( element );
    g_return_val_if_fail( GST_IS_KIOSRC( element ),
                          GST_STATE_FAILURE );

    switch ( GST_STATE_TRANSITION ( element ) ) {
    case GST_STATE_READY_TO_PAUSED:
        if ( GST_FLAG_IS_SET( element, KIOSRC_OPEN ) ) {
            QThread::postEvent( src->receiver, new PauseEvent() );
        }
        break;
    case GST_STATE_PAUSED_TO_READY:
        if ( GST_FLAG_IS_SET( element, KIOSRC_OPEN ) ) {
            QThread::postEvent( src->receiver, new UnPauseEvent() );
        }
        break;
    case GST_STATE_NULL_TO_READY:
    case GST_STATE_READY_TO_NULL:
    default:
        break;
    }

    if ( GST_ELEMENT_CLASS( parent_class ) ->change_state )
        return GST_ELEMENT_CLASS( parent_class ) ->
               change_state( element );

    return GST_STATE_SUCCESS;
}


static gboolean plugin_init( GModule *, GstPlugin *plugin ) {
    GstElementFactory * factory;

    /* create an elementfactory for the aasink element */
    factory = gst_element_factory_new( "kiosrc", GST_TYPE_KIOSRC,
                                       &kiosrc_details );
    g_return_val_if_fail( factory != NULL, FALSE );

    gst_plugin_add_feature ( plugin, GST_PLUGIN_FEATURE ( factory ) );

    return TRUE;
}


static gboolean kiosrc_srcpad_query( GstPad *pad, GstPadQueryType type,
                                     GstFormat *format, gint64 *value ) {
    KioSrc * src = KIOSRC ( gst_pad_get_parent ( pad ) );

    switch ( type ) {
    case GST_PAD_QUERY_TOTAL: {
            if ( *format != GST_FORMAT_BYTES ) {
                return FALSE;
            }
            long long s = src->receiver->fileSize();
            if ( s < 0 )
                return FALSE;
            *value = s;
            break;
        }
    case GST_PAD_QUERY_POSITION:
        if ( *format != GST_FORMAT_BYTES ) {
            return FALSE;
        }
        *value = src->receiver->currentPosition();
        break;
    default:
        return FALSE;
        break;
    }
    return TRUE;
}


GstPluginDesc plugin_desc = {
                                0,     /* version major */
                                4,     /* version minor */
                                ( gchar* ) "kiosrc",
                                plugin_init
                            };

                            
#endif /* AMAROK_WITH_GSTKIOSRC */

