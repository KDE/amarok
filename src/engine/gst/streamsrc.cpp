// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.


#include "streamsrc.h"

#include "config.h"
#include <kdebug.h>

#define DEFAULT_BLOCKSIZE 4096

GST_DEBUG_CATEGORY_STATIC ( gst_streamsrc_debug );
#define GST_CAT_DEFAULT gst_streamsrc_debug


/* signals and args */
enum {
    SIGNAL_TIMEOUT,
    SIGNAL_KIO_RESUME,
    LAST_SIGNAL
};

enum {
    ARG_0,
    ARG_BLOCKSIZE,
    ARG_TIMEOUT,
    ARG_BUFFER_MIN
};

GstElementDetails gst_streamsrc_details =
    GST_ELEMENT_DETAILS ( (gchar*) "Stream Source",
                          (gchar*) "Source",
                          (gchar*) "Reads streaming audio from TitleProxy",
                          (gchar*) "Mark Kretschmann <markey@web.de>" );

static guint gst_streamsrc_signals[ LAST_SIGNAL ] = { 0 };

#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_streamsrc_debug, "streamsrc", 0, "streamsrc element");


GST_BOILERPLATE_FULL ( GstStreamSrc, gst_streamsrc, GstElement, (GTypeFlags) GST_TYPE_ELEMENT, _do_init );


// Forward declarations

static void gst_streamsrc_set_property ( GObject * object, guint prop_id,
        const GValue * value, GParamSpec * pspec );

static void gst_streamsrc_get_property ( GObject * object, guint prop_id,
        GValue * value, GParamSpec * pspec );

static GstData *gst_streamsrc_get ( GstPad * pad );

static void gst_streamsrc_dispose ( GObject* );


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

static void
gst_streamsrc_base_init ( gpointer g_class )
{
    kdDebug() << k_funcinfo << endl;

    GstElementClass * gstelement_class = GST_ELEMENT_CLASS ( g_class );
    gst_element_class_set_details ( gstelement_class, &gst_streamsrc_details );
}


static void
gst_streamsrc_class_init ( GstStreamSrcClass * klass )
{
    kdDebug() << k_funcinfo << endl;

    GObjectClass * gobject_class;
    gobject_class = G_OBJECT_CLASS ( klass );

    g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_BLOCKSIZE,
                                      g_param_spec_ulong ( "blocksize", "Block size",
                                                           "Size in bytes to read per buffer", 1, G_MAXULONG, DEFAULT_BLOCKSIZE,
                                                           ( GParamFlags ) G_PARAM_READWRITE ) );

    g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_TIMEOUT,
                                      g_param_spec_uint64 ( "timeout", "Timeout", "Read timeout in nanoseconds",
                                                            0, G_MAXUINT64, 0, ( GParamFlags ) G_PARAM_READWRITE ) );

    g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_BUFFER_MIN,
                                      g_param_spec_uint ( "buffer_min", "Buffer_Min", "Minimum buffer fill until playback starts",
                                                            0, G_MAXUINT, 50000, ( GParamFlags ) G_PARAM_READWRITE ) );

    gst_streamsrc_signals[SIGNAL_TIMEOUT] =
        g_signal_new ( "timeout", G_TYPE_FROM_CLASS ( klass ), G_SIGNAL_RUN_LAST,
                       G_STRUCT_OFFSET ( GstStreamSrcClass, timeout ), NULL, NULL,
                       g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0 );

    gst_streamsrc_signals[SIGNAL_KIO_RESUME] =
        g_signal_new ( "kio_resume", G_TYPE_FROM_CLASS ( klass ), G_SIGNAL_RUN_LAST,
                       G_STRUCT_OFFSET ( GstStreamSrcClass, kio_resume ), NULL, NULL,
                       g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0 );

    gobject_class->set_property = gst_streamsrc_set_property;
    gobject_class->get_property = gst_streamsrc_get_property;
    gobject_class->dispose = gst_streamsrc_dispose;
}


