// (c) 2004 Mark Kretschmann <markey@web.de>
// Portions Copyright (C) 2006 Paul Cifarelli <paul@cifarelli.net>
// See COPYING file for licensing information.


#include "streamsrc.h"

#include "config.h"
#include <kdebug.h>

#define DEBUG_PREFIX "[StreamSrc] "
#define DEFAULT_BLOCKSIZE 4096

/* signals and args */
enum {
    SIGNAL_TIMEOUT,
    SIGNAL_KIO_RESUME,
    LAST_SIGNAL
};

enum {
    ARG_0,
    ARG_BLOCKSIZE,
    ARG_BUFFER_MIN,
};

GstElementDetails gst_streamsrc_details =
    GST_ELEMENT_DETAILS ( (gchar*) "Stream Source",
                          (gchar*) "Source",
                          (gchar*) "Reads streaming audio from TitleProxy",
                          (gchar*) "Mark Kretschmann <markey@web.de>, Paul Cifarelli <paul@cifarelli.net>" );


GstStaticPadTemplate src_factory =
GST_STATIC_PAD_TEMPLATE (
  "src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("ANY")
);

static guint gst_streamsrc_signals[ LAST_SIGNAL ] = { 0 };

#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_streamsrc_debug, "streamsrc", 0, "streamsrc element");


static gboolean gst_streamsrc_setcaps(GstPad *pad, GstCaps *caps);
static void     gst_streamsrc_loop(GstStreamSrc * filter);

GST_BOILERPLATE ( GstStreamSrc, gst_streamsrc, GstElement, GST_TYPE_ELEMENT );


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void
gst_streamsrc_base_init ( gpointer g_class )
{
    kDebug() << k_funcinfo << endl;

    GstElementClass * gstelement_class = GST_ELEMENT_CLASS ( g_class );
    gst_element_class_set_details ( gstelement_class, &gst_streamsrc_details );

    gst_element_class_add_pad_template ( gstelement_class,
                                         gst_static_pad_template_get (&src_factory));
}


void
gst_streamsrc_class_init ( GstStreamSrcClass * klass )
{
    kDebug() << k_funcinfo << endl;

    GObjectClass* gobject_class;
    GstElementClass* gstelement_class = GST_ELEMENT_CLASS( klass );
    gobject_class = G_OBJECT_CLASS( klass );
    parent_class = GST_ELEMENT_CLASS( g_type_class_ref( GST_TYPE_ELEMENT ) );

    if (!G_IS_OBJECT_CLASS(gobject_class))
       kDebug() << "THIS IS NOT AN OBJECT CLASS\n";

//why do these property installs fail?
    g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_BLOCKSIZE,
                                      g_param_spec_ulong ( "blocksize", "Block size",
                                                           "Size in bytes to read per buffer", 1, G_MAXULONG, DEFAULT_BLOCKSIZE,
                                                           ( GParamFlags ) G_PARAM_READWRITE ) );

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
    gobject_class->dispose      = gst_streamsrc_dispose;

    gstelement_class->change_state = gst_streamsrc_change_state;
}


void
gst_streamsrc_init ( GstStreamSrc *streamsrc, GstStreamSrcClass *streamsrcclass )
{
    kDebug() << k_funcinfo << endl;

    GstElementClass * klass = GST_ELEMENT_CLASS(streamsrcclass);

    streamsrc->srcpad  = gst_pad_new_from_template(gst_element_class_get_pad_template(klass, "src"), "src");

    kDebug() << "srcpad: " << streamsrc->srcpad << endl; 

    gst_pad_set_setcaps_function( streamsrc->srcpad, gst_streamsrc_setcaps);

    gst_element_add_pad ( GST_ELEMENT ( streamsrc ), streamsrc->srcpad );

    kDebug() << "here\n"; 

    streamsrc->stopped = false;
    streamsrc->curoffset = 0;

    // Properties
    streamsrc->blocksize = DEFAULT_BLOCKSIZE;
}

static void
gst_streamsrc_loop(GstStreamSrc *streamsrc)
{
   GstFlowReturn ret;
   GstState state = GST_STATE( GST_ELEMENT( streamsrc ) );


   if ( state == GST_STATE_VOID_PENDING ||
        state == GST_STATE_NULL )
      return;

   if ( streamsrc->stopped )
   {
      gst_element_send_event( GST_ELEMENT( streamsrc ), gst_event_new_flush_start());
      return;
   }

   // Signal KIO/StreamProvider to resume transfer when buffer reaches our low limit
   if ( *streamsrc->m_bufIndex < (int) streamsrc->buffer_resume )
      g_signal_emit ( G_OBJECT( streamsrc ), gst_streamsrc_signals[SIGNAL_KIO_RESUME], 0 );
   
   if ( *streamsrc->m_bufStop && *streamsrc->m_bufIndex == 0 ) {
      // Send EOS event when buffer is empty
      kDebug() << "Streamsrc EOS\n";
      streamsrc->stopped = true;
      gst_element_send_event( GST_ELEMENT( streamsrc ), gst_event_new_eos());

      // commit suicide
      gst_pad_stop_task( streamsrc->srcpad );
      return;
   }

   // When buffering and buffer index is below minimum level, do nothing
   else if ( *streamsrc->m_buffering && *streamsrc->m_bufIndex < (int) streamsrc->buffer_min )
      return;
   
   *streamsrc->m_buffering = *streamsrc->m_bufIndex ? false : true;
   const int readBytes = MIN( *streamsrc->m_bufIndex, streamsrc->blocksize );

   if ( readBytes )
   {
      GstBuffer* buf = gst_buffer_new_and_alloc( readBytes );
      guint8*    data = GST_BUFFER_DATA( buf );
      
      // Copy stream buffer content into gst buffer
      memcpy( data, streamsrc->m_buf, readBytes );
      // Move stream buffer content to beginning
      memmove( streamsrc->m_buf, streamsrc->m_buf + readBytes, *streamsrc->m_bufIndex );
      
      // Adjust buffer index
      *streamsrc->m_bufIndex -= readBytes;

      gst_buffer_set_data(buf, data, readBytes);
      streamsrc->curoffset += readBytes;
      
      /* now push buffer downstream */
      ret = gst_pad_push (streamsrc->srcpad, buf);
      
      buf = NULL; /* gst_pad_push() took ownership of buffer */
      
      if (ret != GST_FLOW_OK) 
      {
         //kDebug() << "pad_push failed: " << gst_flow_get_name (ret) << endl;
         return;
      }
   }

}



