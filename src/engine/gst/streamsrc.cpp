// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.


#include "streamsrc.h"

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define DEFAULT_BLOCKSIZE 4096

GST_DEBUG_CATEGORY_STATIC ( gst_streamsrc_debug );
#define GST_CAT_DEFAULT gst_streamsrc_debug


/* signals and args */
enum {
    SIGNAL_TIMEOUT,
    LAST_SIGNAL
};

enum {
    ARG_0,
    ARG_FD,
    ARG_BLOCKSIZE,
    ARG_TIMEOUT
};

GstElementDetails gst_streamsrc_details =
    GST_ELEMENT_DETAILS ( "Stream Source",
                          "Source",
                          "Push empty (no data) buffers around",
                          "Mark Kretschmann <markey@web.de>" );

static guint gst_streamsrc_signals[ LAST_SIGNAL ] = { 0 };

#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_streamsrc_debug, "streamsrc", 0, "streamsrc element");


GST_BOILERPLATE_FULL ( GstStreamSrc, gst_streamsrc, GstElement, GST_TYPE_ELEMENT,
                       _do_init );


static void gst_streamsrc_set_property ( GObject * object, guint prop_id,
        const GValue * value, GParamSpec * pspec );


static void gst_streamsrc_get_property ( GObject * object, guint prop_id,
        GValue * value, GParamSpec * pspec );


static GstData *gst_streamsrc_get ( GstPad * pad );


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

static void
gst_streamsrc_base_init ( gpointer g_class )
{
    GstElementClass * gstelement_class = GST_ELEMENT_CLASS ( g_class );
    gst_element_class_set_details ( gstelement_class, &gst_streamsrc_details );
}


static void
gst_streamsrc_class_init ( GstStreamSrcClass * klass )
{
    GObjectClass * gobject_class;

    gobject_class = G_OBJECT_CLASS ( klass );

    g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_BLOCKSIZE,
                                      g_param_spec_ulong ( "blocksize", "Block size",
                                                           "Size in bytes to read per buffer", 1, G_MAXULONG, DEFAULT_BLOCKSIZE,
                                                           ( GParamFlags ) G_PARAM_READWRITE ) );
    g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_TIMEOUT,
                                      g_param_spec_uint64 ( "timeout", "Timeout", "Read timeout in nanoseconds",
                                                            0, G_MAXUINT64, 0, ( GParamFlags ) G_PARAM_READWRITE ) );

    gst_streamsrc_signals[ SIGNAL_TIMEOUT ] =
        g_signal_new ( "timeout", G_TYPE_FROM_CLASS ( klass ), G_SIGNAL_RUN_LAST,
                       G_STRUCT_OFFSET ( GstStreamSrcClass, timeout ), NULL, NULL,
                       g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0 );

    gobject_class->set_property = gst_streamsrc_set_property;
    gobject_class->get_property = gst_streamsrc_get_property;
}


static void
gst_streamsrc_init ( GstStreamSrc * streamsrc )
{
    streamsrc->srcpad = gst_pad_new ( "src", GST_PAD_SRC );

    gst_pad_set_get_function ( streamsrc->srcpad, gst_streamsrc_get );
    gst_element_add_pad ( GST_ELEMENT ( streamsrc ), streamsrc->srcpad );

    streamsrc->curoffset = 0;
    streamsrc->blocksize = DEFAULT_BLOCKSIZE;
    streamsrc->timeout = 0;
    streamsrc->seq = 0;
    streamsrc->streamBufIndex = 0;
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
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
        break;
    }
}


static GstData *
gst_streamsrc_get ( GstPad * pad )
{
    GstStreamSrc* src = GST_STREAMSRC ( GST_OBJECT_PARENT ( pad ) );
    GstBuffer* buffer = gst_buffer_new_and_alloc( DEFAULT_BLOCKSIZE );
    
/*    if ( src->streamBufIndex + DEFAULT_BLOCKSIZE > src->streamBufSize ) {
        const int remaining = src->streamBufSize - src->streamBufIndex;
        memcpy( GST_BUFFER_DATA( buffer ), src->streamBuf + src->streamBufIndex, remaining );
        memcpy( GST_BUFFER_DATA( buffer + remaining ), src->streamBuf, DEFAULT_BLOCKSIZE - remaining );
        src->streamBufIndex = 0;
    }            
    else {*/
        memcpy( GST_BUFFER_DATA( buffer ), src->streamBuf /*+ src->streamBufIndex*/, DEFAULT_BLOCKSIZE );
/*        src->streamBufIndex += DEFAULT_BLOCKSIZE;
    }*/
                
    return GST_DATA ( buffer );
}


GstStreamSrc*
gst_streamsrc_new ( char* buf, int size )
{
    GstStreamSrc * object = GST_STREAMSRC ( g_object_new ( GST_TYPE_STREAMSRC, NULL ) );
    gst_object_set_name( (GstObject*) object, "StreamSrc" );
    
    object->streamBuf = buf;
    object->streamBufSize = size;

    return object;
}


