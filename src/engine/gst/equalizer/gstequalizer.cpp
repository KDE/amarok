// PCM time-domain equalizer
// (c) 2002 Felipe Rivera <liebremx at users sourceforge net>
// (c) 2004 Mark Kretschmann <markey@web.de>
// License: GPL V2

#include "config.h"
#include "gstequalizer.h"

#include <kdebug.h>


GST_DEBUG_CATEGORY_STATIC ( gst_equalizer_debug );
#define GST_CAT_DEFAULT gst_equalizer_debug


/* signals and args */
enum {
    LAST_SIGNAL
};

enum {
    ARG_0
};

GstElementDetails gst_equalizer_details =
    GST_ELEMENT_DETAILS ( (gchar*) "Equalizer",
                          (gchar*) "Source",
                          (gchar*) "Parametric Equalizer",
                          (gchar*) "Mark Kretschmann <markey@web.de>" );

static guint gst_equalizer_signals[ LAST_SIGNAL ] = { 0 };

#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_equalizer_debug, "streamsrc", 0, "streamsrc element");


GST_BOILERPLATE_FULL ( GstEqualizer, gst_equalizer, GstElement, (GTypeFlags) GST_TYPE_ELEMENT, _do_init );


// Forward declarations

static void gst_equalizer_set_property ( GObject * object, guint prop_id,
        const GValue * value, GParamSpec * pspec );

static void gst_equalizer_get_property ( GObject * object, guint prop_id,
        GValue * value, GParamSpec * pspec );

static GstElementStateReturn gst_equalizer_change_state (GstElement* element);

static GstData *gst_equalizer_chain ( GstPad* pad, GstData* data );


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

static void
gst_equalizer_base_init ( gpointer g_class )
{
    kdDebug() << k_funcinfo << endl;

    GstElementClass * gstelement_class = GST_ELEMENT_CLASS ( g_class );
    gst_element_class_set_details ( gstelement_class, &gst_equalizer_details );
}


static void
gst_equalizer_class_init ( GstEqualizerClass * klass )
{
    kdDebug() << k_funcinfo << endl;

    GObjectClass* gobject_class;
    GstElementClass* gstelement_class = GST_ELEMENT_CLASS( klass );
    gobject_class = G_OBJECT_CLASS( klass );

//     g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_BLOCKSIZE,
//                                       g_param_spec_ulong ( "blocksize", "Block size",
//                                                            "Size in bytes to read per buffer", 1, G_MAXULONG, DEFAULT_BLOCKSIZE,
//                                                            ( GParamFlags ) G_PARAM_READWRITE ) );

    gobject_class->set_property = gst_equalizer_set_property;
    gobject_class->get_property = gst_equalizer_get_property;

    gstelement_class->change_state = gst_equalizer_change_state;
}


static void
gst_equalizer_init ( GstEqualizer * streamsrc )
{
    kdDebug() << k_funcinfo << endl;

    streamsrc->srcpad = gst_pad_new ( "src", GST_PAD_SRC );

    gst_pad_set_get_function ( streamsrc->srcpad, gst_equalizer_get );
    gst_element_add_pad ( GST_ELEMENT ( streamsrc ), streamsrc->srcpad );

    // Properties
//     streamsrc->blocksize = DEFAULT_BLOCKSIZE;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

static void
gst_equalizer_set_property ( GObject * object, guint prop_id, const GValue * value,
                             GParamSpec * pspec )
{
    /* it's not null if we got it, but it might not be ours */
    g_return_if_fail ( GST_IS_EQUALIZER ( object ) );

    GstEqualizer* obj = GST_EQUALIZER ( object );

    switch ( prop_id )
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
            break;
    }
}


static void
gst_equalizer_get_property ( GObject * object, guint prop_id, GValue * value, GParamSpec * pspec )
{
    /* it's not null if we got it, but it might not be ours */
    g_return_if_fail ( GST_IS_EQUALIZER ( object ) );

    GstEqualizer* obj = GST_EQUALIZER ( object );

    switch ( prop_id )
    {
/*        case ARG_BLOCKSIZE:
            g_value_set_ulong ( value, obj->blocksize );
            break;*/
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
            break;
    }
}


