// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Heikki Orsila <heikki.orsila@iki.fi>
// See COPYING file for licensing information.

#include "config.h"
#include "gstuade.h"

#include <kdebug.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>

#define DEFAULT_BLOCKSIZE 4096
#define MAPFILE_PATH "/home/mark/mapfile"


GST_DEBUG_CATEGORY_STATIC ( gst_uade_debug );
#define GST_CAT_DEFAULT gst_uade_debug

/* signals and args */
enum {
    SIGNAL_TIMEOUT,
    LAST_SIGNAL
};

enum {
    ARG_0,
    ARG_BLOCKSIZE,
    ARG_TIMEOUT,
    ARG_LOCATION
};

GstElementDetails gst_uade_details =
    GST_ELEMENT_DETAILS ( ( gchar* ) "UADE",
                          ( gchar* ) "Codec/Decoder/Audio",
                          ( gchar* ) "Module decoder based on UADE engine",
                          ( gchar* ) "Mark Kretschmann <markey@web.de>" );
                          
static GstStaticPadTemplate gstuade_src_template =
    GST_STATIC_PAD_TEMPLATE ("gstuade_src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ( "audio/x-raw-int" )
    );
                          
static guint gst_uade_signals[ LAST_SIGNAL ] = { 0 };

#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_uade_debug, "uade", 0, "uade element");


GST_BOILERPLATE_FULL ( GstUade, gst_uade, GstElement, ( GTypeFlags ) GST_TYPE_ELEMENT, _do_init );


static void gst_uade_set_property ( GObject * object, guint prop_id,
                                    const GValue * value, GParamSpec * pspec );

static void gst_uade_get_property ( GObject * object, guint prop_id,
                                    GValue * value, GParamSpec * pspec );

static void gst_uade_chain ( GstPad*, GstData* );


/////////////////////////////////////////////////////////////////////////////////////
// IMPORTED FROM UADE
/////////////////////////////////////////////////////////////////////////////////////

struct uade_msgstruct *uade_mmap_file( const char *filename )
{
    void * mmapptr;
    int fd;
    const int length = sizeof( struct uade_msgstruct );
    int ret;
    int written;

    unlink( filename );
    /* race :-) */
    fd = open( filename, O_RDWR | O_CREAT, 0600 );
    if ( fd < 0 )
    {
        fprintf( stderr, "uade: can not create sharedmem file!\n" );
        return 0;
    }

    // Make file big enough to hold our struc
    char buf[ 256 ];
    memset( buf, 0, sizeof( buf ) );
    written = 0;
    while ( written < length )
    {
        ret = write( fd, buf, sizeof( buf ) );
        if ( ret < 0 ) {
            if ( errno != EINTR ) {
                perror( "uade: could not initialize shared memory file" );
                close( fd );
                return 0;
            }
            continue;
        }
        written += ret;
    }

    kdDebug() << "mmapfile length: " << length << endl;
    mmapptr = mmap( 0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    if ( mmapptr == MAP_FAILED )
    {
        fprintf( stderr, "uade: can not mmap sharedmem file!\n" );
        return 0;
    }
    return ( uade_msgstruct* ) mmapptr;
}


void uade_exit( int code )
{
    exit( code );
}


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

static void
gst_uade_base_init ( gpointer g_class )
{
    GstElementClass * gstelement_class = GST_ELEMENT_CLASS ( g_class );
    gst_element_class_set_details ( gstelement_class, &gst_uade_details );
}


static void
gst_uade_class_init ( GstUadeClass * klass )
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

    g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_LOCATION,
                                      g_param_spec_string ( "location", "Location", "Set file location", "test",
                                                            ( GParamFlags ) G_PARAM_READWRITE ) );

    gst_uade_signals[ SIGNAL_TIMEOUT ] =
        g_signal_new ( "timeout", G_TYPE_FROM_CLASS ( klass ), G_SIGNAL_RUN_LAST,
                       G_STRUCT_OFFSET ( GstUadeClass, timeout ), NULL, NULL,
                       g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0 );

    gobject_class->set_property = gst_uade_set_property;
    gobject_class->get_property = gst_uade_get_property;
}