static void
gst_streamsrc_init ( GstStreamSrc * streamsrc )
{
    kdDebug() << k_funcinfo << endl;

    streamsrc->srcpad = gst_pad_new ( "src", GST_PAD_SRC );

    gst_pad_set_get_function ( streamsrc->srcpad, gst_streamsrc_get );
    gst_element_add_pad ( GST_ELEMENT ( streamsrc ), streamsrc->srcpad );

    streamsrc->playing = false;
    streamsrc->stopped = false;
    streamsrc->curoffset = 0;

    // Properties
    streamsrc->blocksize = DEFAULT_BLOCKSIZE;
    streamsrc->timeout = 0;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

static void
gst_streamsrc_set_property ( GObject * object, guint prop_id, const GValue * value,
                             GParamSpec * pspec )
{
    GstStreamSrc * src;

    /* it's not null if we got it, but it might not be ours */
    g_return_if_fail ( GST_IS_STREAMSRC ( object ) );

    src = GST_STREAMSRC ( object );

    switch ( prop_id ) {
    case ARG_BLOCKSIZE:
        src->blocksize = g_value_get_ulong ( value );
        break;
    case ARG_TIMEOUT:
        src->timeout = g_value_get_uint64 ( value );
        break;
    case ARG_BUFFER_MIN:
        src->buffer_min = g_value_get_uint ( value );
        src->buffer_resume = src->buffer_min + 100000;
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
        break;
    }
}


static void
gst_streamsrc_get_property ( GObject * object, guint prop_id, GValue * value, GParamSpec * pspec )
{
    GstStreamSrc * src;

    /* it's not null if we got it, but it might not be ours */
    g_return_if_fail ( GST_IS_STREAMSRC ( object ) );

    src = GST_STREAMSRC ( object );

    switch ( prop_id ) {
    case ARG_BLOCKSIZE:
        g_value_set_ulong ( value, src->blocksize );
        break;
    case ARG_TIMEOUT:
        g_value_set_uint64 ( value, src->timeout );
        break;
    case ARG_BUFFER_MIN:
        g_value_set_uint ( value, src->buffer_min );
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
        break;
    }
}


static GstData*
gst_streamsrc_get ( GstPad * pad )
{
    GstStreamSrc* src = GST_STREAMSRC ( GST_OBJECT_PARENT ( pad ) );

    if ( src->stopped )
        return GST_DATA( gst_event_new( GST_EVENT_FLUSH ) );

    // Signal KIO to resume transfer when buffer reaches our low limit
    if ( *src->streamBufIndex < src->buffer_resume )
        g_signal_emit ( G_OBJECT( src ), gst_streamsrc_signals[SIGNAL_KIO_RESUME], 0 );

    if ( *src->streamBufStop ) {
        // Send EOS event when buffer is empty
        if ( *src->streamBufIndex == 0 ) {
            kdDebug() << "Streamsrc EOS\n";
            src->stopped = true;
            gst_element_set_eos (GST_ELEMENT( src ) );
            return GST_DATA( gst_event_new( GST_EVENT_EOS ) );
        }
    }
    // Return when buffer is not filled
    else if ( !src->playing && *src->streamBufIndex < src->buffer_min )
        return GST_DATA( gst_event_new( GST_EVENT_FILLER ) );

    src->playing = true;
    int readBytes = MIN( *src->streamBufIndex, src->blocksize );

    GstBuffer* buf = gst_buffer_new_and_alloc( readBytes );
    guint8* data = GST_BUFFER_DATA( buf );

    // Copy stream buffer content into gst buffer
    memcpy( data, src->streamBuf, readBytes );
    // Move stream buffer content to beginning
    memmove( src->streamBuf, src->streamBuf + readBytes, *src->streamBufIndex );

    // Adjust buffer index
    *src->streamBufIndex -= readBytes;

    GST_BUFFER_OFFSET ( buf ) = src->curoffset;
    GST_BUFFER_OFFSET_END ( buf ) = src->curoffset + readBytes;
    src->curoffset += readBytes;

    return GST_DATA ( buf );
}


GstStreamSrc*
gst_streamsrc_new ( char* buf, int* index, bool* stop )
{
    GstStreamSrc * object = GST_STREAMSRC ( g_object_new ( GST_TYPE_STREAMSRC, NULL ) );
    gst_object_set_name( (GstObject*) object, "StreamSrc" );

    object->streamBuf = buf;
    object->streamBufIndex = index;
    object->streamBufStop = stop;

    return object;
}


static void
gst_streamsrc_dispose( GObject* object )
{
    G_OBJECT_CLASS (parent_class)->dispose (object);
}


