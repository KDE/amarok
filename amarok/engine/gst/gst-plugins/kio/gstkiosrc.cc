/*
* GStreamer KIO source (based on GnomeVFS source)
* Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
*                    2000 Wim Taymans <wtay@chello.be>
*                    2001 Bastien Nocera <hadess@hadess.net>
*                    2002 Tim Jansen <tim@tjansen.de>
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

#include "kioreceiver.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include <qthread.h>
#include <qevent.h>
#include <kapplication.h>

extern "C"
{

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

    struct _KioSrc
    {
        GstElement element;
        /* pads */
        GstPad *srcpad;

        /* filename */
        gchar *filename;

        /* Class that manages communication with KIO */
        KioReceiver *receiver;
    };

    struct _KioSrcClass
    {
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

class EventThread : public QThread
{
    public:
        EventThread()
        {}
        void run()
        {
            KApplication a;
            a.enter_loop();
        }
};

static EventThread *eventThread = 0;

GType kiosrc_get_type( void )
{
    static GType kiosrc_type = 0;

    if ( !kiosrc_type )
    {
        static const GTypeInfo kiosrc_info =
            {
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

static void kiosrc_class_init( KioSrcClass *klass )
{
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

static void kiosrc_init( KioSrc *kiosrc )
{
    kiosrc->srcpad = gst_pad_new( "src", GST_PAD_SRC );
    gst_pad_set_get_function( kiosrc->srcpad, kiosrc_get );
    gst_pad_set_query_function ( kiosrc->srcpad,
                                 kiosrc_srcpad_query );
    gst_element_add_pad( GST_ELEMENT( kiosrc ), kiosrc->srcpad );

    kiosrc->filename = NULL;

    g_static_mutex_lock ( &count_lock );
    if ( ref_count == 0 )
    {
        if ( !KApplication::kApplication() )
            eventThread = new EventThread();
    }
    ref_count++;
    g_static_mutex_unlock ( &count_lock );

    kiosrc->receiver = new KioReceiver();
}

static void kiosrc_dispose( GObject *object )
{
    KioSrc * src = KIOSRC( object );
    g_static_mutex_lock ( &count_lock );
    ref_count--;
    if ( ref_count == 0 )
    {
        if ( eventThread )
            QThread::postEvent( src->receiver, new QuitEvent() );
        else
            QThread::postEvent( src->receiver, new CloseFileEvent() );
    }
    g_static_mutex_unlock ( &count_lock );
    delete src->receiver;

    G_OBJECT_CLASS ( parent_class ) ->dispose ( object );
}


static void kiosrc_set_property( GObject *object, guint prop_id, const GValue *value, GParamSpec * )
{
    KioSrc * src;
    const gchar *location;
    gchar cwd[ PATH_MAX ];

    /* it's not null if we got it, but it might not be ours */
    if ( !GST_IS_KIOSRC( object ) )
        return ;

    src = KIOSRC( object );

    switch ( prop_id )
    {
        case ARG_LOCATION:
            /* the element must be stopped or paused in order to do this */
            g_return_if_fail( ( GST_STATE( src ) < GST_STATE_PLAYING )
                              || ( GST_STATE( src ) == GST_STATE_PAUSED ) );

            g_free( src->filename );

            /* clear the filename if we get a NULL (is that possible?) */
            if ( g_value_get_string ( value ) == NULL )
            {
                gst_element_set_state( GST_ELEMENT( object ), GST_STATE_NULL );
                src->filename = NULL;
            }
            else
            {
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
                    && ( src->filename != NULL ) )
            {
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

static void kiosrc_get_property( GObject *object, guint prop_id, GValue *value, GParamSpec * )
{
    KioSrc * src = KIOSRC( object );

    /* it's not null if we got it, but it might not be ours */
    if ( !GST_IS_KIOSRC( object ) )
        return ;

    switch ( prop_id )
    {
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
static GstBuffer *kiosrc_get( GstPad *pad )
{
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
    if ( !src->receiver->read( bufferPtr, readbytes ) )
    {
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

static GstElementStateReturn kiosrc_change_state( GstElement *element )
{
    KioSrc * src = KIOSRC( element );
    g_return_val_if_fail( GST_IS_KIOSRC( element ),
                          GST_STATE_FAILURE );

    switch ( GST_STATE_TRANSITION ( element ) )
    {
        case GST_STATE_READY_TO_PAUSED:
            if ( GST_FLAG_IS_SET( element, KIOSRC_OPEN ) )
            {
                QThread::postEvent( src->receiver, new PauseEvent() );
            }
            break;
        case GST_STATE_PAUSED_TO_READY:
            if ( GST_FLAG_IS_SET( element, KIOSRC_OPEN ) )
            {
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

static gboolean plugin_init( GModule *, GstPlugin *plugin )
{
    GstElementFactory * factory;

    /* create an elementfactory for the aasink element */
    factory = gst_element_factory_new( "kiosrc", GST_TYPE_KIOSRC,
                                       &kiosrc_details );
    g_return_val_if_fail( factory != NULL, FALSE );

    gst_plugin_add_feature ( plugin, GST_PLUGIN_FEATURE ( factory ) );

    return TRUE;
}

static gboolean kiosrc_srcpad_query( GstPad *pad, GstPadQueryType type,
                                     GstFormat *format, gint64 *value )
{
    KioSrc * src = KIOSRC ( gst_pad_get_parent ( pad ) );

    switch ( type )
    {
        case GST_PAD_QUERY_TOTAL:
            {
                if ( *format != GST_FORMAT_BYTES )
                {
                    return FALSE;
                }
                long long s = src->receiver->fileSize();
                if ( s < 0 )
                    return FALSE;
                *value = s;
                break;
            }
        case GST_PAD_QUERY_POSITION:
            if ( *format != GST_FORMAT_BYTES )
            {
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
                                0,  /* version major */
                                4,  /* version minor */
                                ( gchar* ) "kiosrc",
                                plugin_init
                            };











/*
*
* GStreamer
* Copyright (C) 1999-2001 Erik Walthinsen <omega@cse.ogi.edu>
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

#include <gst/gst.h>

/* include this header if you want to use dynamic parameters
#include <gst/control/control.h>
*/

#include "gstplugin.h"

/* Filter signals and args */
enum {
    /* FILL ME */
    LAST_SIGNAL
};

enum {
    ARG_0,
    ARG_SILENT
};

static GstStaticPadTemplate sink_factory =
    GST_STATIC_PAD_TEMPLATE (
        "sink",
        GST_PAD_SINK,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS ( "ANY" )
    );

static GstStaticPadTemplate src_factory =
    GST_STATIC_PAD_TEMPLATE (
        "src",
        GST_PAD_SRC,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS ( "ANY" )
    );

static void	gst_plugin_template_class_init	( GstPluginTemplateClass *klass );
static void	gst_plugin_template_base_init	( GstPluginTemplateClass *klass );
static void	gst_plugin_template_init	( GstPluginTemplate *filter );

static void	gst_plugin_template_set_property( GObject *object, guint prop_id,
        const GValue *value,
        GParamSpec *pspec );
static void	gst_plugin_template_get_property( GObject *object, guint prop_id,
        GValue *value,
        GParamSpec *pspec );

static void	gst_plugin_template_update_plugin( const GValue *value,
        gpointer data );
static void	gst_plugin_template_update_mute	( const GValue *value,
        gpointer data );

static void	gst_plugin_template_chain	( GstPad *pad, GstData *in );

static GstElementClass *parent_class = NULL;

/* this function handles the link with other plug-ins */
static GstPadLinkReturn
gst_plugin_template_link ( GstPad *pad, const GstCaps *caps )
{
    GstPluginTemplate * filter;
    GstPad *otherpad;

    filter = GST_PLUGIN_TEMPLATE ( gst_pad_get_parent ( pad ) );
    g_return_val_if_fail ( filter != NULL, GST_PAD_LINK_REFUSED );
    g_return_val_if_fail ( GST_IS_PLUGIN_TEMPLATE ( filter ),
                           GST_PAD_LINK_REFUSED );
    otherpad = ( pad == filter->srcpad ? filter->sinkpad : filter->srcpad );

    /* set caps on next or previous element's pad, and see what they
     * think. In real cases, we would (after this step) extract
     * properties from the caps such as video size or audio samplerat. */ 
    return gst_pad_try_set_caps ( otherpad, caps );
}

GType
gst_gst_plugin_template_get_type ( void )
{
    static GType plugin_type = 0;

    if ( !plugin_type )
    {
        static const GTypeInfo plugin_info =
            {
                sizeof ( GstPluginTemplateClass ),
                ( GBaseInitFunc ) gst_plugin_template_base_init,
                NULL,
                ( GClassInitFunc ) gst_plugin_template_class_init,
                NULL,
                NULL,
                sizeof ( GstPluginTemplate ),
                0,
                ( GInstanceInitFunc ) gst_plugin_template_init,
            };
        plugin_type = g_type_register_static ( GST_TYPE_ELEMENT,
                                               "GstPluginTemplate",
                                               &plugin_info, 0 );
    }
    return plugin_type;
}

static void
gst_plugin_template_base_init ( GstPluginTemplateClass *klass )
{
    static GstElementDetails plugin_details = {
                "PluginTemplate",
                "Generic/PluginTemplate",
                "Generic Template Plugin",
                "Thomas Vander Stichele <thomas@apestaart.org>"
            };
    GstElementClass *element_class = GST_ELEMENT_CLASS ( klass );

    gst_element_class_add_pad_template ( element_class,
                                         gst_static_pad_template_get ( &src_factory ) );
    gst_element_class_add_pad_template ( element_class,
                                         gst_static_pad_template_get ( &sink_factory ) );
    gst_element_class_set_details ( element_class, &plugin_details );
}

/* initialize the plugin's class */
static void
gst_plugin_template_class_init ( GstPluginTemplateClass *klass )
{
    GObjectClass * gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = ( GObjectClass* ) klass;
    gstelement_class = ( GstElementClass* ) klass;

    parent_class = g_type_class_ref ( GST_TYPE_ELEMENT );

    g_object_class_install_property ( gobject_class, ARG_SILENT,
                                      g_param_spec_boolean ( "silent", "Silent", "Produce verbose output ?",
                                                             FALSE, G_PARAM_READWRITE ) );

    gobject_class->set_property = gst_plugin_template_set_property;
    gobject_class->get_property = gst_plugin_template_get_property;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set functions
 * initialize structure
 */
static void
gst_plugin_template_init ( GstPluginTemplate *filter )
{
    GstElementClass * klass = GST_ELEMENT_GET_CLASS ( filter );

    filter->sinkpad = gst_pad_new_from_template (
                          gst_element_class_get_pad_template ( klass, "sink" ), "sink" );
    gst_pad_set_link_function ( filter->sinkpad, gst_plugin_template_link );
    gst_pad_set_getcaps_function ( filter->sinkpad, gst_pad_proxy_getcaps );

    filter->srcpad = gst_pad_new_from_template (
                         gst_element_class_get_pad_template ( klass, "src" ), "src" );
    gst_pad_set_link_function ( filter->srcpad, gst_plugin_template_link );
    gst_pad_set_getcaps_function ( filter->srcpad, gst_pad_proxy_getcaps );

    gst_element_add_pad ( GST_ELEMENT ( filter ), filter->sinkpad );
    gst_element_add_pad ( GST_ELEMENT ( filter ), filter->srcpad );
    gst_pad_set_chain_function ( filter->sinkpad, gst_plugin_template_chain );
    filter->silent = FALSE;
}

/* chain function
 * this function does the actual processing
 */

static void
gst_plugin_template_chain ( GstPad *pad, GstData *in )
{
    GstPluginTemplate * filter;
    GstBuffer *out_buf, *buf = GST_BUFFER ( in );
    gfloat *data;
    gint i, num_samples;

    g_return_if_fail ( GST_IS_PAD ( pad ) );
    g_return_if_fail ( buf != NULL );

    filter = GST_PLUGIN_TEMPLATE ( GST_OBJECT_PARENT ( pad ) );
    g_return_if_fail ( GST_IS_PLUGIN_TEMPLATE ( filter ) );

    if ( filter->silent == FALSE )
        g_print ( "I'm plugged, therefore I'm in.\n" );

    /* just push out the incoming buffer without touching it */
    gst_pad_push ( filter->srcpad, GST_DATA ( buf ) );
}

static void
gst_plugin_template_set_property ( GObject *object, guint prop_id,
                                   const GValue *value, GParamSpec *pspec )
{
    GstPluginTemplate * filter;

    g_return_if_fail ( GST_IS_PLUGIN_TEMPLATE ( object ) );
    filter = GST_PLUGIN_TEMPLATE ( object );

    switch ( prop_id )
    {
        case ARG_SILENT:
            filter->silent = g_value_get_boolean ( value );
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
            break;
    }
}

static void
gst_plugin_template_get_property ( GObject *object, guint prop_id,
                                   GValue *value, GParamSpec *pspec )
{
    GstPluginTemplate * filter;

    g_return_if_fail ( GST_IS_PLUGIN_TEMPLATE ( object ) );
    filter = GST_PLUGIN_TEMPLATE ( object );

    switch ( prop_id )
    {
        case ARG_SILENT:
            g_value_set_boolean ( value, filter->silent );
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
            break;
    }
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and pad templates
 * register the features
 */
static gboolean
plugin_init ( GstPlugin *plugin )
{
    return gst_element_register ( plugin, "plugin",
                                  GST_RANK_NONE,
                                  GST_TYPE_PLUGIN_TEMPLATE );
}

/* this is the structure that gst-register looks for
 * so keep the name plugin_desc, or you cannot get your plug-in registered */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "plugin",
    "Template plugin",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