static GstElementStateReturn
gst_equalizer_change_state (GstElement * element)
{
//   GstEqualizer *src = GST_EQUALIZER (element);

    switch (GST_STATE_TRANSITION (element))
    {
        case GST_STATE_NULL_TO_READY:
            break;
        case GST_STATE_READY_TO_NULL:
            break;
        case GST_STATE_READY_TO_PAUSED:
            break;
        case GST_STATE_PAUSED_TO_READY:
            break;
        default:
            break;
    }

    if (GST_ELEMENT_CLASS (parent_class)->change_state)
        return GST_ELEMENT_CLASS (parent_class)->change_state (element);

    return GST_STATE_SUCCESS;
}


static GstData*
gst_equalizer_chain ( GstPad* pad, GstData* data )
{
    g_return_val_if_fail( pad != NULL, NULL );
    GstEqualizer* obj = GST_EQUALIZER ( GST_OBJECT_PARENT ( pad ) );

__inline__ int iir(gpointer * d, gint length, gint srate, gint nch)
{
    gint16 *data = (gint16 *) * d;
    /* Indexes for the history arrays
     * These have to be kept between calls to this function
     * hence they are static */
    static gint i = 0, j = 2, k = 1;

    gint index, band, channel;
    gint tempgint, halflength;
    float out[EQ_CHANNELS], pcm[EQ_CHANNELS];

    // Load the correct filter table according to the sampling rate if needed
    if (srate != rate)
    {
        set_filters(eqcfg.band_num, srate, eqcfg.use_xmms_original_freqs);
        clean_history();
    }

    /**
     * IIR filter equation is
     * y[n] = 2 * (alpha*(x[n]-x[n-2]) + gamma*y[n-1] - beta*y[n-2])
     *
     * NOTE: The 2 factor was introduced in the coefficients to save
     * 			a multiplication
     *
     * This algorithm cascades two filters to get nice filtering
     * at the expense of extra CPU cycles
     */
    /* 16bit, 2 bytes per sample, so divide by two the length of
     * the buffer (length is in bytes)
     */
    halflength = (length >> 1);
    for (index = 0; index < halflength; index+=2)
    {
        /* For each channel */
        for (channel = 0; channel < nch; channel++)
        {
            pcm[channel] = data[index+channel];
            /* Preamp gain */
            pcm[channel] *= preamp[channel];

            out[channel] = 0.;
            /* For each band */
            for (band = 0; band < *band_count; band++)
            {
                /* Store Xi(n) */
                data_history[band][channel].x[i] = pcm[channel];
                /* Calculate and store Yi(n) */
                data_history[band][channel].y[i] =
                    (
                     /* 		= alpha * [x(n)-x(n-2)] */
                     iir_cf[band].alpha * ( data_history[band][channel].x[i]
                         -  data_history[band][channel].x[k])
                     /* 		+ gamma * y(n-1) */
                     + iir_cf[band].gamma * data_history[band][channel].y[j]
                     /* 		- beta * y(n-2) */
                     - iir_cf[band].beta * data_history[band][channel].y[k]
                    );
                /*
                 * The multiplication by 2.0 was 'moved' into the coefficients to save
                 * CPU cycles here */
                /* Apply the gain  */
                out[channel] +=  data_history[band][channel].y[i]*gain[band][channel]; // * 2.0;
            } /* For each band */

            /* Volume stuff
               Scale down original PCM sample and add it to the filters
               output. This substitutes the multiplication by 0.25
               Go back to use the floating point multiplication before the
               conversion to give more dynamic range
               */
            out[channel] += pcm[channel]*0.25;

            /* Round and convert to integer */
#ifdef ARCH_PPC
            tempgint = round_ppc(out[channel]);
#else
#ifdef ARCH_X86
            tempgint = round_trick(out[channel]);
#else
            tempgint = (int)out[channel];
#endif
#endif

            /* Limit the output */
            if (tempgint < -32768)
                data[index+channel] = -32768;
            else if (tempgint > 32767)
                data[index+channel] = 32767;
            else
                data[index+channel] = tempgint;
        } /* For each channel */

        i++; j++; k++;

        /* Wrap around the indexes */
        if (i == 3) i = 0;
        else if (j == 3) j = 0;
        else k = 0;


    }/* For each pair of samples */

    return length;
    gst_pad_push( obj->srcpad, GST_DATA( buf ) );
}


GstEqualizer*
gst_equalizer_new ( char* buf, int* index, bool* stop, bool* buffering )
{
    GstEqualizer* obj = GST_EQUALIZER ( g_object_new ( GST_TYPE_EQUALIZER, NULL ) );
    gst_object_set_name( (GstObject*) obj, "Equalizer" );

//     object->m_buffering = buffering;

    return object;
}