GstStateChangeReturn
gst_streamsrc_change_state (GstElement * element, GstStateChange trans)
{
   GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
   GstStreamSrc *streamsrc = GST_STREAMSRC (element);

   kDebug() << k_funcinfo << endl;

   switch ( trans ) {
      case GST_STATE_CHANGE_NULL_TO_READY:
         break;
      case GST_STATE_CHANGE_READY_TO_PAUSED:
         gst_pad_start_task (streamsrc->srcpad, (GstTaskFunction) gst_streamsrc_loop, streamsrc);
         break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
         break;
      default:
         break;
   }

   if ( parent_class->change_state )
   {
      ret =  GST_ELEMENT_CLASS(parent_class)->change_state( element, trans );
      kDebug() << "parent class is not null and returns " << ret << endl;
   }
   else
   {
      kDebug() << "parent class is null\n";
      return GST_STATE_CHANGE_FAILURE;
   }

   switch ( trans ) {
      case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
         break;
      case GST_STATE_CHANGE_PAUSED_TO_READY:
         gst_pad_stop_task (streamsrc->srcpad);
         break;
      case GST_STATE_CHANGE_READY_TO_NULL:
         break;
      default:
         break;
   }
         
   return GST_STATE_CHANGE_SUCCESS;
}



gboolean
gst_streamsrc_setcaps(GstPad *pad, GstCaps *caps)
{
   //GstStructure *structure = gst_caps_get_structure (caps, 0);
   GstStreamSrc *streamsrc = GST_STREAMSRC (GST_OBJECT_PARENT (pad));

   /* we don't touch the properties of the data.
    * That means we can set the given caps unmodified on the next
    * element, and use that negotiation return value as ours. */
   if (!gst_pad_set_caps (streamsrc->srcpad, caps))
      return false;
   
   /* Capsnego succeeded, get the stream properties for internal
    * usage and return success. */
   //gst_structure_get_int (structure, "rate", &streamsrc->samplerate);
   //gst_structure_get_int (structure, "channels", &stream->channels);
   
   kDebug() << "Caps negotiation succeeded in streamsrc\n";
   
   return true;
}


/////////////////////////////////////////////////////////////////////////////////////
// CONSTRUCTOR / DESTRUCTOR
/////////////////////////////////////////////////////////////////////////////////////

GstStreamSrc*
gst_streamsrc_new ( char* buf, int* index, bool* stop, bool* buffering )
{
    GstStreamSrc* object = GST_STREAMSRC( g_object_new( GST_TYPE_STREAMSRC, NULL ) );
    gst_object_set_name( (GstObject*) object, "StreamSrc" );

    
    object->m_buf = buf;
    object->m_bufIndex = index;
    object->m_bufStop = stop;
    object->m_buffering = buffering;

    return object;
}


void
gst_streamsrc_dispose( GObject *object )
{
   kDebug() << "BEGIN: " << k_funcinfo << endl;

   GstStreamSrc* obj = GST_STREAMSRC( object );
   *obj->m_buffering = false;

   gst_pad_stop_task (obj->srcpad);
      
   G_OBJECT_CLASS( parent_class )->dispose( object );

   kDebug() << "END: " << k_funcinfo << endl;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

void
gst_streamsrc_set_property( GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec )
{
    GstStreamSrc* src;

    /* it's not null if we got it, but it might not be ours */
    g_return_if_fail ( GST_IS_STREAMSRC ( object ) );

    src = GST_STREAMSRC ( object );

    switch ( prop_id ) {
        case ARG_BLOCKSIZE:
            src->blocksize = g_value_get_ulong ( value );
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


void
gst_streamsrc_get_property ( GObject * object, guint prop_id, GValue * value, GParamSpec * pspec )
{
    GstStreamSrc* src;

    /* it's not null if we got it, but it might not be ours */
    g_return_if_fail ( GST_IS_STREAMSRC ( object ) );

    src = GST_STREAMSRC ( object );

    switch ( prop_id ) {
        case ARG_BLOCKSIZE:
            g_value_set_ulong ( value, src->blocksize );
            break;
        case ARG_BUFFER_MIN:
            g_value_set_uint ( value, src->buffer_min );
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
            break;
    }
}