static void
gst_uade_init ( GstUade* gstuade )
{
//     gstuade->srcpad = gst_pad_new ( "sink", GST_PAD_SINK );
    gstuade->srcpad = gst_pad_new_from_template (gst_static_pad_template_get (&gstuade_src_template), "src");
    
    gst_element_add_pad ( GST_ELEMENT ( gstuade ), gstuade->srcpad );
    gst_pad_set_chain_function ( gstuade->srcpad, gst_uade_chain );
    
    gstuade->blocksize = DEFAULT_BLOCKSIZE;
    gstuade->timeout = 0;
    gstuade->streamBufIndex = 0;

    gstuade->uade_struct = uade_mmap_file( MAPFILE_PATH );
    if ( !gstuade->uade_struct ) {
        kdWarning() << "uade.c/uade: couldn't mmap file: " << MAPFILE_PATH << endl;
        uade_exit( -1 );
    }
    memset( gstuade->uade_struct, 0, sizeof( struct uade_msgstruct ) );

    gstuade->uade_struct->masterpid = getpid();
    gstuade->uade_struct->ntscbit = 0;

    /* score name must be in uade_struct before uade process is executed */
    strcpy( gstuade->uade_struct->scorename, "/home/mark/srcdir/uade-0.90-pre2/score" );

    int uadepid = fork();
    if ( !uadepid ) {
        char * newargv[] = { "/usr/local/bin/uade", "--xmms-slave", MAPFILE_PATH, 0 };
        execv( newargv[ 0 ], newargv );
        kdWarning() << "uade: shit fuck. couldn't exec uade exe. not found probably\n";
        abort();
    }
    while ( gstuade->uade_struct->uade_inited_boolean == 0 ) {
        sleep( 1 );
    }
    kdDebug() << "CHECKPOINT\n";
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

static void
gst_uade_set_property ( GObject * object, guint prop_id, const GValue * value,
                        GParamSpec * pspec )
{
    GstUade * src;

    /* it's not null if we got it, but it might not be ours */
    g_return_if_fail ( GST_IS_GSTUADE ( object ) );

    src = GST_GSTUADE ( object );

    switch ( prop_id ) {
            case ARG_BLOCKSIZE:
            src->blocksize = g_value_get_ulong ( value );
            break;

            case ARG_TIMEOUT:
            src->timeout = g_value_get_uint64 ( value );
            break;

            case ARG_LOCATION:
            kdDebug() << "ARG_LOCATION\n";

            /* playername should be determined from 'value' */
            strcpy( src->uade_struct->playername, "/home/mark/srcdir/uade-0.90-pre2/players/FC1.3" );
            /* this is where we _would_ copy 'value' */
            strcpy( src->uade_struct->modulename, g_value_get_string( value ) );
            /* this will always be the same */
            strcpy( src->uade_struct->scorename, "/home/mark/srcdir/uade-0.90-pre2/score" );

            src->uade_struct->songendpossible = -1;
            src->uade_struct->use_filter = 0;

            src->uade_struct->force_by_default = 1;
            src->uade_struct->set_subsong = 0;
            src->uade_struct->subsong = 0;
            src->uade_struct->dontwritebit = 0;
            src->uade_struct->song_end = 0;
            src->uade_struct->plugin_pause_boolean = 0;
            src->uade_struct->sbuf_writeoffset = 0;
            src->uade_struct->sbuf_readoffset = 0;
            src->uade_struct->touaemsgtype = UADE_PLAYERNAME;
            src->uade_struct->loadnewsongboolean = 1;
            /* now one should send SIGHUP to uade process to let it know there
               is a song to be played. if signal is not sent, then there is
               on average 1 second delay before the song starts, 2 seconds
               at most. see src/xmms-slave.c/xms_slave_get_next() */
            break;

            default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
    }
}


static void
gst_uade_get_property ( GObject * object, guint prop_id, GValue * value, GParamSpec * pspec )
{
    GstUade * src;

    /* it's not null if we got it, but it might not be ours */
    g_return_if_fail ( GST_IS_GSTUADE ( object ) );

    src = GST_GSTUADE ( object );

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


static GstElementStateReturn
gst_uade_change_state ( GstElement * element )
{
    GstUade* gstuade = GST_GSTUADE ( element );

    switch ( GST_STATE_TRANSITION ( element ) ) {
            case GST_STATE_NULL_TO_READY:
            break;
            case GST_STATE_READY_TO_PAUSED:
            break;
            case GST_STATE_PAUSED_TO_PLAYING:
            break;
            case GST_STATE_PLAYING_TO_PAUSED:
            break;
            case GST_STATE_PAUSED_TO_READY:
            break;
            case GST_STATE_READY_TO_NULL:
            break;
            default:
            break;
    }

    if ( GST_ELEMENT_CLASS ( parent_class ) ->change_state )
        return GST_ELEMENT_CLASS ( parent_class ) ->change_state ( element );

    return GST_STATE_SUCCESS;
}


static void
gst_uade_chain ( GstPad* pad, GstData* data )
{
   
    GstUade* src = GST_GSTUADE ( gst_pad_get_parent( pad ) );
    GstBuffer* buf = GST_BUFFER( data );
    
    struct uade_msgstruct *uade_struct = src->uade_struct;
    int datainbuffer;
    int read_bytes = 0;

    while ( read_bytes < src->blocksize ) {
        if ( uade_struct->sbuf_readoffset <= uade_struct->sbuf_writeoffset ) {
            datainbuffer = uade_struct->sbuf_writeoffset - uade_struct->sbuf_readoffset;
        } else {
            datainbuffer = sizeof( uade_struct->soundbuffer ) - uade_struct->sbuf_readoffset;
        }

        if ( datainbuffer == 0 ) {
            /* sleep for a while to get more sound data */
            usleep( 10000 );
            continue;
        }

        if ( datainbuffer > ( src->blocksize - read_bytes ) )
            datainbuffer = src->blocksize - read_bytes;

        memcpy( data + read_bytes, uade_struct->soundbuffer + uade_struct->sbuf_readoffset, datainbuffer );

        uade_struct->sbuf_readoffset = ( uade_struct->sbuf_readoffset + datainbuffer ) % sizeof( uade_struct->soundbuffer );
        read_bytes += datainbuffer;
    }

    kdDebug() << "read_bytes == " << read_bytes << endl;
    GST_BUFFER_SIZE ( buf ) = read_bytes;
    GST_BUFFER_TIMESTAMP ( buf ) = GST_CLOCK_TIME_NONE;

    gst_pad_push ( src->srcpad, GST_DATA( buf ) );
}


GstUade*
gst_uade_new ()
{
    GstUade * object = GST_GSTUADE ( g_object_new ( GST_TYPE_GSTUADE, NULL ) );
    gst_object_set_name( ( GstObject* ) object, "UADE" );

    return object;
}

