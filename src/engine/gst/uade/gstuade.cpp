// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.


#include "gstuade.h"

#include "config.h"
#include <kdebug.h>

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>


#define DEFAULT_BLOCKSIZE 4096

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

static guint gst_uade_signals[ LAST_SIGNAL ] = { 0 };

#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_uade_debug, "uade", 0, "uade element");


GST_BOILERPLATE_FULL ( GstUade, gst_uade, GstElement, ( GTypeFlags ) GST_TYPE_ELEMENT, _do_init );


static void gst_uade_set_property ( GObject * object, guint prop_id,
        const GValue * value, GParamSpec * pspec );


static void gst_uade_get_property ( GObject * object, guint prop_id,
        GValue * value, GParamSpec * pspec );


static GstData *gst_uade_get ( GstPad * pad );


void *uade_mmap_file(const char *filename, int length) {
  void *mmapptr;
  int fd;
  fd = open(filename,O_RDWR);
  if(fd < 0) {
    fprintf(stderr,"uade: can not open sharedmem file!\n");
    return 0;
  }
  mmapptr = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(mmapptr == MAP_FAILED) {
    fprintf(stderr,"uade: can not mmap sharedmem file!\n");
    return 0;
  }
  return mmapptr;
}


void uade_exit(int code) {
  exit(code);
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
                                                           (GParamFlags) G_PARAM_READWRITE ) );
    
    g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_TIMEOUT,
                                      g_param_spec_uint64 ( "timeout", "Timeout", "Read timeout in nanoseconds",
                                                            0, G_MAXUINT64, 0, (GParamFlags) G_PARAM_READWRITE ) );
    
    g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_LOCATION,
                                      g_param_spec_string ( "location", "Location", "Set file location", "test", 
                                                            (GParamFlags) G_PARAM_READWRITE ) );

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
    gstuade->srcpad = gst_pad_new ( "src", GST_PAD_SRC );

    gst_pad_set_get_function ( gstuade->srcpad, gst_uade_get );
    gst_element_add_pad ( GST_ELEMENT ( gstuade ), gstuade->srcpad );

    gstuade->blocksize = DEFAULT_BLOCKSIZE;
    gstuade->timeout = 0;
    gstuade->streamBufIndex = 0;

    #define mapfilename "mapfile"
        
    gstuade->uade_struct = ( struct uade_msgstruct* ) uade_mmap_file( mapfilename, sizeof( struct uade_msgstruct ) );
    if ( !gstuade->uade_struct ) {
        kdWarning() << "uade.c/uade: couldn't mmap file: " << mapfilename << endl;
        uade_exit( -1 );
    }
    gstuade->uade_struct->masterpid = getpid();

    int uadepid = fork();
    if ( !uadepid ) {
        execl( "uade", "--xmms-slave", mapfilename, 0 );
        kdWarning() << "uade: shit fuck. couldn't exec uade exe. not found probably\n";
        abort();
    }

    while ( gstuade->uade_struct->uade_inited_boolean == 0 ) {
        sleep( 1 );
    }
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
            strcpy( src->uade_struct->playername, "/chip/shit.custom" );
            src->uade_struct->modulename[ 0 ] = 0; /* we played a custom */
            strcpy( src->uade_struct->scorename, "/shit/uadescore" );
            src->uade_struct->set_subsong = 0;
            src->uade_struct->subsong = 0;
            src->uade_struct->dontwritebit = 0;
            src->uade_struct->song_end = 0;
            src->uade_struct->plugin_pause_boolean = 0;
            src->uade_struct->sbuf_writeoffset = 0;
            src->uade_struct->sbuf_readoffset = 0;
            src->uade_struct->loadnewsongboolean = 1;
            src->uade_struct->touaemsgtype = UADE_PLAYERNAME;

            default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
            break;
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


static GstData*
gst_uade_get ( GstPad * pad )
{
    GstUade * src = GST_GSTUADE ( GST_OBJECT_PARENT ( pad ) );
    GstBuffer* buf = gst_buffer_new_and_alloc( src->blocksize );
    guint8* data = GST_BUFFER_DATA( buf );
    int readBytes = *src->streamBufIndex;
    int rbsize = src->blocksize;
    
    uade_msgstruct* uade_struct = src->uade_struct;
    
//     if ( uade_struct->sbuf_readoffset <= uade_struct->sbuf_writeoffset ) 
//         datainbuffer = uade_struct->sbuf_writeoffset - uade_struct->sbuf_readoffset;
//     else
//         datainbuffer = uade_struct->sbuf_writeoffset + rbsize - uade_struct->sb;
    
    if ( ( uade_struct->sbuf_readoffset + src->blocksize ) > rbsize ) {
        int firstsize = rbsize - uade_struct->sbuf_readoffset;
        memcpy( data, uade_struct->soundbuffer + uade_struct->sbuf_readoffset, firstsize );
        memcpy( data + firstsize, uade_struct->soundbuffer, src->blocksize - firstsize );
    } else {
        memcpy( data, uade_struct->soundbuffer + uade_struct->sbuf_readoffset, src->blocksize );
    }
    uade_struct->sbuf_readoffset = ( uade_struct->sbuf_readoffset + src->blocksize ) % rbsize;
    
    
    GST_BUFFER_SIZE ( buf ) = readBytes;
    GST_BUFFER_TIMESTAMP ( buf ) = GST_CLOCK_TIME_NONE;

    return GST_DATA ( buf );
}


GstUade*
gst_uade_new ( char * buf, int * index )
{
    GstUade * object = GST_GSTUADE ( g_object_new ( GST_TYPE_GSTUADE, NULL ) );
    gst_object_set_name( ( GstObject* ) object, "UADE" );

    object->streamBuf = buf;
    object->streamBufIndex = index;

    return object;
}

